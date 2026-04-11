//=============================================================================================================
/**
 * @file     test_python_runner.cpp
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
 * @brief    Tests for PythonRunner and PythonTestHelper classes.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/python_runner.h>
#include <utils/python_test_helper.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestPythonRunner
 *
 * @brief The TestPythonRunner class provides tests for PythonRunner and PythonTestHelper.
 */
class TestPythonRunner : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // PythonRunner tests
    void testIsPythonAvailable();
    void testPythonVersion();
    void testRunCodeHelloWorld();
    void testRunCodeStdErr();
    void testRunCodeExitCode();
    void testRunCodeTimeout();
    void testRunCodeMultiline();
    void testRunCodeEnvironmentVars();
    void testIsPackageAvailable();
    void testProgressParsing();

    // PythonTestHelper tests
    void testHelperIsAvailable();
    void testHelperEval();
    void testHelperEvalDouble();
    void testHelperEvalVector();
    void testHelperEvalMatrix();
    void testHelperTestDataPath();

    void cleanupTestCase();

private:
    bool m_bPythonAvailable = false;
};

//=============================================================================================================

void TestPythonRunner::initTestCase()
{
    PythonRunner runner;
    m_bPythonAvailable = runner.isPythonAvailable();
    if (!m_bPythonAvailable) {
        qWarning() << "Python not available - some tests will be skipped";
    }
}

//=============================================================================================================

void TestPythonRunner::testIsPythonAvailable()
{
    PythonRunner runner;
    // Just verify the method is callable and returns a bool
    bool available = runner.isPythonAvailable();
    Q_UNUSED(available);
    QVERIFY(true); // Method didn't crash
}

//=============================================================================================================

void TestPythonRunner::testPythonVersion()
{
    if (!m_bPythonAvailable) {
        QSKIP("Python not available");
    }

    PythonRunner runner;
    QString version = runner.pythonVersion();
    QVERIFY(!version.isEmpty());
    // Version should start with a digit (e.g., "3.10.12")
    QVERIFY(version[0].isDigit());
}

//=============================================================================================================

void TestPythonRunner::testRunCodeHelloWorld()
{
    if (!m_bPythonAvailable) {
        QSKIP("Python not available");
    }

    PythonRunner runner;
    PythonRunnerConfig config;
    config.timeoutMs = 10000;

    PythonRunnerResult result = runner.runCode("print('Hello World')", config);
    QVERIFY(result.success);
    QCOMPARE(result.exitCode, 0);
    QCOMPARE(result.stdOut.trimmed(), QString("Hello World"));
}

//=============================================================================================================

void TestPythonRunner::testRunCodeStdErr()
{
    if (!m_bPythonAvailable) {
        QSKIP("Python not available");
    }

    PythonRunner runner;
    PythonRunnerConfig config;
    config.timeoutMs = 10000;

    PythonRunnerResult result = runner.runCode(
        "import sys; sys.stderr.write('error msg\\n')", config);
    QVERIFY(result.success);
    QVERIFY(result.stdErr.contains("error msg"));
}

//=============================================================================================================

void TestPythonRunner::testRunCodeExitCode()
{
    if (!m_bPythonAvailable) {
        QSKIP("Python not available");
    }

    PythonRunner runner;
    PythonRunnerConfig config;
    config.timeoutMs = 10000;

    // Exit with non-zero code
    PythonRunnerResult result = runner.runCode("import sys; sys.exit(42)", config);
    QVERIFY(!result.success);
    QCOMPARE(result.exitCode, 42);
}

//=============================================================================================================

void TestPythonRunner::testRunCodeTimeout()
{
    if (!m_bPythonAvailable) {
        QSKIP("Python not available");
    }

    PythonRunner runner;
    PythonRunnerConfig config;
    config.timeoutMs = 500; // Very short timeout

    // This should time out
    PythonRunnerResult result = runner.runCode(
        "import time; time.sleep(30)", config);
    QVERIFY(!result.success);
    QVERIFY(result.timedOut);
}

//=============================================================================================================

void TestPythonRunner::testRunCodeMultiline()
{
    if (!m_bPythonAvailable) {
        QSKIP("Python not available");
    }

    PythonRunner runner;
    PythonRunnerConfig config;
    config.timeoutMs = 10000;

    QString code =
        "x = 2\n"
        "y = 3\n"
        "print(x * y)";

    PythonRunnerResult result = runner.runCode(code, config);
    QVERIFY(result.success);
    QCOMPARE(result.stdOut.trimmed(), QString("6"));
}

//=============================================================================================================

void TestPythonRunner::testRunCodeEnvironmentVars()
{
    if (!m_bPythonAvailable) {
        QSKIP("Python not available");
    }

    PythonRunner runner;
    PythonRunnerConfig config;
    config.timeoutMs = 10000;
    config.environment.insert("MNE_CPP_TEST_VAR", "test_value_42");

    PythonRunnerResult result = runner.runCode(
        "import os; print(os.environ.get('MNE_CPP_TEST_VAR', 'missing'))", config);
    QVERIFY(result.success);
    QCOMPARE(result.stdOut.trimmed(), QString("test_value_42"));
}

//=============================================================================================================

void TestPythonRunner::testIsPackageAvailable()
{
    if (!m_bPythonAvailable) {
        QSKIP("Python not available");
    }

    PythonRunner runner;

    // 'sys' is always available — it's a built-in module
    QVERIFY(runner.isPackageAvailable("sys"));

    // A definitely-not-installed package
    QVERIFY(!runner.isPackageAvailable("nonexistent_package_xyz_12345"));
}

//=============================================================================================================

void TestPythonRunner::testProgressParsing()
{
    if (!m_bPythonAvailable) {
        QSKIP("Python not available");
    }

    PythonRunner runner;
    PythonRunnerConfig config;
    config.timeoutMs = 10000;

    QList<double> progressValues;
    config.lineCallback = [&progressValues](const QString& line, bool /*isStdErr*/) {
        // The runner should parse progress lines like "[progress] 50.0%"
        Q_UNUSED(line);
    };
    config.progressCallback = [&progressValues](double pct) {
        progressValues.append(pct);
    };

    PythonRunnerResult result = runner.runCode(
        "print('[progress] 25.0%')\n"
        "print('[progress] 50.0%')\n"
        "print('[progress] 100.0%')\n"
        "print('done')", config);
    QVERIFY(result.success);

    // Verify progress values were parsed
    QVERIFY(progressValues.size() >= 3);
    QCOMPARE(progressValues[0], 25.0);
    QCOMPARE(progressValues[1], 50.0);
    QCOMPARE(progressValues[2], 100.0);
}

//=============================================================================================================

void TestPythonRunner::testHelperIsAvailable()
{
    PythonTestHelper helper;
    // Just check the method is callable
    bool available = helper.isPythonAvailable();
    Q_UNUSED(available);
    QVERIFY(true);
}

//=============================================================================================================

void TestPythonRunner::testHelperEval()
{
    PythonTestHelper helper;
    if (!helper.isPythonAvailable()) {
        QSKIP("Python not available");
    }

    PythonRunnerResult result = helper.eval("print(2 + 3)");
    QVERIFY(result.success);
    QCOMPARE(result.stdOut.trimmed(), QString("5"));
}

//=============================================================================================================

void TestPythonRunner::testHelperEvalDouble()
{
    PythonTestHelper helper;
    if (!helper.isPythonAvailable()) {
        QSKIP("Python not available");
    }

    bool ok = false;
    double value = helper.evalDouble("import math; print(math.pi)", &ok);
    QVERIFY(ok);
    QVERIFY(qAbs(value - 3.14159265358979) < 1e-6);
}

//=============================================================================================================

void TestPythonRunner::testHelperEvalVector()
{
    PythonTestHelper helper;
    if (!helper.isPythonAvailable()) {
        QSKIP("Python not available");
    }

    bool ok = false;
    Eigen::VectorXd vec = helper.evalVector(
        "for x in [1.0, 2.0, 3.0, 4.0, 5.0]: print(x)", &ok);
    QVERIFY(ok);
    QCOMPARE(vec.size(), 5);
    QCOMPARE(vec(0), 1.0);
    QCOMPARE(vec(4), 5.0);
}

//=============================================================================================================

void TestPythonRunner::testHelperEvalMatrix()
{
    PythonTestHelper helper;
    if (!helper.isPythonAvailable()) {
        QSKIP("Python not available");
    }

    bool ok = false;
    Eigen::MatrixXd mat = helper.evalMatrix(
        "print('1.0 2.0 3.0')\n"
        "print('4.0 5.0 6.0')", &ok);
    QVERIFY(ok);
    QCOMPARE(mat.rows(), 2);
    QCOMPARE(mat.cols(), 3);
    QCOMPARE(mat(0, 0), 1.0);
    QCOMPARE(mat(1, 2), 6.0);
}

//=============================================================================================================

void TestPythonRunner::testHelperTestDataPath()
{
    QString path = PythonTestHelper::testDataPath();
    QVERIFY(!path.isEmpty());
    QVERIFY(path.contains("mne-cpp-test-data"));
}

//=============================================================================================================

void TestPythonRunner::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestPythonRunner)
#include "test_python_runner.moc"
