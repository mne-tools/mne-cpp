//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file ml_trainer.cpp
 * @since 2026
 * @date  May 2026
 * @brief Implementation of @ref MLLIB::MLTrainer that dispatches training scripts through @ref UTILSLIB::PythonRunner.
 *
 * The class is a few-line orchestrator on top of
 * @ref UTILSLIB::PythonRunner: @c run first checks that a Python
 * interpreter is reachable (returning an error-populated
 * @ref UTILSLIB::PythonRunnerResult if not), then branches on whether
 * a @c venvDir is configured. With a venv it calls @c runInVenv which
 * creates / refreshes the virtual environment and installs the
 * required packages before launching the script; without a venv it
 * forwards directly to @c run and uses whatever interpreter the
 * caller's configuration points at.
 *
 * @ref MLLIB::MLTrainer::checkPrerequisites iterates the caller's
 * package list through @c isPackageAvailable and returns the missing
 * ones so the UI can surface a single "install these to enable
 * training" message instead of failing mid-script. The whole TU is
 * gated by @c WASMBUILD because @c QProcess is not part of Qt-for-WASM.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_trainer.h"

#ifndef WASMBUILD // QProcess (used by PythonRunner) is not available in Qt WASM

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MLTrainer::MLTrainer()
    : m_runner()
{
}

//=============================================================================================================

MLTrainer::MLTrainer(const PythonRunnerConfig& config)
    : m_runner(config)
{
}

//=============================================================================================================

PythonRunner& MLTrainer::runner()
{
    return m_runner;
}

//=============================================================================================================

PythonRunnerResult MLTrainer::run(const QString& scriptPath,
                                   const QStringList& args)
{
    if (!m_runner.isPythonAvailable()) {
        PythonRunnerResult result;
        result.stdErr = QStringLiteral("Python interpreter not found: ") + m_runner.config().pythonExe;
        qWarning() << "[MLTrainer]" << result.stdErr;
        return result;
    }

    qDebug() << "[MLTrainer] Running training script:" << scriptPath;

    // If a venv is configured, use runInVenv (creates venv + installs deps automatically)
    if (!m_runner.config().venvDir.isEmpty()) {
        return m_runner.runInVenv(scriptPath, args);
    }

    return m_runner.run(scriptPath, args);
}

//=============================================================================================================

QStringList MLTrainer::checkPrerequisites(const QStringList& packages) const
{
    QStringList missing;
    for (const QString& pkg : packages) {
        if (!m_runner.isPackageAvailable(pkg)) {
            missing << pkg;
        }
    }
    if (!missing.isEmpty()) {
        qWarning() << "[MLTrainer] Missing Python packages:" << missing.join(QStringLiteral(", "));
    }
    return missing;
}

#endif // WASMBUILD
