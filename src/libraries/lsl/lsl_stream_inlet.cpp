//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_stream_inlet.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implements the receive path of liblsl's data protocol 1.10, its fullinfo request and its time-correction exchange.
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

#include <base/process.h>
#include <base/string_utils.h>
#include <com/tcp_socket.h>
#include <com/udp_socket.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <limits>
#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

namespace
{

constexpr std::chrono::milliseconds kConnect{5000};

using Clock = std::chrono::steady_clock;

std::span<const std::byte> bytes(std::string_view text)
{
    return std::as_bytes(std::span(text));
}


/** @return The time left until @p deadline, at least zero. */
std::chrono::milliseconds left(Clock::time_point deadline)
{
    return std::max(std::chrono::ceil<std::chrono::milliseconds>(deadline - Clock::now()), std::chrono::milliseconds(0));
}

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
        if (m_socket.is_open()) {
            return;
        }
        const std::string& host = m_info.data_host();
        const int port = m_info.data_port();
        if (host.empty() || port <= 0 || port > 65535) {
            throw failure("Invalid data host or port in stream_info");
        }
        if (m_info.channel_format() == ChannelFormat::Undefined || m_info.channel_count() <= 0) {
            throw failure("stream_info has no channel format or channels");
        }
        auto connection = TcpSocket::connect(host, static_cast<std::uint16_t>(port), kConnect);
        if (!connection) {
            throw failure("Failed to connect to outlet: " + connection.error().message());
        }
        m_socket = std::move(*connection);
        // The outlet buffers this many samples for us (liblsl: 360 s, or 36000 samples for irregular streams).
        const auto bufferSamples = static_cast<long long>(m_info.nominal_srate() > 0.0 ? 360.0 * m_info.nominal_srate() : 36000.0);
        const std::string request = "LSL:streamfeed/110 " + m_info.uid() + "\r\nNative-Byte-Order: " + byteOrderName() +
            "\r\nEndian-Performance: 0\r\nHas-IEEE754-Floats: 1\r\nSupports-Subnormals: 1\r\nValue-Size: " +
            std::to_string(valueSize(m_info.channel_format())) + "\r\nData-Protocol-Version: 110\r\nMax-Buffer-Length: " +
            std::to_string(bufferSamples) + "\r\nMax-Chunk-Length: 0\r\nHostname: " + host_name() + "\r\nSource-Id: " +
            m_info.source_id() + "\r\nSession-Id: default\r\n\r\n";
        try {
            if (const std::error_code error = m_socket.write(bytes(request))) {
                throw failure("Failed to send the request: " + error.message());
            }
            readHeaders();
            checkTestPatterns();
        } catch (...) {
            closeStream();
            throw;
        }
    }

    void closeStream()
    {
        m_socket.close();
        m_buffer.clear();
        m_pos = 0;
    }

    bool readPending()
    {
        if (!m_socket.is_open()) {
            return false;
        }
        receive(std::chrono::milliseconds(0));
        std::size_t pos = m_pos;
        std::uint8_t tag = 0;
        double timestamp = 0.0;
        return m_reader.read(m_buffer, pos, tag, timestamp, m_values) == 1;
    }

    template<typename T>
    std::vector<std::vector<T>> pullChunk(std::vector<double>* timestamps)
    {
        std::vector<std::vector<T>> chunk;
        if (timestamps != nullptr) {
            timestamps->clear();
        }
        if (!m_socket.is_open()) {
            return chunk;
        }
        receive(std::chrono::milliseconds(0));
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
            m_buffer.erase(0, m_pos);
            m_pos = 0;
        }
        return chunk;
    }

    stream_info fullInfo(double timeout)
    {
        if (m_hasFullInfo) {
            return m_info;
        }
        const std::string& host = m_info.data_host();
        const int port = m_info.data_port();
        if (host.empty() || port <= 0 || port > 65535) {
            throw failure("Invalid data host or port in stream_info");
        }
        const Clock::time_point deadline = Clock::now() + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(timeout));
        auto connection = TcpSocket::connect(host, static_cast<std::uint16_t>(port), left(deadline));
        if (!connection) {
            throw failure("Failed to connect to outlet: " + connection.error().message());
        }
        if (const std::error_code error = connection->write(bytes("LSL:fullinfo\r\n"))) {
            throw failure("Failed to send the fullinfo request: " + error.message());
        }
        // The outlet sends the XML and closes the connection.
        std::string xml;
        std::array<std::byte, 65536> chunk{};
        while (xml.find("</info>") == std::string::npos) {
            const auto got = connection->read_some(chunk, left(deadline));
            if (!got) {
                break;
            }
            xml.append(reinterpret_cast<const char*>(chunk.data()), *got);
        }
        const stream_info full = stream_info::from_string(xml);
        if (full.name().empty()) {
            throw failure("The outlet sent no valid stream info");
        }
        m_info.set_channels(full.channels());
        m_hasFullInfo = true;
        return m_info;
    }

    double timeCorrection(double timeout)
    {
        const int port = m_info.service_port();
        if (port <= 0 || port > 65535 || m_info.data_host().empty()) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        auto socket = detail::bindQuerySocket(0);
        if (!socket) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        constexpr int kProbes = 8;
        double bestRtt = std::numeric_limits<double>::infinity();
        double best = std::numeric_limits<double>::quiet_NaN();
        const Clock::time_point deadline = Clock::now() + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(timeout));
        std::array<std::byte, 512> buffer{};
        for (int wave = 0; wave < kProbes && Clock::now() < deadline; ++wave) {
            const double t0 = local_clock();
            const std::string probe = std::format("LSL:timedata\r\n{} {:.17g}\r\n", wave, t0);
            if (socket->send_to(bytes(probe), m_info.data_host(), static_cast<std::uint16_t>(port))) {
                break;
            }
            const Clock::time_point waitUntil = std::min(deadline, Clock::now() + std::chrono::milliseconds(100));
            bool answered = false;
            while (!answered && Clock::now() < waitUntil) {
                const auto datagram = socket->receive_from(buffer, left(waitUntil));
                if (!datagram) {
                    continue;
                }
                const double t3 = local_clock();
                const std::vector<std::string> reply = split(trimmed(std::string_view(reinterpret_cast<const char*>(buffer.data()), datagram->size)), ' ');
                if (reply.size() != 4 || to_int(reply[0]) != wave) {
                    continue;
                }
                const double t1 = to_double(reply[2]).value_or(std::numeric_limits<double>::quiet_NaN());
                const double t2 = to_double(reply[3]).value_or(std::numeric_limits<double>::quiet_NaN());
                const double rtt = (t3 - t0) - (t2 - t1);
                if (rtt < bestRtt) {
                    bestRtt = rtt;
                    best = -((t1 - t0) + (t2 - t3)) / 2.0; // remote clock minus local clock, negated
                }
                answered = true;
            }
        }
        return best;
    }

private:
    /** Appends what has arrived within @p timeout to m_buffer; closes the stream if the outlet went away. */
    bool receive(std::chrono::milliseconds timeout)
    {
        std::array<std::byte, 65536> chunk{};
        bool any = false;
        while (true) {
            const auto got = m_socket.read_some(chunk, any ? std::chrono::milliseconds(0) : timeout);
            if (!got) {
                if (got.error() != std::errc::timed_out) {
                    m_socket.close();
                }
                return any;
            }
            m_buffer.append(reinterpret_cast<const char*>(chunk.data()), *got);
            any = true;
        }
    }

    /** Reads the response headers; the bytes after them start the sample stream. */
    void readHeaders()
    {
        const Clock::time_point deadline = Clock::now() + kConnect;
        std::size_t end = std::string::npos;
        while ((end = m_buffer.find("\r\n\r\n")) == std::string::npos) {
            if (Clock::now() >= deadline || m_buffer.size() > 65536 || !m_socket.is_open() || !receive(left(deadline))) {
                throw failure("Timeout waiting for the outlet's response");
            }
        }
        const std::vector<std::string> lines = split(std::string_view(m_buffer).substr(0, end), '\n');
        const std::vector<std::string> status = split(trimmed(lines.front()), ' ');
        if (status.size() < 2 || !status[0].starts_with("LSL/") || status[1] != "200") {
            throw failure("Outlet refused the stream: " + std::string(trimmed(lines.front())));
        }
        bool reverse = false;
        for (const std::string& line : lines) {
            const std::size_t colon = line.find(':');
            if (colon == std::string::npos) {
                continue;
            }
            const std::string key(trimmed(std::string_view(line).substr(0, colon)));
            const std::string value(trimmed(std::string_view(line).substr(colon + 1)));
            if (key == "Byte-Order") {
                reverse = value != byteOrderName();
            } else if (key == "Data-Protocol-Version" && to_int(value) != kProtocolVersion) {
                throw failure("Outlet speaks data protocol " + value + "; 110 is supported");
            }
        }
        m_reader = SampleReader(m_info.channel_format(), m_info.channel_count(), reverse);
        m_buffer.erase(0, end + 4);
        m_pos = 0;
    }

    /** Reads the two test-pattern samples and checks them against the protocol's values. */
    void checkTestPatterns()
    {
        const Clock::time_point deadline = Clock::now() + kConnect;
        for (const int offset : {4, 2}) {
            std::uint8_t tag = 0;
            double timestamp = 0.0;
            int result = 0;
            while ((result = m_reader.read(m_buffer, m_pos, tag, timestamp, m_values)) == 0) {
                if (Clock::now() >= deadline || !m_socket.is_open() || !receive(left(deadline))) {
                    throw failure("Timeout waiting for the test-pattern samples");
                }
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

    stream_info m_info;           /**< The stream info for this inlet. */
    TcpSocket m_socket;           /**< TCP connection of the stream feed; closed while not streaming. */
    SampleReader m_reader;        /**< Decoder for the negotiated format and byte order. */
    std::string m_buffer;         /**< Received bytes not yet decoded, from m_pos on. */
    std::size_t m_pos = 0;        /**< Read position in m_buffer. */
    std::vector<double> m_values; /**< Scratch for one decoded sample. */
    double m_lastTimestamp = 0.0; /**< Timestamp of the last decoded sample. */
    bool m_hasFullInfo = false;   /**< Whether m_info holds the description from the outlet's full info. */
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
    return m_pImpl->pullChunk<float>(nullptr);
}

//=============================================================================================================

std::vector<std::vector<float>> stream_inlet::pull_chunk_float(std::vector<double>& timestamps)
{
    return m_pImpl->pullChunk<float>(&timestamps);
}

//=============================================================================================================

std::vector<std::vector<double>> stream_inlet::pull_chunk_double()
{
    return m_pImpl->pullChunk<double>(nullptr);
}

//=============================================================================================================

std::vector<std::vector<double>> stream_inlet::pull_chunk_double(std::vector<double>& timestamps)
{
    return m_pImpl->pullChunk<double>(&timestamps);
}

//=============================================================================================================

stream_info stream_inlet::info(double timeout)
{
    return m_pImpl->fullInfo(timeout);
}

//=============================================================================================================

double stream_inlet::time_correction(double timeout)
{
    return m_pImpl->timeCorrection(timeout);
}
