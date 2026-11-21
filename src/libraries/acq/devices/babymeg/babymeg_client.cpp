//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     babymeg_client.cpp
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2013
 * @brief    The BabyMEG server protocol over TCP.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_transport.h"
#include "../transport_error.h"
#include <com/tcp_socket.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <optional>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;
using mne::TcpSocket;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

constexpr std::chrono::milliseconds kRest(5000); // for the rest of a message once it began
constexpr std::chrono::milliseconds kGoodbye(2000);
constexpr std::uint32_t kMaxBody = 1U << 30; // a corrupt length must not allocate gigabytes

/** A message: its command and body. */
struct Message
{
    std::string command;
    std::vector<std::byte> body;
};

std::span<const std::byte> bytes(std::string_view text)
{
    return std::as_bytes(std::span(text.data(), text.size()));
}

/** @return The next message on @p socket. */
AcqExpected<Message> receive(TcpSocket& socket, std::chrono::milliseconds timeout, std::vector<std::byte>& body, const std::string& name)
{
    std::array<std::byte, 8> header{};
    if (const std::error_code e = socket.read(header, timeout)) {
        return e == std::errc::connection_reset ? AcqError(AcqErrorCode::EndOfStream)
                                                : transport_error(e, AcqErrorCode::DeviceIo, std::format("{} failed", name));
    }
    std::uint32_t length = 0;
    for (std::size_t i = 4; i < 8; ++i) {
        length = (length << 8) | std::to_integer<std::uint32_t>(header[i]);
    }
    if (length > kMaxBody) {
        return AcqError(AcqErrorCode::DeviceIo, std::format("{} sent a message of {} bytes", name, length));
    }
    body.resize(length);
    if (socket.read(body, kRest)) {
        return AcqError(AcqErrorCode::DeviceIo, std::format("{} sent an incomplete message", name));
    }
    return Message{std::string(reinterpret_cast<const char*>(header.data()), 4), {}};
}

class BabyMegClient final : public BabyMegTransport
{
public:
    BabyMegClient(std::string host, std::uint16_t data_port, std::uint16_t command_port, std::chrono::milliseconds timeout, TcpSocket data)
    : m_host(std::move(host))
    , m_data_port(data_port)
    , m_command_port(command_port)
    , m_timeout(timeout)
    , m_data(std::move(data))
    , m_name(std::format("BabyMEG server {}:{}", m_host, m_data_port))
    {
    }

    ~BabyMegClient() override
    {
        stop();
    }

    BabyMegClient(const BabyMegClient&) = delete;
    BabyMegClient& operator=(const BabyMegClient&) = delete;

    std::string description() const override
    {
        return m_name;
    }

    AcqExpected<std::string> info(std::chrono::milliseconds timeout) override
    {
        // The command port serves one request per connection
        auto socket = TcpSocket::connect(m_host, m_command_port, timeout);
        if (!socket) {
            return AcqError(AcqErrorCode::DeviceNotFound,
                            std::format("no BabyMEG command port: cannot connect to {}:{} ({})", m_host, m_command_port, socket.error().message()));
        }
        if (const std::error_code e = socket->write(bytes("INFO"))) {
            return transport_error(e, AcqErrorCode::DeviceIo, "cannot ask the BabyMEG server for its channels");
        }
        std::vector<std::byte> body;
        while (true) {
            auto message = receive(*socket, timeout, body, m_name);
            if (!message) {
                return message.error() == AcqErrorCode::Timeout ? AcqError(AcqErrorCode::Timeout, std::format("{} does not describe its channels", m_name))
                                                                : message.error();
            }
            if (message->command == "INFO") {
                return std::string(reinterpret_cast<const char*>(body.data()), body.size());
            }
        }
    }

    AcqError start() override
    {
        if (!m_data) {
            auto socket = TcpSocket::connect(m_host, m_data_port, m_timeout);
            if (!socket) {
                return {AcqErrorCode::DeviceNotFound, std::format("cannot connect to {} ({})", m_name, socket.error().message())};
            }
            m_data = std::move(*socket);
        }
        return send("DATA");
    }

    AcqExpected<std::span<const std::byte>> next(std::chrono::milliseconds timeout) override
    {
        if (!m_data) {
            return AcqError(AcqErrorCode::InvalidState, "the BabyMEG data connection is closed");
        }
        while (true) {
            auto message = receive(*m_data, timeout, m_body, m_name);
            if (!message) {
                return message.error();
            }
            if (message->command == "DATR") {
                if (AcqError e = send("DATA"); !e.ok()) {
                    return e;
                }
                return std::span<const std::byte>(m_body);
            }
            if (message->command == "QUIT") {
                static_cast<void>(send("QREL"));
                m_data.reset();
                return AcqError(AcqErrorCode::EndOfStream);
            }
            timeout = kRest; // a message for someone else; the package may follow right away
        }
    }

    void stop() override
    {
        if (!m_data) {
            return;
        }
        if (send("QUIT").ok()) {
            // Packages already on their way come before the server's answer
            const auto deadline = std::chrono::steady_clock::now() + kGoodbye;
            while (std::chrono::steady_clock::now() < deadline) {
                auto message = receive(*m_data, kGoodbye, m_body, m_name);
                if (!message || message->command == "QUIT") {
                    break;
                }
            }
            static_cast<void>(send("QREL"));
        }
        m_data.reset();
    }

private:
    AcqError send(std::string_view command)
    {
        return transport_error(m_data->write(bytes(command)), AcqErrorCode::DeviceIo, std::format("cannot send to {}", m_name));
    }

    std::string m_host;
    std::uint16_t m_data_port;
    std::uint16_t m_command_port;
    std::chrono::milliseconds m_timeout;
    std::optional<TcpSocket> m_data;
    std::string m_name;
    std::vector<std::byte> m_body;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqExpected<std::unique_ptr<BabyMegTransport>> mne::detail::connect_babymeg(const std::string& host, std::uint16_t data_port,
                                                                            std::uint16_t command_port, std::chrono::milliseconds timeout)
{
    auto data = TcpSocket::connect(host, data_port, timeout);
    if (!data) {
        return AcqError(AcqErrorCode::DeviceNotFound,
                        std::format("no BabyMEG server: cannot connect to {}:{} ({})", host, data_port, data.error().message()));
    }
    return std::unique_ptr<BabyMegTransport>(std::make_unique<BabyMegClient>(host, data_port, command_port, timeout, std::move(*data)));
}
