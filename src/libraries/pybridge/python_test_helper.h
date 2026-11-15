//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     python_test_helper.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Test convenience layer on top of @ref mne::PythonRunner for cross-validating MNE-CPP results against MNE-Python.
 *
 * Built on @ref mne::PythonRunner, this helper adds what test code needs: an @c is_available() probe,
 * @c has_package() lookups so a test can announce that it needs e.g. @c sklearn, and @c eval shorthands that run
 * a snippet and parse its stdout into a number, vector or matrix.
 *
 * The companion @c GUARD_PYTHON / @c GUARD_PYTHON_PACKAGE macros @c QSKIP a test cleanly when Python is missing on
 * developer machines but escalate to @c QFAIL when @c MNE_REQUIRE_PYTHON=true, so CI runs can never silently skip
 * cross-language regression coverage. They expand to QtTest macros and are meant for QtTest test cases only.
 */

#ifndef PYBRIDGELIB_PYTHON_TEST_HELPER_H
#define PYBRIDGELIB_PYTHON_TEST_HELPER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pybridge_global.h"
#include "python_runner.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

//=============================================================================================================
// DEFINE MACROS
//=============================================================================================================

/**
 * @brief Skips (or fails) the current QtTest slot if a condition does not hold.
 *
 * Without @c MNE_REQUIRE_PYTHON the slot is skipped with a clear message; with @c MNE_REQUIRE_PYTHON=true it fails,
 * so a missing Python installation is never silently ignored in CI.
 *
 * @param available  Condition, e.g. @c helper.is_available().
 * @param reason     Message, anything a @c std::string can be constructed from.
 */
#define GUARD_PYTHON(available, reason)                                                  \
    do {                                                                                 \
        if (!(available)) {                                                              \
            const std::string guardReason_(reason);                                      \
            if (mne::PythonTestHelper::is_python_required())                             \
                QFAIL(("HARD FAIL (MNE_REQUIRE_PYTHON=true): " + guardReason_).c_str()); \
            QSKIP(("SKIP: " + guardReason_).c_str());                                    \
        }                                                                                \
    } while (0)

/**
 * @brief Skips (or fails) the current QtTest slot if a Python package is missing.
 *
 * @param helper       A @ref mne::PythonTestHelper.
 * @param packageName  Module name, e.g. "sklearn".
 */
#define GUARD_PYTHON_PACKAGE(helper, packageName) \
    GUARD_PYTHON((helper).has_package(packageName), std::string("Python package '") + (packageName) + "' not available")

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Test-frame convenience class for cross-validating MNE-CPP outputs against MNE-Python reference implementations.
 *
 * @code
 *   PythonTestHelper py;
 *   GUARD_PYTHON(py.is_available(), "MNE-Python not available");
 *
 *   // Run a Python snippet that prints a reference value
 *   const std::optional<double> norm = py.eval_double("import numpy as np; print(np.linalg.norm(np.ones(10)))");
 *   QVERIFY(norm && std::abs(*norm - cppNorm) < 1e-12);
 * @endcode
 *
 * Each @c eval call honours its timeout; a run that exceeds it is killed and reported as failed.
 *
 * @brief Test helper for MNE-Python cross-validation.
 *
 * @snippet ex_pybridge/main.cpp python_test_helper_usage
 */
class PYBRIDGESHARED_EXPORT PythonTestHelper
{
public:
    //=========================================================================================================
    /**
     * @return True if Python is available and MNE-Python can be imported.
     */
    bool is_available() const;

    //=========================================================================================================
    /**
     * @return True if the Python interpreter can be started.
     */
    bool is_python_available() const;

    //=========================================================================================================
    /**
     * @param[in] package    Module name, e.g. "numpy", "scipy" or "mne".
     *
     * @return True if the package can be imported.
     */
    bool has_package(std::string_view package) const;

    //=========================================================================================================
    /**
     * Runs Python code.
     *
     * @param[in] code       Python source executed with @c python -c.
     * @param[in] timeout    Wall-clock limit.
     *
     * @return Exit code and captured output.
     */
    PythonRunnerResult eval(std::string_view code, std::chrono::milliseconds timeout = std::chrono::seconds(30)) const;

    //=========================================================================================================
    /**
     * Runs Python code that prints a single number.
     *
     * @param[in] code       Python source whose stdout is one number.
     * @param[in] timeout    Wall-clock limit.
     *
     * @return The number, or nothing if the run failed or printed something else.
     */
    std::optional<double> eval_double(std::string_view code, std::chrono::milliseconds timeout = std::chrono::seconds(30)) const;

    //=========================================================================================================
    /**
     * Runs Python code that prints numbers, one per line or whitespace-separated, all read into one vector.
     *
     * @param[in] code       Python source whose stdout is numeric.
     * @param[in] timeout    Wall-clock limit.
     *
     * @return The values, or nothing if the run failed, printed nothing or printed a non-numeric value.
     */
    std::optional<Eigen::VectorXd> eval_vector(std::string_view code, std::chrono::milliseconds timeout = std::chrono::seconds(60)) const;

    //=========================================================================================================
    /**
     * Runs Python code that prints a matrix, one row per line with whitespace-separated values.
     *
     * @param[in] code       Python source whose stdout is a matrix.
     * @param[in] timeout    Wall-clock limit.
     *
     * @return The matrix, or nothing if the run failed, printed nothing or printed ragged or non-numeric rows.
     */
    std::optional<Eigen::MatrixXd> eval_matrix(std::string_view code, std::chrono::milliseconds timeout = std::chrono::seconds(60)) const;

    //=========================================================================================================
    /**
     * Runs Python code that writes a matrix to @p file (e.g. with @c numpy.savetxt) and reads it back. Preferred
     * over @ref eval_matrix for large matrices, since nothing is parsed from stdout.
     *
     * @param[in] code       Python source that writes @p file.
     * @param[in] file       File the code writes; read with mne::read_matrix.
     * @param[in] timeout    Wall-clock limit.
     *
     * @return The matrix, or nothing if the run failed or the file is missing, empty or malformed.
     */
    std::optional<Eigen::MatrixXd> eval_matrix_via_file(std::string_view code,
                                                        const std::filesystem::path& file,
                                                        std::chrono::milliseconds timeout = std::chrono::seconds(120)) const;

    //=========================================================================================================
    /**
     * Runs a Python script.
     *
     * @param[in] script     Path of the .py file.
     * @param[in] args       Arguments passed to the script.
     * @param[in] timeout    Wall-clock limit.
     *
     * @return Exit code and captured output.
     */
    PythonRunnerResult run_script(const std::filesystem::path& script,
                                  const mne::StringList& args = {},
                                  std::chrono::milliseconds timeout = std::chrono::seconds(120)) const;

    //=========================================================================================================
    /**
     * @return The mne-cpp-test-data directory relative to the running test binary.
     */
    static std::filesystem::path test_data_path();

    //=========================================================================================================
    /**
     * @return True if the environment variable @c MNE_REQUIRE_PYTHON is "true" or "1", i.e. missing Python must
     *         fail tests instead of skipping them.
     */
    static bool is_python_required();

private:
    PythonRunner runner(std::chrono::milliseconds timeout) const;

    PythonRunner m_runner; /**< Probes availability; copied with a timeout for each run. */
};

} // namespace mne

#endif // PYBRIDGELIB_PYTHON_TEST_HELPER_H
