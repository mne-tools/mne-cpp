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
#include <utils/python_runner.h>
#include <utils/python_test_helper.h>
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
// USED NAMESPACES
//=============================================================================================================

#ifndef WASMBUILD
using namespace MLLIB;
using namespace UTILSLIB;
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
    m_pythonAvailable = trainer.runner().isPythonAvailable();
    qDebug() << "Python detection: available=" << m_pythonAvailable
             << " MNE_REQUIRE_PYTHON=" << UTILSLIB::PythonTestHelper::isPythonRequired();
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
    config.pythonExe = "python3";
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
    QStringList missing = trainer.checkPrerequisites({"os"});
    QVERIFY2(missing.isEmpty(),
             qPrintable(QString("Expected 'os' to be importable, missing: %1").arg(missing.join(", "))));
#endif
}

//=============================================================================================================

void TestMlTrainer::testCheckPrerequisitesNonexistent()
{
#ifndef WASMBUILD
    GUARD_PYTHON(m_pythonAvailable, "Python not available for MLTrainer");

    MLTrainer trainer;

    // A nonexistent package should appear in the missing list
    QStringList missing = trainer.checkPrerequisites({"nonexistent_xyz_package_12345"});
    QVERIFY2(missing.contains("nonexistent_xyz_package_12345"),
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
    PythonRunnerResult result = trainer.run(scriptPath, {});

    QVERIFY2(result.success,
             qPrintable(QString("Script failed: exit=%1, stderr=%2")
                       .arg(result.exitCode).arg(result.stdErr)));
    QVERIFY(result.stdOut.contains("hello from trainer"));
#endif
}

//=============================================================================================================

void TestMlTrainer::testRunnerAccess()
{
#ifndef WASMBUILD
    MLTrainer trainer;

    // The runner() accessor should return a valid reference
    PythonRunner& runner = trainer.runner();
    // Calling isPythonAvailable on runner should not crash
    runner.isPythonAvailable();
#endif
}

//=============================================================================================================

void TestMlTrainer::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMlTrainer)
#include "test_ml_trainer.moc"
