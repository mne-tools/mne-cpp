//=============================================================================================================
/**
 * @file     python_runner.cpp
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
 * @brief    PythonRunner class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "python_runner.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QProcess>
#include <QDebug>
#include <QRegularExpression>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QElapsedTimer>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PythonRunner::PythonRunner(QObject* pParent)
    : QObject(pParent)
{
}

//=============================================================================================================

PythonRunner::PythonRunner(const PythonRunnerConfig& config, QObject* pParent)
    : QObject(pParent)
    , m_config(config)
{
}

//=============================================================================================================

void PythonRunner::setConfig(const PythonRunnerConfig& config)
{
    m_config = config;
}

//=============================================================================================================

const PythonRunnerConfig& PythonRunner::config() const
{
    return m_config;
}

//=============================================================================================================

void PythonRunner::setLineCallback(PythonLineCallback cb)
{
    m_lineCb = std::move(cb);
}

//=============================================================================================================

void PythonRunner::setProgressCallback(PythonProgressCallback cb)
{
    m_progressCb = std::move(cb);
}

//=============================================================================================================

PythonRunnerResult PythonRunner::run(const QString& scriptPath,
                                     const QStringList& args)
{
    QStringList fullArgs;
    if (m_config.unbuffered) {
        fullArgs << QStringLiteral("-u");
    }
    fullArgs << scriptPath << args;

    return execute(fullArgs);
}

//=============================================================================================================

PythonRunnerResult PythonRunner::runCode(const QString& code,
                                          const QStringList& args)
{
    QStringList fullArgs;
    if (m_config.unbuffered) {
        fullArgs << QStringLiteral("-u");
    }
    fullArgs << QStringLiteral("-c") << code;
    fullArgs << args;

    return execute(fullArgs);
}

//=============================================================================================================

bool PythonRunner::isPythonAvailable() const
{
    QProcess proc;
    proc.start(m_config.pythonExe, {QStringLiteral("--version")});
    return proc.waitForFinished(5000) &&
           proc.exitStatus() == QProcess::NormalExit &&
           proc.exitCode() == 0;
}

//=============================================================================================================

QString PythonRunner::pythonVersion() const
{
    QProcess proc;
    proc.start(m_config.pythonExe, {QStringLiteral("--version")});
    if (!proc.waitForFinished(5000) || proc.exitCode() != 0) {
        return {};
    }
    // "Python 3.11.5\n" → "3.11.5"
    QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    if (out.startsWith(QStringLiteral("Python "))) {
        return out.mid(7);
    }
    return out;
}

//=============================================================================================================

bool PythonRunner::isPackageAvailable(const QString& packageName) const
{
    // Sanitize: only allow valid Python identifiers to prevent injection
    static const QRegularExpression validPkg(QStringLiteral("^[A-Za-z_][A-Za-z0-9_.]*$"));
    if (!validPkg.match(packageName).hasMatch()) {
        qWarning() << "[PythonRunner] Invalid package name:" << packageName;
        return false;
    }

    QProcess proc;
    proc.start(m_config.pythonExe,
               {QStringLiteral("-c"),
                QStringLiteral("import %1").arg(packageName)});
    return proc.waitForFinished(10000) &&
           proc.exitStatus() == QProcess::NormalExit &&
           proc.exitCode() == 0;
}

//=============================================================================================================

QString PythonRunner::venvPythonPath() const
{
    if (m_config.venvDir.isEmpty()) {
        return {};
    }
#ifdef Q_OS_WIN
    return QDir(m_config.venvDir).absoluteFilePath(QStringLiteral("Scripts/python.exe"));
#else
    return QDir(m_config.venvDir).absoluteFilePath(QStringLiteral("bin/python"));
#endif
}

//=============================================================================================================

PythonRunnerResult PythonRunner::ensureVenv()
{
    PythonRunnerResult result;

    if (m_config.venvDir.isEmpty()) {
        result.success = true;
        return result;
    }

    QString venvPython = venvPythonPath();
    bool venvExists = QFileInfo::exists(venvPython);

    // Step 1: Create venv if it doesn't exist
    if (!venvExists) {
        qDebug() << "[PythonRunner] Creating virtual environment at:" << m_config.venvDir;

        QProcess venvProc;
        venvProc.start(m_config.pythonExe,
                       {QStringLiteral("-m"), QStringLiteral("venv"), m_config.venvDir});

        if (!venvProc.waitForStarted(10000)) {
            result.stdErr = QStringLiteral("Failed to start venv creation: ") + venvProc.errorString();
            qWarning() << "[PythonRunner]" << result.stdErr;
            return result;
        }

        if (!venvProc.waitForFinished(120000)) {  // 2 min timeout for venv creation
            result.stdErr = QStringLiteral("Venv creation timed out.");
            venvProc.kill();
            venvProc.waitForFinished(5000);
            qWarning() << "[PythonRunner]" << result.stdErr;
            return result;
        }

        if (venvProc.exitCode() != 0) {
            result.exitCode = venvProc.exitCode();
            result.stdErr = QString::fromUtf8(venvProc.readAllStandardError());
            qWarning() << "[PythonRunner] venv creation failed:" << result.stdErr;
            return result;
        }

        qDebug() << "[PythonRunner] Virtual environment created.";
    } else {
        qDebug() << "[PythonRunner] Virtual environment already exists at:" << m_config.venvDir;
    }

    // Step 2: Install dependencies
    // Prefer pyproject.toml (packageDir) over requirements.txt
    bool needsInstall = false;
    QStringList pipArgs;

#ifdef Q_OS_WIN
    QString pipExe = QDir(m_config.venvDir).absoluteFilePath(QStringLiteral("Scripts/pip.exe"));
#else
    QString pipExe = QDir(m_config.venvDir).absoluteFilePath(QStringLiteral("bin/pip"));
#endif

    if (!m_config.packageDir.isEmpty()) {
        // Check for pyproject.toml
        QString tomlPath = QDir(m_config.packageDir).absoluteFilePath(QStringLiteral("pyproject.toml"));
        if (QFileInfo::exists(tomlPath)) {
            pipArgs << QStringLiteral("install") << m_config.packageDir;
            needsInstall = true;
            qDebug() << "[PythonRunner] Installing from pyproject.toml in:" << m_config.packageDir;
        } else {
            qWarning() << "[PythonRunner] packageDir set but no pyproject.toml found at:" << tomlPath;
        }
    }

    if (!needsInstall && !m_config.requirementsFile.isEmpty()) {
        if (QFileInfo::exists(m_config.requirementsFile)) {
            pipArgs << QStringLiteral("install")
                    << QStringLiteral("-r") << m_config.requirementsFile;
            needsInstall = true;
            qDebug() << "[PythonRunner] Installing from requirements.txt:" << m_config.requirementsFile;
        } else {
            qWarning() << "[PythonRunner] requirementsFile not found:" << m_config.requirementsFile;
        }
    }

    if (needsInstall) {
        QProcess pipProc;
        pipProc.start(pipExe, pipArgs);

        if (!pipProc.waitForStarted(10000)) {
            result.stdErr = QStringLiteral("Failed to start pip: ") + pipProc.errorString();
            qWarning() << "[PythonRunner]" << result.stdErr;
            return result;
        }

        // pip install can take a while (torch alone is ~2 GB)
        if (!pipProc.waitForFinished(1800000)) {  // 30 min timeout
            result.stdErr = QStringLiteral("pip install timed out.");
            result.timedOut = true;
            pipProc.kill();
            pipProc.waitForFinished(5000);
            qWarning() << "[PythonRunner]" << result.stdErr;
            return result;
        }

        result.stdOut = QString::fromUtf8(pipProc.readAllStandardOutput());
        result.stdErr = QString::fromUtf8(pipProc.readAllStandardError());
        result.exitCode = pipProc.exitCode();

        if (pipProc.exitCode() != 0) {
            qWarning() << "[PythonRunner] pip install failed with code" << pipProc.exitCode();
            qWarning() << "[PythonRunner] stderr:" << result.stdErr;
            return result;
        }

        qDebug() << "[PythonRunner] Dependencies installed successfully.";
    }

    // Step 3: Switch interpreter to the venv Python
    m_config.pythonExe = venvPython;
    result.success = true;
    result.exitCode = 0;

    qDebug() << "[PythonRunner] Using venv Python:" << m_config.pythonExe;
    return result;
}

//=============================================================================================================

PythonRunnerResult PythonRunner::runInVenv(const QString& scriptPath,
                                            const QStringList& args)
{
    // Ensure venv is set up
    PythonRunnerResult setupResult = ensureVenv();
    if (!setupResult.success) {
        return setupResult;
    }

    // Now run the script with the venv Python
    return run(scriptPath, args);
}

//=============================================================================================================

PythonRunnerResult PythonRunner::execute(const QStringList& fullArgs)
{
    PythonRunnerResult result;

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);

    // Working directory
    if (!m_config.workingDir.isEmpty()) {
        process.setWorkingDirectory(m_config.workingDir);
    }

    // Extra environment variables
    if (!m_config.extraEnv.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        for (const QString& kv : m_config.extraEnv) {
            int idx = kv.indexOf(QLatin1Char('='));
            if (idx > 0) {
                env.insert(kv.left(idx), kv.mid(idx + 1));
            }
        }
        process.setProcessEnvironment(env);
    }

    qDebug() << "[PythonRunner] Launching:" << m_config.pythonExe << fullArgs.join(QLatin1Char(' '));

    process.start(m_config.pythonExe, fullArgs);

    if (!process.waitForStarted(10000)) {
        result.stdErr = QStringLiteral("Failed to start Python process: ") + process.errorString();
        qWarning() << "[PythonRunner]" << result.stdErr;
        return result;
    }

    // Read output line-by-line until process finishes
    QByteArray stdOutBuf, stdErrBuf;
    QStringList stdOutLines, stdErrLines;

    auto processLines = [&](QByteArray& buffer, int channel) {
        while (buffer.contains('\n')) {
            int idx = buffer.indexOf('\n');
            QString line = QString::fromUtf8(buffer.left(idx));
            buffer.remove(0, idx + 1);

            // Accumulate full output
            if (channel == 0) stdOutLines << line;
            else              stdErrLines << line;

            // Dispatch to callback
            if (m_lineCb) {
                m_lineCb(channel, line);
            }
            emit lineReceived(channel, line);

            // Check for progress protocol
            float pct = 0.0f;
            QString msg;
            if (parseProgressLine(line, pct, msg)) {
                result.progressPct = pct;
                if (m_progressCb) {
                    m_progressCb(pct, msg);
                }
                emit progressUpdated(pct, msg);
            }

            // Log to Qt debug system
            if (channel == 0) {
                qDebug() << "[Python stdout]" << line;
            } else {
                qWarning() << "[Python stderr]" << line;
            }
        }
    };

    // Poll loop with proper elapsed-time tracking
    QElapsedTimer elapsed;
    elapsed.start();

    while (!process.waitForFinished(100)) {
        stdOutBuf.append(process.readAllStandardOutput());
        stdErrBuf.append(process.readAllStandardError());
        processLines(stdOutBuf, 0);
        processLines(stdErrBuf, 1);

        // Check timeout
        if (m_config.timeoutMsec > 0 && elapsed.elapsed() >= m_config.timeoutMsec) {
            // Flush remaining data
            stdOutBuf.append(process.readAllStandardOutput());
            stdErrBuf.append(process.readAllStandardError());
            processLines(stdOutBuf, 0);
            processLines(stdErrBuf, 1);

            result.timedOut = true;
            result.stdErr += QStringLiteral("\nProcess timed out after %1 ms.").arg(m_config.timeoutMsec);
            process.kill();
            process.waitForFinished(5000);
            qWarning() << "[PythonRunner] Process timed out after" << m_config.timeoutMsec << "ms.";
            break;
        }
    }

    // Flush any remaining data
    stdOutBuf.append(process.readAllStandardOutput());
    stdErrBuf.append(process.readAllStandardError());
    // Process remaining complete lines
    processLines(stdOutBuf, 0);
    processLines(stdErrBuf, 1);
    // Handle trailing data without newline
    if (!stdOutBuf.isEmpty()) {
        QString line = QString::fromUtf8(stdOutBuf);
        stdOutLines << line;
        if (m_lineCb) m_lineCb(0, line);
        emit lineReceived(0, line);
    }
    if (!stdErrBuf.isEmpty()) {
        QString line = QString::fromUtf8(stdErrBuf);
        stdErrLines << line;
        if (m_lineCb) m_lineCb(1, line);
        emit lineReceived(1, line);
    }

    // Assemble full output from accumulated lines
    result.stdOut  = stdOutLines.join(QLatin1Char('\n'));
    result.stdErr += stdErrLines.join(QLatin1Char('\n'));

    result.exitCode = process.exitCode();
    result.success  = (!result.timedOut &&
                       process.exitStatus() == QProcess::NormalExit &&
                       result.exitCode == 0);

    if (result.success) {
        qDebug() << "[PythonRunner] Script finished successfully.";
    } else if (!result.timedOut) {
        qWarning() << "[PythonRunner] Script exited with code" << result.exitCode;
    }

    emit finished(result);

    return result;
}

//=============================================================================================================

bool PythonRunner::parseProgressLine(const QString& line,
                                      float& pct,
                                      QString& msg) const
{
    // Match: [progress] 42.5%  or  [progress] 42.5% Training epoch 10/50
    static const QRegularExpression re(
        QStringLiteral(R"(^\[progress\]\s+(\d+(?:\.\d+)?)\s*%\s*(.*)?$)"),
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatch match = re.match(line.trimmed());
    if (!match.hasMatch()) {
        return false;
    }

    bool ok = false;
    pct = match.captured(1).toFloat(&ok);
    if (!ok) {
        return false;
    }
    msg = match.captured(2).trimmed();
    return true;
}
