//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     socket_platform.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November, 2026
 * @brief    What BSD sockets and Winsock do differently, for the socket classes.
 */

#ifndef COM_DETAIL_SOCKET_PLATFORM_H
#define COM_DETAIL_SOCKET_PLATFORM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <chrono>
#include <cstdint>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

#ifdef _WIN32
using Native = SOCKET;
inline constexpr Native kInvalid = INVALID_SOCKET;
using Handle = std::uintptr_t;

inline std::error_code last_error() noexcept
{
    return {WSAGetLastError(), std::system_category()};
}

inline void close_native(Native s) noexcept
{
    closesocket(s);
}

/** @return 1 when @p s is ready for @p events within @p timeout, 0 on timeout, -1 on failure. */
inline int wait_for(Native s, short events, std::chrono::milliseconds timeout) noexcept
{
    WSAPOLLFD p{s, events, 0};
    return WSAPoll(&p, 1, static_cast<INT>(timeout.count()));
}

/** @return Whether Winsock is ready, starting it on first use. */
inline bool started() noexcept
{
    static const bool ok = [] {
        WSADATA data{};
        return WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();
    return ok;
}

inline bool would_block(const std::error_code& error) noexcept
{
    return error.value() == WSAEWOULDBLOCK || error.value() == WSAEINPROGRESS;
}
#else
using Native = int;
inline constexpr Native kInvalid = -1;
using Handle = int;

inline std::error_code last_error() noexcept
{
    return {errno, std::generic_category()};
}

inline void close_native(Native s) noexcept
{
    ::close(s);
}

inline int wait_for(Native s, short events, std::chrono::milliseconds timeout) noexcept
{
    pollfd p{s, events, 0};
    int ready = 0;
    do {
        ready = ::poll(&p, 1, static_cast<int>(timeout.count()));
    } while (ready < 0 && errno == EINTR);
    return ready;
}

inline bool started() noexcept
{
    return true;
}

inline bool would_block(const std::error_code& error) noexcept
{
    return error == std::errc::operation_in_progress || error == std::errc::resource_unavailable_try_again ||
        error == std::errc::operation_would_block || error == std::errc::interrupted;
}
#endif

inline constexpr auto kInvalidHandle = static_cast<Handle>(kInvalid);

/** Puts @p s into non-blocking mode; reads and writes then wait in wait_for(). */
inline bool set_non_blocking(Native s) noexcept
{
#ifdef _WIN32
    u_long on = 1;
    return ioctlsocket(s, FIONBIO, &on) == 0;
#else
    const int flags = ::fcntl(s, F_GETFL, 0);
    return flags >= 0 && ::fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

} // namespace mne::detail

#endif // COM_DETAIL_SOCKET_PLATFORM_H
