//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_pybridge_python_runner.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for PythonRunner and PythonTestHelper.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <pybridge/python_runner.h>
#include <pybridge/python_test_helper.h>

#include "../common/python_guard.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cctype>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

//=============================================================================================================
/**
 * @brief Tests for PythonRunner and PythonTestHelper.
 */
class TestPythonRunner : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // PythonRunner
    void testParseProgressLine();
    void testPythonVersion();
    void testRunCodeHelloWorld();
    void testRunCodeStdErr();
    void testRunCodeExitCode();
    void testRunCodeTimeout();
    void testRunCodeArgumentsAndMultiline();
    void testRunCodeEnvironmentVars();
    void testIsPackageAvailable();
    void testCallbacks();

    // PythonTestHelper
    void testHelperIsAvailable();
    void testHelperEval();
    void testHelperEvalTimeout();
    void testHelperEvalDouble();
    void testHelperEvalVector();
    void testHelperEvalMatrix();
    void testHelperTestDataPath();

private:
    static PythonRunner runner();

    bool m_bPythonAvailable = false;
};

//=============================================================================================================

PythonRunner TestPythonRunner::runner()
{
    PythonRunnerConfig config;
    config.timeout = 10s;
    return PythonRunner(config);
}

//=============================================================================================================

void TestPythonRunner::initTestCase()
{
    m_bPythonAvailable = PythonRunner().is_python_available();
    qDebug() << "Python detection: available=" << m_bPythonAvailable
             << " MNE_REQUIRE_PYTHON=" << PythonTestHelper::is_python_required();
}

//=============================================================================================================

void TestPythonRunner::testParseProgressLine()
{
    const auto plain = parse_progress_line("[progress] 42.5%");
    QVERIFY(plain);
    QCOMPARE(plain->first, 42.5f);
    QVERIFY(plain->second.empty());

    const auto message = parse_progress_line("  [PROGRESS]   7 %  Training epoch 10/50  ");
    QVERIFY(message);
    QCOMPARE(message->first, 7.0f);
    QCOMPARE(message->second, std::string("Training epoch 10/50"));

    QVERIFY(!parse_progress_line("progress 42%"));
    QVERIFY(!parse_progress_line("[progress]42%"));
    QVERIFY(!parse_progress_line("[progress] 42"));
    QVERIFY(!parse_progress_line("[progress] -5%"));
    QVERIFY(!parse_progress_line("[progress] half%"));
    QVERIFY(!parse_progress_line(""));
}

//=============================================================================================================

void TestPythonRunner::testPythonVersion()
{
    GUARD_PYTHON(m_bPythonAvailable, "Python not available");

    const std::string version = PythonRunner().python_version();
    QVERIFY(!version.empty());
    QVERIFY(std::isdigit(static_cast<unsigned char>(version.front()))); // e.g. "3.12.4"
}

//=============================================================================================================

void TestPythonRunner::testRunCodeHelloWorld()
{
    GUARD_PYTHON(m_bPythonAvailable, "Python not available");

    const PythonRunnerResult result = runner().run_code("print('Hello World')");
    QVERIFY(result.success);
    QCOMPARE(result.exit_code, 0);
    QCOMPARE(result.std_out, std::string("Hello World"));
}

//=============================================================================================================

void TestPythonRunner::testRunCodeStdErr()
{
    GUARD_PYTHON(m_bPythonAvailable, "Python not available");

    const PythonRunnerResult result = runner().run_code("import sys; sys.stderr.write('error msg\\n')");
    QVERIFY(result.success);
    QCOMPARE(result.std_err, std::string("error msg"));
}

//=============================================================================================================

void TestPythonRunner::testRunCodeExitCode()
{
    GUARD_PYTHON(m_bPythonAvailable, "Python not available");

    const PythonRunnerResult result = runner().run_code("import sys; sys.exit(42)");
    QVERIFY(!result.success);
    QCOMPARE(result.exit_code, 42);
}

//=============================================================================================================

void TestPythonRunner::testRunCodeTimeout()
{
    GUARD_PYTHON(m_bPythonAvailable, "Python not available");

    PythonRunnerConfig config;
    config.timeout = 500ms;
    const PythonRunnerResult result = PythonRunner(config).run_code("import time; time.sleep(30)");
    QVERIFY(!result.success);
    QVERIFY(result.timed_out);
    QVERIFY(result.std_err.find("timed out") != std::string::npos);
}

//=============================================================================================================

void TestPythonRunner::testRunCodeArgumentsAndMultiline()
{
    GUARD_PYTHON(m_bPythonAvailable, "Python not available");

    const PythonRunnerResult result = runner().run_code("import sys\n"
                                                        "x = int(sys.argv[1])\n"
                                                        "print(x * 3, end='')", // no trailing newline
                                                        {"2"});
    QVERIFY(result.success);
    QCOMPARE(result.std_out, std::string("6"));
}

//=============================================================================================================

void TestPythonRunner::testRunCodeEnvironmentVars()
{
    GUARD_PYTHON(m_bPythonAvailable, "Python not available");

    PythonRunnerConfig config;
    config.timeout = 10s;
    config.extra_env["MNE_CPP_TEST_VAR"] = "test_value_42";
    const PythonRunnerResult result = PythonRunner(config).run_code("import os; print(os.environ.get('MNE_CPP_TEST_VAR', 'missing'))");
    QVERIFY(result.success);
    QCOMPARE(result.std_out, std::string("test_value_42"));
}

//=============================================================================================================

void TestPythonRunner::testIsPackageAvailable()
{
    GUARD_PYTHON(m_bPythonAvailable, "Python not available");

    const PythonRunner python;
    QVERIFY(python.is_package_available("sys"));
    QVERIFY(python.is_package_available("importlib.util"));
    QVERIFY(!python.is_package_available("nonexistent_package_xyz_12345"));
    QVERIFY(!python.is_package_available("nonexistent_package_xyz_12345.sub"));
    QVERIFY(!python.is_package_available("os'); import shutil; ('")); // not an identifier: never executed
    QVERIFY(!python.is_package_available(""));
}

//=============================================================================================================

void TestPythonRunner::testCallbacks()
{
    GUARD_PYTHON(m_bPythonAvailable, "Python not available");

    PythonRunner python = runner();
    std::vector<std::string> outLines;
    std::vector<std::string> errLines;
    std::vector<float> progress;
    std::string lastMessage;
    python.set_line_callback([&](PythonChannel channel, std::string_view line) {
        (channel == PythonChannel::StdOut ? outLines : errLines).emplace_back(line);
    });
    python.set_progress_callback([&](float pct, std::string_view message) {
        progress.push_back(pct);
        lastMessage = message;
    });

    const PythonRunnerResult result = python.run_code("import sys\n"
                                                      "print('[progress] 25.0%')\n"
                                                      "print('[progress] 50.0%')\n"
                                                      "sys.stderr.write('warn\\n')\n"
                                                      "print('[progress] 100% finished')\n"
                                                      "print('done')");
    QVERIFY(result.success);
    QCOMPARE(progress, (std::vector<float>{25.0f, 50.0f, 100.0f}));
    QCOMPARE(lastMessage, std::string("finished"));
    QCOMPARE(result.progress_pct, 100.0f);
    QCOMPARE(outLines.size(), std::size_t(4));
    QCOMPARE(outLines.back(), std::string("done"));
    QCOMPARE(errLines, (std::vector<std::string>{"warn"}));
}

//=============================================================================================================

void TestPythonRunner::testHelperIsAvailable()
{
    // The test helper finds an interpreter exactly when the runner does
    QCOMPARE(PythonTestHelper().is_python_available(), PythonRunner().is_python_available());
}

//=============================================================================================================

void TestPythonRunner::testHelperEval()
{
    const PythonTestHelper helper;
    GUARD_PYTHON(helper.is_python_available(), "Python not available");

    const PythonRunnerResult result = helper.eval("print(2 + 3)");
    QVERIFY(result.success);
    QCOMPARE(result.std_out, std::string("5"));
}

//=============================================================================================================

void TestPythonRunner::testHelperEvalTimeout()
{
    const PythonTestHelper helper;
    GUARD_PYTHON(helper.is_python_available(), "Python not available");

    const PythonRunnerResult result = helper.eval("import time; time.sleep(30)", 500ms);
    QVERIFY(result.timed_out);
    QVERIFY(!helper.eval_double("import time; time.sleep(30); print(1)", 500ms));
}

//=============================================================================================================

void TestPythonRunner::testHelperEvalDouble()
{
    const PythonTestHelper helper;
    GUARD_PYTHON(helper.is_python_available(), "Python not available");

    const std::optional<double> value = helper.eval_double("import math; print(repr(math.pi))");
    QVERIFY(value);
    QCOMPARE(*value, 3.141592653589793);
    QVERIFY(!helper.eval_double("print('not a number')"));
}

//=============================================================================================================

void TestPythonRunner::testHelperEvalVector()
{
    const PythonTestHelper helper;
    GUARD_PYTHON(helper.is_python_available(), "Python not available");

    const std::optional<Eigen::VectorXd> lines = helper.eval_vector("for x in [1.0, 2.0, 3.0, 4.0, 5.0]: print(x)");
    QVERIFY(lines);
    QCOMPARE(lines->size(), Eigen::Index(5));
    QCOMPARE((*lines)(0), 1.0);
    QCOMPARE((*lines)(4), 5.0);

    const std::optional<Eigen::VectorXd> mixed = helper.eval_vector("print('1 2 3')\nprint('4')");
    QVERIFY(mixed);
    QCOMPARE(mixed->size(), Eigen::Index(4));

    QVERIFY(!helper.eval_vector("print('')"));
}

//=============================================================================================================

void TestPythonRunner::testHelperEvalMatrix()
{
    const PythonTestHelper helper;
    GUARD_PYTHON(helper.is_python_available(), "Python not available");

    const std::optional<Eigen::MatrixXd> mat = helper.eval_matrix("print('1.0 2.0 3.0')\n"
                                                                  "print('4.0 5.0 6.0')");
    QVERIFY(mat);
    QCOMPARE(mat->rows(), Eigen::Index(2));
    QCOMPARE(mat->cols(), Eigen::Index(3));
    QCOMPARE((*mat)(0, 0), 1.0);
    QCOMPARE((*mat)(1, 2), 6.0);

    QVERIFY(!helper.eval_matrix("print('1 2')\nprint('3')"));
}

//=============================================================================================================

void TestPythonRunner::testHelperTestDataPath()
{
    const std::filesystem::path path = PythonTestHelper::test_data_path();
    QCOMPARE(path.filename().string(), std::string("mne-cpp-test-data"));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestPythonRunner)
#include "test_pybridge_python_runner.moc"
