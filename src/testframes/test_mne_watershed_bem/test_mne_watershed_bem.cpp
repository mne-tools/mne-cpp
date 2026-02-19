//=============================================================================================================
/**
 * @file     test_mne_watershed_bem.cpp
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
 * @brief    Integration tests for the mne_watershed_bem application.
 *
 *           Tests cover:
 *             - Running with --help exits cleanly and produces expected output
 *             - Running without required environment/args produces an error
 *             - Full execution with sample data (requires FreeSurfer)
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
 * DECLARE CLASS TestMneWatershedBem
 *
 * @brief Integration tests for the mne_watershed_bem application.
 */
class TestMneWatershedBem : public QObject
{
    Q_OBJECT

public:
    TestMneWatershedBem();

private slots:
    void initTestCase();

    void testHelp();
    void testVersion();
    void testMissingSubject();
    void testMissingSubjectsDir();
    void testMissingFreeSurferHome();
    void testNonExistentSubjectDir();
    void testFullRun();

    void cleanupTestCase();

private:
    QString findApplication();
    QString findSubjectsDir();
    bool hasFreeSurfer();

    QString m_sAppPath;         /**< Path to the mne_watershed_bem executable. */
    bool m_bAppAvailable;       /**< Whether the app is found. */
    bool m_bDataAvailable;      /**< Whether sample data is found. */
    bool m_bFreeSurferAvailable;/**< Whether FreeSurfer is installed. */
    QString m_sSubjectsDir;     /**< Path to subjects dir. */
    QString m_sFreeSurferHome;  /**< Path to FreeSurfer home. */
    QTemporaryDir m_tempDir;    /**< Temporary directory for test output. */
};

//=============================================================================================================

TestMneWatershedBem::TestMneWatershedBem()
: m_bAppAvailable(false)
, m_bDataAvailable(false)
, m_bFreeSurferAvailable(false)
{
}

//=============================================================================================================

QString TestMneWatershedBem::findApplication()
{
    QString appDir = QCoreApplication::applicationDirPath();

    QStringList candidates;
#ifdef Q_OS_WIN
    candidates << appDir + "/../apps/mne_watershed_bem.exe"
               << appDir + "/mne_watershed_bem.exe";
#elif defined(Q_OS_MAC)
    candidates << appDir + "/../apps/mne_watershed_bem"
               << appDir + "/mne_watershed_bem";
#else
    candidates << appDir + "/../apps/mne_watershed_bem"
               << appDir + "/mne_watershed_bem"
               << appDir + "/../bin/mne_watershed_bem";
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

QString TestMneWatershedBem::findSubjectsDir()
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

bool TestMneWatershedBem::hasFreeSurfer()
{
    m_sFreeSurferHome = qEnvironmentVariable("FREESURFER_HOME");
    if (!m_sFreeSurferHome.isEmpty()) {
        QFileInfo fi(m_sFreeSurferHome + "/bin/mri_watershed");
        return fi.exists() && fi.isExecutable();
    }
    return false;
}

//=============================================================================================================

void TestMneWatershedBem::initTestCase()
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);

    QVERIFY(m_tempDir.isValid());

    m_sAppPath = findApplication();
    m_bAppAvailable = !m_sAppPath.isEmpty();
    if (m_bAppAvailable) {
        qDebug() << "Application found at" << m_sAppPath;
    } else {
        qDebug() << "mne_watershed_bem executable not found — skipping app tests";
    }

    m_sSubjectsDir = findSubjectsDir();
    m_bDataAvailable = !m_sSubjectsDir.isEmpty();
    if (m_bDataAvailable) {
        qDebug() << "Sample data found at" << m_sSubjectsDir;
    } else {
        qDebug() << "MNE sample data not found — skipping data-dependent tests";
    }

    m_bFreeSurferAvailable = hasFreeSurfer();
    if (m_bFreeSurferAvailable) {
        qDebug() << "FreeSurfer found at" << m_sFreeSurferHome;
    } else {
        qDebug() << "FreeSurfer not found — skipping full-run test";
    }
}

//=============================================================================================================

void TestMneWatershedBem::testHelp()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_watershed_bem executable not found");
    }

    QProcess proc;
    proc.start(m_sAppPath, QStringList() << "--help");
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QCOMPARE(proc.exitCode(), 0);

    QString output = proc.readAllStandardOutput();
    QVERIFY2(output.contains("mne_watershed_bem"),
             "Help output should mention the application name");
    QVERIFY2(output.contains("--subject"),
             "Help output should mention --subject option");
    QVERIFY2(output.contains("--overwrite"),
             "Help output should mention --overwrite option");
    QVERIFY2(output.contains("--volume"),
             "Help output should mention --volume option");
    QVERIFY2(output.contains("--atlas"),
             "Help output should mention --atlas option");
    QVERIFY2(output.contains("--preflood"),
             "Help output should mention --preflood option");
}

//=============================================================================================================

void TestMneWatershedBem::testVersion()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_watershed_bem executable not found");
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

void TestMneWatershedBem::testMissingSubject()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_watershed_bem executable not found");
    }

    // Run without SUBJECT set and without --subject flag
    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("SUBJECT");
    env.insert("SUBJECTS_DIR", m_tempDir.path());
    env.insert("FREESURFER_HOME", "/tmp/fake_freesurfer_home");
    proc.setProcessEnvironment(env);

    proc.start(m_sAppPath, QStringList());
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QVERIFY2(proc.exitCode() != 0,
             "Should fail when SUBJECT is not set");
}

//=============================================================================================================

void TestMneWatershedBem::testMissingSubjectsDir()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_watershed_bem executable not found");
    }

    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("SUBJECTS_DIR");
    env.insert("SUBJECT", "test_subject");
    env.insert("FREESURFER_HOME", "/tmp/fake_freesurfer_home");
    proc.setProcessEnvironment(env);

    proc.start(m_sAppPath, QStringList());
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QVERIFY2(proc.exitCode() != 0,
             "Should fail when SUBJECTS_DIR is not set");
}

//=============================================================================================================

void TestMneWatershedBem::testMissingFreeSurferHome()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_watershed_bem executable not found");
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
}

//=============================================================================================================

void TestMneWatershedBem::testNonExistentSubjectDir()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_watershed_bem executable not found");
    }

    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("FREESURFER_HOME", "/tmp/fake_freesurfer_home");
    proc.setProcessEnvironment(env);

    QStringList args;
    args << "--subject" << "nonexistent_subject"
         << "--subjects-dir" << "/tmp/nonexistent_subjects_dir";

    proc.start(m_sAppPath, args);
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QVERIFY2(proc.exitCode() != 0,
             "Should fail when subject directory does not exist");
}

//=============================================================================================================

void TestMneWatershedBem::testFullRun()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_watershed_bem executable not found");
    }
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }
    if (!m_bFreeSurferAvailable) {
        QSKIP("FreeSurfer not available — mri_watershed required for full run");
    }

    //
    // Set up a temporary subject directory with symlinked MRI data
    //
    QString tempSubjectsDir = m_tempDir.path() + "/subjects_ws";
    QString tempSubjectDir = tempSubjectsDir + "/sample";
    QString tempMriDir = tempSubjectDir + "/mri";
    QDir().mkpath(tempMriDir);

    // Symlink T1.mgz
    QString srcT1 = m_sSubjectsDir + "/sample/mri/T1.mgz";
    if (!QFile::exists(srcT1)) {
        QSKIP("T1.mgz not found in sample data");
    }
    QFile::link(srcT1, tempMriDir + "/T1.mgz");

    // Symlink transforms directory if present
    QString srcTransforms = m_sSubjectsDir + "/sample/mri/transforms";
    if (QDir(srcTransforms).exists()) {
        QFile::link(srcTransforms, tempMriDir + "/transforms");
    }

    // Run mne_watershed_bem
    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("FREESURFER_HOME", m_sFreeSurferHome);
    proc.setProcessEnvironment(env);

    QStringList args;
    args << "--subject" << "sample"
         << "--subjects-dir" << tempSubjectsDir
         << "--overwrite";

    proc.start(m_sAppPath, args);
    QVERIFY2(proc.waitForFinished(300000),
             "mne_watershed_bem timed out (5 min limit)");

    if (proc.exitCode() != 0) {
        QString stdErr = proc.readAllStandardError();
        QString stdOut = proc.readAllStandardOutput();
        qDebug() << "stdout:" << stdOut;
        qDebug() << "stderr:" << stdErr;
    }
    QCOMPARE(proc.exitCode(), 0);

    // Verify watershed surfaces were created
    QString wsDir = tempSubjectDir + "/bem/watershed";
    QVERIFY2(QDir(wsDir).exists(),
             "Watershed directory should be created");

    QStringList expectedSurfaces;
    expectedSurfaces << "sample_brain_surface"
                     << "sample_inner_skull_surface"
                     << "sample_outer_skull_surface"
                     << "sample_outer_skin_surface";

    for (const QString& surfName : expectedSurfaces) {
        QString surfPath = wsDir + "/" + surfName;
        QVERIFY2(QFileInfo::exists(surfPath),
                 qPrintable("Surface not found: " + surfPath));
        QFileInfo fi(surfPath);
        QVERIFY2(fi.size() > 0,
                 qPrintable("Surface file is empty: " + surfPath));
    }

    // Verify head BEM FIFF file was created
    QString headFif = tempSubjectDir + "/bem/sample-head.fif";
    QVERIFY2(QFileInfo::exists(headFif),
             qPrintable("Head BEM FIFF not found: " + headFif));
    QFileInfo fiHead(headFif);
    QVERIFY2(fiHead.size() > 1024,
             "Head BEM FIFF file too small");
}

//=============================================================================================================

void TestMneWatershedBem::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneWatershedBem)
#include "test_mne_watershed_bem.moc"
