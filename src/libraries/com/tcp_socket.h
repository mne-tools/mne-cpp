//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     tcp_socket.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November, 2026
 * @brief    A blocking TCP client connection with timeouts, for devices and servers that send their data over TCP.
 */

#ifndef COM_TCP_SOCKET_H
#define COM_TCP_SOCKET_H

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
 * A connected TCP socket; move-only, closed on destruction.
 *
 * @snippet ex_com/main.cpp tcp_usage
 */
class COMSHARED_EXPORT TcpSocket
{
public:
    TcpSocket() = default;
    ~TcpSocket();

    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    /**
     * @param[in] host      Host name or address.
     * @param[in] port      Port.
     * @param[in] timeout   Maximum wait for the connection.
     *
     * @return The connection, or why there is none: host_unreachable for an unknown host, timed_out if nothing
     *         answers, the system's error otherwise (e.g. connection_refused).
     */
    [[nodiscard]] static std::expected<TcpSocket, std::error_code> connect(const std::string& host, std::uint16_t port,
                                                                           std::chrono::milliseconds timeout);

    /**
     * @param[in] data   Bytes to send, all of them.
     *
     * @return The system's error if the connection fails.
     */
    std::error_code write(std::span<const std::byte> data);

    /**
     * Reads exactly @p buffer.size() bytes.
     *
     * @param[out] buffer    Destination.
     * @param[in]  timeout   Maximum wait for the first byte; the rest must follow within it too.
     *
     * @return timed_out if nothing arrived (nothing is consumed then), io_error if the rest did not follow,
     *         connection_reset if the peer closed the connection, or the system's error.
     */
    std::error_code read(std::span<std::byte> buffer, std::chrono::milliseconds timeout);

    //=========================================================================================================
    /**
     * Reads what has arrived, up to @p buffer.size() bytes.
     *
     * @param[out] buffer    Destination.
     * @param[in]  timeout   Maximum wait for the first byte; 0 only takes what has arrived.
     *
     * @return The number of bytes read (at least one), timed_out if nothing arrived, connection_reset if the peer
     *         closed the connection, or the system's error.
     */
    [[nodiscard]] std::expected<std::size_t, std::error_code> read_some(std::span<std::byte> buffer, std::chrono::milliseconds timeout);

    /** @return Whether the socket holds a connection; one the peer closed is still open until a read reports it. */
    [[nodiscard]] bool is_open() const noexcept;

    /** Closes the connection; nothing happens if there is none. */
    void close() noexcept;

private:
    friend class TcpServer;

#ifdef _WIN32
    std::uintptr_t m_socket = ~std::uintptr_t{0};
#else
    int m_socket = -1;
#endif
};

} // namespace mne

#endif // COM_TCP_SOCKET_H
