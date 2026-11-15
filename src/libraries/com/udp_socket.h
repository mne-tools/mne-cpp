//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     udp_socket.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November, 2026
 * @brief    A UDP socket bound to a local port or a multicast group, for devices that send their data as datagrams.
 */

#ifndef COM_UDP_SOCKET_H
#define COM_UDP_SOCKET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "com_global.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <system_error>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/**
 * A bound UDP socket; move-only, closed on destruction.
 *
 * @snippet ex_com/main.cpp udp_usage
 */
class COMSHARED_EXPORT UdpSocket
{
public:
    UdpSocket() = default;
    ~UdpSocket();

    UdpSocket(UdpSocket&& other) noexcept;
    UdpSocket& operator=(UdpSocket&& other) noexcept;
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    /**
     * @param[in] address   Local IPv4 address to receive on, e.g. 0.0.0.0 for all; a multicast group (224.0.0.0 to
     *                      239.255.255.255) is joined on the interface of the default route, and its port is shared
     *                      with the other members on this machine.
     * @param[in] port      Local port; 0 lets the system choose one.
     *
     * @return The bound socket, invalid_argument for an address that is no IPv4 address, or the system's error
     *         (e.g. address_in_use).
     */
    [[nodiscard]] static std::expected<UdpSocket, std::error_code> bind(const std::string& address, std::uint16_t port);

    /** @return The local port the socket is bound to. */
    [[nodiscard]] std::uint16_t port() const noexcept;

    /**
     * @param[out] buffer    Destination; a longer datagram is cut.
     * @param[in]  timeout   Maximum wait.
     *
     * @return The size of the next datagram, timed_out if none arrived, or the system's error.
     */
    [[nodiscard]] std::expected<std::size_t, std::error_code> receive(std::span<std::byte> buffer, std::chrono::milliseconds timeout);

    /**
     * Sends one datagram. To a multicast group it leaves through the interface of the default route and also
     * reaches the members on this machine.
     *
     * @param[in] datagram   The datagram.
     * @param[in] address    Destination IPv4 address.
     * @param[in] port       Destination port.
     *
     * @return Empty, invalid_argument for an address that is no IPv4 address, or the system's error.
     */
    [[nodiscard]] std::error_code send_to(std::span<const std::byte> datagram, const std::string& address, std::uint16_t port);

private:
#ifdef _WIN32
    std::uintptr_t m_socket = ~std::uintptr_t{0};
#else
    int m_socket = -1;
#endif
    bool m_multicast_out = false; // the outgoing multicast interface is set
};

} // namespace mne

#endif // COM_UDP_SOCKET_H
