//=============================================================================================================
/**
 * @file     ml_trainer.cpp
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
 * @brief    MLTrainer class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_trainer.h"

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
