//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ml_trainer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    @ref MLLIB::MLTrainer convenience wrapper that drives Python training scripts via @ref UTILSLIB::PythonRunner.
 *
 * Inference in mne-cpp goes through ONNX Runtime, but training itself
 * stays in Python where the ecosystem lives (PyTorch, scikit-learn,
 * MNE-Python). @ref MLLIB::MLTrainer is the thin C++ side that
 * launches those scripts as child processes, optionally inside a
 * managed virtual environment, without embedding a Python interpreter
 * or linking against @c libpython.
 *
 * The class adds three things on top of the generic
 * @ref UTILSLIB::PythonRunner: prerequisite checking via
 * @c isPackageAvailable so callers can fail fast with a useful message,
 * automatic delegation to @c runInVenv when a @c venvDir is configured
 * (which creates and updates the venv before the script runs), and
 * convention-friendly defaults that match the @c scripts/ml/training/
 * layout shipped with the repository. The whole class is compiled
 * out under @c WASMBUILD because @c QProcess is not available in the
 * Qt-for-WASM port.
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
 * @brief Launches Python training scripts via @ref UTILSLIB::PythonRunner with automatic venv handling and prerequisite checks.
 *
 * Training logic (PyTorch, scikit-learn, MNE-Python, …) lives entirely
 * in the @c .py scripts under @c scripts/ml/training/ and runs in a
 * child process; no Python embedding or @c libpython linkage is
 * required. If the configured @ref UTILSLIB::PythonRunnerConfig
 * carries a @c venvDir, the runner creates and updates the venv before
 * each script invocation so dependencies are pinned per training job.
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
