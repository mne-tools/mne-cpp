//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     python_runner.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Embedding-free launcher that runs MNE-Python (or any user) scripts as a child process and streams their output back to the C++ caller.
 *
 * @ref mne::PythonRunner runs Python for cross-validation, reference data generation and pipeline glue
 * without linking against @c libpython: unbuffered launch for real-time streaming, line callbacks for stdout and
 * stderr, a @c "[progress] <pct>%" protocol parsed into progress events, an optional virtual environment, and a
 * one-shot @c pip install of a pyproject directory or @c requirements.txt before the run.
 *
 * The runner is the foundation under @ref mne::PythonTestHelper, which every cross-language unit test
 * uses, and under the training front ends of the ML and inverse libraries.
 */

#ifndef PYBRIDGELIB_PYTHON_RUNNER_H
#define PYBRIDGELIB_PYTHON_RUNNER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pybridge_global.h"

#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Output stream of the child process a line came from.
 */
enum class PythonChannel
{
    StdOut, /**< Standard output. */
    StdErr  /**< Standard error. */
};

//=============================================================================================================
/**
 * @brief Outcome of a Python run.
 *
 * @snippet ex_pybridge/main.cpp python_runner_usage
 */
struct PYBRIDGESHARED_EXPORT PythonRunnerResult
{
    bool success = false;       /**< True when the process exited normally with code 0. */
    int exit_code = -1;         /**< Process exit code (-1 if it never started). */
    bool timed_out = false;     /**< True if the timeout was reached and the process was killed. */
    std::string std_out;        /**< Captured standard output, lines joined by '\n'. */
    std::string std_err;        /**< Captured standard error, lines joined by '\n'. */
    float progress_pct = -1.0f; /**< Last reported progress (0 to 100), or -1 if none was reported. */
};

//=============================================================================================================
/**
 * @brief How and where a @ref PythonRunner starts the interpreter.
 *
 * @snippet ex_pybridge/main.cpp python_runner_usage
 */
struct PYBRIDGESHARED_EXPORT PythonRunnerConfig
{
    std::string python_exe = "python3";               /**< Interpreter path, or a command found on PATH. */
    std::filesystem::path working_dir;                /**< Working directory; empty inherits the caller's. */
    std::map<std::string, std::string> extra_env;     /**< Variables added to the inherited environment. */
    std::optional<std::chrono::milliseconds> timeout; /**< Wall-clock limit; none by default. */
    bool unbuffered = true;                           /**< Pass -u so output streams in real time. */
    std::filesystem::path venv_dir;                   /**< Virtual environment; empty uses @ref python_exe. */
    std::filesystem::path package_dir;                /**< pyproject.toml directory to pip install into the venv. */
    std::filesystem::path requirements_file;          /**< requirements.txt to install when there is no package_dir. */
};

//=============================================================================================================
/** Called for every output line (without the trailing newline). */
using PythonLineCallback = std::function<void(PythonChannel channel, std::string_view line)>;

/** Called for every @c "[progress] <pct>% [message]" line, with pct in 0 to 100. */
using PythonProgressCallback = std::function<void(float pct, std::string_view message)>;

//=============================================================================================================
/**
 * Runs Python scripts and one-liners in a child process.
 *
 * Output is streamed line by line to the optional callbacks and to mne::Log, and collected into the
 * @ref PythonRunnerResult. Lines of the form @c "[progress] 42.5%" or @c "[progress] 42.5% Training epoch 10/50"
 * are reported as progress. No Python embedding or linkage is required.
 *
 * @brief Python script launcher with streaming output and progress reporting.
 *
 * @snippet ex_pybridge/main.cpp python_runner_usage
 */
class PYBRIDGESHARED_EXPORT PythonRunner
{
public:
    //=========================================================================================================
    /**
     * Constructs a runner for @c python3 from PATH.
     */
    PythonRunner() = default;

    //=========================================================================================================
    /**
     * Constructs a runner with an explicit configuration.
     *
     * @param[in] config     Interpreter, environment and limits.
     */
    explicit PythonRunner(PythonRunnerConfig config);

    //=========================================================================================================
    /**
     * Replaces the configuration.
     *
     * @param[in] config     Interpreter, environment and limits.
     */
    void set_config(PythonRunnerConfig config);

    //=========================================================================================================
    /**
     * @return The current configuration; after a successful @ref ensure_venv it names the venv interpreter.
     */
    const PythonRunnerConfig& config() const;

    //=========================================================================================================
    /**
     * Sets the callback invoked for every stdout and stderr line.
     *
     * @param[in] callback   Line callback; an empty function disables it.
     */
    void set_line_callback(PythonLineCallback callback);

    //=========================================================================================================
    /**
     * Sets the callback invoked for every progress line.
     *
     * @param[in] callback   Progress callback; an empty function disables it.
     */
    void set_progress_callback(PythonProgressCallback callback);

    //=========================================================================================================
    /**
     * Runs a script and waits for it to finish.
     *
     * @param[in] script     Path of the .py file.
     * @param[in] args       Arguments passed to the script.
     *
     * @return Exit code, captured output and last progress.
     */
    PythonRunnerResult run(const std::filesystem::path& script, const mne::StringList& args = {}) const;

    //=========================================================================================================
    /**
     * Runs Python code (@c python -c) and waits for it to finish.
     *
     * @param[in] code       Python source.
     * @param[in] args       Arguments available as @c sys.argv[1:].
     *
     * @return Exit code, captured output and last progress.
     */
    PythonRunnerResult run_code(std::string_view code, const mne::StringList& args = {}) const;

    //=========================================================================================================
    /**
     * @return True if @c "python --version" succeeds.
     */
    bool is_python_available() const;

    //=========================================================================================================
    /**
     * @return The interpreter version, e.g. "3.12.4", or an empty string if Python is not available.
     */
    std::string python_version() const;

    //=========================================================================================================
    /**
     * Checks whether a package can be imported, without importing it.
     *
     * @param[in] package    Module name, e.g. "mne" or "sklearn"; anything but a dotted Python identifier is
     *                       rejected.
     *
     * @return True if @c importlib.util.find_spec finds the package.
     */
    bool is_package_available(std::string_view package) const;

    //=========================================================================================================
    /**
     * Prepares the configured virtual environment: creates it if needed, installs @c package_dir (preferred) or
     * @c requirements_file into it, and switches @c python_exe to the venv interpreter. Without a @c venv_dir it
     * does nothing and succeeds.
     *
     * @return The outcome of the venv setup.
     */
    PythonRunnerResult ensure_venv();

    //=========================================================================================================
    /**
     * Calls @ref ensure_venv, then runs the script inside the venv.
     *
     * @param[in] script     Path of the .py file.
     * @param[in] args       Arguments passed to the script.
     *
     * @return The venv setup error, or the outcome of the run.
     */
    PythonRunnerResult run_in_venv(const std::filesystem::path& script, const mne::StringList& args = {});

    //=========================================================================================================
    /**
     * @return The interpreter inside @c venv_dir, or an empty path if no venv is configured.
     */
    std::filesystem::path venv_python_path() const;

private:
    PythonRunnerResult execute(const mne::StringList& args) const;

    PythonRunnerConfig m_config;               /**< Interpreter, environment and limits. */
    PythonLineCallback m_lineCallback;         /**< Optional line callback. */
    PythonProgressCallback m_progressCallback; /**< Optional progress callback. */
};

//=============================================================================================================
/**
 * Parses a progress line of the form @c "[progress] 42.5%" or @c "[progress] 42.5% message" (case-insensitive tag,
 * surrounding whitespace ignored).
 *
 * @param[in] line   One output line.
 *
 * @return The percentage and the trimmed message, or nothing if the line is not a progress line.
 */
PYBRIDGESHARED_EXPORT std::optional<std::pair<float, std::string>> parse_progress_line(std::string_view line);

} // namespace mne

#endif // PYBRIDGELIB_PYTHON_RUNNER_H
