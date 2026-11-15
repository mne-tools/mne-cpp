//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_acq_tool.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Command-line tests of every mne_acq subcommand, with the output checked against the source itself.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq.h>
#include <base/string_utils.h>

#include <fiff/fiff_raw_data.h>

#include <lsl/lsl_stream_discovery.h>
#include <lsl/lsl_stream_inlet.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QImage>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QUdpSocket>
#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

namespace
{

struct ToolResult
{
    int exitCode = -1;
    QString out;
    QString err;
};

QString toolPath()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/../bin/mne_acq");
}

ToolResult runTool(const QStringList& arguments, int timeoutMs = 60000)
{
    QProcess process;
    process.start(toolPath(), arguments);
    ToolResult result;
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished();
        result.err = QStringLiteral("timed out");
        return result;
    }
    result.exitCode = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
    result.out = QString::fromUtf8(process.readAllStandardOutput());
    result.err = QString::fromUtf8(process.readAllStandardError());
    return result;
}

/** A source read to its end: channel names and physical values, column-major channels x samples. */
struct Source
{
    StringList names;
    std::vector<double> values;
};

Source readSource(const std::string& uri)
{
    Source source;
    auto device = AcqDevice::open(uri);
    if (!device) {
        return source;
    }
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    if (!reader || !device->start().ok()) {
        return source;
    }
    for (const AcqChannelInfo& channel : reader->stream().info()->channels) {
        source.names.push_back(channel.name);
    }
    for (;;) {
        auto lease = reader->lease(1, 1s);
        if (!lease) {
            break;
        }
        const AcqBlockView& view = **lease;
        std::vector<double> block(view.channelCount() * view.sampleCount());
        if (!decode(view, AcqMatrixRef<double>::columnMajor(block.data(), view.channelCount(), view.sampleCount())).ok()) {
            return {};
        }
        source.values.insert(source.values.end(), block.begin(), block.end());
    }
    device->close();
    return source;
}

/** Whether this host delivers a multicast datagram it sends on the interface BrainFlow joins groups on. */
bool multicastLoopsBack(const QHostAddress& group, quint16 port)
{
    // STREAMING_BOARD joins on the interface that routes to 8.8.8.8, as mne_acq sends on it.
    QNetworkInterface iface;
    QUdpSocket probe;
    probe.connectToHost(QHostAddress(QStringLiteral("8.8.8.8")), 53);
    if (probe.waitForConnected(1000)) {
        for (const QNetworkInterface& candidate : QNetworkInterface::allInterfaces()) {
            for (const QNetworkAddressEntry& entry : candidate.addressEntries()) {
                if (entry.ip() == probe.localAddress()) {
                    iface = candidate;
                }
            }
        }
    }
    QUdpSocket receiver;
    if (!receiver.bind(QHostAddress(QHostAddress::AnyIPv4), port, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint) || !(iface.isValid() ? receiver.joinMulticastGroup(group, iface) : receiver.joinMulticastGroup(group))) {
        return false;
    }
    QUdpSocket sender;
    sender.bind(QHostAddress(QHostAddress::AnyIPv4), 0);
    sender.setSocketOption(QAbstractSocket::MulticastLoopbackOption, 1);
    if (iface.isValid()) {
        sender.setMulticastInterface(iface);
    }
    sender.writeDatagram("probe", group, port);
    return receiver.waitForReadyRead(1000) && receiver.hasPendingDatagrams();
}

QString python()
{
    const QString exe = QStandardPaths::findExecutable(QStringLiteral("python3"));
    return exe.isEmpty() ? QStandardPaths::findExecutable(QStringLiteral("python")) : exe;
}

} // namespace

//=============================================================================================================
/**
 * Runs the built mne_acq next to the test binaries.
 */
class TestMneAcqTool : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void usageErrors();
    void listShowsSchemesAndSources();
    void infoShowsStreamsAndChannels();
    void infoFailsOnBadUri();
    void recordWritesTheSource();
    void recordFailsOnUnwritablePath();
    void streamPublishesLsl();
    void streamPublishesBrainFlow();
    void viewShowsLiveTraces();
};

//=============================================================================================================

void TestMneAcqTool::initTestCase()
{
    if (!QFile::exists(toolPath()) && !QFile::exists(toolPath() + QStringLiteral(".exe"))) {
        QSKIP("mne_acq was not built");
    }
}

//=============================================================================================================

void TestMneAcqTool::usageErrors()
{
    QCOMPARE(runTool({}).exitCode, 1);
    QCOMPARE(runTool({QStringLiteral("frobnicate")}).exitCode, 1);
    QCOMPARE(runTool({QStringLiteral("info")}).exitCode, 1);
    QCOMPARE(runTool({QStringLiteral("record"), QStringLiteral("synthetic://")}).exitCode, 1);
    QCOMPARE(runTool({QStringLiteral("list"), QStringLiteral("--timeout"), QStringLiteral("-1")}).exitCode, 1);
    QCOMPARE(runTool({QStringLiteral("record"), QStringLiteral("synthetic://"), QStringLiteral("x.fif"),
                      QStringLiteral("--duration"), QStringLiteral("soon")})
                 .exitCode,
             1);
    const ToolResult noOutput = runTool({QStringLiteral("stream"), QStringLiteral("synthetic://")});
    QCOMPARE(noOutput.exitCode, 1);
    QVERIFY(noOutput.err.contains(QStringLiteral("--lsl")));
    const QString synthetic = QStringLiteral("synthetic://?duration=1&realtime=0");
    const QString bf = QStringLiteral("--brainflow-multicast");
    QCOMPARE(runTool({QStringLiteral("stream"), synthetic, QStringLiteral("--lsl"), bf, QStringLiteral("225.1.1.1:6677")}).exitCode, 1);
    QCOMPARE(runTool({QStringLiteral("stream"), synthetic, bf, QStringLiteral("10.0.0.1:6677")}).exitCode, 1);
    QCOMPARE(runTool({QStringLiteral("stream"), synthetic, bf, QStringLiteral("225.1.1.1")}).exitCode, 1);
    QCOMPARE(runTool({QStringLiteral("stream"), synthetic, bf, QStringLiteral("225.1.1.1:6677"), QStringLiteral("--master-board"), QStringLiteral("x")}).exitCode, 1);
    QCOMPARE(runTool({QStringLiteral("stream"), synthetic, bf, QStringLiteral("225.1.1.1:6677"), QStringLiteral("--master-board"), QStringLiteral("99999")}).exitCode, 2);

    const ToolResult help = runTool({QStringLiteral("--help")});
    QCOMPARE(help.exitCode, 0);
    for (const char* command : {"list", "info", "record", "stream"}) {
        QVERIFY2(help.out.contains(QStringLiteral("mne_acq %1").arg(QLatin1String(command))), command);
    }
}

//=============================================================================================================

void TestMneAcqTool::listShowsSchemesAndSources()
{
    const ToolResult list = runTool({QStringLiteral("list"), QStringLiteral("--timeout"), QStringLiteral("0.2")});
    QCOMPARE(list.exitCode, 0);
    for (const char* scheme : {"synthetic://", "fiff://", "lsl://", "brainflow://", "eego://", "natus://", "babymeg://", "rtserver://"}) {
        QVERIFY2(list.out.contains(QLatin1String(scheme)), qPrintable(list.out));
    }
    // The synthetic source is always reachable.
    QVERIFY2(list.out.section(QStringLiteral("Sources"), 1).contains(QStringLiteral("synthetic://")), qPrintable(list.out));
}

//=============================================================================================================

void TestMneAcqTool::infoShowsStreamsAndChannels()
{
    const ToolResult info = runTool({QStringLiteral("info"), QStringLiteral("synthetic://?eeg=3&sfreq=500")});
    QCOMPARE(info.exitCode, 0);
    QVERIFY2(info.out.contains(QStringLiteral("backend   synthetic")), qPrintable(info.out));
    QVERIFY2(info.out.contains(QStringLiteral("float32, 500 Hz")), qPrintable(info.out));
    for (const char* channel : {"EEG 001", "EEG 002", "EEG 003", "STI 014"}) {
        QVERIFY2(info.out.contains(QLatin1String(channel)), qPrintable(info.out));
    }
    QVERIFY(info.out.contains(QRegularExpression(QStringLiteral("EEG 001\\s+EEG\\s+V"))));
    QVERIFY(info.out.contains(QRegularExpression(QStringLiteral("STI 014\\s+stim"))));
}

//=============================================================================================================

void TestMneAcqTool::infoFailsOnBadUri()
{
    const ToolResult unknown = runTool({QStringLiteral("info"), QStringLiteral("nope://x")});
    QCOMPARE(unknown.exitCode, 2);
    QVERIFY(unknown.err.contains(QStringLiteral("nope://")));
    QCOMPARE(runTool({QStringLiteral("info"), QStringLiteral("fiff:///does/not/exist_raw.fif")}).exitCode, 2);
}

//=============================================================================================================

void TestMneAcqTool::recordWritesTheSource()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("cli_raw.fif"));
    const std::string uri = "synthetic://?eeg=4&sfreq=1000&duration=2&realtime=0&seed=11";
    const ToolResult record = runTool({QStringLiteral("record"), QString::fromStdString(uri), path});
    QVERIFY2(record.exitCode == 0, qPrintable(record.out + record.err));
    QVERIFY2(record.out.contains(QStringLiteral("Recorded 2000 samples (0 lost)")), qPrintable(record.out));

    const Source source = readSource(uri);
    const StringList& names = source.names;
    const std::vector<double>& expected = source.values;
    QCOMPARE(expected.size(), names.size() * 2000);

    FiffRawData raw(path.toStdString());
    QCOMPARE(raw.last_samp - raw.first_samp + 1, 2000);
    Eigen::MatrixXd data, times;
    QVERIFY(raw.read_raw_segment(data, times));
    int compared = 0;
    for (std::size_t c = 0; c < names.size(); ++c) {
        if (names[c].rfind("EEG", 0) != 0) {
            continue;
        }
        const auto row = static_cast<Eigen::Index>(mne::indexOf(raw.info.ch_names, names[c]));
        QVERIFY2(row >= 0, names[c].c_str());
        for (std::size_t s = 0; s < 2000; ++s) {
            const double want = expected[s * names.size() + c];
            const double got = data(row, static_cast<Eigen::Index>(s));
            if (std::abs(got - want) > 1e-6 * std::max(1e-12, std::abs(want))) {
                QFAIL(qPrintable(QStringLiteral("%1 sample %2: %3 != %4").arg(names[c].c_str()).arg(s).arg(got).arg(want)));
            }
        }
        ++compared;
    }
    QCOMPARE(compared, 4);
}

//=============================================================================================================

void TestMneAcqTool::recordFailsOnUnwritablePath()
{
    // The recorder creates missing directories, so the parent is a regular file instead.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile blocker(dir.filePath(QStringLiteral("not_a_dir")));
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();
    const ToolResult record = runTool({QStringLiteral("record"), QStringLiteral("synthetic://?duration=1&realtime=0"),
                                       blocker.fileName() + QStringLiteral("/cli_raw.fif")});
    QCOMPARE(record.exitCode, 2);
    QVERIFY(!record.err.isEmpty());
}

//=============================================================================================================

void TestMneAcqTool::streamPublishesLsl()
{
    const std::string uri = "synthetic://?eeg=4&sfreq=1000&seed=5";
    const QString name = QStringLiteral("mne_acq_cli_%1").arg(QDateTime::currentMSecsSinceEpoch());
    QProcess tool;
    tool.start(toolPath(), {QStringLiteral("stream"), QString::fromStdString(uri), QStringLiteral("--lsl"), QStringLiteral("--name"), name, QStringLiteral("--duration"), QStringLiteral("6")});
    QVERIFY(tool.waitForStarted());

    const auto found = mne::resolve_stream("name", name.toStdString(), 10.0, 1);
    QVERIFY2(!found.empty(), qPrintable(QString::fromUtf8(tool.readAllStandardError())));
    QCOMPARE(found.front().channel_count(), 5);
    QCOMPARE(found.front().nominal_srate(), 1000.0);

    mne::stream_inlet inlet(found.front());
    inlet.open_stream();
    std::vector<std::vector<float>> received;
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    while (received.size() < 500 && std::chrono::steady_clock::now() < deadline) {
        auto chunk = inlet.pull_chunk_float();
        received.insert(received.end(), chunk.begin(), chunk.end());
        std::this_thread::sleep_for(10ms);
    }
    inlet.close_stream();
    QVERIFY2(received.size() >= 500, qPrintable(QString::number(received.size())));
    QVERIFY(tool.waitForFinished(20000));
    QCOMPARE(tool.exitCode(), 0);
    QVERIFY(QString::fromUtf8(tool.readAllStandardOutput()).contains(QStringLiteral("Published")));

    // The inlet joins mid-stream: find its first sample in the seeded source, then compare all received samples.
    const Source source = readSource(uri + "&realtime=0&duration=10");
    std::vector<float> expected(source.values.begin(), source.values.end());
    const std::size_t channels = source.names.size();
    QCOMPARE(channels, std::size_t(5));
    const std::size_t total = expected.size() / channels;
    std::size_t offset = total;
    for (std::size_t s = 0; s + received.size() <= total; ++s) {
        if (std::equal(received.front().begin(), received.front().end(), expected.begin() + static_cast<std::ptrdiff_t>(s * channels))) {
            offset = s;
            break;
        }
    }
    QVERIFY2(offset < total, "first received sample is not in the source");
    for (std::size_t s = 0; s < received.size(); ++s) {
        QVERIFY(std::equal(received[s].begin(), received[s].end(),
                           expected.begin() + static_cast<std::ptrdiff_t>((offset + s) * channels)));
    }
}

//=============================================================================================================

void TestMneAcqTool::streamPublishesBrainFlow()
{
    const QString exe = python();
    if (exe.isEmpty()) {
        QSKIP("no python");
    }
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString received = dir.filePath(QStringLiteral("brainflow.bin"));
    // A port of its own per run, so parallel runs on one host do not mix.
    const QString port = QString::number(20000 + QCoreApplication::applicationPid() % 20000);
    if (!multicastLoopsBack(QHostAddress(QStringLiteral("225.1.1.1")), port.toUShort())) {
        QSKIP("This host does not loop multicast back (BrainFlow's STREAMING_BOARD receives multicast only)");
    }
    QProcess consumer;
    consumer.start(exe, {QStringLiteral(MNE_ACQ_BRAINFLOW_RECEIVE), QStringLiteral("225.1.1.1"), port, QStringLiteral("-1"), QStringLiteral("4"), received});
    QVERIFY(consumer.waitForStarted());
    QThread::msleep(1000); // the consumer joins the group before the first packet

    const std::string uri = "synthetic://?eeg=4&sfreq=250&seed=9";
    const ToolResult tool = runTool({QStringLiteral("stream"), QString::fromStdString(uri),
                                     QStringLiteral("--brainflow-multicast"), QStringLiteral("225.1.1.1:") + port,
                                     QStringLiteral("--duration"), QStringLiteral("4")});
    QVERIFY(consumer.waitForFinished(60000));
    const QString consumerOut = QString::fromUtf8(consumer.readAllStandardOutput());
    if (consumer.exitCode() == 2) {
        QSKIP(qPrintable(consumerOut.trimmed()));
    }
    QVERIFY2(tool.exitCode == 0, qPrintable(tool.out + tool.err));
    QVERIFY2(tool.out.contains(QStringLiteral("as board -1 (Synthetic)")), qPrintable(tool.out));
    QVERIFY2(consumer.exitCode() == 0, qPrintable(consumerOut + QString::fromUtf8(consumer.readAllStandardError())));

    QFile file(received);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray bytes = file.readAll();
    QVERIFY(bytes.size() >= 16);
    qint64 shape[2] = {0, 0};
    std::memcpy(shape, bytes.constData(), sizeof(shape));
    const auto rows = static_cast<std::size_t>(shape[0]);
    const auto count = static_cast<std::size_t>(shape[1]);
    QCOMPARE(rows, std::size_t(32));
    QVERIFY2(count >= 250, qPrintable(QString::number(count)));
    QCOMPARE(static_cast<std::size_t>(bytes.size()), 16 + rows * count * sizeof(double));
    std::vector<double> data(rows * count);
    std::memcpy(data.data(), bytes.constData() + 16, data.size() * sizeof(double));
    const auto at = [&](std::size_t row, std::size_t sample) {
        return data[row * count + sample];
    };

    // STREAMING_BOARD joins mid-stream: locate its first sample by the EEG rows (microvolts), then compare all.
    const Source source = readSource(uri + "&realtime=0&duration=30");
    const std::size_t channels = source.names.size();
    QCOMPARE(channels, std::size_t(5));
    const std::size_t total = source.values.size() / channels;
    const auto expectedRow = [&](std::size_t sample, std::size_t c) {
        const double value = source.values[sample * channels + c];
        return c < 4 ? value * 1e6 : value; // EEG in volts, the stim channel without unit
    };
    std::size_t offset = total;
    for (std::size_t s = 0; s + count <= total && offset == total; ++s) {
        bool same = true;
        for (std::size_t c = 0; c < channels && same; ++c) {
            same = at(c + 1, 0) == expectedRow(s, c);
        }
        offset = same ? s : total;
    }
    QVERIFY2(offset < total, "the first received sample is not in the source");
    int markers = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t sample = offset + i;
        for (std::size_t c = 0; c < channels; ++c) {
            if (at(c + 1, i) != expectedRow(sample, c)) {
                QFAIL(qPrintable(QStringLiteral("sample %1 channel %2: %3 != %4").arg(sample).arg(c).arg(at(c + 1, i)).arg(expectedRow(sample, c))));
            }
        }
        QCOMPARE(at(0, i), static_cast<double>(sample % 256));
        // STREAMING_BOARD fills the marker row from its own insert_marker(); triggers travel in the stim row.
        markers += at(5, i) != 0.0 ? 1 : 0;
        if (i > 0) {
            QVERIFY(std::abs(at(30, i) - at(30, i - 1) - 1.0 / 250.0) < 1e-6);
        }
    }
    QVERIFY2(markers > 0, "no trigger reached the stim row");
}

//=============================================================================================================

void TestMneAcqTool::viewShowsLiveTraces()
{
    QCOMPARE(runTool({QStringLiteral("view"), QStringLiteral("synthetic://"), QStringLiteral("--window"), QStringLiteral("0")}).exitCode, 1);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString shot = dir.filePath(QStringLiteral("view.png"));
    QProcess tool;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
    tool.setProcessEnvironment(env);
    tool.start(toolPath(), {QStringLiteral("view"), QStringLiteral("synthetic://?eeg=4&sfreq=500&seed=2"), QStringLiteral("--window"), QStringLiteral("2"), QStringLiteral("--duration"), QStringLiteral("2"), QStringLiteral("--screenshot"), shot});
    QVERIFY(tool.waitForFinished(60000));
    const QString out = QString::fromUtf8(tool.readAllStandardOutput());
    const QString err = QString::fromUtf8(tool.readAllStandardError());
    if (tool.exitCode() == 2 && err.contains(QStringLiteral("display library"))) {
        QSKIP("mne_acq was built without mne_disp");
    }
    QVERIFY2(tool.exitCode() == 0, qPrintable(out + err));
    const QRegularExpressionMatch shown = QRegularExpression(QStringLiteral("Showed (\\d+) samples")).match(out);
    QVERIFY2(shown.hasMatch(), qPrintable(out));
    // Paced at 500 Hz for 2 s.
    QVERIFY2(shown.captured(1).toInt() >= 500, qPrintable(out));
    QImage image(shot);
    QVERIFY(!image.isNull());
    QCOMPARE(image.size(), QSize(1200, 700));

    QProcess bad;
    bad.setProcessEnvironment(env);
    bad.start(toolPath(), {QStringLiteral("view"), QStringLiteral("nope://x")});
    QVERIFY(bad.waitForFinished(60000));
    QVERIFY(bad.exitCode() == 2 || QString::fromUtf8(bad.readAllStandardError()).contains(QStringLiteral("display library")));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneAcqTool)
#include "test_mne_acq_tool.moc"
