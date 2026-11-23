//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     tcp_socket.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November, 2026
 * @brief    TcpSocket on BSD sockets and Winsock.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tcp_socket.h"

#include "detail/socket_platform.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <chrono>
#include <span>
#include <string>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

namespace
{

/**
 * Bytes handed to one send(). A non-blocking Winsock send() takes all of a large buffer at once, past the send
 * buffer, so a write to a peer that never reads would neither block nor time out; in chunks, the system refuses
 * the next one once its buffers are full, on every platform.
 */
constexpr std::size_t kMaxSend = std::size_t{64} << 10;

/** @return The number of bytes of @p data the system took, or a negative number with the reason in last_error(). */
auto send_some(Native s, std::span<const std::byte> data) noexcept
{
    data = data.first(std::min(data.size(), kMaxSend));
#ifdef _WIN32
    return ::send(s, reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()), 0);
#elif defined(MSG_NOSIGNAL)
    return ::send(s, data.data(), data.size(), MSG_NOSIGNAL);
#else
    return ::send(s, data.data(), data.size(), 0); // SO_NOSIGPIPE is set
#endif
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TcpSocket::~TcpSocket()
{
    if (m_socket != kInvalidHandle) {
        close_native(static_cast<Native>(m_socket));
    }
}

//=============================================================================================================

TcpSocket::TcpSocket(TcpSocket&& other) noexcept
: m_socket(std::exchange(other.m_socket, kInvalidHandle))
{
}

//=============================================================================================================

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept
{
    if (this != &other) {
        TcpSocket closing(std::move(*this));
        m_socket = std::exchange(other.m_socket, kInvalidHandle);
    }
    return *this;
}

//=============================================================================================================

std::expected<TcpSocket, std::error_code> TcpSocket::connect(const std::string& host, std::uint16_t port, std::chrono::milliseconds timeout)
{
    if (!started()) {
        return std::unexpected(std::make_error_code(std::errc::not_supported));
    }
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* found = nullptr;
    const std::string service = std::to_string(port);
    if (::getaddrinfo(host.c_str(), service.c_str(), &hints, &found) != 0 || found == nullptr) {
        return std::unexpected(std::make_error_code(std::errc::host_unreachable));
    }
    std::error_code why = std::make_error_code(std::errc::address_not_available);
    TcpSocket socket;
    for (const addrinfo* a = found; a != nullptr; a = a->ai_next) {
        const Native s = ::socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if (s == kInvalid) {
            why = last_error();
            continue;
        }
        socket = TcpSocket();
        socket.m_socket = static_cast<Handle>(s);
        if (!set_non_blocking(s)) {
            why = last_error();
            continue;
        }
#ifdef SO_NOSIGPIPE
        int no_signal = 1; // a closed peer is an error, not a signal
        ::setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &no_signal, sizeof(no_signal));
#endif
        if (::connect(s, a->ai_addr, static_cast<socklen_t>(a->ai_addrlen)) != 0 && !would_block(last_error())) {
            why = last_error();
            continue;
        }
        if (wait_for(s, POLLOUT, timeout) <= 0) {
            why = std::make_error_code(std::errc::timed_out);
            continue;
        }
        int error = 0;
        socklen_t length = sizeof(error);
        ::getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &length);
        if (error != 0) {
#ifdef _WIN32
            why = {error, std::system_category()};
#else
            why = {error, std::generic_category()};
#endif
            continue;
        }
        int on = 1; // requests are small and answered one by one
        ::setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&on), sizeof(on));
        ::freeaddrinfo(found);
        return socket;
    }
    ::freeaddrinfo(found);
    return std::unexpected(why);
}

//=============================================================================================================

std::error_code TcpSocket::write(std::span<const std::byte> data)
{
    const auto s = static_cast<Native>(m_socket);
    while (!data.empty()) {
        const auto sent = send_some(s, data);
        if (sent < 0) {
            const std::error_code error = last_error();
            if (would_block(error) && wait_for(s, POLLOUT, std::chrono::milliseconds(1000)) > 0) {
                continue;
            }
            return error;
        }
        data = data.subspan(static_cast<std::size_t>(sent));
    }
    return {};
}

//=============================================================================================================

std::error_code TcpSocket::write(std::span<const std::byte> data, std::chrono::milliseconds timeout)
{
    const auto s = static_cast<Native>(m_socket);
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!data.empty()) {
        const auto sent = send_some(s, data);
        if (sent < 0) {
            const std::error_code error = last_error();
            if (!would_block(error)) {
                return error;
            }
            const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
            if (left <= std::chrono::milliseconds::zero()) {
                return std::make_error_code(std::errc::timed_out);
            }
            const int ready = wait_for(s, POLLOUT, left);
            if (ready == 0) {
                return std::make_error_code(std::errc::timed_out);
            }
            if (ready < 0) {
                return last_error();
            }
            continue;
        }
        data = data.subspan(static_cast<std::size_t>(sent));
    }
    return {};
}

//=============================================================================================================

std::error_code TcpSocket::read(std::span<std::byte> buffer, std::chrono::milliseconds timeout)
{
    const auto s = static_cast<Native>(m_socket);
    bool started_reading = false;
    while (!buffer.empty()) {
        const int ready = wait_for(s, POLLIN, timeout);
        if (ready == 0) {
            return std::make_error_code(started_reading ? std::errc::io_error : std::errc::timed_out);
        }
        if (ready < 0) {
            return last_error();
        }
#ifdef _WIN32
        const int got = ::recv(s, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0);
#else
        const auto got = ::recv(s, buffer.data(), buffer.size(), 0);
#endif
        if (got == 0) {
            return std::make_error_code(std::errc::connection_reset);
        }
        if (got < 0) {
            const std::error_code error = last_error();
            if (would_block(error)) {
                continue;
            }
            return error;
        }
        started_reading = true;
        buffer = buffer.subspan(static_cast<std::size_t>(got));
    }
    return {};
}

//=============================================================================================================

std::expected<std::size_t, std::error_code> TcpSocket::read_some(std::span<std::byte> buffer, std::chrono::milliseconds timeout)
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
#ifdef _WIN32
        const int got = ::recv(s, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0);
#else
        const auto got = ::recv(s, buffer.data(), buffer.size(), 0);
#endif
        if (got == 0) {
            return std::unexpected(std::make_error_code(std::errc::connection_reset));
        }
        if (got < 0) {
            const std::error_code error = last_error();
            if (would_block(error)) {
                continue;
            }
            return std::unexpected(error);
        }
        return static_cast<std::size_t>(got);
    }
}

//=============================================================================================================

bool TcpSocket::is_open() const noexcept
{
    return m_socket != kInvalidHandle;
}

//=============================================================================================================

void TcpSocket::close() noexcept
{
    if (m_socket != kInvalidHandle) {
        close_native(static_cast<Native>(std::exchange(m_socket, kInvalidHandle)));
    }
}
