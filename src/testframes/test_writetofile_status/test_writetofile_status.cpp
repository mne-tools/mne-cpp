//=============================================================================================================
/**
 * @file     test_writetofile_status.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Unit tests for the WriteToFile plugin recording-status indicator.
 */

#include <writetofile/writetofile.h>
#include <writetofile/FormFiles/writetofilestatuswidget.h>

#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

using namespace WRITETOFILEPLUGIN;

class TestWriteToFileStatus : public QObject
{
    Q_OBJECT

private slots:
    void formatHelpers_produceExpectedStrings();
    void emitRecordingStatus_isPeriodicAndMonotonic();
    void statusWidget_reflectsLatestSummary();
};

//=============================================================================================================

void TestWriteToFileStatus::formatHelpers_produceExpectedStrings()
{
    QCOMPARE(WriteToFile::formatElapsed(0),                  QStringLiteral("00:00:00"));
    QCOMPARE(WriteToFile::formatElapsed(7'500),              QStringLiteral("00:00:07"));
    QCOMPARE(WriteToFile::formatElapsed(125'000),            QStringLiteral("00:02:05"));
    QCOMPARE(WriteToFile::formatElapsed(3'661'000),          QStringLiteral("01:01:01"));

    QCOMPARE(WriteToFile::formatBytes(0),                    QStringLiteral("0 B"));
    QCOMPARE(WriteToFile::formatBytes(512),                  QStringLiteral("512 B"));
    QCOMPARE(WriteToFile::formatBytes(2048),                 QStringLiteral("2.0 KB"));
    QCOMPARE(WriteToFile::formatBytes(2 * 1024 * 1024),      QStringLiteral("2.0 MB"));
}

//=============================================================================================================

void TestWriteToFileStatus::emitRecordingStatus_isPeriodicAndMonotonic()
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

    // Drive a synthetic file growth + periodic status emission for ~3.2s.
    QTimer growTimer;
    growTimer.setInterval(250);
    qint64 written = 0;
    QObject::connect(&growTimer, &QTimer::timeout, [&]() {
        const QByteArray chunk(1024, 'x');
        plugin.m_qFileOut.write(chunk);
        plugin.m_qFileOut.flush();
        written += chunk.size();
    });
    growTimer.start();

    QTimer emitTimer;
    emitTimer.setInterval(1000);
    QObject::connect(&emitTimer, &QTimer::timeout,
                     &plugin, &WriteToFile::emitRecordingStatus);
    emitTimer.start();

    QTest::qWait(3200);

    growTimer.stop();
    emitTimer.stop();
    plugin.m_bWriteToFile = false;
    plugin.m_qFileOut.close();

    QVERIFY2(spy.count() >= 3,
             qPrintable(QStringLiteral("Expected >=3 recordingStatus emissions, got %1").arg(spy.count())));

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
        if (unit == QLatin1String("KB")) value *= 1024.0;
        else if (unit == QLatin1String("MB")) value *= 1024.0 * 1024.0;
        else if (unit == QLatin1String("GB")) value *= 1024.0 * 1024.0 * 1024.0;
        QVERIFY2(value >= lastBytes,
                 qPrintable(QStringLiteral("File size regressed: %1 -> %2").arg(lastBytes).arg(value)));
        lastBytes = value;
    }

    // The final summary's elapsed time must approximately match the QElapsedTimer reading.
    const QString last = spy.last().first().toString();
    const auto lastMatch = re.match(last);
    QVERIFY(lastMatch.hasMatch());
    const int lastSecs = lastMatch.captured(1).toInt() * 3600
                       + lastMatch.captured(2).toInt() * 60
                       + lastMatch.captured(3).toInt();
    // We waited ~3.2s — expect at least 2s elapsed in the final summary.
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

QTEST_MAIN(TestWriteToFileStatus)
#include "test_writetofile_status.moc"
