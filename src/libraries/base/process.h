//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     process.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Runs external programs and describes the host, the standard C++ counterpart of QProcess, QSysInfo and
 *           the hardware addresses of Qt's network interface list.
 *
 * run_process() starts a program without a shell, so arguments reach it verbatim, reads its standard output and
 * error while it runs and waits for it to finish or time out. Standard input is empty.
 */

#ifndef BASE_PROCESS_H
#define BASE_PROCESS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "base_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/** The output streams of a child process. */
enum class ProcessChannel
{
    StdOut, /**< Standard output. */
    StdErr  /**< Standard error. */
};

//=============================================================================================================
/**
 * @brief How run_process() starts a program and what it does with its output.
 *
 * @snippet ex_base/main.cpp process_usage
 */
struct BASESHARED_EXPORT ProcessOptions
{
    std::filesystem::path working_dir;                               /**< Working directory; empty keeps the caller's. */
    std::vector<std::pair<std::string, std::string>> environment;    /**< Variables added to or replacing the caller's. */
    std::optional<std::chrono::milliseconds> timeout;                /**< Kill the program after this long; none waits. */
    std::function<void(ProcessChannel, std::string_view)> on_output; /**< Receives output as it arrives instead of the result. */
};

//=============================================================================================================
/**
 * @brief What happened when run_process() ran a program.
 *
 * @snippet ex_base/main.cpp process_usage
 */
struct BASESHARED_EXPORT ProcessResult
{
    bool started = false;   /**< The program was found and started. */
    bool timed_out = false; /**< The program was killed after ProcessOptions::timeout. */
    bool crashed = false;   /**< The program was ended by a signal. */
    int exit_code = -1;     /**< The exit code, if the program exited normally. */
    std::string std_out;    /**< Standard output, unless ProcessOptions::on_output took it. */
    std::string std_err;    /**< Standard error, unless ProcessOptions::on_output took it. */
    std::string error;      /**< Why the program could not be started. */

    /** @return Whether the program ran to completion and exited with code 0. */
    [[nodiscard]] bool ok() const
    {
        return started && !timed_out && !crashed && exit_code == 0;
    }
};

//=============================================================================================================
/**
 * Runs a program and waits for it.
 *
 * @param[in] program   The program; a bare name is looked up in @c PATH.
 * @param[in] args      The arguments, passed verbatim (no shell).
 * @param[in] options   Working directory, environment, timeout and output callback.
 *
 * @return The exit status and the collected output.
 */
BASESHARED_EXPORT ProcessResult run_process(const std::filesystem::path& program, std::span<const std::string> args, const ProcessOptions& options = {});

//=============================================================================================================
/**
 * @return The name of this machine, like QSysInfo::machineHostName().
 */
BASESHARED_EXPORT std::string host_name();

//=============================================================================================================
/**
 * @return The operating system with its version, e.g. "macOS 15.3" or "Ubuntu 24.04.1 LTS".
 */
BASESHARED_EXPORT std::string os_pretty_name();

//=============================================================================================================
/**
 * @return The CPU architecture this code was built for, with the names QSysInfo::currentCpuArchitecture() uses.
 */
BASESHARED_EXPORT std::string_view cpu_architecture();

/** A 48-bit hardware (MAC) address, most significant byte first. */
using HardwareAddress = std::array<std::uint8_t, 6>;

//=============================================================================================================
/**
 * Lists the hardware addresses of the network interfaces that are up and running and are neither loopback nor
 * point-to-point, in interface order, as the operating system lists them.
 * Interfaces without a 48-bit address are left out.
 *
 * @return The addresses; empty where the platform has no network interfaces (WebAssembly).
 */
BASESHARED_EXPORT std::vector<HardwareAddress> hardware_addresses();

} // namespace mne

#endif // BASE_PROCESS_H
