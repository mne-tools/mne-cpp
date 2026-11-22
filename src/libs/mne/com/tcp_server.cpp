//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     tcp_server.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TcpServer on BSD sockets and Winsock.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tcp_server.h"

#include "detail/socket_platform.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TcpServer::~TcpServer()
{
    if (m_socket != kInvalidHandle) {
        close_native(static_cast<Native>(m_socket));
    }
}

//=============================================================================================================

TcpServer::TcpServer(TcpServer&& other) noexcept
: m_socket(std::exchange(other.m_socket, kInvalidHandle))
{
}

//=============================================================================================================

TcpServer& TcpServer::operator=(TcpServer&& other) noexcept
{
    if (this != &other) {
        TcpServer closing(std::move(*this));
        m_socket = std::exchange(other.m_socket, kInvalidHandle);
    }
    return *this;
}

//=============================================================================================================

std::expected<TcpServer, std::error_code> TcpServer::listen(const std::string& address, std::uint16_t port)
{
    if (!started()) {
        return std::unexpected(std::make_error_code(std::errc::not_supported));
    }
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    if (::inet_pton(AF_INET, address.c_str(), &local.sin_addr) != 1) {
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }
    const Native s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s == kInvalid) {
        return std::unexpected(last_error());
    }
    TcpServer server;
    server.m_socket = static_cast<Handle>(s);
#ifndef _WIN32
    const int on = 1; // a restarted server gets its port back while old connections linger
    ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#endif
    if (::bind(s, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) != 0 || ::listen(s, SOMAXCONN) != 0 ||
        !set_non_blocking(s)) {
        return std::unexpected(last_error());
    }
    return server;
}

//=============================================================================================================

std::uint16_t TcpServer::port() const noexcept
{
    sockaddr_in local{};
    socklen_t length = sizeof(local);
    if (::getsockname(static_cast<Native>(m_socket), reinterpret_cast<sockaddr*>(&local), &length) != 0) {
        return 0;
    }
    return ntohs(local.sin_port);
}

//=============================================================================================================

std::expected<TcpSocket, std::error_code> TcpServer::accept(std::chrono::milliseconds timeout)
{
    const auto s = static_cast<Native>(m_socket);
    while (true) {
        const int ready = wait_for(s, POLLIN, timeout);
        if (ready == 0) {
            return std::unexpected(std::make_error_code(std::errc::timed_out));
        }
        if (ready < 0) {
            return std::unexpected(last_error());
        }
        const Native client = ::accept(s, nullptr, nullptr);
        if (client == kInvalid) {
            const std::error_code error = last_error();
            if (would_block(error)) {
                continue; // the client gave up between poll and accept
            }
            return std::unexpected(error);
        }
        TcpSocket socket;
        socket.m_socket = static_cast<Handle>(client);
        if (!set_non_blocking(client)) {
            return std::unexpected(last_error());
        }
#ifdef SO_NOSIGPIPE
        int no_signal = 1;
        ::setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &no_signal, sizeof(no_signal));
#endif
        int on = 1;
        ::setsockopt(client, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof(on));
        return socket;
    }
}
