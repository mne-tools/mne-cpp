//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_batch_processor.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
 * @brief    Unit tests for BatchProcessor (mne_process_raw processing pipeline).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../tools/preprocessing/mne_process_raw/batchprocessor.h"

#include <mne/mne/mne_process_description.h>

#include <mne/fiff/fiff_events.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_raw_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEPROCESSRAWAPP;
using namespace mne;

//=============================================================================================================
/**
 * @brief Tests for BatchProcessor: composeSaveNames, writeLog, and run pipeline.
 */
class TestBatchProcessor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // composeSaveNames tests
    void testComposeSaveNamesBasic();
    void testComposeSaveNamesStripRawSuffix();
    void testComposeSaveNamesStripFifSuffix();
    void testComposeSaveNamesStripSssSuffix();
    void testComposeSaveNamesStripDir();
    void testComposeSaveNamesEmptyTag();
    void testComposeSaveNamesEmptyRawName();

    // writeLog tests
    void testWriteLogBasic();
    void testWriteLogEmptyParams();
    void testWriteLogOpenFailure();

    // run pipeline tests
    void testRunNoRawFiles();
    void testRunInvalidRawFile();
    void testRunWithSampleData();
    void testRunSaveFilteredData();
    void testRunInvalidAverageDescription();
    void testRunInvalidCovarianceDescription();
    void testRunDetectsEventsWithoutMask();
    void testRunGrandAverageWeightsByNave();

    void cleanupTestCase();

private:
    QString m_sResourcePath;
    QTemporaryDir m_tempDir;
};

//=============================================================================================================

void TestBatchProcessor::initTestCase()
{
    QString binDir = QCoreApplication::applicationDirPath();
    m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================
// composeSaveNames tests
//=============================================================================================================

void TestBatchProcessor::testComposeSaveNamesBasic()
{
    std::string saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "/path/to/data.fif", "_processed", false, saveName, logName);

    QVERIFY(ok);
    QVERIFY(saveName.ends_with("_processed.fif"));
    QVERIFY(logName.ends_with("_processed.log"));
}

void TestBatchProcessor::testComposeSaveNamesStripRawSuffix()
{
    std::string saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "/path/to/data_raw.fif", "_ave", false, saveName, logName);

    QVERIFY(ok);
    QCOMPARE(saveName, std::string("/path/to/data_ave.fif"));
    QCOMPARE(logName, std::string("/path/to/data_ave.log"));
}

void TestBatchProcessor::testComposeSaveNamesStripFifSuffix()
{
    std::string saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "/path/to/sample.fif", "_cov", false, saveName, logName);

    QVERIFY(ok);
    QCOMPARE(saveName, std::string("/path/to/sample_cov.fif"));
    QCOMPARE(logName, std::string("/path/to/sample_cov.log"));
}

void TestBatchProcessor::testComposeSaveNamesStripSssSuffix()
{
    std::string saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "/path/to/data_sss_raw.fif", "_ave", false, saveName, logName);

    QVERIFY(ok);
    // Both _sss and _raw.fif should be stripped
    QCOMPARE(saveName, std::string("/path/to/data_ave.fif"));
    QCOMPARE(logName, std::string("/path/to/data_ave.log"));
}

void TestBatchProcessor::testComposeSaveNamesStripDir()
{
    std::string saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "/long/path/to/data_raw.fif", "_processed", true, saveName, logName);

    QVERIFY(ok);
    // Directory should be stripped
    QVERIFY(!saveName.contains("/long/path/"));
    QCOMPARE(saveName, std::string("data_processed.fif"));
}

void TestBatchProcessor::testComposeSaveNamesEmptyTag()
{
    std::string saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "/path/to/data.fif", "", false, saveName, logName);

    QVERIFY(!ok);
}

void TestBatchProcessor::testComposeSaveNamesEmptyRawName()
{
    std::string saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "", "_tag", false, saveName, logName);

    QVERIFY(!ok);
}

//=============================================================================================================
// writeLog tests
//=============================================================================================================

void TestBatchProcessor::testWriteLogBasic()
{
    const QString logPath = m_tempDir.path() + "/test_log.log";
    const std::string logContent = "Test log content\nLine 2\n";

    bool ok = BatchProcessor::writeLog(logPath.toStdString(), logContent);
    QVERIFY(ok);

    QFile logFile(logPath);
    QVERIFY(logFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString readBack = logFile.readAll();
    logFile.close();

    QCOMPARE(readBack, QString::fromStdString(logContent));
}

void TestBatchProcessor::testWriteLogEmptyParams()
{
    // Empty log file path or content should return true (no-op)
    QVERIFY(BatchProcessor::writeLog("", "content"));
    QVERIFY(BatchProcessor::writeLog("/some/path.log", ""));
}

void TestBatchProcessor::testWriteLogOpenFailure()
{
    QVERIFY(!BatchProcessor::writeLog(m_tempDir.path().toStdString(), "content"));
}

//=============================================================================================================
// run pipeline tests
//=============================================================================================================

void TestBatchProcessor::testRunNoRawFiles()
{
    ProcessingSettings settings;
    // No raw files specified — should fail
    int result = BatchProcessor::run(settings);
    QVERIFY(result != 0);
}

void TestBatchProcessor::testRunInvalidRawFile()
{
    QString rawPath = m_tempDir.path() + "/not_raw.fif";
    QFile rawFile(rawPath);
    QVERIFY(rawFile.open(QIODevice::WriteOnly | QIODevice::Text));
    rawFile.write("not a FIFF file");
    rawFile.close();

    ProcessingSettings settings;
    settings.rawFiles.push_back(rawPath.toStdString());
    QVERIFY(BatchProcessor::run(settings) != 0);
}

void TestBatchProcessor::testRunWithSampleData()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    ProcessingSettings settings;
    settings.rawFiles.push_back(rawPath.toStdString());
    // Default settings: no averaging, no covariance, just open and process
    int result = BatchProcessor::run(settings);
    QCOMPARE(result, 0);
}

void TestBatchProcessor::testRunSaveFilteredData()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    QString outPath = m_tempDir.path() + "/processed_raw.fif";

    ProcessingSettings settings;
    settings.rawFiles.push_back(rawPath.toStdString());
    settings.saveFiles.push_back(outPath.toStdString());
    settings.decimation = 1;

    int result = BatchProcessor::run(settings);
    QCOMPARE(result, 0);

    // Verify output file was created
    QVERIFY(QFile::exists(outPath));
    QFileInfo fi(outPath);
    QVERIFY(fi.size() > 0);
}

void TestBatchProcessor::testRunInvalidAverageDescription()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    ProcessingSettings settings;
    settings.rawFiles.push_back(rawPath.toStdString());
    settings.aveFiles.push_back((m_tempDir.path() + "/missing-average-description.txt").toStdString());

    QVERIFY(BatchProcessor::run(settings) != 0);
}

void TestBatchProcessor::testRunInvalidCovarianceDescription()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    ProcessingSettings settings;
    settings.rawFiles.push_back(rawPath.toStdString());
    settings.covFiles.push_back((m_tempDir.path() + "/missing-covariance-description.txt").toStdString());

    QVERIFY(BatchProcessor::run(settings) != 0);
}

void TestBatchProcessor::testRunDetectsEventsWithoutMask()
{
    // As in MNE-C (mne_raw_scan_triggers), a trigger mask of 0 means "no masking".
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    ProcessingSettings settings;
    settings.rawFiles.push_back(rawPath.toStdString());
    settings.eventsOutFiles.push_back((m_tempDir.path() + "/detected-eve.fif").toStdString());
    QCOMPARE(BatchProcessor::run(settings), 0);

    const mne::FiffEvents events = mne::FiffEvents::read_fif(settings.eventsOutFiles.front()).value();
    QVERIFY2(events.num_events() > 0, "No events detected on STI 014");
}

void TestBatchProcessor::testRunGrandAverageWeightsByNave()
{
    // MNE-C mne_process_raw (mne_browse_raw/grand_average.c) weights every run by its
    // nave, sums the naves, keeps all channels and the category comments.
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    const QString aveDesc = m_tempDir.path() + "/grand.ave";
    {
        QFile f(aveDesc);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        f.write("average {\n    category {\n        name \"Auditory Left\"\n        event 1\n"
                "        tmin -0.1\n        tmax 0.3\n    }\n}\n");
    }

    ProcessingSettings settings;
    settings.rawFiles = {rawPath.toStdString(), rawPath.toStdString()};
    settings.aveFiles.push_back(aveDesc.toStdString());
    settings.grandAveFile = (m_tempDir.path() + "/grand-ave.fif").toStdString();
    QCOMPARE(BatchProcessor::run(settings), 0);

    const mne::FiffRawData raw(rawPath.toStdString());
    const mne::FiffEvokedSet grand(mne::path_from_utf8(settings.grandAveFile));
    QCOMPARE(grand.evoked.size(), 1);
    const mne::FiffEvoked& g = grand.evoked.front();
    QCOMPARE(g.comment, std::string("Auditory Left"));
    QCOMPARE(g.info.nchan, raw.info.nchan);
    QVERIFY2(g.nave > 2 && g.nave % 2 == 0, qPrintable(QString("nave %1").arg(g.nave)));
}

//=============================================================================================================

void TestBatchProcessor::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestBatchProcessor)
#include "test_batch_processor.moc"
