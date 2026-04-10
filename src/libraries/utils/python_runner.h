//=============================================================================================================
/**
 * @file     python_runner.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    PythonRunner class declaration — standardized interface for calling Python scripts.
 *
 */

#ifndef PYTHON_RUNNER_H
#define PYTHON_RUNNER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcessEnvironment>

#include <functional>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * Result of a Python script execution.
 *
 * @brief Script execution result container.
 */
struct UTILSSHARED_EXPORT PythonRunnerResult
{
    bool    success    = false;     /**< True when process exited with code 0. */
    int     exitCode   = -1;        /**< Process exit code (-1 if never started). */
    bool    timedOut   = false;     /**< True if the timeout was reached. */
    QString stdOut;                 /**< Complete captured standard output. */
    QString stdErr;                 /**< Complete captured standard error. */
    float   progressPct = -1.0f;   /**< Last parsed progress percentage (0–100), or -1 if none. */
};

//=============================================================================================================
/**
 * Configuration for a PythonRunner invocation.
 *
 * @brief Script execution configuration.
 */
struct UTILSSHARED_EXPORT PythonRunnerConfig
{
    QString       pythonExe = QStringLiteral("python3");  /**< Python interpreter path or command. */
    QString       workingDir;                             /**< Working directory (empty = inherit). */
    QStringList   extraEnv;                               /**< KEY=VALUE pairs appended to environment. */
    int           timeoutMsec = -1;                       /**< Wall-clock timeout (-1 = no limit). */
    bool          unbuffered  = true;                     /**< Pass -u to Python for unbuffered I/O. */
    QString       venvDir;                                /**< Virtual-env directory (empty = no venv). */
    QString       packageDir;                             /**< Directory containing pyproject.toml for `pip install .` (preferred). */
    QString       requirementsFile;                       /**< Fallback: path to requirements.txt for `pip install -r`. */
};

//=============================================================================================================
/**
 * Line-level callback signature.
 *
 * @param channel  0 = stdout, 1 = stderr.
 * @param line     The text line (without trailing newline).
 */
using PythonLineCallback = std::function<void(int channel, const QString& line)>;

//=============================================================================================================
/**
 * Progress callback signature.
 *
 * @param pct  Progress percentage 0.0 – 100.0.
 * @param msg  Optional message accompanying the progress update.
 */
using PythonProgressCallback = std::function<void(float pct, const QString& msg)>;

//=============================================================================================================
/**
 * Standardized interface for launching Python scripts from C++.
 *
 * Features:
 * - Auto-discovers Python via PATH or explicit config.
 * - Runs scripts with `-u` (unbuffered) by default for real-time output.
 * - Streams stdout/stderr line-by-line through optional callbacks.
 * - Parses a simple progress protocol: lines matching
 *   `[progress] <float>%` or `[progress] <float>% <message>`.
 * - Integrates with MNE-CPP's logging (qDebug/qWarning).
 * - Supports timeout, environment injection, and working directory.
 *
 * No Python embedding or linkage is required.
 *
 * @brief Python script launcher with logging and progress support.
 */
class UTILSSHARED_EXPORT PythonRunner : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Construct a PythonRunner with default configuration.
     *
     * @param[in] pParent    Parent QObject (optional).
     */
    explicit PythonRunner(QObject* pParent = nullptr);

    //=========================================================================================================
    /**
     * Construct a PythonRunner with explicit configuration.
     *
     * @param[in] config     Execution configuration.
     * @param[in] pParent    Parent QObject (optional).
     */
    explicit PythonRunner(const PythonRunnerConfig& config, QObject* pParent = nullptr);

    //=========================================================================================================
    /**
     * Set the configuration.
     *
     * @param[in] config     Execution configuration.
     */
    void setConfig(const PythonRunnerConfig& config);

    //=========================================================================================================
    /**
     * Get the current configuration.
     *
     * @return Current configuration.
     */
    const PythonRunnerConfig& config() const;

    //=========================================================================================================
    /**
     * Set a callback invoked for every stdout/stderr line.
     *
     * @param[in] cb     Line callback (channel, line).
     */
    void setLineCallback(PythonLineCallback cb);

    //=========================================================================================================
    /**
     * Set a callback invoked when a progress line is detected.
     *
     * Progress protocol: the script prints lines matching:
     *   [progress] 42.5%
     *   [progress] 42.5% Training epoch 10/50
     *
     * @param[in] cb     Progress callback (pct, msg).
     */
    void setProgressCallback(PythonProgressCallback cb);

    //=========================================================================================================
    /**
     * Run a Python script synchronously.
     *
     * @param[in] scriptPath     Path to the .py file.
     * @param[in] args           Arguments forwarded to the script.
     *
     * @return PythonRunnerResult with exit code, output, and progress.
     */
    PythonRunnerResult run(const QString& scriptPath,
                           const QStringList& args = {});

    //=========================================================================================================
    /**
     * Run a Python one-liner synchronously (`python -c "..."`).
     *
     * @param[in] code      Python code string.
     * @param[in] args      Extra arguments appended after -c code.
     *
     * @return PythonRunnerResult.
     */
    PythonRunnerResult runCode(const QString& code,
                               const QStringList& args = {});

    //=========================================================================================================
    /**
     * Check that the configured Python interpreter is reachable.
     *
     * @return True if `python --version` succeeds.
     */
    bool isPythonAvailable() const;

    //=========================================================================================================
    /**
     * Query the version string of the configured Python interpreter.
     *
     * @return Version string (e.g. "3.11.5"), or empty if unavailable.
     */
    QString pythonVersion() const;

    //=========================================================================================================
    /**
     * Check whether a Python package is importable.
     *
     * @param[in] packageName    Package name (e.g. "torch", "mne").
     *
     * @return True if the import succeeds.
     */
    bool isPackageAvailable(const QString& packageName) const;

    //=========================================================================================================
    /**
     * Ensure a virtual environment exists, is up-to-date, and has all
     * required packages installed.
     *
     * Workflow:
     *  1. If config().venvDir is empty, returns immediately (no-op).
     *  2. Creates the venv if it doesn't exist yet (`python -m venv ...`).
     *  3. Installs dependencies:
     *     - If config().packageDir is set (contains pyproject.toml):
     *       runs `pip install <packageDir>` inside the venv.
     *     - Else if config().requirementsFile is set:
     *       runs `pip install -r <file>` inside the venv.
     *  4. On success, updates the internal config so that subsequent
     *     run()/runCode() calls use the venv's Python interpreter.
     *
     * @return PythonRunnerResult describing the venv setup outcome.
     */
    PythonRunnerResult ensureVenv();

    //=========================================================================================================
    /**
     * Convenience: ensure venv, then run a script inside it.
     *
     * Equivalent to calling ensureVenv() + run(), but returns early with
     * an error result if venv setup fails.
     *
     * @param[in] scriptPath     Path to the .py file.
     * @param[in] args           Arguments forwarded to the script.
     *
     * @return PythonRunnerResult.
     */
    PythonRunnerResult runInVenv(const QString& scriptPath,
                                 const QStringList& args = {});

    //=========================================================================================================
    /**
     * Get the Python interpreter path inside the configured venv.
     *
     * @return Absolute path to venv python, or empty if no venv configured.
     */
    QString venvPythonPath() const;

signals:
    //=========================================================================================================
    /**
     * Emitted for every stdout/stderr line (mirrors the line callback).
     *
     * @param[in] channel    0 = stdout, 1 = stderr.
     * @param[in] line       Text line.
     */
    void lineReceived(int channel, const QString& line);

    //=========================================================================================================
    /**
     * Emitted when a progress line is parsed (mirrors the progress callback).
     *
     * @param[in] pct     Progress 0–100.
     * @param[in] msg     Optional message.
     */
    void progressUpdated(float pct, const QString& msg);

    //=========================================================================================================
    /**
     * Emitted when the script finishes.
     *
     * @param[in] result  Execution result.
     */
    void finished(const UTILSLIB::PythonRunnerResult& result);

private:
    PythonRunnerConfig      m_config;           /**< Current configuration.   */
    PythonLineCallback      m_lineCb;           /**< Optional line callback.  */
    PythonProgressCallback  m_progressCb;       /**< Optional progress callback. */

    //=========================================================================================================
    /**
     * Internal: launch process with assembled argument list.
     */
    PythonRunnerResult execute(const QStringList& fullArgs);

    //=========================================================================================================
    /**
     * Internal: parse a line for progress protocol.
     *
     * @return True if the line matched.
     */
    bool parseProgressLine(const QString& line, float& pct, QString& msg) const;
};

} // namespace UTILSLIB

#endif // PYTHON_RUNNER_H
