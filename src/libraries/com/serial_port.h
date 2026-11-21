//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     serial_port.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November, 2026
 * @brief    A raw 8N1 serial port without flow control, for devices that talk over RS-232 or USB-serial.
 */

#ifndef COM_SERIAL_PORT_H
#define COM_SERIAL_PORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "com_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/**
 * An open serial port; move-only, closed on destruction after its output has gone out.
 *
 * @snippet ex_com/main.cpp serial_usage
 */
class COMSHARED_EXPORT SerialPort
{
public:
    SerialPort() = default;
    ~SerialPort();

    SerialPort(SerialPort&& other) noexcept;
    SerialPort& operator=(SerialPort&& other) noexcept;
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    /**
     * @param[in] name   Device path ("/dev/cu.usbserial-1", "/dev/ttyUSB0") or port name ("COM3").
     * @param[in] baud   Bits per second.
     *
     * @return The port in raw mode, invalid_argument for an unsupported baud rate, inappropriate_io_control_operation for a file that is
     *         no serial port, or the system's error: no_such_file_or_directory if it does not exist,
     *         permission_denied, device_or_resource_busy (another program has it).
     */
    [[nodiscard]] static std::expected<SerialPort, std::error_code> open(const std::string& name, int baud);

    /** @return Whether the port is open. */
    [[nodiscard]] bool is_open() const noexcept;

    /**
     * Writes all of @p data.
     *
     * @param[in] data   Bytes to send.
     *
     * @return The system's error if the port fails.
     */
    std::error_code write(std::string_view data);

    /**
     * Waits for bytes and reads what is there, at most @p buffer.size().
     *
     * @param[out] buffer    Destination.
     * @param[in]  timeout   Maximum wait.
     *
     * @return The bytes read, timed_out if none arrived, no_such_device if the port went away, or the system's
     *         error.
     */
    [[nodiscard]] std::expected<std::size_t, std::error_code> read(std::span<char> buffer, std::chrono::milliseconds timeout);

private:
#ifdef _WIN32
    void* m_handle = nullptr;
#else
    int m_fd = -1;
#endif
};

/** @return The serial ports of this machine, USB-serial adapters first. */
COMSHARED_EXPORT std::vector<std::string> list_serial_ports();

} // namespace mne

#endif // COM_SERIAL_PORT_H
