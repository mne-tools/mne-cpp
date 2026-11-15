//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ml_trainer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref mne::MLTrainer that dispatches training scripts through @ref mne::PythonRunner.
 *
 * The class is a few-line orchestrator on top of
 * @ref mne::PythonRunner: @c run first checks that a Python
 * interpreter is reachable (returning an error-populated
 * @ref mne::PythonRunnerResult if not), then branches on whether
 * a @c venv_dir is configured. With a venv it calls @c run_in_venv which
 * creates / refreshes the virtual environment and installs the
 * required packages before launching the script; without a venv it
 * forwards directly to @c run and uses whatever interpreter the
 * caller's configuration points at.
 *
 * @ref mne::MLTrainer::check_prerequisites iterates the caller's
 * package list through @c is_package_available and returns the missing
 * ones so the UI can surface a single "install these to enable
 * training" message instead of failing mid-script. The whole TU is
 * gated by @c WASMBUILD because @c QProcess is not part of Qt-for-WASM.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_trainer.h"
#include <base/log.h>

#ifndef WASMBUILD // QProcess (used by PythonRunner) is not available in Qt WASM

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

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

PythonRunnerResult MLTrainer::run(const std::filesystem::path& scriptPath, const StringList& args)
{
    if (!m_runner.is_python_available()) {
        PythonRunnerResult result;
        result.std_err = "Python interpreter not found: " + m_runner.config().python_exe;
        Log::warning("[MLTrainer] {}", result.std_err);
        return result;
    }

    Log::debug("[MLTrainer] Running training script: {}", utf8FromPath(scriptPath));

    // If a venv is configured, use run_in_venv (creates venv + installs deps automatically)
    if (!m_runner.config().venv_dir.empty()) {
        return m_runner.run_in_venv(scriptPath, args);
    }

    return m_runner.run(scriptPath, args);
}

//=============================================================================================================

StringList MLTrainer::check_prerequisites(const StringList& packages) const
{
    StringList missing;
    for (const std::string& package : packages) {
        if (!m_runner.is_package_available(package)) {
            missing.push_back(package);
        }
    }
    if (!missing.empty()) {
        Log::warning("[MLTrainer] Missing Python packages: {}", join(missing, ", "));
    }
    return missing;
}

#endif // WASMBUILD
