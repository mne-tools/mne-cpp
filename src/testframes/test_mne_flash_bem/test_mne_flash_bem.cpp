//=============================================================================================================
/**
 * @file     test_mne_flash_bem.cpp
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
 * @brief    Integration tests for the mne_flash_bem application.
 *
 *           Tests cover:
 *             - Running with --help exits cleanly and produces expected output
 *             - Running with --version exits cleanly
 *             - Running without FREESURFER_HOME fails gracefully
 *             - Running without SUBJECTS_DIR fails gracefully
 *             - Running without SUBJECT fails gracefully
 *             - Full execution requires FreeSurfer + flash data (skipped if not available)
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
 * DECLARE CLASS TestMneFlashBem
 *
 * @brief Integration tests for the mne_flash_bem application.
 */
class TestMneFlashBem : public QObject
{
    Q_OBJECT

public:
    TestMneFlashBem();

private slots:
    void initTestCase();

    void testHelp();
    void testVersion();
    void testMissingFreeSurferHome();
    void testMissingSubjectsDir();
    void testMissingSubject();
    void testNonExistentSubjectDir();
    void testFullRun();

    void cleanupTestCase();

private:
    QString findApplication();
    bool hasFreeSurfer();

    QString m_sAppPath;             /**< Path to the mne_flash_bem executable. */
    bool m_bAppAvailable;           /**< Whether the app is found. */
    bool m_bFreeSurferAvailable;    /**< Whether FreeSurfer is installed. */
    QString m_sFreeSurferHome;      /**< Path to FreeSurfer home. */
    QTemporaryDir m_tempDir;        /**< Temporary directory for test output. */
};

//=============================================================================================================

TestMneFlashBem::TestMneFlashBem()
: m_bAppAvailable(false)
, m_bFreeSurferAvailable(false)
{
}

//=============================================================================================================

QString TestMneFlashBem::findApplication()
{
    QString appDir = QCoreApplication::applicationDirPath();

    QStringList candidates;
#ifdef Q_OS_WIN
    candidates << appDir + "/../apps/mne_flash_bem.exe"
               << appDir + "/mne_flash_bem.exe";
#elif defined(Q_OS_MAC)
    candidates << appDir + "/../apps/mne_flash_bem"
               << appDir + "/mne_flash_bem";
#else
    candidates << appDir + "/../apps/mne_flash_bem"
               << appDir + "/mne_flash_bem"
               << appDir + "/../bin/mne_flash_bem";
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

bool TestMneFlashBem::hasFreeSurfer()
{
    m_sFreeSurferHome = qEnvironmentVariable("FREESURFER_HOME");
    if (!m_sFreeSurferHome.isEmpty()) {
        QFileInfo fi(m_sFreeSurferHome + "/bin/mri_convert");
        return fi.exists() && fi.isExecutable();
    }
    return false;
}

//=============================================================================================================

void TestMneFlashBem::initTestCase()
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);

    QVERIFY(m_tempDir.isValid());

    m_sAppPath = findApplication();
    m_bAppAvailable = !m_sAppPath.isEmpty();
    if (m_bAppAvailable) {
        qDebug() << "Application found at" << m_sAppPath;
    } else {
        qDebug() << "mne_flash_bem executable not found — skipping app tests";
    }

    m_bFreeSurferAvailable = hasFreeSurfer();
    if (m_bFreeSurferAvailable) {
        qDebug() << "FreeSurfer found at" << m_sFreeSurferHome;
    } else {
        qDebug() << "FreeSurfer not found — skipping full-run test";
    }
}

//=============================================================================================================

void TestMneFlashBem::testHelp()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_flash_bem executable not found");
    }

    QProcess proc;
    proc.start(m_sAppPath, QStringList() << "--help");
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QCOMPARE(proc.exitCode(), 0);

    QString output = proc.readAllStandardOutput();
    QVERIFY2(output.contains("mne_flash_bem") || output.contains("flash"),
             "Help output should mention the application or flash");
    QVERIFY2(output.contains("--noflash30"),
             "Help output should mention --noflash30 option");
    QVERIFY2(output.contains("--noconvert"),
             "Help output should mention --noconvert option");
    QVERIFY2(output.contains("--subject"),
             "Help output should mention --subject option");
    QVERIFY2(output.contains("--subjects-dir"),
             "Help output should mention --subjects-dir option");
    QVERIFY2(output.contains("FREESURFER_HOME") || output.contains("FreeSurfer"),
             "Help output should mention FreeSurfer");
}

//=============================================================================================================

void TestMneFlashBem::testVersion()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_flash_bem executable not found");
    }

    QProcess proc;
    proc.start(m_sAppPath, QStringList() << "--version");
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QCOMPARE(proc.exitCode(), 0);

    QString output = proc.readAllStandardOutput();
    QVERIFY2(!output.trimmed().isEmpty(),
             "Version output should not be empty");
}

//=============================================================================================================

void TestMneFlashBem::testMissingFreeSurferHome()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_flash_bem executable not found");
    }

    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("FREESURFER_HOME");
    env.insert("SUBJECTS_DIR", m_tempDir.path());
    env.insert("SUBJECT", "test_subject");
    proc.setProcessEnvironment(env);

    proc.start(m_sAppPath, QStringList());
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QVERIFY2(proc.exitCode() != 0,
             "Should fail when FREESURFER_HOME is not set");

    QString stdErr = proc.readAllStandardError();
    QVERIFY2(stdErr.contains("FREESURFER_HOME") || stdErr.contains("FreeSurfer"),
             "Error message should mention FREESURFER_HOME");
}

//=============================================================================================================

void TestMneFlashBem::testMissingSubjectsDir()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_flash_bem executable not found");
    }

    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("SUBJECTS_DIR");
    env.insert("FREESURFER_HOME", "/tmp/fake_freesurfer_home");
    env.insert("SUBJECT", "test_subject");
    proc.setProcessEnvironment(env);

    proc.start(m_sAppPath, QStringList());
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QVERIFY2(proc.exitCode() != 0,
             "Should fail when SUBJECTS_DIR is not set");

    QString stdErr = proc.readAllStandardError();
    QVERIFY2(stdErr.contains("SUBJECTS_DIR"),
             "Error message should mention SUBJECTS_DIR");
}

//=============================================================================================================

void TestMneFlashBem::testMissingSubject()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_flash_bem executable not found");
    }

    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("SUBJECT");
    env.insert("FREESURFER_HOME", "/tmp/fake_freesurfer_home");
    env.insert("SUBJECTS_DIR", m_tempDir.path());
    proc.setProcessEnvironment(env);

    proc.start(m_sAppPath, QStringList());
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QVERIFY2(proc.exitCode() != 0,
             "Should fail when SUBJECT is not set");

    QString stdErr = proc.readAllStandardError();
    QVERIFY2(stdErr.contains("SUBJECT"),
             "Error message should mention SUBJECT");
}

//=============================================================================================================

void TestMneFlashBem::testNonExistentSubjectDir()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_flash_bem executable not found");
    }

    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("FREESURFER_HOME", "/tmp/fake_freesurfer_home");
    proc.setProcessEnvironment(env);

    QStringList args;
    args << "--subject" << "nonexistent_subject"
         << "--subjects-dir" << "/tmp/nonexistent_subjects_dir_xyz"
         << "--noconvert";

    proc.start(m_sAppPath, args);
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QVERIFY2(proc.exitCode() != 0,
             "Should fail when subject directory does not exist");
}

//=============================================================================================================

void TestMneFlashBem::testFullRun()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_flash_bem executable not found");
    }
    if (!m_bFreeSurferAvailable) {
        QSKIP("FreeSurfer not available — mri_convert and mri_watershed required");
    }

    //
    // Full flash_bem run requires:
    //  1. FreeSurfer installation (mri_convert, mri_watershed, etc.)
    //  2. Flash MRI data (multi-echo DICOM or pre-converted MGZ)
    //  3. T1 structural data
    //
    // This test is expected to be skipped in most CI environments.
    // It is provided for local development testing when FreeSurfer
    // and appropriate data are available.
    //
    QSKIP("Full flash_bem run requires FreeSurfer and flash MRI data — "
          "enable manually for local testing");
}

//=============================================================================================================

void TestMneFlashBem::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneFlashBem)
#include "test_mne_flash_bem.moc"
