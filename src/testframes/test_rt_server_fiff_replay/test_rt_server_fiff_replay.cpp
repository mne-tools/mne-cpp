//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_rt_server_fiff_replay.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Loopback test of mne_rt_server's FIFF simulator: the buffers a client receives equal the file.
 *
 * Starts mne_rt_server, points its FIFF simulator at the test data over the command port, streams it through
 * rtserver:// and compares the received buffers with read_raw_segment of the same samples.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq_device.h>
#include <acq/devices/acq_devices.h>
#include <acq/fiff/acq_fiff.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#include <rtserver_connector/iconnector.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QPluginLoader>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <format>
#include <thread>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

namespace
{

/** @return The bytes of the tags @p write puts on a FIFF stream. */
template<typename Write>
QByteArray tagBytes(Write&& write)
{
    auto bytes = std::make_unique<mne::MemoryByteStream>();
    const mne::MemoryByteStream& written = *bytes;
    FiffStream stream(std::move(bytes));
    write(stream);
    return QByteArray(reinterpret_cast<const char*>(written.data().data()), static_cast<qsizetype>(written.size()));
}

/** @return The reply of the server's command port to @p line, framed as mne_rt_server frames its commands. */
QString send(QTcpSocket& socket, const QString& line)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint16(0) << line;
    out.device()->seek(0);
    out << quint16(block.size() - sizeof(quint16));
    socket.write(block);
    QString reply;
    QDataStream in(&socket);
    in.setVersion(QDataStream::Qt_6_0);
    while (socket.waitForReadyRead(10000)) {
        in.startTransaction();
        quint16 size = 0;
        in >> size >> reply;
        if (in.commitTransaction()) {
            break;
        }
    }
    return reply;
}

/**
 * Serves @p info as mne_rt_server does on two consecutive local ports: the data client gets id 1, "measinfo 1"
 * the bytes of FiffInfo::writeToStream, every command the reply "ok".
 */
class MeasInfoServer
{
public:
    explicit MeasInfoServer(const FiffInfo& info)
    {
        m_info = tagBytes([&info](FiffStream& out) { info.writeToStream(out); });
        for (quint16 port = 40217; port < 40417 && m_port == 0; port += 2) {
            if (m_commands.listen(QHostAddress::LocalHost, port) && m_data.listen(QHostAddress::LocalHost, port + 1)) {
                m_port = port;
            } else {
                m_commands.close();
            }
        }
    }

    [[nodiscard]] quint16 port() const
    {
        return m_port;
    }

    /** Answers the client in this thread until it has the measurement info. */
    void serve()
    {
        QVERIFY(m_commands.waitForNewConnection(5000) && m_data.waitForNewConnection(5000));
        // Closing a socket with unread data (the alias) resets the connection, so both stay open until the end
        m_command_socket.reset(m_commands.nextPendingConnection());
        m_data_socket.reset(m_data.nextPendingConnection());
        QTcpSocket* commands = m_command_socket.get();
        QTcpSocket* data = m_data_socket.get();
        // The id request: a command tag of 20 bytes; then the alias tag
        while (data->bytesAvailable() < 20 && data->waitForReadyRead(5000)) {
        }
        data->read(20);
        data->write(tagBytes([](FiffStream& out) {
            const fiff_int_t id = 1;
            out.write_int(FIFF_MNE_RT_CLIENT_ID, &id);
        }));
        data->waitForBytesWritten(5000);
        QDataStream in(commands);
        in.setVersion(QDataStream::Qt_6_0);
        while (commands->waitForReadyRead(5000)) {
            in.startTransaction();
            quint16 size = 0;
            QString line;
            in >> size >> line;
            if (!in.commitTransaction()) {
                continue;
            }
            if (line == QStringLiteral("measinfo 1")) {
                data->write(m_info);
                data->waitForBytesWritten(5000);
            }
            QByteArray block;
            QDataStream reply(&block, QIODevice::WriteOnly);
            reply.setVersion(QDataStream::Qt_6_0);
            reply << quint16(0) << QStringLiteral("ok");
            reply.device()->seek(0);
            reply << quint16(block.size() - sizeof(quint16));
            commands->write(block);
            commands->waitForBytesWritten(5000);
            if (line.startsWith(QStringLiteral("measinfo"))) {
                break;
            }
        }
    }

private:
    QTcpServer m_commands;
    QTcpServer m_data;
    std::unique_ptr<QTcpSocket> m_command_socket;
    std::unique_ptr<QTcpSocket> m_data_socket;
    QByteArray m_info;
    quint16 m_port = 0;
};

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestRtServerFiffReplay
 */
class TestRtServerFiffReplay : public QObject
{
    Q_OBJECT

private slots:
    void readsTheMeasurementInfo_data();
    void readsTheMeasurementInfo();
    void pluginSharesTheConnectorType();
    void streamsTheFile();
    void cleanupTestCase();

private:
    QProcess m_server;
};

//=============================================================================================================

void TestRtServerFiffReplay::readsTheMeasurementInfo_data()
{
    QTest::addColumn<bool>("withProjector");
    QTest::newRow("ctf compensators") << false;
    QTest::newRow("ctf compensators + projector") << true;
}

void TestRtServerFiffReplay::readsTheMeasurementInfo()
{
    // What the server writes of a measurement info reaches the client field by field
    QFETCH(bool, withProjector);
    FiffRawData rawData(QStringLiteral(MNE_CTF_COMP_DATA_DIR "/ctf_grade1_raw.fif").toStdString());
    FiffInfo info = rawData.info;
    QCOMPARE(info.nchan, 36);
    QCOMPARE(info.comps.size(), 5);
    QCOMPARE(info.dig.size(), 3);
    if (withProjector) {
        Eigen::MatrixXd vec(2, 3);
        vec << 0.6, 0.8, 0.0, 0.0, 0.6, -0.8;
        FiffNamedMatrix vectors(2, 3, {}, StringList(info.ch_names.begin(), info.ch_names.begin() + 3), vec);
        info.projs.push_back(FiffProj(FIFFV_PROJ_ITEM_FIELD, true, "PCA-v1", vectors));
    }
    info.experimenter = "rt test";
    info.description = "wire round trip";
    info.proj_name = "rt";
    info.proj_id = 7;
    info.utc_offset = "+02:00";
    info.acq_pars = "ACQch001 110113";
    info.acq_stim = "STI 014";
    info.bads = {info.ch_names.front()};

    MeasInfoServer server(info);
    QVERIFY(server.port() > 0);
    QVERIFY(mne::register_rtserver_scheme().ok());
    mne::AcqExpected<mne::AcqDevice> device = mne::AcqError(mne::AcqErrorCode::Timeout);
    std::thread client([&] { device = mne::AcqDevice::open(std::format("rtserver://127.0.0.1:{}?timeout=5", server.port())); });
    server.serve();
    client.join();
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    const auto converted = mne::toFiffInfo(*device->stream().info());
    QVERIFY2(converted, converted ? "" : converted.error().text().c_str());
    const FiffInfo& got = *converted;

    QCOMPARE(got.nchan, info.nchan);
    QCOMPARE(got.ch_names, info.ch_names);
    for (int k = 0; k < info.nchan; ++k) {
        QCOMPARE(got.chs[k].kind, info.chs[k].kind);
        QCOMPARE(got.chs[k].cal, 1.0F); // the server sends calibrated values
        QCOMPARE(got.chs[k].unit, info.chs[k].unit);
        QVERIFY(got.chs[k].chpos.r0 == info.chs[k].chpos.r0);
    }
    QCOMPARE(got.sfreq, info.sfreq);
    QCOMPARE(got.highpass, info.highpass);
    QCOMPARE(got.lowpass, info.lowpass);
    QCOMPARE(got.linefreq, info.linefreq);
    QCOMPARE(got.meas_date[0], info.meas_date[0]);
    QCOMPARE(got.meas_date[1], info.meas_date[1]);
    QCOMPARE(got.experimenter, info.experimenter);
    QCOMPARE(got.description, info.description);
    QCOMPARE(got.proj_name, info.proj_name);
    QCOMPARE(got.proj_id, info.proj_id);
    QCOMPARE(got.gantry_angle, info.gantry_angle);
    QCOMPARE(got.utc_offset, info.utc_offset);
    QCOMPARE(got.acq_pars, info.acq_pars);
    QCOMPARE(got.acq_stim, info.acq_stim);
    QCOMPARE(got.bads, info.bads);
    QVERIFY(got.dev_head_t.trans == info.dev_head_t.trans);
    QCOMPARE(got.dig.size(), info.dig.size());
    for (int k = 0; k < std::ssize(info.dig); ++k) {
        QCOMPARE(got.dig[k].ident, info.dig[k].ident);
        QCOMPARE(got.dig[k].r[0], info.dig[k].r[0]);
    }
    QCOMPARE(got.projs.size(), info.projs.size());
    for (int p = 0; p < std::ssize(info.projs); ++p) {
        QCOMPARE(got.projs[p].desc, info.projs[p].desc);
        QCOMPARE(got.projs[p].kind, info.projs[p].kind);
        QCOMPARE(got.projs[p].active, info.projs[p].active);
        QCOMPARE(got.projs[p].data.col_names, info.projs[p].data.col_names);
        QVERIFY(got.projs[p].data.data.isApprox(info.projs[p].data.data, 1e-6));
    }
    QCOMPARE(got.comps.size(), info.comps.size());
    for (int c = 0; c < std::ssize(info.comps); ++c) {
        QCOMPARE(got.comps[c].kind, info.comps[c].kind);
        QCOMPARE(got.comps[c].save_calibrated, info.comps[c].save_calibrated);
        QCOMPARE(got.comps[c].data.row_names, info.comps[c].data.row_names);
        QCOMPARE(got.comps[c].data.col_names, info.comps[c].data.col_names);
        QVERIFY(got.comps[c].data.data.isApprox(info.comps[c].data.data, 1e-6));
    }
}

//=============================================================================================================

void TestRtServerFiffReplay::pluginSharesTheConnectorType()
{
    // The server connects to IConnector's slots and signals by member pointer. That only works if the plugin derives
    // from the very IConnector the server knows; a second copy compiled into the plugin makes Qt's checks abort.
#ifdef STATICBUILD
    QSKIP("Connectors are linked into the server in static builds");
#endif
    const QDir plugins(QCoreApplication::applicationDirPath() + "/../bin/mne_rt_server_plugins");
    const QStringList files = plugins.entryList(QDir::Files);
    if (files.isEmpty()) {
        QSKIP("mne_rt_server plugins not built");
    }
    for (const QString& name : files) {
        if (!QLibrary::isLibrary(name)) { // the import libraries and export files next to a Windows plugin
            continue;
        }
        QPluginLoader loader(plugins.absoluteFilePath(name));
        QObject* instance = loader.instance();
        if (instance == nullptr) {
            QFAIL(qPrintable(name + ": " + loader.errorString()));
            return;
        }
        QVERIFY2(instance->metaObject()->inherits(&RTSERVER::IConnector::staticMetaObject), qPrintable(name));
        QVERIFY2(dynamic_cast<RTSERVER::IConnector*>(instance), qPrintable(name));
    }
}

//=============================================================================================================

void TestRtServerFiffReplay::streamsTheFile()
{
    const QString base = QCoreApplication::applicationDirPath() + "/..";
    const QString file = base + "/resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(file)) {
        QSKIP("MNE-CPP test data not found");
    }
    m_server.setProcessChannelMode(QProcess::MergedChannels);
    m_server.start(base + "/bin/mne_rt_server", {});
    QVERIFY(m_server.waitForStarted(10000));
    QByteArray output;
    while (!output.contains("fiff data port") && m_server.waitForReadyRead(30000)) {
        output += m_server.readAll();
    }
    QVERIFY2(output.contains("fiff data port"), output.constData());
    QVERIFY2(output.contains("Fiff File Simulator"), output.constData());

    QTcpSocket command;
    command.connectToHost(QStringLiteral("127.0.0.1"), 4217);
    QVERIFY(command.waitForConnected(5000));
    const QString help = send(command, QStringLiteral("help"));
    QVERIFY2(help.contains(QStringLiteral("simfile")),
             qPrintable(QStringLiteral("help reply: ") + help.left(400) + "\nserver: " + QString::fromUtf8(output + m_server.readAll())));
    // A file the simulator cannot read is refused, and the simulator still accepts the next one.
    QTemporaryDir dir;
    QFile notFiff(dir.filePath(QStringLiteral("not_fiff.fif")));
    QVERIFY(notFiff.open(QIODevice::WriteOnly));
    notFiff.write("not a FIFF file");
    notFiff.close();
    QVERIFY(mne::register_rtserver_scheme().ok());
    auto refused = mne::AcqDevice::open(std::format("rtserver://127.0.0.1:4217?simfile={}", notFiff.fileName().toStdString()));
    QVERIFY(!refused);
    QCOMPARE(refused.error().code(), mne::AcqErrorCode::InvalidParameter);
    QVERIFY(!send(command, QStringLiteral("accel 100")).isEmpty());

    const int bufsize = 100;
    auto device = mne::AcqDevice::open(std::format("rtserver://127.0.0.1:4217?simfile={}&buffer={}", file.toStdString(), bufsize));
    QVERIFY2(device, device ? "" : device.error().text().c_str());
    const auto info = device->stream().info();
    FiffRawData raw(file.toStdString());
    QCOMPARE(info->channelCount(), std::size_t(raw.info.nchan));
    QCOMPARE(info->sfreq, double(raw.info.sfreq * 100.0F)); // the acceleration factor scales the advertised rate

    // The reader gates the stream, so the client stops reading the socket once the reader stops reading
    mne::AcqReaderOptions gating;
    gating.gating = true;
    auto reader = device->stream().reader(mne::AcqReader::ReadMode::Lossless, gating);
    QVERIFY(reader);
    QVERIFY(device->start().ok());
    // Buffers go out calibrated, as read_raw_segment returns them, and follow each other without gaps. A client
    // joins a running stream, so the first buffer it receives may lie anywhere in the file.
    const auto segment = [&raw](int b) {
        Eigen::MatrixXd expected, times;
        const fiff_int_t first = raw.first_samp + b * bufsize;
        return raw.read_raw_segment(expected, times, first, first + bufsize - 1) ? Eigen::MatrixXd(expected.cast<float>().cast<double>())
                                                                                 : Eigen::MatrixXd();
    };
    const int buffers = (raw.last_samp - raw.first_samp + 1) / bufsize;
    int offset = -1;
    Eigen::MatrixXd received(raw.info.nchan, bufsize);
    // At 100 times real time a buffer is due every 1.7 ms, faster than the server polls its socket.
    for (int b = 0; b < 30; ++b) {
        const auto read = reader->read(mne::AcqMatrixRef<double>::columnMajor(received.data(), std::size_t(received.rows()), std::size_t(bufsize)),
                                       std::chrono::seconds(10));
        QVERIFY2(read, read ? "" : read.error().text().c_str());
        for (int k = 0; offset < 0 && k < buffers; ++k) {
            if (received == segment(k)) {
                offset = k;
            }
        }
        QVERIFY2(offset >= 0, "the received buffer is no segment of the file");
        QVERIFY(received == segment(offset + b));
    }
    // A client that stops reading must not keep the server from answering commands.
    QThread::msleep(1000);
    QElapsedTimer reply;
    reply.start();
    QVERIFY(!send(command, QStringLiteral("stop-all")).isEmpty());
    QVERIFY2(reply.elapsed() < 2000, qPrintable(QStringLiteral("stop-all took %1 ms").arg(reply.elapsed())));
    device->close();
    command.disconnectFromHost();
}

//=============================================================================================================

void TestRtServerFiffReplay::cleanupTestCase()
{
    if (m_server.state() == QProcess::NotRunning) {
        qWarning("mne_rt_server exited early (%d): %s", m_server.exitCode(), m_server.readAll().constData());
    } else {
        m_server.kill();
        m_server.waitForFinished(10000);
    }
}

QTEST_GUILESS_MAIN(TestRtServerFiffReplay)
#include "test_rt_server_fiff_replay.moc"
