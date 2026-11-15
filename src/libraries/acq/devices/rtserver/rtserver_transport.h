//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     rtserver_transport.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>
 * @since    0.1.0
 * @date     December, 2012
 * @brief    The mne_rt_server protocol: its command port and its data port, over TCP or from a simulator.
 *
 * The command port takes text commands, e.g. "measinfo 3", each framed as a 16-bit block size and a string of a
 * 32-bit byte length and UTF-16 characters, all big-endian; the replies have the same form. The data port sends
 * FIFF tags, big-endian as in FIFF files: the measurement info (FIFFB_MEAS_INFO) on "measinfo", then FIFFB_RAW_DATA
 * with FIFF_DATA_BUFFER tags of calibrated floats, sample after sample, from "start" to "stop".
 */

#ifndef RTSERVER_TRANSPORT_H
#define RTSERVER_TRANSPORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../neuromag/neuromag_fiff.h"

#include "../../acq_error.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** Tags and commands of the data port. */
namespace rt
{
inline constexpr int kCommand = 3700;           /**< FIFF_MNE_RT_COMMAND: an int command, then its data. */
inline constexpr int kClientId = 3701;          /**< FIFF_MNE_RT_CLIENT_ID: the answer to kGetClientId. */
inline constexpr std::int32_t kGetClientId = 1; /**< Asks for the id the commands name this client by. */
inline constexpr std::int32_t kSetAlias = 2;    /**< Names the client, for "clist". */
} // namespace rt

/** An mne_rt_server as the backend sees it. */
class RtServerTransport
{
public:
    virtual ~RtServerTransport() = default;

    /** @return The server, e.g. "localhost:4217". */
    [[nodiscard]] virtual std::string description() const = 0;

    /**
     * Sends a command on the command port and waits for its reply.
     *
     * @param[in] line      The command and its parameters, e.g. "measinfo 3".
     * @param[in] timeout   Maximum wait for the reply.
     *
     * @return The reply, Timeout, or DeviceIo if the connection failed.
     */
    [[nodiscard]] virtual AcqExpected<std::string> command(std::string_view line, std::chrono::milliseconds timeout) = 0;

    /**
     * Sends a command tag on the data port.
     *
     * @param[in] command   rt::kGetClientId or rt::kSetAlias.
     * @param[in] text      Data after the command, e.g. the alias.
     *
     * @return DeviceIo if the connection failed.
     */
    virtual AcqError send(std::int32_t command, std::string_view text) = 0;

    /**
     * @param[in] timeout   Maximum wait.
     *
     * @return The next tag of the data port in native byte order, valid until the next call, or Timeout,
     *         EndOfStream when a simulated session ends, DeviceIo when the server fails.
     */
    [[nodiscard]] virtual AcqExpected<const NeuromagTag*> next(std::chrono::milliseconds timeout) = 0;
};

/** Settings of the simulated server. */
struct RtServerSimulation
{
    std::size_t meg = 306; /**< Magnetometers of the replayed recording. */
    std::size_t eeg = 0;   /**< EEG channels of the replayed recording. */
    double rate = 1000.0;  /**< Hz. */
    std::uint64_t seed = 0;
    bool realtime = true;
    double duration = 0.0; /**< Seconds; 0 runs until stopped. */
};

/**
 * @param[in] simulation   The replayed recording and pacing.
 *
 * @return An mne_rt_server whose connector replays a simulated Neuromag recording, with calibrated floats.
 */
std::unique_ptr<RtServerTransport> make_rtserver_simulator(const RtServerSimulation& simulation);

/**
 * @param[in] host      Host of the server.
 * @param[in] port      Its command port; the data port is the next one.
 * @param[in] timeout   Maximum wait for the connections.
 *
 * @return Connections to both ports, or DeviceNotFound.
 */
AcqExpected<std::unique_ptr<RtServerTransport>> connect_rtserver(const std::string& host, std::uint16_t port, std::chrono::milliseconds timeout);

} // namespace mne::detail

#endif // RTSERVER_TRANSPORT_H
