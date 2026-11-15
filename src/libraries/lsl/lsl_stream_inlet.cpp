//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_stream_inlet.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implements the receive path of liblsl's data protocol 1.10 and its time-correction exchange.
 *
 * open_stream() sends the stream-feed request, checks the response headers and the two test-pattern samples,
 * and keeps the bytes that follow. Pulls decode every complete sample in the buffer; a sample whose timestamp
 * was not transmitted gets the previous one plus 1 / nominal rate, as liblsl does.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_stream_inlet.h"
#include <base/log.h>
#include "lsl_protocol.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QElapsedTimer>
#include <QHostInfo>
#include <QNetworkDatagram>
#include <QTcpSocket>
#include <QUdpSocket>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <limits>
#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

namespace
{

constexpr int kConnectMs = 5000;

std::runtime_error failure(const std::string& what)
{
    return std::runtime_error("[lsl::stream_inlet] " + what);
}

} // namespace

//=============================================================================================================

class mne::StreamInletPrivate
{
public:
    explicit StreamInletPrivate(const stream_info& info)
    : m_info(info)
    {
    }

    ~StreamInletPrivate()
    {
        closeStream();
    }

    StreamInletPrivate(const StreamInletPrivate&) = delete;
    StreamInletPrivate& operator=(const StreamInletPrivate&) = delete;

    void openStream()
    {
        if (m_pSocket) {
            return;
        }
        const QString host = QString::fromStdString(m_info.data_host());
        const auto port = static_cast<quint16>(m_info.data_port());
        if (host.isEmpty() || port == 0) {
            throw failure("Invalid data host or port in stream_info");
        }
        if (m_info.channel_format() == ChannelFormat::Undefined || m_info.channel_count() <= 0) {
            throw failure("stream_info has no channel format or channels");
        }
        m_pSocket = std::make_unique<QTcpSocket>();
        m_pSocket->connectToHost(host, port);
        if (!m_pSocket->waitForConnected(kConnectMs)) {
            const std::string error = m_pSocket->errorString().toStdString();
            m_pSocket.reset();
            throw failure("Failed to connect to outlet: " + error);
        }
        // The outlet buffers this many samples for us (liblsl: 360 s, or 36000 samples for irregular streams).
        const auto bufferSamples = static_cast<qlonglong>(m_info.nominal_srate() > 0.0 ? 360.0 * m_info.nominal_srate() : 36000.0);
        const QByteArray request = "LSL:streamfeed/110 " + QByteArray::fromStdString(m_info.uid()) +
            "\r\nNative-Byte-Order: " + byteOrderName() +
            "\r\nEndian-Performance: 0\r\nHas-IEEE754-Floats: 1\r\nSupports-Subnormals: 1\r\nValue-Size: " +
            QByteArray::number(static_cast<qulonglong>(valueSize(m_info.channel_format()))) +
            "\r\nData-Protocol-Version: 110\r\nMax-Buffer-Length: " + QByteArray::number(bufferSamples) +
            "\r\nMax-Chunk-Length: 0\r\nHostname: " +
            QHostInfo::localHostName().toUtf8() + "\r\nSource-Id: " + QByteArray::fromStdString(m_info.source_id()) +
            "\r\nSession-Id: default\r\n\r\n";
        m_pSocket->write(request);
        m_pSocket->flush();
        try {
            readHeaders();
            checkTestPatterns();
        } catch (...) {
            closeStream();
            throw;
        }
    }

    void closeStream()
    {
        if (m_pSocket) {
            if (m_pSocket->state() == QAbstractSocket::ConnectedState) {
                m_pSocket->disconnectFromHost();
                if (m_pSocket->state() != QAbstractSocket::UnconnectedState) {
                    m_pSocket->waitForDisconnected(1000);
                }
            }
            m_pSocket.reset();
        }
        m_buffer.clear();
        m_pos = 0;
    }

    bool readPending()
    {
        if (!m_pSocket) {
            return false;
        }
        if (m_pSocket->bytesAvailable() > 0 || m_pSocket->waitForReadyRead(0)) {
            m_buffer.append(m_pSocket->readAll());
        }
        qsizetype pos = m_pos;
        std::uint8_t tag = 0;
        double timestamp = 0.0;
        return m_reader.read(m_buffer, pos, tag, timestamp, m_values) == 1;
    }

    std::vector<std::vector<float>> pullChunk(std::vector<double>* timestamps)
    {
        std::vector<std::vector<float>> chunk;
        if (timestamps != nullptr) {
            timestamps->clear();
        }
        if (!m_pSocket) {
            return chunk;
        }
        readPending();
        std::uint8_t tag = 0;
        double timestamp = 0.0;
        int result = 0;
        while ((result = m_reader.read(m_buffer, m_pos, tag, timestamp, m_values)) == 1) {
            m_lastTimestamp = stamp(tag, timestamp);
            chunk.emplace_back(m_values.begin(), m_values.end());
            if (timestamps != nullptr) {
                timestamps->push_back(m_lastTimestamp);
            }
        }
        if (result < 0) {
            mne::Log::warning("[lsl::stream_inlet] Malformed sample from the outlet; closing the stream");
            closeStream();
            return chunk;
        }
        if (m_pos > (1 << 20)) {
            m_buffer.remove(0, m_pos);
            m_pos = 0;
        }
        return chunk;
    }

    double timeCorrection(double timeout)
    {
        const auto port = static_cast<quint16>(m_info.service_port());
        if (port == 0 || m_info.data_host().empty()) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        QUdpSocket socket;
        if (!detail::bindQuerySocket(socket, 0)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        const quint16 localPort = socket.localPort();
        const QHostAddress host(QString::fromStdString(m_info.data_host()));
        constexpr int kProbes = 8;
        double bestRtt = std::numeric_limits<double>::infinity();
        double best = std::numeric_limits<double>::quiet_NaN();
        QElapsedTimer timer;
        timer.start();
        const auto budget = static_cast<qint64>(timeout * 1000.0);
        for (int wave = 0; wave < kProbes && timer.elapsed() < budget; ++wave) {
            const double t0 = local_clock();
            socket.writeDatagram("LSL:timedata\r\n" + QByteArray::number(wave) + " " + QByteArray::number(t0, 'g', 17) + "\r\n",
                                 host, port);
            const qint64 waitUntil = std::min(budget, timer.elapsed() + 100);
            bool answered = false;
            while (!answered && timer.elapsed() < waitUntil) {
                if (!detail::waitForDatagrams(socket, localPort, static_cast<int>(waitUntil - timer.elapsed()))) {
                    continue;
                }
                while (socket.hasPendingDatagrams()) {
                    const double t3 = local_clock();
                    const QList<QByteArray> reply = socket.receiveDatagram().data().trimmed().split(' ');
                    if (reply.size() != 4 || reply[0].toInt() != wave) {
                        continue;
                    }
                    const double t1 = reply[2].toDouble();
                    const double t2 = reply[3].toDouble();
                    const double rtt = (t3 - t0) - (t2 - t1);
                    if (rtt < bestRtt) {
                        bestRtt = rtt;
                        best = -((t1 - t0) + (t2 - t3)) / 2.0; // remote clock minus local clock, negated
                    }
                    answered = true;
                }
            }
        }
        return best;
    }

private:
    /** Reads the response headers; the bytes after them start the sample stream. */
    void readHeaders()
    {
        QByteArray data;
        QElapsedTimer timer;
        timer.start();
        qsizetype end = -1;
        while ((end = data.indexOf("\r\n\r\n")) < 0) {
            if (timer.elapsed() > kConnectMs || data.size() > 65536 ||
                (m_pSocket->bytesAvailable() == 0 && !m_pSocket->waitForReadyRead(static_cast<int>(kConnectMs - timer.elapsed())))) {
                throw failure("Timeout waiting for the outlet's response");
            }
            data.append(m_pSocket->readAll());
        }
        const QList<QByteArray> lines = data.left(end).split('\n');
        const QList<QByteArray> status = lines.value(0).trimmed().split(' ');
        if (status.size() < 2 || !status[0].startsWith("LSL/") || status[1] != "200") {
            throw failure("Outlet refused the stream: " + lines.value(0).trimmed().toStdString());
        }
        bool reverse = false;
        for (const QByteArray& line : lines) {
            const qsizetype colon = line.indexOf(':');
            if (colon < 0) {
                continue;
            }
            const QByteArray key = line.left(colon).trimmed();
            const QByteArray value = line.mid(colon + 1).trimmed();
            if (key == "Byte-Order") {
                reverse = value != byteOrderName();
            } else if (key == "Data-Protocol-Version" && value.toInt() != kProtocolVersion) {
                throw failure("Outlet speaks data protocol " + value.toStdString() + "; 110 is supported");
            }
        }
        m_reader = SampleReader(m_info.channel_format(), m_info.channel_count(), reverse);
        m_buffer = data.mid(end + 4);
        m_pos = 0;
    }

    /** Reads the two test-pattern samples and checks them against the protocol's values. */
    void checkTestPatterns()
    {
        QElapsedTimer timer;
        timer.start();
        for (const int offset : {4, 2}) {
            std::uint8_t tag = 0;
            double timestamp = 0.0;
            int result = 0;
            while ((result = m_reader.read(m_buffer, m_pos, tag, timestamp, m_values)) == 0) {
                if (timer.elapsed() > kConnectMs || !m_pSocket->waitForReadyRead(static_cast<int>(kConnectMs - timer.elapsed()))) {
                    throw failure("Timeout waiting for the test-pattern samples");
                }
                m_buffer.append(m_pSocket->readAll());
            }
            bool match = result == 1 && tag == kTagTransmitted && timestamp == kTestPatternTime;
            for (std::size_t k = 0; match && k < m_values.size(); ++k) {
                const double expected = testPatternValue(m_info.channel_format(), static_cast<int>(k), offset);
                match = m_info.channel_format() == ChannelFormat::Float32 ? static_cast<float>(m_values[k]) == static_cast<float>(expected)
                                                                          : m_values[k] == expected;
            }
            if (!match) {
                throw failure("The outlet's test-pattern samples do not match; the protocol formats are incompatible");
            }
        }
    }

    double stamp(std::uint8_t tag, double timestamp) const noexcept
    {
        if (tag == kTagTransmitted) {
            return timestamp;
        }
        return m_info.nominal_srate() > 0.0 ? m_lastTimestamp + 1.0 / m_info.nominal_srate() : m_lastTimestamp;
    }

    stream_info m_info;                    /**< The stream info for this inlet. */
    std::unique_ptr<QTcpSocket> m_pSocket; /**< TCP socket of the stream feed; null while closed. */
    SampleReader m_reader;                 /**< Decoder for the negotiated format and byte order. */
    QByteArray m_buffer;                   /**< Received bytes not yet decoded, from m_pos on. */
    qsizetype m_pos = 0;                   /**< Read position in m_buffer. */
    std::vector<double> m_values;          /**< Scratch for one decoded sample. */
    double m_lastTimestamp = 0.0;          /**< Timestamp of the last decoded sample. */
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

stream_inlet::stream_inlet(const stream_info& info)
: m_pImpl(std::make_unique<StreamInletPrivate>(info))
{
}

//=============================================================================================================

stream_inlet::~stream_inlet() = default;

//=============================================================================================================

void stream_inlet::open_stream()
{
    m_pImpl->openStream();
}

//=============================================================================================================

void stream_inlet::close_stream()
{
    m_pImpl->closeStream();
}

//=============================================================================================================

bool stream_inlet::samples_available()
{
    return m_pImpl->readPending();
}

//=============================================================================================================

std::vector<std::vector<float>> stream_inlet::pull_chunk_float()
{
    return m_pImpl->pullChunk(nullptr);
}

//=============================================================================================================

std::vector<std::vector<float>> stream_inlet::pull_chunk_float(std::vector<double>& timestamps)
{
    return m_pImpl->pullChunk(&timestamps);
}

//=============================================================================================================

double stream_inlet::time_correction(double timeout)
{
    return m_pImpl->timeCorrection(timeout);
}
