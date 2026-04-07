//=============================================================================================================
/**
 * @file     test_batch_processor.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
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
 * @brief    Unit tests for BatchProcessor (mne_process_raw processing pipeline).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../tools/preprocessing/mne_process_raw/batchprocessor.h"

#include <mne/mne_process_description.h>

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
using namespace MNELIB;

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

    // run pipeline tests
    void testRunNoRawFiles();
    void testRunWithSampleData();
    void testRunSaveFilteredData();

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
    QString saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "/path/to/data.fif", "_processed", false, saveName, logName);

    QVERIFY(ok);
    QVERIFY(saveName.endsWith("_processed.fif"));
    QVERIFY(logName.endsWith("_processed.log"));
}

void TestBatchProcessor::testComposeSaveNamesStripRawSuffix()
{
    QString saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "/path/to/data_raw.fif", "_ave", false, saveName, logName);

    QVERIFY(ok);
    QCOMPARE(saveName, QString("/path/to/data_ave.fif"));
    QCOMPARE(logName, QString("/path/to/data_ave.log"));
}

void TestBatchProcessor::testComposeSaveNamesStripFifSuffix()
{
    QString saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "/path/to/sample.fif", "_cov", false, saveName, logName);

    QVERIFY(ok);
    QCOMPARE(saveName, QString("/path/to/sample_cov.fif"));
    QCOMPARE(logName, QString("/path/to/sample_cov.log"));
}

void TestBatchProcessor::testComposeSaveNamesStripSssSuffix()
{
    QString saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "/path/to/data_sss_raw.fif", "_ave", false, saveName, logName);

    QVERIFY(ok);
    // Both _sss and _raw.fif should be stripped
    QCOMPARE(saveName, QString("/path/to/data_ave.fif"));
    QCOMPARE(logName, QString("/path/to/data_ave.log"));
}

void TestBatchProcessor::testComposeSaveNamesStripDir()
{
    QString saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "/long/path/to/data_raw.fif", "_processed", true, saveName, logName);

    QVERIFY(ok);
    // Directory should be stripped
    QVERIFY(!saveName.contains("/long/path/"));
    QCOMPARE(saveName, QString("data_processed.fif"));
}

void TestBatchProcessor::testComposeSaveNamesEmptyTag()
{
    QString saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "/path/to/data.fif", "", false, saveName, logName);

    QVERIFY(!ok);
}

void TestBatchProcessor::testComposeSaveNamesEmptyRawName()
{
    QString saveName, logName;
    bool ok = BatchProcessor::composeSaveNames(
        "", "_tag", false, saveName, logName);

    QVERIFY(!ok);
}

//=============================================================================================================
// writeLog tests
//=============================================================================================================

void TestBatchProcessor::testWriteLogBasic()
{
    QString logPath = m_tempDir.path() + "/test_log.log";
    QString logContent = "Test log content\nLine 2\n";

    bool ok = BatchProcessor::writeLog(logPath, logContent);
    QVERIFY(ok);

    QFile logFile(logPath);
    QVERIFY(logFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString readBack = logFile.readAll();
    logFile.close();

    QCOMPARE(readBack, logContent);
}

void TestBatchProcessor::testWriteLogEmptyParams()
{
    // Empty log file path or content should return true (no-op)
    QVERIFY(BatchProcessor::writeLog("", "content"));
    QVERIFY(BatchProcessor::writeLog("/some/path.log", ""));
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

void TestBatchProcessor::testRunWithSampleData()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample raw data not available");

    ProcessingSettings settings;
    settings.rawFiles << rawPath;
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
    settings.rawFiles << rawPath;
    settings.saveFiles << outPath;
    settings.decimation = 1;

    int result = BatchProcessor::run(settings);
    QCOMPARE(result, 0);

    // Verify output file was created
    QVERIFY(QFile::exists(outPath));
    QFileInfo fi(outPath);
    QVERIFY(fi.size() > 0);
}

//=============================================================================================================

void TestBatchProcessor::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestBatchProcessor)
#include "test_batch_processor.moc"
