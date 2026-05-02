//=============================================================================================================
/**
 * @file     ml_trainer.h
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
 * @brief    MLTrainer class declaration — ML-specific convenience wrapper over PythonRunner.
 *
 */

#ifndef ML_TRAINER_H
#define ML_TRAINER_H

#ifndef WASMBUILD // QProcess (used by PythonRunner) is not available in Qt WASM

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_global.h"

#include <utils/python_runner.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE MLLIB
//=============================================================================================================

namespace MLLIB
{

//=============================================================================================================
/**
 * ML-specific convenience wrapper over UTILSLIB::PythonRunner.
 *
 * Adds:
 * - Automatic prerequisite checking (Python + required packages).
 * - Default scripts directory resolution (scripts/ml/training/).
 * - Forwards line/progress callbacks to PythonRunner.
 *
 * Training logic lives entirely in Python (PyTorch, scikit-learn, …).
 * No Python embedding or linkage is required.
 *
 * @brief ML training script launcher.
 */
class MLSHARED_EXPORT MLTrainer
{
public:
    //=========================================================================================================
    /**
     * Construct an MLTrainer with default PythonRunner configuration.
     */
    MLTrainer();

    //=========================================================================================================
    /**
     * Construct an MLTrainer with explicit PythonRunner configuration.
     *
     * @param[in] config     PythonRunner configuration to use.
     */
    explicit MLTrainer(const UTILSLIB::PythonRunnerConfig& config);

    //=========================================================================================================
    /**
     * Access the underlying PythonRunner for callback/config changes.
     *
     * @return Reference to the PythonRunner instance.
     */
    UTILSLIB::PythonRunner& runner();

    //=========================================================================================================
    /**
     * Run a training script.
     *
     * If the PythonRunner is configured with a venvDir and packageDir,
     * the venv is created/updated automatically before the script runs.
     *
     * @param[in] scriptPath     Path to the .py training script.
     * @param[in] args           Arguments forwarded to the script.
     *
     * @return PythonRunnerResult with exit code, output, and progress.
     */
    UTILSLIB::PythonRunnerResult run(const QString& scriptPath,
                                     const QStringList& args = {});

    //=========================================================================================================
    /**
     * Check that the required Python packages are importable.
     *
     * @param[in] packages   List of package names (e.g. {"torch", "mne"}).
     *
     * @return List of packages that could NOT be imported (empty = all OK).
     */
    QStringList checkPrerequisites(const QStringList& packages) const;

private:
    UTILSLIB::PythonRunner m_runner;    /**< Underlying Python process launcher. */
};

} // namespace MLLIB

#endif // WASMBUILD

#endif // ML_TRAINER_H
