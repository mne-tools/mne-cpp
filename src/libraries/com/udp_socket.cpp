//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     udp_socket.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November, 2026
 * @brief    UdpSocket on BSD sockets and Winsock.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "udp_socket.h"

#include "detail/socket_platform.h"

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

bool is_multicast(const in_addr& address) noexcept
{
    return (ntohl(address.s_addr) >> 28) == 0xE;
}

/** @return The local address of the default route, as BrainFlow chooses it; INADDR_ANY without a route. */
in_addr default_interface() noexcept
{
    in_addr any{};
    any.s_addr = htonl(INADDR_ANY);
    const Native probe = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (probe == kInvalid) {
        return any;
    }
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_port = htons(53);
    ::inet_pton(AF_INET, "8.8.8.8", &to.sin_addr);
    sockaddr_in local{};
    socklen_t length = sizeof(local);
    // Connecting a UDP socket sends nothing; it only picks the route
    const bool routed = ::connect(probe, reinterpret_cast<const sockaddr*>(&to), sizeof(to)) == 0 &&
        ::getsockname(probe, reinterpret_cast<sockaddr*>(&local), &length) == 0;
    close_native(probe);
    return routed ? local.sin_addr : any;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

UdpSocket::~UdpSocket()
{
    if (m_socket != kInvalidHandle) {
        close_native(static_cast<Native>(m_socket));
    }
}

//=============================================================================================================

UdpSocket::UdpSocket(UdpSocket&& other) noexcept
: m_socket(std::exchange(other.m_socket, kInvalidHandle))
, m_multicast_out(std::exchange(other.m_multicast_out, false))
{
}

//=============================================================================================================

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept
{
    if (this != &other) {
        UdpSocket closing(std::move(*this));
        m_socket = std::exchange(other.m_socket, kInvalidHandle);
        m_multicast_out = std::exchange(other.m_multicast_out, false);
    }
    return *this;
}

//=============================================================================================================

std::expected<UdpSocket, std::error_code> UdpSocket::bind(const std::string& address, std::uint16_t port)
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
    const Native s = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (s == kInvalid) {
        return std::unexpected(last_error());
    }
    UdpSocket socket;
    socket.m_socket = static_cast<Handle>(s);
    int size = 4 << 20; // a burst of datagrams must not overflow while the reader waits
    ::setsockopt(s, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&size), sizeof(size));
    ip_mreq membership{};
    const bool group = is_multicast(local.sin_addr);
    if (group) {
        const int on = 1;
        ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&on), sizeof(on));
#ifdef SO_REUSEPORT
        ::setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)); // BSD shares a wildcard-bound port only so
#endif
        membership.imr_multiaddr = local.sin_addr;
        membership.imr_interface = default_interface();
        local.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    if (::bind(s, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) != 0) {
        return std::unexpected(last_error());
    }
    if (group) {
        // Senders that leave the interface to the system use its multicast route, others the default route
        // (BrainFlow's streamer the former, its receiver the latter); both differ behind a VPN
        bool joined = false;
        std::error_code error;
        for (const std::uint32_t network_interface : {htonl(INADDR_ANY), membership.imr_interface.s_addr}) {
            membership.imr_interface.s_addr = network_interface;
            if (::setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&membership), sizeof(membership)) == 0) {
                joined = true;
            } else {
                error = last_error();
            }
        }
        if (!joined) {
            return std::unexpected(error);
        }
    }
    return socket;
}

//=============================================================================================================

std::uint16_t UdpSocket::port() const noexcept
{
    sockaddr_in local{};
    socklen_t length = sizeof(local);
    if (::getsockname(static_cast<Native>(m_socket), reinterpret_cast<sockaddr*>(&local), &length) != 0) {
        return 0;
    }
    return ntohs(local.sin_port);
}

//=============================================================================================================

std::expected<std::size_t, std::error_code> UdpSocket::receive(std::span<std::byte> buffer, std::chrono::milliseconds timeout)
{
    const auto s = static_cast<Native>(m_socket);
    const int ready = wait_for(s, POLLIN, timeout);
    if (ready == 0) {
        return std::unexpected(std::make_error_code(std::errc::timed_out));
    }
    if (ready < 0) {
        return std::unexpected(last_error());
    }
#ifdef _WIN32
    const int got = ::recv(s, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0);
    if (got < 0 && WSAGetLastError() == WSAEMSGSIZE) {
        return buffer.size();
    }
#else
    const auto got = ::recv(s, buffer.data(), buffer.size(), 0);
#endif
    if (got < 0) {
        return std::unexpected(last_error());
    }
    return static_cast<std::size_t>(got);
}

//=============================================================================================================

std::error_code UdpSocket::send_to(std::span<const std::byte> datagram, const std::string& address, std::uint16_t port)
{
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_port = htons(port);
    if (::inet_pton(AF_INET, address.c_str(), &to.sin_addr) != 1) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    const auto s = static_cast<Native>(m_socket);
    if (is_multicast(to.sin_addr) && !m_multicast_out) {
        const in_addr out = default_interface();
#ifdef _WIN32
        const DWORD loop = 1;
#else
        const unsigned char loop = 1;
#endif
        ::setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast<const char*>(&out), sizeof(out));
        ::setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast<const char*>(&loop), sizeof(loop));
        m_multicast_out = true;
    }
#ifdef _WIN32
    const int sent = ::sendto(s, reinterpret_cast<const char*>(datagram.data()), static_cast<int>(datagram.size()), 0,
                              reinterpret_cast<const sockaddr*>(&to), sizeof(to));
#else
    const auto sent = ::sendto(s, datagram.data(), datagram.size(), 0, reinterpret_cast<const sockaddr*>(&to), sizeof(to));
#endif
    if (sent < 0) {
        return last_error();
    }
    return {};
}
