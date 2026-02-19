//=============================================================================================================
/**
 * @file     test_mne_setup_mri.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    Integration tests for the mne_setup_mri application.
 *
 *           Tests cover:
 *             - Running with --help exits cleanly
 *             - Running without required args produces an error
 *             - Full execution with sample data producing COR.fif output
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestMneSetupMri
 *
 * @brief Integration tests for the mne_setup_mri application.
 */
class TestMneSetupMri : public QObject
{
    Q_OBJECT

public:
    TestMneSetupMri();

private slots:
    void initTestCase();

    void testHelp();
    void testMissingArgs();
    void testFullRun();
    void testOverwrite();

    void cleanupTestCase();

private:
    QString findApplication();
    QString findSubjectsDir();

    QString m_sAppPath;         /**< Path to the mne_setup_mri executable. */
    bool m_bAppAvailable;       /**< Whether the app is found. */
    bool m_bDataAvailable;      /**< Whether sample data is found. */
    QString m_sSubjectsDir;     /**< Path to subjects dir. */
    QTemporaryDir m_tempDir;    /**< Temporary subjects dir for test output. */
};

//=============================================================================================================

TestMneSetupMri::TestMneSetupMri()
: m_bAppAvailable(false)
, m_bDataAvailable(false)
{
}

//=============================================================================================================

QString TestMneSetupMri::findApplication()
{
    // Try multiple locations relative to test binary
    QString appDir = QCoreApplication::applicationDirPath();

    // Standard location: same directory as test binary (both in out/Release/tests or out/Release/apps)
    QStringList candidates;
#ifdef Q_OS_WIN
    candidates << appDir + "/../apps/mne_setup_mri.exe"
               << appDir + "/mne_setup_mri.exe";
#else
    candidates << appDir + "/../apps/mne_setup_mri"
               << appDir + "/mne_setup_mri"
               << appDir + "/../bin/mne_setup_mri";
#endif

    for (const QString& path : candidates) {
        QFileInfo fi(path);
        if (fi.exists() && fi.isExecutable()) {
            return fi.canonicalFilePath();
        }
    }

    return QString();
}

//=============================================================================================================

QString TestMneSetupMri::findSubjectsDir()
{
    QString envDir = qEnvironmentVariable("SUBJECTS_DIR");
    if (!envDir.isEmpty() && QDir(envDir + "/sample/mri").exists()) {
        return envDir;
    }

    QString home = QDir::homePath();
    QString stdPath = home + "/mne_data/MNE-sample-data/subjects";
    if (QDir(stdPath + "/sample/mri").exists()) {
        return stdPath;
    }

    return QString();
}

//=============================================================================================================

void TestMneSetupMri::initTestCase()
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);

    QVERIFY(m_tempDir.isValid());

    m_sAppPath = findApplication();
    m_bAppAvailable = !m_sAppPath.isEmpty();
    if (m_bAppAvailable) {
        qDebug() << "Application found at" << m_sAppPath;
    } else {
        qDebug() << "mne_setup_mri executable not found — skipping app tests";
    }

    m_sSubjectsDir = findSubjectsDir();
    m_bDataAvailable = !m_sSubjectsDir.isEmpty();
    if (m_bDataAvailable) {
        qDebug() << "Sample data found at" << m_sSubjectsDir;
    } else {
        qDebug() << "MNE sample data not found — skipping data-dependent tests";
    }
}

//=============================================================================================================

void TestMneSetupMri::testHelp()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_setup_mri executable not found");
    }

    QProcess proc;
    proc.start(m_sAppPath, QStringList() << "--help");
    QVERIFY(proc.waitForFinished(10000));

    // --help should exit with code 0 and produce output
    QCOMPARE(proc.exitCode(), 0);
    QString output = proc.readAllStandardOutput();
    QVERIFY2(output.contains("mne_setup_mri"), "Help output should mention mne_setup_mri");
    QVERIFY2(output.contains("--subject"), "Help output should mention --subject option");
    QVERIFY2(output.contains("--subjects-dir"), "Help output should mention --subjects-dir option");
}

//=============================================================================================================

void TestMneSetupMri::testMissingArgs()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_setup_mri executable not found");
    }

    // Running with no args should produce an error (non-zero exit code or error output)
    QProcess proc;
    proc.start(m_sAppPath, QStringList());
    QVERIFY(proc.waitForFinished(10000));

    // Should fail because --subject is required
    QVERIFY2(proc.exitCode() != 0, "Should fail without required --subject");
}

//=============================================================================================================

void TestMneSetupMri::testFullRun()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_setup_mri executable not found");
    }
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }

    // Create a temporary subject directory structure replicating the expected layout
    // Copy (or symlink) the mri directory from sample data
    QString tempSubjectsDir = m_tempDir.path() + "/subjects_run1";
    QString tempSubjectDir = tempSubjectsDir + "/sample";
    QString tempMriDir = tempSubjectDir + "/mri";
    QDir().mkpath(tempMriDir);

    // Symlink the T1.mgz from sample data
    QString srcT1 = m_sSubjectsDir + "/sample/mri/T1.mgz";
    QString dstT1 = tempMriDir + "/T1.mgz";
    if (QFile::exists(srcT1)) {
        QFile::link(srcT1, dstT1);
    } else {
        QSKIP("T1.mgz not found in sample data");
    }

    // Also need the transforms directory for talairach xfm
    QString srcTransforms = m_sSubjectsDir + "/sample/mri/transforms";
    QString dstTransforms = tempMriDir + "/transforms";
    if (QDir(srcTransforms).exists()) {
        QFile::link(srcTransforms, dstTransforms);
    }

    // Run mne_setup_mri
    QProcess proc;
    QStringList args;
    args << "--subjects-dir" << tempSubjectsDir
         << "--subject" << "sample"
         << "--mri" << "T1"
         << "--overwrite"
         << "--verbose";

    proc.start(m_sAppPath, args);
    QVERIFY2(proc.waitForFinished(60000), "mne_setup_mri timed out");

    if (proc.exitCode() != 0) {
        QString stdErr = proc.readAllStandardError();
        QString stdOut = proc.readAllStandardOutput();
        qDebug() << "stdout:" << stdOut;
        qDebug() << "stderr:" << stdErr;
    }
    QCOMPARE(proc.exitCode(), 0);

    // Verify COR.fif was created in the neuromag sets directory
    QString corFif = tempMriDir + "/T1-neuromag/sets/COR.fif";
    QVERIFY2(QFile::exists(corFif), qPrintable("COR.fif not found at: " + corFif));

    // Check file size (256×256×256 bytes of pixel data + FIFF overhead ≈ 16MB)
    QFileInfo fi(corFif);
    QVERIFY2(fi.size() > 10 * 1024 * 1024, "COR.fif too small");
    QVERIFY2(fi.size() < 30 * 1024 * 1024, "COR.fif too large");
}

//=============================================================================================================

void TestMneSetupMri::testOverwrite()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_setup_mri executable not found");
    }
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }

    // Setup temp directory
    QString tempSubjectsDir = m_tempDir.path() + "/subjects_run2";
    QString tempSubjectDir = tempSubjectsDir + "/sample";
    QString tempMriDir = tempSubjectDir + "/mri";
    QDir().mkpath(tempMriDir);

    QString srcT1 = m_sSubjectsDir + "/sample/mri/T1.mgz";
    QString dstT1 = tempMriDir + "/T1.mgz";
    QFile::link(srcT1, dstT1);

    QString srcTransforms = m_sSubjectsDir + "/sample/mri/transforms";
    QString dstTransforms = tempMriDir + "/transforms";
    if (QDir(srcTransforms).exists()) {
        QFile::link(srcTransforms, dstTransforms);
    }

    QStringList baseArgs;
    baseArgs << "--subjects-dir" << tempSubjectsDir
             << "--subject" << "sample"
             << "--mri" << "T1";

    // First run — should succeed
    {
        QProcess proc;
        proc.start(m_sAppPath, baseArgs + QStringList{"--overwrite"});
        QVERIFY(proc.waitForFinished(60000));
        QCOMPARE(proc.exitCode(), 0);
    }

    // Second run WITHOUT --overwrite — should fail because COR.fif already exists
    {
        QProcess proc;
        proc.start(m_sAppPath, baseArgs);
        QVERIFY(proc.waitForFinished(60000));
        QVERIFY2(proc.exitCode() != 0, "Should fail without --overwrite when COR.fif exists");
    }

    // Third run WITH --overwrite — should succeed again
    {
        QProcess proc;
        proc.start(m_sAppPath, baseArgs + QStringList{"--overwrite"});
        QVERIFY(proc.waitForFinished(60000));
        QCOMPARE(proc.exitCode(), 0);
    }
}

//=============================================================================================================

void TestMneSetupMri::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneSetupMri)
#include "test_mne_setup_mri.moc"
