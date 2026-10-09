//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *

 * @file     test_writetofile_status.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Unit tests for the WriteToFile plugin recording-status indicator.
 */

#include <writetofile/writetofile.h>
#include <writetofile/FormFiles/writetofilestatuswidget.h>

#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_stream.h>

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QObject>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace WRITETOFILEPLUGIN;

class TestWriteToFileStatus : public QObject
{
    Q_OBJECT

private slots:
    void formatHelpers_produceExpectedStrings();
    void emitRecordingStatus_isRepeatableAndMonotonic();
    void statusWidget_reflectsLatestSummary();
    void recording_refusesUnwritableFile();
};

//=============================================================================================================

void TestWriteToFileStatus::formatHelpers_produceExpectedStrings()
{
    QCOMPARE(WriteToFile::formatElapsed(0), QStringLiteral("00:00:00"));
    QCOMPARE(WriteToFile::formatElapsed(7'500), QStringLiteral("00:00:07"));
    QCOMPARE(WriteToFile::formatElapsed(125'000), QStringLiteral("00:02:05"));
    QCOMPARE(WriteToFile::formatElapsed(3'661'000), QStringLiteral("01:01:01"));

    QCOMPARE(WriteToFile::formatBytes(0), QStringLiteral("0 B"));
    QCOMPARE(WriteToFile::formatBytes(512), QStringLiteral("512 B"));
    QCOMPARE(WriteToFile::formatBytes(2048), QStringLiteral("2.0 KB"));
    QCOMPARE(WriteToFile::formatBytes(2 * 1024 * 1024), QStringLiteral("2.0 MB"));
}

//=============================================================================================================

void TestWriteToFileStatus::emitRecordingStatus_isRepeatableAndMonotonic()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString filePath = tmpDir.filePath(QStringLiteral("recording.fif"));

    WriteToFile plugin;

    // Open a real on-disk file under the plugin's QFile member so QFileInfo(...).size()
    // returns growing values; bypasses the GUI-driven toggleRecordingFile() flow.
    plugin.m_qFileOut.setFileName(filePath);
    QVERIFY(plugin.m_qFileOut.open(QIODevice::ReadWrite));
    plugin.m_bWriteToFile = true;
    plugin.m_recordingStartedTime.start();

    QSignalSpy spy(&plugin, &WriteToFile::recordingStatus);

    // Emissions are driven directly rather than from a repeating timer. Asking a 1 s
    // timer to fire three times inside a 3.2 s wait left 200 ms of slack, which a
    // loaded macOS runner spent, delivering two ticks and failing the count.
    const int iExpectedEmissions = 4;
    for (int i = 0; i < iExpectedEmissions; ++i) {
        const QByteArray chunk(1024, 'x');
        plugin.m_qFileOut.write(chunk);
        plugin.m_qFileOut.flush();
        QTest::qWait(600);
        plugin.emitRecordingStatus();
    }

    plugin.m_bWriteToFile = false;
    plugin.m_qFileOut.close();

    QCOMPARE(spy.count(), iExpectedEmissions);

    // Each emitted summary must match "HH:MM:SS  <num><unit>" and have non-decreasing size.
    static const QRegularExpression re(
        QStringLiteral("^(\\d{2}):(\\d{2}):(\\d{2})\\s+([0-9]+(?:\\.[0-9]+)?)\\s*(B|KB|MB|GB)$"));

    double lastBytes = -1.0;
    int lastTotalSecs = -1;
    for (const QList<QVariant>& args : spy) {
        const QString summary = args.first().toString();
        const auto m = re.match(summary);
        QVERIFY2(m.hasMatch(), qPrintable(QStringLiteral("Bad summary format: %1").arg(summary)));

        const int h = m.captured(1).toInt();
        const int mi = m.captured(2).toInt();
        const int s = m.captured(3).toInt();
        const int totalSecs = h * 3600 + mi * 60 + s;
        QVERIFY(totalSecs >= lastTotalSecs);
        lastTotalSecs = totalSecs;

        double value = m.captured(4).toDouble();
        const QString unit = m.captured(5);
        if (unit == QLatin1String("KB"))
            value *= 1024.0;
        else if (unit == QLatin1String("MB"))
            value *= 1024.0 * 1024.0;
        else if (unit == QLatin1String("GB"))
            value *= 1024.0 * 1024.0 * 1024.0;
        QVERIFY2(value >= lastBytes,
                 qPrintable(QStringLiteral("File size regressed: %1 -> %2").arg(lastBytes).arg(value)));
        lastBytes = value;
    }

    // The final summary's elapsed time must approximately match the QElapsedTimer reading.
    const QString last = spy.last().first().toString();
    const auto lastMatch = re.match(last);
    QVERIFY(lastMatch.hasMatch());
    const int lastSecs = lastMatch.captured(1).toInt() * 3600 + lastMatch.captured(2).toInt() * 60 + lastMatch.captured(3).toInt();
    // qWait() guarantees a lower bound, so the four 600 ms waits put at least 2 s on
    // the recording clock however slow the machine is.
    QVERIFY2(lastSecs >= 2,
             qPrintable(QStringLiteral("Last elapsed=%1s too small").arg(lastSecs)));
}

//=============================================================================================================

void TestWriteToFileStatus::statusWidget_reflectsLatestSummary()
{
    WriteToFile plugin;
    WriteToFileStatusWidget widget(&plugin);

    QVERIFY(!widget.isActive());
    QCOMPARE(widget.currentText(), QStringLiteral("Not recording"));

    emit plugin.recordingActiveChanged(true);
    emit plugin.recordingStatus(QStringLiteral("00:00:01  1.5 MB"));
    QCoreApplication::processEvents();

    QVERIFY(widget.isActive());
    QCOMPARE(widget.currentText(), QStringLiteral("00:00:01  1.5 MB"));

    emit plugin.recordingStatus(QStringLiteral("00:00:02  3.1 MB"));
    QCoreApplication::processEvents();
    QCOMPARE(widget.currentText(), QStringLiteral("00:00:02  3.1 MB"));

    emit plugin.recordingActiveChanged(false);
    QCoreApplication::processEvents();
    QVERIFY(!widget.isActive());
    QCOMPARE(widget.currentText(), QStringLiteral("Not recording"));
}

void TestWriteToFileStatus::recording_refusesUnwritableFile()
{
    QTemporaryDir tmpDir;
    WriteToFile plugin;
    plugin.m_pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>::create();
    plugin.m_pFiffInfo->sfreq = 1000.0;
    FIFFLIB::FiffChInfo ch;
    ch.ch_name = QStringLiteral("EEG001");
    ch.kind = FIFFV_EEG_CH;
    plugin.m_pFiffInfo->chs << ch;
    plugin.m_pFiffInfo->ch_names << ch.ch_name;
    plugin.m_pFiffInfo->nchan = 1;
    plugin.m_pFiffInfo->dev_head_t.trans(0, 3) = 0.01f; // not identity: no "HPI fitting" question
    plugin.m_sRecordFileName = tmpDir.filePath(QStringLiteral("no/such/dir/rec_raw.fif"));

    // Starting is refused with a message box instead of writing through a null stream
    QTimer::singleShot(0, [] {
        if (QWidget* box = QApplication::activeModalWidget()) {
            box->close();
        }
    });
    plugin.toggleRecordingFile();
    QVERIFY(!plugin.m_bWriteToFile);
    QVERIFY(!plugin.m_pOutfid);

    // A split whose next file cannot be created stops writing instead of crashing
    QFile first(tmpDir.filePath(QStringLiteral("rec_raw.fif")));
    plugin.m_pOutfid = FIFFLIB::FiffStream::start_writing_raw(first, *plugin.m_pFiffInfo, plugin.m_mCals);
    QVERIFY(plugin.m_pOutfid);
    plugin.m_bWriteToFile = true;
    QTest::ignoreMessage(QtCriticalMsg, QRegularExpression("Cannot write"));
    plugin.splitRecordingFile();
    QVERIFY(!plugin.m_pOutfid);
    QVERIFY(!plugin.m_bWriteToFile);
    FIFFLIB::FiffStream finished(&first); // the first file was finished and links to the next one
    QVERIFY(finished.open());
    QVERIFY(finished.dirtree()->has_kind(FIFFB_REF));

    // A successful split continues in <name>-<n>_raw.fif and keeps the configured file name
    const QString recordFileName = tmpDir.filePath(QStringLiteral("rec_raw.fif"));
    plugin.m_sRecordFileName = recordFileName;
    plugin.m_iSplitCount = 0;
    plugin.m_qFileOut.setFileName(recordFileName);
    plugin.m_pOutfid = FIFFLIB::FiffStream::start_writing_raw(plugin.m_qFileOut, *plugin.m_pFiffInfo, plugin.m_mCals);
    plugin.m_bWriteToFile = true;
    plugin.splitRecordingFile();
    QVERIFY(plugin.m_pOutfid);
    QCOMPARE(plugin.m_qFileOut.fileName(), tmpDir.filePath(QStringLiteral("rec-1_raw.fif")));
    QCOMPARE(plugin.m_sRecordFileName, recordFileName);
    plugin.splitRecordingFile();
    QCOMPARE(plugin.m_qFileOut.fileName(), tmpDir.filePath(QStringLiteral("rec-2_raw.fif")));
    plugin.m_pOutfid->finish_writing_raw();
    plugin.m_bWriteToFile = false;
}

//=============================================================================================================

QTEST_MAIN(TestWriteToFileStatus)
#include "test_writetofile_status.moc"
