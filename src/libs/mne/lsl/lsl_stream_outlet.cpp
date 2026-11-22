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
 * The PIMPL class @c StreamOutletPrivate runs a dedicated @c std::thread that owns every socket, so no socket
 * crosses threads. It answers discovery queries on the LSL discovery port 16571 (multicast groups,
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

#include <mne/base/log.h>
#include <mne/base/string_utils.h>
#include <mne/com/tcp_server.h>
#include <mne/com/tcp_socket.h>
#include <mne/com/udp_socket.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <future>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
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

constexpr std::size_t kMaxRequestBytes = 65536;

/** One accepted TCP connection: reading its request, or receiving the stream. */
struct Client
{
    TcpSocket socket;
    std::string request;
    bool streaming = false;
    bool closing = false;
};

std::span<const std::byte> bytes(std::string_view text)
{
    return std::as_bytes(std::span(text));
}

/** Sends a reply; a resolver or inlet that went away is not an error of the outlet. */
void reply(UdpSocket& socket, std::string_view text, const std::string& address, std::uint16_t port)
{
    if (const std::error_code error = socket.send_to(bytes(text), address, port)) {
        mne::Log::debug("[lsl::stream_outlet] Cannot answer {}:{}: {}", address, port, error.message());
    }
}


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
    template<typename T>
    void enqueue(const std::vector<std::vector<T>>& samples)
    {
        if (samples.empty()) {
            return;
        }
        const double rate = m_info.nominal_srate();
        const double now = local_clock();
        const double first = rate > 0.0 ? now - static_cast<double>(samples.size() - 1) / rate : now;
        std::string block;
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
        auto server = TcpServer::listen("0.0.0.0", 0);
        if (!server) {
            m_listening.set_value();
            return;
        }
        // The service port has the data port's number where it is free, as in liblsl.
        auto service = UdpSocket::bind("0.0.0.0", server->port());
        if (!service) {
            service = UdpSocket::bind("0.0.0.0", 0);
        }
        auto discovery = UdpSocket::bind("0.0.0.0", kDiscoveryPort, true);
        if (discovery) {
            joinDiscoveryGroups(*discovery);
        }
        m_info.set_data_port(server->port());
        m_info.set_service_port(service ? service->port() : 0);
        m_shortinfo = m_info.to_string();
        m_fullinfo = m_info.as_xml();
        m_listening.set_value();

        std::vector<Client> clients;
        std::array<std::byte, 4096> chunk{};
        while (m_bRunning) {
            while (auto connection = server->accept(std::chrono::milliseconds(0))) {
                Client client;
                client.socket = std::move(*connection);
                clients.push_back(std::move(client));
            }
            if (discovery) {
                answerQueries(*discovery, false);
            }
            if (service) {
                answerQueries(*service, true);
            }
            for (Client& client : clients) {
                if (client.closing) {
                    continue;
                }
                // A streaming inlet sends nothing; reading still notices when it goes away
                while (true) {
                    const auto got = client.socket.read_some(chunk, std::chrono::milliseconds(0));
                    if (!got) {
                        if (got.error() != std::errc::timed_out) {
                            client.socket.close();
                        }
                        break;
                    }
                    if (!client.streaming) {
                        client.request.append(reinterpret_cast<const char*>(chunk.data()), *got);
                    }
                }
                if (!client.streaming && client.socket.is_open()) {
                    handleRequest(client);
                }
            }
            std::vector<std::string> blocks;
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                blocks.swap(m_blocks);
            }
            for (Client& client : clients) {
                if (!client.streaming || !client.socket.is_open()) {
                    continue;
                }
                for (const std::string& block : blocks) {
                    if (client.socket.write(bytes(block))) {
                        client.socket.close();
                        break;
                    }
                }
            }
            std::erase_if(clients, [](const Client& client) { return client.closing || !client.socket.is_open(); });
            m_nClients.store(static_cast<int>(std::count_if(clients.begin(), clients.end(),
                                                            [](const Client& c) { return c.streaming; })));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        m_nClients.store(0);
    }

    /** Answers shortinfo queries and, on the service port, time-correction requests. */
    void answerQueries(UdpSocket& socket, bool timedata)
    {
        std::array<std::byte, 65536> buffer{};
        while (const auto datagram = socket.receive_from(buffer, std::chrono::milliseconds(0))) {
            const std::vector<std::string> lines = split(std::string_view(reinterpret_cast<const char*>(buffer.data()), datagram->size), '\n');
            if (lines.size() < 2) {
                continue;
            }
            const std::string_view kind = trimmed(lines[0]);
            const std::vector<std::string> words = lines.size() > 2 ? split(trimmed(lines[2]), ' ') : std::vector<std::string>();
            if (kind == "LSL:shortinfo" && words.size() == 2) {
                const std::optional<int> port = to_int(words[0]);
                if (matchesQuery(m_info, std::string(trimmed(lines[1]))) && port && *port > 0 && *port <= 65535) {
                    reply(socket, words[1] + "\r\n" + m_shortinfo, datagram->address, static_cast<std::uint16_t>(*port));
                }
            } else if (timedata && kind == "LSL:timedata") {
                const double received = local_clock();
                const std::vector<std::string> request = split(trimmed(lines[1]), ' ');
                if (request.size() == 2) {
                    reply(socket, " " + request[0] + " " + request[1] + " " + number(received) + " " + number(local_clock()), datagram->address, datagram->port);
                }
            }
        }
    }

    /** Writes the stream info, with its description if @p full, to a client that asked for it. */
    void sendInfo(Client& client, bool full) const
    {
        if (const std::error_code error = client.socket.write(bytes(full ? m_fullinfo : m_shortinfo))) {
            mne::Log::debug("[lsl::stream_outlet] Cannot send the stream info: {}", error.message());
        }
    }

    void handleRequest(Client& client)
    {
        const std::string& r = client.request;
        if (r.starts_with("LSL:streamfeed/")) {
            if (r.find("\r\n\r\n") == std::string::npos) {
                client.closing = r.size() > kMaxRequestBytes;
                return;
            }
            const std::vector<std::string> head = split(std::string_view(r).substr(0, r.find("\r\n")), ' ');
            const bool ours = head.size() < 2 || head[1].empty() || head[1] == m_info.uid();
            if (!ours) {
                client.closing = true;
                if (const std::error_code error = client.socket.write(bytes("LSL/110 404 Not found\r\n\r\n"))) {
                    mne::Log::debug("[lsl::stream_outlet] Cannot refuse a stream feed: {}", error.message());
                }
                return;
            }
            std::string reply = "LSL/110 200 OK\r\nUID: " + m_info.uid() + "\r\nByte-Order: " + byteOrderName() +
                "\r\nSuppress-Subnormals: 0\r\nData-Protocol-Version: " + std::to_string(kProtocolVersion) + "\r\n\r\n";
            std::vector<double> pattern(static_cast<std::size_t>(m_info.channel_count()));
            for (const int offset : {4, 2}) {
                for (std::size_t k = 0; k < pattern.size(); ++k) {
                    pattern[k] = testPatternValue(m_info.channel_format(), static_cast<int>(k), offset);
                }
                appendSample(reply, m_info.channel_format(), pattern, kTestPatternTime, false);
            }
            if (client.socket.write(bytes(reply))) {
                client.closing = true;
                return;
            }
            client.streaming = true;
            client.request.clear();
        } else if (r.starts_with("LSL:shortinfo\r\n")) {
            const std::size_t end = r.find("\r\n", 15);
            if (end == std::string::npos) {
                client.closing = r.size() > kMaxRequestBytes;
                return;
            }
            client.closing = true;
            if (matchesQuery(m_info, std::string(trimmed(std::string_view(r).substr(15, end - 15))))) {
                sendInfo(client, false);
            }
        } else if (r.starts_with("LSL:fullinfo")) {
            if (r.find('\n') != std::string::npos) {
                client.closing = true;
                sendInfo(client, true);
            }
        } else if (r.find('\n') != std::string::npos || r.size() > kMaxRequestBytes) {
            client.closing = true; // e.g. a protocol 1.00 stream feed, which is not served
        }
    }

    stream_info m_info;                  /**< Stream description, ports set once listening. */
    std::string m_shortinfo;             /**< m_info as XML, written by the worker before m_listening. */
    std::string m_fullinfo;              /**< m_info as XML with its description, written with m_shortinfo. */
    std::atomic<bool> m_bRunning{false}; /**< Worker running flag. */
    std::thread m_bgThread;              /**< Worker thread owning all sockets. */
    std::promise<void> m_listening;      /**< Fulfilled once the ports are recorded in m_info. */
    std::mutex m_queueMutex;             /**< Protects m_blocks. */
    std::vector<std::string> m_blocks;   /**< Encoded sample blocks not yet sent. */
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
    m_pImpl->enqueue(std::vector<std::vector<float>>{sample});
}

//=============================================================================================================

void stream_outlet::push_chunk(const std::vector<std::vector<float>>& chunk)
{
    m_pImpl->enqueue(chunk);
}

//=============================================================================================================

void stream_outlet::push_chunk_double(const std::vector<std::vector<double>>& chunk)
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
