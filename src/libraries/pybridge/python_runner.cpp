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

#include <base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QElapsedTimer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>

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

constexpr int kStartTimeoutMs = 10000;
constexpr int kQueryTimeoutMs = 10000;
constexpr int kVenvTimeoutMs = 120000;
constexpr int kPipTimeoutMs = 1800000; // pip install can take a while (torch alone is ~2 GB)
constexpr int kPollMs = 100;

QString toQString(std::string_view text)
{
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

QString pathToQString(const std::filesystem::path& path)
{
    return toQString(std::string_view(utf8_from_path(path)));
}

QStringList toQStringList(const StringList& list)
{
    QStringList result;
    result.reserve(static_cast<qsizetype>(list.size()));
    for (const std::string& item : list) {
        result << toQString(item);
    }
    return result;
}

std::string fromBytes(const QByteArray& bytes)
{
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

//=============================================================================================================

/** Runs a short-lived helper command; on success returns its stdout, otherwise nothing. */
std::optional<std::string> query(const std::string& program, const QStringList& args, int timeoutMs = kQueryTimeoutMs)
{
    QProcess process;
    process.start(toQString(program), args);
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        return std::nullopt;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return std::nullopt;
    }
    return fromBytes(process.readAllStandardOutput());
}

//=============================================================================================================

/** Runs a setup step (venv creation, pip install) and fills @p result on failure; returns true on success. */
bool runSetupStep(const std::string& program, const QStringList& args, int timeoutMs, std::string_view what, PythonRunnerResult& result)
{
    QProcess process;
    process.start(toQString(program), args);
    if (!process.waitForStarted(kStartTimeoutMs)) {
        result.std_err = std::format("Failed to start {}: {}", what, process.errorString().toStdString());
        Log::warning("[PythonRunner] {}", result.std_err);
        return false;
    }
    if (!process.waitForFinished(timeoutMs)) {
        result.std_err = std::format("{} timed out.", what);
        result.timed_out = true;
        process.kill();
        process.waitForFinished(5000);
        Log::warning("[PythonRunner] {}", result.std_err);
        return false;
    }
    result.std_out = fromBytes(process.readAllStandardOutput());
    result.std_err = fromBytes(process.readAllStandardError());
    result.exit_code = process.exitCode();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        Log::warning("[PythonRunner] {} failed with code {}: {}", what, process.exitCode(), result.std_err);
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
    return query(m_config.python_exe, {QStringLiteral("--version")}, 5000).has_value();
}

//=============================================================================================================

std::string PythonRunner::python_version() const
{
    const std::optional<std::string> out = query(m_config.python_exe, {QStringLiteral("--version")}, 5000);
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
    const QString code = toQString(std::format("import importlib.util, sys; sys.exit(0 if importlib.util.find_spec('{}') else 1)", package));
    return query(m_config.python_exe, {QStringLiteral("-c"), code}).has_value();
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
        if (!runSetupStep(m_config.python_exe, {QStringLiteral("-m"), QStringLiteral("venv"), pathToQString(m_config.venv_dir)},
                          kVenvTimeoutMs, "venv creation", result)) {
            return result;
        }
    } else {
        Log::debug("[PythonRunner] Virtual environment already exists at: {}", utf8_from_path(m_config.venv_dir));
    }

    // Prefer pyproject.toml (package_dir) over requirements.txt
    QStringList pipArgs;
    if (!m_config.package_dir.empty()) {
        const std::filesystem::path toml = m_config.package_dir / "pyproject.toml";
        if (std::filesystem::exists(toml, ec)) {
            pipArgs << QStringLiteral("install") << pathToQString(m_config.package_dir);
            Log::debug("[PythonRunner] Installing from pyproject.toml in: {}", utf8_from_path(m_config.package_dir));
        } else {
            Log::warning("[PythonRunner] package_dir set but no pyproject.toml found at: {}", utf8_from_path(toml));
        }
    }
    if (pipArgs.isEmpty() && !m_config.requirements_file.empty()) {
        if (std::filesystem::exists(m_config.requirements_file, ec)) {
            pipArgs << QStringLiteral("install") << QStringLiteral("-r") << pathToQString(m_config.requirements_file);
            Log::debug("[PythonRunner] Installing from requirements.txt: {}", utf8_from_path(m_config.requirements_file));
        } else {
            Log::warning("[PythonRunner] requirements_file not found: {}", utf8_from_path(m_config.requirements_file));
        }
    }
    if (!pipArgs.isEmpty()) {
        // "python -m pip" works whether or not the venv puts a pip launcher next to the interpreter
        pipArgs.prepend(QStringLiteral("pip"));
        pipArgs.prepend(QStringLiteral("-m"));
        if (!runSetupStep(utf8_from_path(venvPython), pipArgs, kPipTimeoutMs, "pip install", result)) {
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

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    if (!m_config.working_dir.empty()) {
        process.setWorkingDirectory(pathToQString(m_config.working_dir));
    }
    if (!m_config.extra_env.empty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        for (const auto& [key, value] : m_config.extra_env) {
            env.insert(toQString(key), toQString(value));
        }
        process.setProcessEnvironment(env);
    }

    Log::debug("[PythonRunner] Launching: {} {}", m_config.python_exe, join(args, " "));
    process.start(toQString(m_config.python_exe), toQStringList(args));
    if (!process.waitForStarted(kStartTimeoutMs)) {
        result.std_err = "Failed to start Python process: " + process.errorString().toStdString();
        Log::warning("[PythonRunner] {}", result.std_err);
        return result;
    }

    std::array<std::string, 2> pending;
    std::array<std::string, 2> collected;
    const auto dispatch = [&](PythonChannel channel, std::string_view line) {
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
    const auto drain = [&](bool flush) {
        pending[0] += fromBytes(process.readAllStandardOutput());
        pending[1] += fromBytes(process.readAllStandardError());
        for (std::size_t i = 0; i < 2; ++i) {
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
        }
    };

    QElapsedTimer elapsed;
    elapsed.start();
    while (!process.waitForFinished(kPollMs)) {
        drain(false);
        if (m_config.timeout && elapsed.elapsed() >= m_config.timeout->count()) {
            result.timed_out = true;
            process.kill();
            process.waitForFinished(5000);
            Log::warning("[PythonRunner] Process timed out after {} ms.", m_config.timeout->count());
            break;
        }
    }
    drain(true);

    result.std_out = std::move(collected[0]);
    result.std_err = std::move(collected[1]);
    if (result.timed_out) {
        result.std_err += std::format("{}Process timed out after {} ms.", result.std_err.empty() ? "" : "\n", m_config.timeout->count());
    }
    result.exit_code = process.exitCode();
    result.success = !result.timed_out && process.exitStatus() == QProcess::NormalExit && result.exit_code == 0;

    if (result.success) {
        Log::debug("[PythonRunner] Script finished successfully.");
    } else if (!result.timed_out) {
        Log::warning("[PythonRunner] Script exited with code {}", result.exit_code);
    }
    return result;
}
