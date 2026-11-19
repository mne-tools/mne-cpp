//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     tcp_server.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    A listening TCP socket that accepts connections with a timeout, for programs that serve data over TCP.
 */

#ifndef COM_TCP_SERVER_H
#define COM_TCP_SERVER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "com_global.h"
#include "tcp_socket.h"

#include <chrono>
#include <cstdint>
#include <expected>
#include <string>
#include <system_error>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * A listening TCP socket; move-only, closed on destruction. Accepted connections are TcpSocket objects.
 *
 * @snippet ex_com/main.cpp tcp_server_usage
 */
class COMSHARED_EXPORT TcpServer
{
public:
    TcpServer() = default;
    ~TcpServer();

    TcpServer(TcpServer&& other) noexcept;
    TcpServer& operator=(TcpServer&& other) noexcept;
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    /**
     * @param[in] address   Local IPv4 address to listen on, e.g. 0.0.0.0 for all.
     * @param[in] port      Local port; 0 lets the system choose one.
     *
     * @return The listening socket, invalid_argument for an address that is no IPv4 address, or the system's
     *         error (e.g. address_in_use).
     */
    [[nodiscard]] static std::expected<TcpServer, std::error_code> listen(const std::string& address, std::uint16_t port);

    /** @return The local port the server listens on. */
    [[nodiscard]] std::uint16_t port() const noexcept;

    /**
     * @param[in] timeout   Maximum wait for a client; 0 only takes one that is already waiting.
     *
     * @return The next connection, timed_out if none came, or the system's error.
     */
    [[nodiscard]] std::expected<TcpSocket, std::error_code> accept(std::chrono::milliseconds timeout);

private:
#ifdef _WIN32
    std::uintptr_t m_socket = ~std::uintptr_t{0};
#else
    int m_socket = -1;
#endif
};

} // namespace mne

#endif // COM_TCP_SERVER_H
