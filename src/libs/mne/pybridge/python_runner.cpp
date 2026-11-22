//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     python_runner.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref mne::PythonRunner: process plumbing, line buffering and progress parsing.
 *
 * Output of the child interpreter is polled, split on @c \\n and dispatched synchronously to the line callback, so
 * progress displays update in real time. The runner keeps no static state; several runners can work concurrently in
 * different threads, each owning its own process.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "python_runner.h"

#include <mne/base/log.h>
#include <mne/base/process.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cctype>
#include <array>
#include <charconv>
#include <format>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

using std::chrono::milliseconds;

constexpr milliseconds kQueryTimeout{10000};
constexpr milliseconds kVenvTimeout{120000};
constexpr milliseconds kPipTimeout{1800000}; // pip install can take a while (torch alone is ~2 GB)

//=============================================================================================================

/** Runs a short-lived helper command; on success returns its stdout, otherwise nothing. */
std::optional<std::string> query(const std::string& program, const StringList& args, milliseconds timeout = kQueryTimeout)
{
    ProcessOptions options;
    options.timeout = timeout;
    ProcessResult process = run_process(program, args, options);
    if (!process.ok()) {
        return std::nullopt;
    }
    return std::move(process.std_out);
}

//=============================================================================================================

/** Runs a setup step (venv creation, pip install) and fills @p result on failure; returns true on success. */
bool runSetupStep(const std::string& program, const StringList& args, milliseconds timeout, std::string_view what, PythonRunnerResult& result)
{
    ProcessOptions options;
    options.timeout = timeout;
    ProcessResult process = run_process(program, args, options);
    if (!process.started) {
        result.std_err = std::format("Failed to start {}: {}", what, process.error);
        Log::warning("[PythonRunner] {}", result.std_err);
        return false;
    }
    if (process.timed_out) {
        result.std_err = std::format("{} timed out.", what);
        result.timed_out = true;
        Log::warning("[PythonRunner] {}", result.std_err);
        return false;
    }
    result.std_out = std::move(process.std_out);
    result.std_err = std::move(process.std_err);
    result.exit_code = process.exit_code;
    if (!process.ok()) {
        Log::warning("[PythonRunner] {} failed with code {}: {}", what, process.exit_code, result.std_err);
        return false;
    }
    return true;
}

//=============================================================================================================

bool isValidModuleName(std::string_view name)
{
    if (name.empty() || !(std::isalpha(static_cast<unsigned char>(name.front())) || name.front() == '_')) {
        return false;
    }
    return std::ranges::all_of(name, [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.';
    });
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::optional<std::pair<float, std::string>> mne::parse_progress_line(std::string_view line)
{
    constexpr std::string_view tag = "[progress]";
    line = trimmed(line);
    if (line.size() <= tag.size() || !iequals(line.substr(0, tag.size()), tag) ||
        !std::isspace(static_cast<unsigned char>(line[tag.size()]))) {
        return std::nullopt;
    }
    line = trimmed(line.substr(tag.size()));
    if (line.empty() || !std::isdigit(static_cast<unsigned char>(line.front()))) {
        return std::nullopt;
    }
    float pct = 0.0f;
    const auto [next, ec] = std::from_chars(line.data(), line.data() + line.size(), pct, std::chars_format::fixed);
    if (ec != std::errc{}) {
        return std::nullopt;
    }
    std::string_view rest = trimmed(line.substr(static_cast<std::size_t>(next - line.data())));
    if (rest.empty() || rest.front() != '%') {
        return std::nullopt;
    }
    return std::make_pair(pct, std::string(trimmed(rest.substr(1))));
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PythonRunner::PythonRunner(PythonRunnerConfig config)
: m_config(std::move(config))
{
}

//=============================================================================================================

void PythonRunner::set_config(PythonRunnerConfig config)
{
    m_config = std::move(config);
}

//=============================================================================================================

const PythonRunnerConfig& PythonRunner::config() const
{
    return m_config;
}

//=============================================================================================================

void PythonRunner::set_line_callback(PythonLineCallback callback)
{
    m_lineCallback = std::move(callback);
}

//=============================================================================================================

void PythonRunner::set_progress_callback(PythonProgressCallback callback)
{
    m_progressCallback = std::move(callback);
}

//=============================================================================================================

PythonRunnerResult PythonRunner::run(const std::filesystem::path& script, const StringList& args) const
{
    StringList fullArgs;
    fullArgs.reserve(args.size() + 2);
    if (m_config.unbuffered) {
        fullArgs.emplace_back("-u");
    }
    fullArgs.push_back(utf8_from_path(script));
    fullArgs.insert(fullArgs.end(), args.begin(), args.end());
    return execute(fullArgs);
}

//=============================================================================================================

PythonRunnerResult PythonRunner::run_code(std::string_view code, const StringList& args) const
{
    StringList fullArgs;
    fullArgs.reserve(args.size() + 3);
    if (m_config.unbuffered) {
        fullArgs.emplace_back("-u");
    }
    fullArgs.emplace_back("-c");
    fullArgs.emplace_back(code);
    fullArgs.insert(fullArgs.end(), args.begin(), args.end());
    return execute(fullArgs);
}

//=============================================================================================================

bool PythonRunner::is_python_available() const
{
    return query(m_config.python_exe, {"--version"}, milliseconds(5000)).has_value();
}

//=============================================================================================================

std::string PythonRunner::python_version() const
{
    const std::optional<std::string> out = query(m_config.python_exe, {"--version"}, milliseconds(5000));
    if (!out) {
        return {};
    }
    // "Python 3.12.4\n" -> "3.12.4"
    std::string_view version = trimmed(*out);
    constexpr std::string_view prefix = "Python ";
    if (version.starts_with(prefix)) {
        version.remove_prefix(prefix.size());
    }
    return std::string(version);
}

//=============================================================================================================

bool PythonRunner::is_package_available(std::string_view package) const
{
    // Only dotted identifiers reach the interpreter, so the name cannot inject code
    if (!isValidModuleName(package)) {
        Log::warning("[PythonRunner] Invalid package name: {}", package);
        return false;
    }
    // find_spec locates the package without executing it; a full import of e.g. sklearn can exceed the timeout
    // on a loaded machine and read as "not installed"
    const std::string code = std::format("import importlib.util, sys; sys.exit(0 if importlib.util.find_spec('{}') else 1)", package);
    return query(m_config.python_exe, {"-c", code}).has_value();
}

//=============================================================================================================

std::filesystem::path PythonRunner::venv_python_path() const
{
    if (m_config.venv_dir.empty()) {
        return {};
    }
#ifdef _WIN32
    return std::filesystem::absolute(m_config.venv_dir / "Scripts" / "python.exe");
#else
    return std::filesystem::absolute(m_config.venv_dir / "bin" / "python");
#endif
}

//=============================================================================================================

PythonRunnerResult PythonRunner::ensure_venv()
{
    PythonRunnerResult result;
    if (m_config.venv_dir.empty()) {
        result.success = true;
        result.exit_code = 0;
        return result;
    }

    const std::filesystem::path venvPython = venv_python_path();
    std::error_code ec;
    if (!std::filesystem::exists(venvPython, ec)) {
        Log::debug("[PythonRunner] Creating virtual environment at: {}", utf8_from_path(m_config.venv_dir));
        if (!runSetupStep(m_config.python_exe, {"-m", "venv", utf8_from_path(m_config.venv_dir)}, kVenvTimeout, "venv creation", result)) {
            return result;
        }
    } else {
        Log::debug("[PythonRunner] Virtual environment already exists at: {}", utf8_from_path(m_config.venv_dir));
    }

    // Prefer pyproject.toml (package_dir) over requirements.txt
    StringList pipArgs;
    if (!m_config.package_dir.empty()) {
        const std::filesystem::path toml = m_config.package_dir / "pyproject.toml";
        if (std::filesystem::exists(toml, ec)) {
            pipArgs = {"-m", "pip", "install", utf8_from_path(m_config.package_dir)};
            Log::debug("[PythonRunner] Installing from pyproject.toml in: {}", utf8_from_path(m_config.package_dir));
        } else {
            Log::warning("[PythonRunner] package_dir set but no pyproject.toml found at: {}", utf8_from_path(toml));
        }
    }
    if (pipArgs.empty() && !m_config.requirements_file.empty()) {
        if (std::filesystem::exists(m_config.requirements_file, ec)) {
            pipArgs = {"-m", "pip", "install", "-r", utf8_from_path(m_config.requirements_file)};
            Log::debug("[PythonRunner] Installing from requirements.txt: {}", utf8_from_path(m_config.requirements_file));
        } else {
            Log::warning("[PythonRunner] requirements_file not found: {}", utf8_from_path(m_config.requirements_file));
        }
    }
    // "python -m pip" works whether or not the venv puts a pip launcher next to the interpreter
    if (!pipArgs.empty()) {
        if (!runSetupStep(utf8_from_path(venvPython), pipArgs, kPipTimeout, "pip install", result)) {
            return result;
        }
        Log::debug("[PythonRunner] Dependencies installed successfully.");
    }

    m_config.python_exe = utf8_from_path(venvPython);
    result.success = true;
    result.exit_code = 0;
    Log::debug("[PythonRunner] Using venv Python: {}", m_config.python_exe);
    return result;
}

//=============================================================================================================

PythonRunnerResult PythonRunner::run_in_venv(const std::filesystem::path& script, const StringList& args)
{
    PythonRunnerResult setup = ensure_venv();
    if (!setup.success) {
        return setup;
    }
    return run(script, args);
}

//=============================================================================================================

PythonRunnerResult PythonRunner::execute(const StringList& args) const
{
    PythonRunnerResult result;
    ProcessOptions options;
    options.working_dir = m_config.working_dir;
    options.environment.assign(m_config.extra_env.begin(), m_config.extra_env.end());
    options.timeout = m_config.timeout;

    std::array<std::string, 2> pending;
    std::array<std::string, 2> collected;
    const auto dispatch = [&](PythonChannel channel, std::string_view line) {
        if (line.ends_with('\r')) {
            line.remove_suffix(1); // Python on Windows ends text lines with CR LF
        }
        std::string& out = collected[channel == PythonChannel::StdOut ? 0 : 1];
        if (!out.empty()) {
            out += '\n';
        }
        out += line;
        if (m_lineCallback) {
            m_lineCallback(channel, line);
        }
        if (const auto progress = parse_progress_line(line)) {
            result.progress_pct = progress->first;
            if (m_progressCallback) {
                m_progressCallback(progress->first, progress->second);
            }
        }
        if (channel == PythonChannel::StdOut) {
            Log::debug("[Python stdout] {}", line);
        } else {
            Log::warning("[Python stderr] {}", line);
        }
    };
    const auto split = [&](std::size_t i, bool flush) {
        const PythonChannel channel = i == 0 ? PythonChannel::StdOut : PythonChannel::StdErr;
        std::size_t start = 0;
        for (std::size_t end = pending.at(i).find('\n'); end != std::string::npos; end = pending.at(i).find('\n', start)) {
            dispatch(channel, std::string_view(pending.at(i)).substr(start, end - start));
            start = end + 1;
        }
        pending.at(i).erase(0, start);
        if (flush && !pending.at(i).empty()) {
            dispatch(channel, pending.at(i)); // trailing output without a newline
            pending.at(i).clear();
        }
    };
    options.on_output = [&](ProcessChannel channel, std::string_view data) {
        const std::size_t i = channel == ProcessChannel::StdOut ? 0 : 1;
        pending.at(i) += data;
        split(i, false);
    };

    Log::debug("[PythonRunner] Launching: {} {}", m_config.python_exe, join(args, " "));
    const ProcessResult process = run_process(m_config.python_exe, args, options);
    if (!process.started) {
        result.std_err = "Failed to start Python process: " + process.error;
        Log::warning("[PythonRunner] {}", result.std_err);
        return result;
    }
    split(0, true);
    split(1, true);
    result.timed_out = process.timed_out;
    if (result.timed_out) {
        Log::warning("[PythonRunner] Process timed out after {} ms.", m_config.timeout->count());
    }

    result.std_out = std::move(collected[0]);
    result.std_err = std::move(collected[1]);
    if (result.timed_out) {
        result.std_err += std::format("{}Process timed out after {} ms.", result.std_err.empty() ? "" : "\n", m_config.timeout->count());
    }
    result.exit_code = process.exit_code;
    result.success = process.ok();

    if (result.success) {
        Log::debug("[PythonRunner] Script finished successfully.");
    } else if (!result.timed_out) {
        Log::warning("[PythonRunner] Script exited with code {}", result.exit_code);
    }
    return result;
}
