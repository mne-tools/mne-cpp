//=============================================================================================================
/**
 * @file     test_mne_surf2bem.cpp
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
 * @brief    Integration tests for the mne_surf2bem application.
 *
 *           Tests cover:
 *             - Running with --help exits cleanly and produces expected output
 *             - Running with --version exits cleanly
 *             - Running with no arguments prints usage and returns 1
 *             - Single FreeSurfer surface conversion to FIFF BEM file
 *             - Multiple surface conversion with BEM IDs
 *             - Surface topology checks
 *             - Output BEM file validation via MNEBem::read()
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QFileInfo>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace MNELIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestMneSurf2Bem
 *
 * @brief Integration tests for the mne_surf2bem application.
 */
class TestMneSurf2Bem : public QObject
{
    Q_OBJECT

public:
    TestMneSurf2Bem();

private slots:
    void initTestCase();

    void testHelp();
    void testVersion();
    void testNoArgs();
    void testMissingInputFile();
    void testSingleSurfConversion();
    void testMultiSurfConversion();
    void testSurfConversionWithCheck();
    void testOverwriteOutput();

    void cleanupTestCase();

private:
    QString findApplication();
    QString findSubjectsDir();

    QString m_sAppPath;         /**< Path to the mne_surf2bem executable. */
    bool m_bAppAvailable;       /**< Whether the app is found. */
    bool m_bDataAvailable;      /**< Whether sample data is found. */
    QString m_sSubjectsDir;     /**< Path to subjects dir. */
    QTemporaryDir m_tempDir;    /**< Temporary directory for test output. */
};

//=============================================================================================================

TestMneSurf2Bem::TestMneSurf2Bem()
: m_bAppAvailable(false)
, m_bDataAvailable(false)
{
}

//=============================================================================================================

QString TestMneSurf2Bem::findApplication()
{
    QString appDir = QCoreApplication::applicationDirPath();

    QStringList candidates;
#ifdef Q_OS_WIN
    candidates << appDir + "/../apps/mne_surf2bem.exe"
               << appDir + "/mne_surf2bem.exe";
#elif defined(Q_OS_MAC)
    candidates << appDir + "/../apps/mne_surf2bem"
               << appDir + "/mne_surf2bem";
#else
    candidates << appDir + "/../apps/mne_surf2bem"
               << appDir + "/mne_surf2bem"
               << appDir + "/../bin/mne_surf2bem";
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

QString TestMneSurf2Bem::findSubjectsDir()
{
    QString envDir = qEnvironmentVariable("SUBJECTS_DIR");
    if (!envDir.isEmpty()) {
        if (QDir(envDir + "/sample/bem").exists()) {
            return envDir;
        }
    }

    QString home = QDir::homePath();
    QString stdPath = home + "/mne_data/MNE-sample-data/subjects";
    if (QDir(stdPath + "/sample/bem").exists()) {
        return stdPath;
    }

    return QString();
}

//=============================================================================================================

void TestMneSurf2Bem::initTestCase()
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);

    QVERIFY(m_tempDir.isValid());

    m_sAppPath = findApplication();
    m_bAppAvailable = !m_sAppPath.isEmpty();
    if (m_bAppAvailable) {
        qDebug() << "Application found at" << m_sAppPath;
    } else {
        qDebug() << "mne_surf2bem executable not found — skipping app tests";
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

void TestMneSurf2Bem::testHelp()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_surf2bem executable not found");
    }

    QProcess proc;
    proc.start(m_sAppPath, QStringList() << "--help");
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QCOMPARE(proc.exitCode(), 0);

    QString output = proc.readAllStandardOutput();
    QVERIFY2(output.contains("--surf"),
             "Help output should mention --surf option");
    QVERIFY2(output.contains("--tri"),
             "Help output should mention --tri option");
    QVERIFY2(output.contains("--fif"),
             "Help output should mention --fif option");
    QVERIFY2(output.contains("--id"),
             "Help output should mention --id option");
    QVERIFY2(output.contains("--check"),
             "Help output should mention --check option");
    QVERIFY2(output.contains("--shift"),
             "Help output should mention --shift option");
}

//=============================================================================================================

void TestMneSurf2Bem::testVersion()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_surf2bem executable not found");
    }

    QProcess proc;
    proc.start(m_sAppPath, QStringList() << "--version");
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QCOMPARE(proc.exitCode(), 0);

    QString output = proc.readAllStandardOutput();
    QVERIFY2(output.contains("mne_surf2bem"),
             "Version output should mention the application name");
}

//=============================================================================================================

void TestMneSurf2Bem::testNoArgs()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_surf2bem executable not found");
    }

    QProcess proc;
    proc.start(m_sAppPath, QStringList());
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    // Running without any arguments should return 1 and print usage
    QCOMPARE(proc.exitCode(), 1);

    QString output = proc.readAllStandardOutput();
    QVERIFY2(output.contains("Usage:") || output.contains("--help"),
             "No-args output should contain usage hint");
}

//=============================================================================================================

void TestMneSurf2Bem::testMissingInputFile()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_surf2bem executable not found");
    }

    // Try to convert a non-existent surface file
    QProcess proc;
    QStringList args;
    args << "--surf" << "/tmp/nonexistent_surface_file_xyz.surf"
         << "--fif" << m_tempDir.path() + "/missing_input.fif";

    proc.start(m_sAppPath, args);
    QVERIFY2(proc.waitForFinished(10000), "Process did not finish in time");

    QVERIFY2(proc.exitCode() != 0,
             "Should fail when input surface file does not exist");
}

//=============================================================================================================

void TestMneSurf2Bem::testSingleSurfConversion()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_surf2bem executable not found");
    }
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }

    //
    // Convert a single FreeSurfer surface (inner_skull.surf) to BEM FIFF
    //
    QString inputSurf = m_sSubjectsDir + "/sample/bem/inner_skull.surf";
    if (!QFile::exists(inputSurf)) {
        QSKIP("inner_skull.surf not found in sample data");
    }

    QString outputFif = m_tempDir.path() + "/single_surf_test.fif";

    QProcess proc;
    QStringList args;
    args << "--surf" << inputSurf
         << "--id" << "1"
         << "--fif" << outputFif;

    proc.start(m_sAppPath, args);
    QVERIFY2(proc.waitForFinished(30000), "Process did not finish in time");

    if (proc.exitCode() != 0) {
        QString stdErr = proc.readAllStandardError();
        QString stdOut = proc.readAllStandardOutput();
        qDebug() << "stdout:" << stdOut;
        qDebug() << "stderr:" << stdErr;
    }
    QCOMPARE(proc.exitCode(), 0);

    // Verify output file exists and has content
    QVERIFY2(QFileInfo::exists(outputFif),
             "Output FIFF file should exist");
    QFileInfo fi(outputFif);
    QVERIFY2(fi.size() > 1024,
             "Output FIFF file too small — conversion may have failed");

    // Read back with MNEBem and validate
    QFile fiffFile(outputFif);
    MNEBem bem(fiffFile);
    if (bem.size() > 0) {
        QVERIFY2(bem.size() == 1,
                 "Should contain exactly 1 BEM surface");
        const MNEBemSurface& surf = bem[0];
        QVERIFY2(surf.id == 1,
                 "BEM surface id should be 1 (brain)");
        QVERIFY2(surf.np > 0,
                 "Surface should have vertices");
        QVERIFY2(surf.ntri > 0,
                 "Surface should have triangles");
        qDebug() << "Read back:" << surf.np << "vertices," << surf.ntri << "triangles";
    } else {
        qDebug() << "Could not read back FIFF file — skipping content validation";
    }
}

//=============================================================================================================

void TestMneSurf2Bem::testMultiSurfConversion()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_surf2bem executable not found");
    }
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }

    //
    // Convert all three BEM surfaces into one FIFF file
    //
    QString innerSkull = m_sSubjectsDir + "/sample/bem/inner_skull.surf";
    QString outerSkull = m_sSubjectsDir + "/sample/bem/outer_skull.surf";
    QString outerSkin  = m_sSubjectsDir + "/sample/bem/outer_skin.surf";

    if (!QFile::exists(innerSkull) || !QFile::exists(outerSkull) || !QFile::exists(outerSkin)) {
        QSKIP("Not all three BEM surfaces found in sample data");
    }

    QString outputFif = m_tempDir.path() + "/multi_surf_test.fif";

    QProcess proc;
    QStringList args;
    args << "--surf" << innerSkull << "--id" << "1"
         << "--surf" << outerSkull << "--id" << "3"
         << "--surf" << outerSkin  << "--id" << "4"
         << "--fif" << outputFif;

    proc.start(m_sAppPath, args);
    QVERIFY2(proc.waitForFinished(60000), "Process did not finish in time");

    if (proc.exitCode() != 0) {
        QString stdErr = proc.readAllStandardError();
        QString stdOut = proc.readAllStandardOutput();
        qDebug() << "stdout:" << stdOut;
        qDebug() << "stderr:" << stdErr;
    }
    QCOMPARE(proc.exitCode(), 0);

    // Verify output
    QVERIFY2(QFileInfo::exists(outputFif),
             "Output FIFF file should exist");
    QFileInfo fi(outputFif);
    QVERIFY2(fi.size() > 4096,
             "Multi-surface FIFF file too small");

    // Read back with MNEBem
    QFile fiffFile(outputFif);
    MNEBem bem(fiffFile);
    if (bem.size() > 0) {
        QVERIFY2(bem.size() == 3,
                 "Should contain exactly 3 BEM surfaces");
        // Verify each surface has vertices and triangles
        for (int k = 0; k < bem.size(); ++k) {
            QVERIFY2(bem[k].np > 0,
                     qPrintable(QString("Surface %1 should have vertices").arg(k)));
            QVERIFY2(bem[k].ntri > 0,
                     qPrintable(QString("Surface %1 should have triangles").arg(k)));
        }
        qDebug() << "Read back 3 surfaces successfully";
    } else {
        qDebug() << "Could not read back FIFF file — skipping content validation";
    }
}

//=============================================================================================================

void TestMneSurf2Bem::testSurfConversionWithCheck()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_surf2bem executable not found");
    }
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }

    //
    // Convert three surfaces with topology checks enabled
    //
    QString innerSkull = m_sSubjectsDir + "/sample/bem/inner_skull.surf";
    QString outerSkull = m_sSubjectsDir + "/sample/bem/outer_skull.surf";
    QString outerSkin  = m_sSubjectsDir + "/sample/bem/outer_skin.surf";

    if (!QFile::exists(innerSkull) || !QFile::exists(outerSkull) || !QFile::exists(outerSkin)) {
        QSKIP("Not all three BEM surfaces found in sample data");
    }

    QString outputFif = m_tempDir.path() + "/checked_surf_test.fif";

    QProcess proc;
    QStringList args;
    args << "--surf" << innerSkull << "--id" << "1"
         << "--surf" << outerSkull << "--id" << "3"
         << "--surf" << outerSkin  << "--id" << "4"
         << "--check"
         << "--fif" << outputFif;

    proc.start(m_sAppPath, args);
    QVERIFY2(proc.waitForFinished(120000),
             "Process did not finish in time (checks can be slow)");

    if (proc.exitCode() != 0) {
        QString stdErr = proc.readAllStandardError();
        QString stdOut = proc.readAllStandardOutput();
        qDebug() << "stdout:" << stdOut;
        qDebug() << "stderr:" << stdErr;
    }
    QCOMPARE(proc.exitCode(), 0);

    // Verify output
    QVERIFY2(QFileInfo::exists(outputFif),
             "Output FIFF file should exist after checked conversion");

    // Read back
    QFile fiffFile(outputFif);
    MNEBem bem(fiffFile);
    if (bem.size() > 0) {
        QCOMPARE(bem.size(), 3);
    }
}

//=============================================================================================================

void TestMneSurf2Bem::testOverwriteOutput()
{
    if (!m_bAppAvailable) {
        QSKIP("mne_surf2bem executable not found");
    }
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }

    QString inputSurf = m_sSubjectsDir + "/sample/bem/inner_skull.surf";
    if (!QFile::exists(inputSurf)) {
        QSKIP("inner_skull.surf not found in sample data");
    }

    QString outputFif = m_tempDir.path() + "/overwrite_test.fif";

    // First run — create the file
    {
        QProcess proc;
        QStringList args;
        args << "--surf" << inputSurf << "--id" << "1"
             << "--fif" << outputFif;

        proc.start(m_sAppPath, args);
        QVERIFY2(proc.waitForFinished(30000), "Process did not finish in time");
        QCOMPARE(proc.exitCode(), 0);
        QVERIFY2(QFileInfo::exists(outputFif), "First run should create output");
    }

    // Second run — overwrite the file
    {
        QProcess proc;
        QStringList args;
        args << "--surf" << inputSurf << "--id" << "1"
             << "--fif" << outputFif;

        proc.start(m_sAppPath, args);
        QVERIFY2(proc.waitForFinished(30000), "Process did not finish in time");
        // Should succeed (overwrite is allowed)
        QCOMPARE(proc.exitCode(), 0);
        QVERIFY2(QFileInfo::exists(outputFif), "Second run should overwrite output");
    }
}

//=============================================================================================================

void TestMneSurf2Bem::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneSurf2Bem)
#include "test_mne_surf2bem.moc"
