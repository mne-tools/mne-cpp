//=============================================================================================================
/**
 * @file     python_test_helper.h
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
 * @brief    PythonTestHelper class declaration — test-frame convenience for MNE-Python cross-validation.
 *
 */

#ifndef PYTHON_TEST_HELPER_H
#define PYTHON_TEST_HELPER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"
#include "python_runner.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * Test-frame convenience class for cross-validating MNE-CPP outputs against
 * MNE-Python reference implementations.
 *
 * Usage inside a QTest test:
 * @code
 *   PythonTestHelper py;
 *   if (!py.isAvailable()) {
 *       QSKIP("Python/MNE-Python not available");
 *   }
 *
 *   // Run a Python snippet that computes a reference value
 *   auto result = py.eval("import mne; print(mne.io.read_info('test.fif')['nchan'])");
 *   QVERIFY(result.success);
 *   QCOMPARE(result.stdOut.trimmed().toInt(), expectedNchan);
 *
 *   // Or run a script and parse a numeric output
 *   double pyValue = py.evalDouble("import numpy as np; print(np.linalg.norm(np.ones(10)))");
 *   QVERIFY(qAbs(cppValue - pyValue) < 1e-6);
 * @endcode
 *
 * When MNE-Python is not installed, tests should QSKIP gracefully — but
 * never silently.  Use the GUARD_PYTHON / GUARD_PYTHON_PACKAGE macros to
 * ensure skips are intentional and visible.  When the environment variable
 * MNE_REQUIRE_PYTHON=true is set (e.g. in CI), those macros QFAIL instead
 * of QSKIP, so a missing Python installation is treated as a hard error.
 *
 * @brief Test helper for MNE-Python cross-validation.
 */

//=============================================================================================================
/**
 * @brief Skip (or fail) the current test if Python is not available.
 *
 * Use this macro at the top of any test slot that requires Python.
 * - If MNE_REQUIRE_PYTHON is **not** set: QSKIP with a clear message.
 * - If MNE_REQUIRE_PYTHON=true: QFAIL so the skip is never silent in CI.
 *
 * @param helper   A PythonTestHelper (or bool) — must support operator bool / isAvailable().
 * @param reason   Human-readable reason string.
 */
#define GUARD_PYTHON(available, reason) \
    do { \
        if (!(available)) { \
            if (UTILSLIB::PythonTestHelper::isPythonRequired()) \
                QFAIL(qPrintable(QString("HARD FAIL (MNE_REQUIRE_PYTHON=true): %1").arg(reason))); \
            QSKIP(qPrintable(QString("SKIP: %1").arg(reason))); \
        } \
    } while (0)

/**
 * @brief Skip (or fail) the current test if a Python package is missing.
 *
 * @param helper       A PythonTestHelper instance.
 * @param packageName  Package name string (e.g. "sklearn").
 */
#define GUARD_PYTHON_PACKAGE(helper, packageName) \
    GUARD_PYTHON((helper).hasPackage(packageName), \
                 QString("Python package '%1' not available").arg(packageName))

class UTILSSHARED_EXPORT PythonTestHelper
{
public:
    //=========================================================================================================
    /**
     * Constructs a PythonTestHelper with auto-detected Python.
     */
    PythonTestHelper();

    //=========================================================================================================
    /**
     * Check if Python is available and MNE-Python is importable.
     *
     * @return True if both python3 and the 'mne' package are available.
     */
    bool isAvailable() const;

    //=========================================================================================================
    /**
     * Check if Python is available (without checking for mne package).
     *
     * @return True if python3 is reachable.
     */
    bool isPythonAvailable() const;

    //=========================================================================================================
    /**
     * Check if a specific Python package is importable.
     *
     * @param[in] packageName    Package name (e.g. "numpy", "scipy", "mne").
     *
     * @return True if the import succeeds.
     */
    bool hasPackage(const QString& packageName) const;

    //=========================================================================================================
    /**
     * Run inline Python code and return the result.
     *
     * @param[in] code       Python code to execute via `python -c`.
     * @param[in] timeoutMs  Timeout in milliseconds (-1 = no limit).
     *
     * @return PythonRunnerResult with captured stdout/stderr.
     */
    PythonRunnerResult eval(const QString& code, int timeoutMs = 30000) const;

    //=========================================================================================================
    /**
     * Run inline Python code that prints a single double value, and parse it.
     *
     * @param[in] code       Python code whose stdout is a single number.
     * @param[in] ok         Set to true on success, false on parse failure.
     * @param[in] timeoutMs  Timeout in milliseconds.
     *
     * @return The parsed double value, or 0.0 on failure.
     */
    double evalDouble(const QString& code, bool* ok = nullptr, int timeoutMs = 30000) const;

    //=========================================================================================================
    /**
     * Run inline Python code that prints a flat array of doubles (one per line
     * or space-separated), and parse it into an Eigen VectorXd.
     *
     * @param[in] code       Python code whose stdout is numeric values.
     * @param[in] ok         Set to true on success.
     * @param[in] timeoutMs  Timeout in milliseconds.
     *
     * @return Eigen VectorXd, or empty vector on failure.
     */
    Eigen::VectorXd evalVector(const QString& code, bool* ok = nullptr, int timeoutMs = 60000) const;

    //=========================================================================================================
    /**
     * Run inline Python code that prints a matrix (one row per line,
     * space-separated values), and parse it into an Eigen MatrixXd.
     *
     * @param[in] code       Python code whose stdout is a space-separated matrix.
     * @param[in] ok         Set to true on success.
     * @param[in] timeoutMs  Timeout in milliseconds.
     *
     * @return Eigen MatrixXd, or empty matrix on failure.
     */
    Eigen::MatrixXd evalMatrix(const QString& code, bool* ok = nullptr, int timeoutMs = 60000) const;

    //=========================================================================================================
    /**
     * Run a Python script file and return the result.
     *
     * @param[in] scriptPath  Path to the .py file.
     * @param[in] args        Arguments forwarded to the script.
     * @param[in] timeoutMs   Timeout in milliseconds.
     *
     * @return PythonRunnerResult.
     */
    PythonRunnerResult runScript(const QString& scriptPath,
                                 const QStringList& args = {},
                                 int timeoutMs = 120000) const;

    //=========================================================================================================
    /**
     * Get the standard path to mne-cpp-test-data from the test binary location.
     *
     * @return Absolute path to the test data directory.
     */
    static QString testDataPath();

    //=========================================================================================================
    /**
     * Check whether the environment demands Python availability.
     *
     * When MNE_REQUIRE_PYTHON=true is set, test guards should QFAIL instead
     * of QSKIP, ensuring skips are never silent in CI.
     *
     * @return True if MNE_REQUIRE_PYTHON environment variable is "true" (or "1").
     */
    static bool isPythonRequired();

    //=========================================================================================================
    /**
     * Write an Eigen matrix to a text file with full double precision.
     * The format is one row per line, space-separated values, using %.17e
     * notation — compatible with Python's numpy.loadtxt().
     *
     * @param[in] filePath   Output file path.
     * @param[in] mat        Matrix to write.
     *
     * @return True on success.
     */
    static bool writeMatrix(const QString& filePath, const Eigen::MatrixXd& mat);

    //=========================================================================================================
    /**
     * Read an Eigen matrix from a text file (space-separated values,
     * one row per line). Compatible with numpy.savetxt() output.
     *
     * @param[in] filePath   Input file path.
     * @param[out] ok        Set to true on success, false on failure.
     *
     * @return Parsed MatrixXd, or empty matrix on failure.
     */
    static Eigen::MatrixXd readMatrix(const QString& filePath, bool* ok = nullptr);

    //=========================================================================================================
    /**
     * Run Python code that writes a matrix to outputFilePath
     * (e.g. via numpy.savetxt), then read the result back as an Eigen MatrixXd.
     *
     * This is preferred over evalMatrix() for large matrices or when full
     * double precision is required, because it avoids stdout parsing.
     *
     * Example:
     * @code
     *   QString code = QString(
     *       "import numpy as np\n"
     *       "cov = np.eye(3)\n"
     *       "np.savetxt('%1', cov, fmt='%%.17e')\n"
     *   ).arg(outPath);
     *   bool ok;
     *   auto mat = helper.evalMatrixViaFile(code, outPath, &ok);
     * @endcode
     *
     * @param[in] code             Python code to execute.
     * @param[in] outputFilePath   Path where Python writes the matrix.
     * @param[out] ok              Set to true on success.
     * @param[in] timeoutMs        Timeout in milliseconds.
     *
     * @return Parsed MatrixXd, or empty matrix on failure.
     */
    Eigen::MatrixXd evalMatrixViaFile(const QString& code,
                                       const QString& outputFilePath,
                                       bool* ok = nullptr,
                                       int timeoutMs = 120000) const;

private:
    mutable PythonRunner m_runner;  /**< Internal PythonRunner instance. */
};

} // namespace UTILSLIB

#endif // PYTHON_TEST_HELPER_H
