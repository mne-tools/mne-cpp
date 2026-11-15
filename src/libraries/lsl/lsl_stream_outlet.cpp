//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_stream_outlet.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implements the outlet worker: liblsl's UDP query and time-correction responders and its TCP data server.
 *
 * The PIMPL class @c StreamOutletPrivate runs a dedicated @c std::thread that owns every socket, so no Qt
 * object crosses threads. It answers discovery queries on the LSL discovery port 16571 (multicast groups,
 * broadcast and localhost) and on its own UDP service port, which also answers time-correction requests. Its
 * TCP server answers shortinfo and fullinfo requests and serves stream feeds of data protocol 1.10.
 *
 * Pushed samples are encoded once, on the producing thread, into blocks whose first sample carries its
 * timestamp; the worker appends each block to every inlet that is streaming, so an inlet always starts at a
 * sample with an explicit timestamp.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_stream_outlet.h"
#include "lsl_protocol.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QHostAddress>
#include <QNetworkDatagram>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <atomic>
#include <chrono>
#include <future>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

namespace
{

constexpr qsizetype kMaxRequestBytes = 65536;

/** One accepted TCP connection: reading its request, or receiving the stream. */
struct Client
{
    std::unique_ptr<QTcpSocket> socket;
    QByteArray request;
    bool streaming = false;
    bool closing = false;
};

std::string number(double value)
{
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text.precision(17);
    text << value;
    return text.str();
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

class mne::StreamOutletPrivate
{
public:
    explicit StreamOutletPrivate(const stream_info& info)
    : m_info(info)
    {
    }

    ~StreamOutletPrivate()
    {
        stop();
    }

    StreamOutletPrivate(const StreamOutletPrivate&) = delete;
    StreamOutletPrivate& operator=(const StreamOutletPrivate&) = delete;

    void start()
    {
        m_bRunning = true;
        std::future<void> listening = m_listening.get_future();
        m_bgThread = std::thread(&StreamOutletPrivate::run, this);
        listening.wait(); // m_info (with its ports) is not modified after this point
    }

    void stop()
    {
        m_bRunning = false;
        if (m_bgThread.joinable()) {
            m_bgThread.join();
        }
    }

    /** Encodes @p samples as one block; the first sample carries the timestamp of the first, the others are deduced. */
    void enqueue(const std::vector<std::vector<float>>& samples)
    {
        if (samples.empty()) {
            return;
        }
        const double rate = m_info.nominal_srate();
        const double now = local_clock();
        const double first = rate > 0.0 ? now - static_cast<double>(samples.size() - 1) / rate : now;
        QByteArray block;
        std::vector<double> values(static_cast<std::size_t>(m_info.channel_count()));
        for (std::size_t s = 0; s < samples.size(); ++s) {
            for (std::size_t c = 0; c < values.size(); ++c) {
                values[c] = c < samples[s].size() ? static_cast<double>(samples[s][c]) : 0.0;
            }
            appendSample(block, m_info.channel_format(), values, first, s > 0 && rate > 0.0);
        }
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_blocks.push_back(std::move(block));
    }

    stream_info info() const
    {
        return m_info;
    }

    bool haveConsumers() const
    {
        return m_nClients.load() > 0;
    }

private:
    void run()
    {
        QTcpServer server;
        if (!server.listen(QHostAddress::AnyIPv4, 0)) {
            m_listening.set_value();
            return;
        }
        // The service port has the data port's number where it is free, as in liblsl.
        QUdpSocket service;
        if (!service.bind(QHostAddress::AnyIPv4, server.serverPort())) {
            service.bind(QHostAddress::AnyIPv4, 0);
        }
        QUdpSocket discovery;
        if (discovery.bind(QHostAddress::AnyIPv4, kDiscoveryPort, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
            joinDiscoveryGroups(discovery);
        }
        m_info.set_data_port(server.serverPort());
        m_info.set_service_port(service.localPort());
        m_shortinfo = QByteArray::fromStdString(m_info.to_string());
        m_listening.set_value();

        std::vector<Client> clients;
        while (m_bRunning) {
            while (server.waitForNewConnection(0)) {
                Client client;
                client.socket.reset(server.nextPendingConnection());
                if (client.socket) {
                    clients.push_back(std::move(client));
                }
            }
            answerQueries(discovery, false);
            answerQueries(service, true);
            for (Client& client : clients) {
                if (!client.streaming && !client.closing) {
                    client.socket->waitForReadyRead(0);
                    client.request += client.socket->readAll();
                    handleRequest(client);
                }
            }
            std::vector<QByteArray> blocks;
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                blocks.swap(m_blocks);
            }
            for (Client& client : clients) {
                if (client.streaming) {
                    for (const QByteArray& block : blocks) {
                        client.socket->write(block);
                    }
                    client.socket->flush();
                }
            }
            std::erase_if(clients, [](Client& client) {
                if (client.closing) {
                    client.socket->flush();
                    client.socket->waitForBytesWritten(100);
                    client.socket->disconnectFromHost();
                    return true;
                }
                return client.socket->state() != QAbstractSocket::ConnectedState;
            });
            m_nClients.store(static_cast<int>(std::count_if(clients.begin(), clients.end(),
                                                            [](const Client& c) { return c.streaming; })));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        for (Client& client : clients) {
            client.socket->disconnectFromHost();
        }
        m_nClients.store(0);
    }

    /** Answers shortinfo queries and, on the service port, time-correction requests. */
    void answerQueries(QUdpSocket& socket, bool timedata)
    {
        while (socket.hasPendingDatagrams()) {
            const QNetworkDatagram datagram = socket.receiveDatagram();
            const auto lines = datagram.data().split('\n');
            if (lines.size() < 2) {
                continue;
            }
            const QByteArray kind = lines[0].trimmed();
            const QList<QByteArray> words = lines.size() > 2 ? lines[2].trimmed().split(' ') : QList<QByteArray>();
            if (kind == "LSL:shortinfo" && words.size() == 2) {
                if (matchesQuery(m_info, lines[1].trimmed().toStdString())) {
                    const quint16 port = words[0].toUShort();
                    socket.writeDatagram(words[1] + "\r\n" + m_shortinfo, datagram.senderAddress(), port);
                }
            } else if (timedata && kind == "LSL:timedata") {
                const double received = local_clock();
                const auto request = lines[1].trimmed().split(' ');
                if (request.size() == 2) {
                    const QByteArray reply = " " + request[0] + " " + request[1] + " " +
                        QByteArray::fromStdString(number(received)) + " " +
                        QByteArray::fromStdString(number(local_clock()));
                    socket.writeDatagram(reply, datagram.senderAddress(), datagram.senderPort());
                }
            }
        }
    }

    /** Answers a complete TCP request: stream feed, shortinfo or fullinfo; anything else closes the connection. */
    void handleRequest(Client& client)
    {
        const QByteArray& r = client.request;
        if (r.startsWith("LSL:streamfeed/")) {
            if (!r.contains("\r\n\r\n")) {
                client.closing = r.size() > kMaxRequestBytes;
                return;
            }
            const QList<QByteArray> head = r.left(r.indexOf("\r\n")).split(' ');
            const bool ours = head.size() < 2 || head[1].isEmpty() || head[1] == QByteArray::fromStdString(m_info.uid());
            if (!ours) {
                client.socket->write("LSL/110 404 Not found\r\n\r\n");
                client.closing = true;
                return;
            }
            QByteArray reply = "LSL/110 200 OK\r\nUID: " + QByteArray::fromStdString(m_info.uid()) +
                "\r\nByte-Order: " + byteOrderName() + "\r\nSuppress-Subnormals: 0\r\nData-Protocol-Version: " +
                QByteArray::number(kProtocolVersion) + "\r\n\r\n";
            std::vector<double> pattern(static_cast<std::size_t>(m_info.channel_count()));
            for (const int offset : {4, 2}) {
                for (std::size_t k = 0; k < pattern.size(); ++k) {
                    pattern[k] = testPatternValue(m_info.channel_format(), static_cast<int>(k), offset);
                }
                appendSample(reply, m_info.channel_format(), pattern, kTestPatternTime, false);
            }
            client.socket->write(reply);
            client.socket->flush();
            client.streaming = true;
            client.request.clear();
        } else if (r.startsWith("LSL:shortinfo\r\n")) {
            const qsizetype end = r.indexOf("\r\n", 15);
            if (end < 0) {
                client.closing = r.size() > kMaxRequestBytes;
                return;
            }
            if (matchesQuery(m_info, r.mid(15, end - 15).trimmed().toStdString())) {
                client.socket->write(m_shortinfo);
            }
            client.closing = true;
        } else if (r.startsWith("LSL:fullinfo")) {
            if (r.contains("\n")) {
                client.socket->write(m_shortinfo);
                client.closing = true;
            }
        } else if (r.contains("\n") || r.size() > kMaxRequestBytes) {
            client.closing = true; // e.g. a protocol 1.00 stream feed, which is not served
        }
    }

    stream_info m_info;                  /**< Stream description, ports set once listening. */
    QByteArray m_shortinfo;              /**< m_info as XML, written by the worker before m_listening. */
    std::atomic<bool> m_bRunning{false}; /**< Worker running flag. */
    std::thread m_bgThread;              /**< Worker thread owning all sockets. */
    std::promise<void> m_listening;      /**< Fulfilled once the ports are recorded in m_info. */
    std::mutex m_queueMutex;             /**< Protects m_blocks. */
    std::vector<QByteArray> m_blocks;    /**< Encoded sample blocks not yet sent. */
    std::atomic<int> m_nClients{0};      /**< Number of streaming inlets. */
};

//=============================================================================================================

stream_outlet::stream_outlet(const stream_info& info)
: m_pImpl(std::make_unique<StreamOutletPrivate>(info))
{
    m_pImpl->start();
}

//=============================================================================================================

stream_outlet::~stream_outlet() = default;

//=============================================================================================================

void stream_outlet::push_sample(const std::vector<float>& sample)
{
    m_pImpl->enqueue({sample});
}

//=============================================================================================================

void stream_outlet::push_chunk(const std::vector<std::vector<float>>& chunk)
{
    m_pImpl->enqueue(chunk);
}

//=============================================================================================================

stream_info stream_outlet::info() const
{
    return m_pImpl->info();
}

//=============================================================================================================

bool stream_outlet::have_consumers() const
{
    return m_pImpl->haveConsumers();
}
