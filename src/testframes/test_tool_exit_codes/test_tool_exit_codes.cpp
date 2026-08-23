//=============================================================================================================
/**
 * @file     test_tool_exit_codes.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, MNE-CPP Authors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @brief    Assert that command line tools report failure through their exit status.
 *
 * A tool that prints an error and then exits zero is worse than one that says
 * nothing at all: every script built on it treats the run as a success. This
 * test drives a handful of tools into their most ordinary failure - a required
 * argument that is missing, or an input file that does not exist - and also
 * exercises representative successful file operations.
 *
 * It deliberately does not assert on message text. Wording changes for good
 * reasons and a test that pins it down turns every improvement to an error
 * message into a test failure. The exit status is the part other programs
 * depend on, so the exit status is what is fixed here.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestToolExitCodes
 *
 * @brief The TestToolExitCodes class provides observable-failure tests for the command line tools.
 *
 */
class TestToolExitCodes : public QObject
{
    Q_OBJECT

public:
    TestToolExitCodes();

private slots:
    void initTestCase();
    void missingRequiredArguments_data();
    void missingRequiredArguments();
    void unreadableInputFile_data();
    void unreadableInputFile();
    void helpSucceeds_data();
    void helpSucceeds();
    void invalidListSourceSpaceInputs();
    void validFiffFileOperations();

private:
    /** Locate a tool next to the test binary, or an empty string when it was not built. */
    QString findTool(const QString& name) const;

    /** Give the child process a chance of finding the Qt and MNE-CPP libraries on Windows. */
    void setupProcess(QProcess& proc) const;

    /** Run a tool and return its exit code, or -1 when it could not be run at all. */
    int runTool(const QString& path, const QStringList& arguments) const;

    /** Delete anything a previous case left behind at the missing-input paths. */
    void clearMissingPaths() const;

    QTemporaryDir m_tempDir;
    QString m_sMissingFile;
    QString m_sMissingOutFile;
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TestToolExitCodes::TestToolExitCodes()
{
}

//=============================================================================================================

void TestToolExitCodes::initTestCase()
{
    qInstallMessageHandler(MNELogger::customLogWriter);

    // Scoped to a directory that is created and removed per run. Some tools open
    // their input read-write and so leave an empty file behind on failure, which
    // would make a fixed path in the system temp directory exist on the second
    // run and quietly turn these into different tests.
    QVERIFY(m_tempDir.isValid());
    m_sMissingFile = m_tempDir.path() + "/no_such_file.fif";
    m_sMissingOutFile = m_tempDir.path() + "/no_such_output.fif";
}

//=============================================================================================================

void TestToolExitCodes::clearMissingPaths() const
{
    QFile::remove(m_sMissingFile);
    QFile::remove(m_sMissingOutFile);
}

//=============================================================================================================

QString TestToolExitCodes::findTool(const QString& name) const
{
    const QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_WIN
    const QString suffix = ".exe";
#else
    const QString suffix = QString();
#endif

    const QStringList candidates = {
        appDir + "/" + name + suffix,
        appDir + "/../apps/" + name + suffix,
        appDir + "/../bin/" + name + suffix
    };

    for (const QString& path : candidates) {
        const QFileInfo info(path);
        if (info.exists() && info.isExecutable()) {
            return info.canonicalFilePath();
        }
    }

    return QString();
}

//=============================================================================================================

void TestToolExitCodes::setupProcess(QProcess& proc) const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // The Windows loader does not resolve ".." inside PATH entries, so these
    // have to be cleaned into absolute paths before they are of any use.
    const QString qtBinDir = QDir::cleanPath(
        QCoreApplication::applicationDirPath() + "/../../../src/external/qt/dynamic/bin");
    const QString appDir = QDir::cleanPath(QCoreApplication::applicationDirPath());

#ifdef Q_OS_WIN
    const QChar separator = ';';
#else
    const QChar separator = ':';
#endif

    env.insert("PATH", appDir + separator + qtBinDir + separator + env.value("PATH"));
    proc.setProcessEnvironment(env);
}

//=============================================================================================================

int TestToolExitCodes::runTool(const QString& path, const QStringList& arguments) const
{
    QProcess proc;
    setupProcess(proc);
    proc.start(path, arguments);

    if (!proc.waitForStarted(30000)) {
        return -1;
    }
    if (!proc.waitForFinished(120000)) {
        proc.kill();
        proc.waitForFinished(5000);
        return -1;
    }
    if (proc.exitStatus() != QProcess::NormalExit) {
        return -1;
    }

    return proc.exitCode();
}

//=============================================================================================================

void TestToolExitCodes::missingRequiredArguments_data()
{
    QTest::addColumn<QString>("tool");

    // Each of these refuses to run without arguments. None of them needs test
    // data to reach that decision, so the case is available on every platform.
    QTest::newRow("mne_fix_mag_coil_types") << "mne_fix_mag_coil_types";
    QTest::newRow("mne_add_triggers")       << "mne_add_triggers";
    QTest::newRow("mne_change_nave")        << "mne_change_nave";
    QTest::newRow("mne_mark_bad_channels")  << "mne_mark_bad_channels";
    QTest::newRow("mne_cov2proj")            << "mne_cov2proj";
    QTest::newRow("mne_compare_fif_files")   << "mne_compare_fif_files";
    QTest::newRow("mne_collect_transforms")  << "mne_collect_transforms";
    QTest::newRow("mne_add_patch_info")      << "mne_add_patch_info";
    QTest::newRow("mne_list_source_space")   << "mne_list_source_space";
    QTest::newRow("mne_epochs2mat")           << "mne_epochs2mat";
    QTest::newRow("mne_toggle_skips")       << "mne_toggle_skips";
}

//=============================================================================================================

void TestToolExitCodes::missingRequiredArguments()
{
    QFETCH(QString, tool);

    const QString path = findTool(tool);
    if (path.isEmpty()) {
        QSKIP("tool was not built in this configuration");
    }

    const int code = runTool(path, QStringList());
    QVERIFY2(code != -1, "the tool could not be started at all");
    QVERIFY2(code != 0,
             qPrintable(tool + " exited successfully although its required arguments were missing"));
}

//=============================================================================================================

void TestToolExitCodes::unreadableInputFile_data()
{
    QTest::addColumn<QString>("tool");
    QTest::addColumn<QStringList>("arguments");

    // mne_fix_mag_coil_types used to report every file as failed and still exit
    // zero, which made it impossible to use from a script.
    QTest::newRow("mne_fix_mag_coil_types")
        << "mne_fix_mag_coil_types" << QStringList{"__MISSING__"};

    QTest::newRow("mne_change_nave")
        << "mne_change_nave"
        << QStringList{"--meas", "__MISSING__", "--nave", "1", "--out", "__MISSING_OUT__"};

    QTest::newRow("mne_mark_bad_channels")
        << "mne_mark_bad_channels"
        << QStringList{"--fif", "__MISSING__", "--bad", "__MISSING__"};

    QTest::newRow("mne_cov2proj")
        << "mne_cov2proj"
        << QStringList{"--cov", "__MISSING__", "--out", "__MISSING_OUT__"};

    QTest::newRow("mne_compare_fif_files")
        << "mne_compare_fif_files"
        << QStringList{"--file1", "__MISSING__", "--file2", "__MISSING__"};

    QTest::newRow("mne_collect_transforms")
        << "mne_collect_transforms" << QStringList{"--meas", "__MISSING__"};

    QTest::newRow("mne_add_patch_info")
        << "mne_add_patch_info"
        << QStringList{"--src", "__MISSING__", "--out", "__MISSING_OUT__"};

    QTest::newRow("mne_list_source_space")
        << "mne_list_source_space" << QStringList{"--src", "__MISSING__"};

    QTest::newRow("mne_epochs2mat")
        << "mne_epochs2mat"
        << QStringList{"--raw", "__MISSING__", "--event", "__MISSING__",
                       "--tmin", "-0.1", "--tmax", "0.2", "--event-id", "1",
                       "--out", "__MISSING_OUT__"};
}

//=============================================================================================================

void TestToolExitCodes::unreadableInputFile()
{
    QFETCH(QString, tool);
    QFETCH(QStringList, arguments);

    const QString path = findTool(tool);
    if (path.isEmpty()) {
        QSKIP("tool was not built in this configuration");
    }

    clearMissingPaths();
    QVERIFY(!QFileInfo::exists(m_sMissingFile));

    arguments.replaceInStrings("__MISSING__", m_sMissingFile);
    arguments.replaceInStrings("__MISSING_OUT__", m_sMissingOutFile);

    const int code = runTool(path, arguments);
    QVERIFY2(code != -1, "the tool could not be started at all");
    QVERIFY2(code != 0,
             qPrintable(tool + " exited successfully although its input file does not exist"));
    QVERIFY2(!QFileInfo::exists(m_sMissingFile),
             qPrintable(tool + " created the input file it was supposed to be unable to read"));
}

//=============================================================================================================

void TestToolExitCodes::helpSucceeds_data()
{
    QTest::addColumn<QString>("tool");

    // The counterpart to the cases above: asking for help is not an error, and
    // a tool that fails here would make the failure cases meaningless.
    QTest::newRow("mne_fix_mag_coil_types") << "mne_fix_mag_coil_types";
    QTest::newRow("mne_mark_bad_channels")  << "mne_mark_bad_channels";
    QTest::newRow("mne_change_nave")        << "mne_change_nave";
    QTest::newRow("mne_cov2proj")            << "mne_cov2proj";
    QTest::newRow("mne_compare_fif_files")   << "mne_compare_fif_files";
    QTest::newRow("mne_collect_transforms")  << "mne_collect_transforms";
    QTest::newRow("mne_add_patch_info")      << "mne_add_patch_info";
    QTest::newRow("mne_list_source_space")   << "mne_list_source_space";
    QTest::newRow("mne_epochs2mat")           << "mne_epochs2mat";
}

//=============================================================================================================

void TestToolExitCodes::helpSucceeds()
{
    QFETCH(QString, tool);

    const QString path = findTool(tool);
    if (path.isEmpty()) {
        QSKIP("tool was not built in this configuration");
    }

    const int code = runTool(path, QStringList{"--help"});
    QVERIFY2(code != -1, "the tool could not be started at all");
    QCOMPARE(code, 0);
}

//=============================================================================================================

void TestToolExitCodes::invalidListSourceSpaceInputs()
{
    const QString listSourceSpace = findTool("mne_list_source_space");
    if (listSourceSpace.isEmpty()) {
        QSKIP("mne_list_source_space was not built in this configuration");
    }

    const QString sourceFile = QCoreApplication::applicationDirPath()
                               + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/"
                                 "sample-oct-6-src.fif";
    if (!QFileInfo::exists(sourceFile)) {
        QSKIP("source-space test data is not available");
    }

    QCOMPARE(runTool(listSourceSpace, {"--src", sourceFile, "--coord", "scanner"}), 1);

    const QString malformedFile = m_tempDir.filePath("malformed-source-space.fif");
    QFile file(malformedFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write("not a FIFF file"), 15);
    file.close();
    QCOMPARE(runTool(listSourceSpace, {"--src", malformedFile}), 1);

    QCOMPARE(runTool(listSourceSpace, {"--src", sourceFile, "--pnt", m_tempDir.path()}), 1);
    QCOMPARE(runTool(listSourceSpace, {"--src", sourceFile, "--dip", m_tempDir.path()}), 1);
}

//=============================================================================================================

void TestToolExitCodes::validFiffFileOperations()
{
    const QString dataDir = QCoreApplication::applicationDirPath()
                            + "/../resources/data/mne-cpp-test-data/MEG/sample/";
    const QString covFile = dataDir + "sample_audvis-cov.fif";
    const QString evokedFile = dataDir + "sample_audvis-ave.fif";
    const QString rawFile = dataDir + "sample_audvis_trunc_raw.fif";
    const QString transFile = dataDir + "all-trans.fif";
    QVERIFY(QFileInfo::exists(covFile));
    QVERIFY(QFileInfo::exists(evokedFile));
    QVERIFY(QFileInfo::exists(rawFile));
    QVERIFY(QFileInfo::exists(transFile));

    const QString cov2proj = findTool("mne_cov2proj");
    const QString compareFif = findTool("mne_compare_fif_files");
    if (!cov2proj.isEmpty()) {
        const QString outputFile = m_tempDir.filePath("covariance-projections.fif");
        QCOMPARE(runTool(cov2proj,
                         {"--cov", covFile, "--nproj", "2", "--out", outputFile}),
                 0);
        QVERIFY(QFileInfo(outputFile).size() > 0);
        if (!compareFif.isEmpty()) {
            QCOMPARE(runTool(compareFif, {"--file1", outputFile, "--file2", outputFile}), 0);
        }
    }

    if (!compareFif.isEmpty()) {
        QCOMPARE(runTool(compareFif, {"--file1", covFile, "--file2", covFile}), 0);
        QCOMPARE(runTool(compareFif, {"--file1", covFile, "--file2", evokedFile}), 1);
    }

    const QString collectTransforms = findTool("mne_collect_transforms");
    if (!collectTransforms.isEmpty()) {
        const QString outputFile = m_tempDir.filePath("collected-transforms.fif");
        QCOMPARE(runTool(collectTransforms,
                         {"--meas", rawFile, "--mri", transFile, "--out", outputFile}),
                 0);
        QVERIFY(QFileInfo(outputFile).size() > 0);
        if (!compareFif.isEmpty()) {
            QCOMPARE(runTool(compareFif, {"--file1", outputFile, "--file2", outputFile}), 0);
        }
    }

    const QString addPatchInfo = findTool("mne_add_patch_info");
    const QString listSourceSpace = findTool("mne_list_source_space");
    const QString sourceFile = QCoreApplication::applicationDirPath()
                               + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/"
                                 "sample-oct-6-src.fif";
    if (!addPatchInfo.isEmpty() && QFileInfo::exists(sourceFile)) {
        const QString outputFile = m_tempDir.filePath("source-space-with-patches.fif");
        QCOMPARE(runTool(addPatchInfo, {"--src", sourceFile, "--out", outputFile}), 0);
        QVERIFY(QFileInfo(outputFile).size() > 0);
        if (!listSourceSpace.isEmpty()) {
            const QString pointFile = m_tempDir.filePath("source-space.pnt");
            const QString dipoleFile = m_tempDir.filePath("source-space.dip");
            const QString vertexFile = m_tempDir.filePath("source-space.vert");
            QCOMPARE(runTool(listSourceSpace,
                             {"--src", outputFile, "--coord", "head",
                              "--pnt", pointFile, "--dip", dipoleFile,
                              "--vert", vertexFile}),
                     0);
            QVERIFY(QFileInfo(pointFile).size() > 0);
            QVERIFY(QFileInfo(dipoleFile).size() > 0);
            QVERIFY(QFileInfo(vertexFile).size() > 0);
        }
    }
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestToolExitCodes)
#include "test_tool_exit_codes.moc"
