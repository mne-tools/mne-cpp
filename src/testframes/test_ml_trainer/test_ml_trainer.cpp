//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_ml_trainer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for MLTrainer — Python training bridge wrapper.
 *           Tests prerequisite checking and trivial script execution.
 *           Requires Python on the system; tests degrade gracefully if absent.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#ifndef WASMBUILD
#include <ml/ml_trainer.h>
#include <pybridge/python_runner.h>
#include <pybridge/python_test_helper.h>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

#ifndef WASMBUILD
using namespace mne;
#endif

//=============================================================================================================
/**
 * DECLARE CLASS TestMlTrainer
 *
 * @brief Tests for MLTrainer Python bridge.
 */
class TestMlTrainer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testConstruction();
    void testCheckPrerequisitesStdlib();
    void testCheckPrerequisitesNonexistent();
    void testRunTrivialScript();
    void testRunnerAccess();

    void cleanupTestCase();

private:
    QTemporaryDir m_tempDir;
    bool m_pythonAvailable = false;
};

//=============================================================================================================

void TestMlTrainer::initTestCase()
{
#ifndef WASMBUILD
    MLTrainer trainer;
    m_pythonAvailable = trainer.runner().is_python_available();
    qDebug() << "Python detection: available=" << m_pythonAvailable
             << " MNE_REQUIRE_PYTHON=" << PythonTestHelper::is_python_required();
#endif
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================

void TestMlTrainer::testConstruction()
{
#ifndef WASMBUILD
    // Default construction should not crash
    MLTrainer trainer;

    // Construction with config
    PythonRunnerConfig config;
    config.python_exe = "python3";
    MLTrainer trainerWithConfig(config);
#endif
}

//=============================================================================================================

void TestMlTrainer::testCheckPrerequisitesStdlib()
{
#ifndef WASMBUILD
    GUARD_PYTHON(m_pythonAvailable, "Python not available for MLTrainer");

    MLTrainer trainer;

    // "os" is always available in Python standard library
    const mne::StringList missing = trainer.check_prerequisites({"os"});
    QVERIFY2(missing.empty(), ("Expected 'os' to be importable, missing: " + mne::join(missing, ", ")).c_str());
#endif
}

//=============================================================================================================

void TestMlTrainer::testCheckPrerequisitesNonexistent()
{
#ifndef WASMBUILD
    GUARD_PYTHON(m_pythonAvailable, "Python not available for MLTrainer");

    MLTrainer trainer;

    // A nonexistent package should appear in the missing list
    const mne::StringList missing = trainer.check_prerequisites({"os", "nonexistent_xyz_package_12345"});
    QVERIFY2(missing == mne::StringList{"nonexistent_xyz_package_12345"},
             "Nonexistent package should be reported as missing");
#endif
}

//=============================================================================================================

void TestMlTrainer::testRunTrivialScript()
{
#ifndef WASMBUILD
    GUARD_PYTHON(m_pythonAvailable, "Python not available for MLTrainer");

    // Write a trivial Python script
    QString scriptPath = m_tempDir.filePath("trivial.py");
    QFile file(scriptPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&file);
    out << "print('hello from trainer')\n";
    file.close();

    MLTrainer trainer;
    const PythonRunnerResult result = trainer.run(scriptPath.toStdString(), {});

    QVERIFY2(result.success, std::format("Script failed: exit={}, stderr={}", result.exit_code, result.std_err).c_str());
    QCOMPARE(result.std_out, std::string("hello from trainer"));
#endif
}

//=============================================================================================================

void TestMlTrainer::testRunnerAccess()
{
#ifndef WASMBUILD
    MLTrainer trainer;

    // The runner() accessor should return a valid reference
    PythonRunner& runner = trainer.runner();
    // The accessor returns the trainer's own runner, so configuration changes reach the next run
    PythonRunnerConfig config = runner.config();
    config.python_exe = "python-that-does-not-exist";
    runner.set_config(config);
    QVERIFY(!trainer.run("unused.py").success);
#endif
}

//=============================================================================================================

void TestMlTrainer::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMlTrainer)
#include "test_ml_trainer.moc"
