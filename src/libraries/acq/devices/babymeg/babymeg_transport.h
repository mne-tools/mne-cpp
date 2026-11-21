//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     babymeg_transport.h
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2013
 * @brief    The BabyMEG acquisition server: its channel description and data packages, over TCP or from a simulator.
 *
 * Every message is a 4-character command, a big-endian 32-bit length and that many bytes. On its command port the
 * server answers "INFO" with the channel description, a text "INFO:<channels>:<samples>:<rate>:" followed by one
 * "<name>|<scale>,<r0>,<ex>,<ey>,<ez>,<coil type>,<cal>,<gain>;" per channel. On its data port it answers "DATA"
 * with one "DATR" package, a format digit (the bytes per value) and big-endian floats, sample after sample, and
 * "QUIT" with "QUIT", which the client acknowledges with "QREL". A physical value is raw * cal / gain.
 */

#ifndef BABYMEG_TRANSPORT_H
#define BABYMEG_TRANSPORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../acq_error.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** One channel of the server's description. */
struct BabyMegChannel
{
    std::string name;
    std::array<float, 12> loc{}; /**< r0, ex, ey, ez in the device frame. */
    int coil_type = 0;           /**< A FIFF coil type, or FIFFV_STIM_CH, FIFFV_EEG_CH, FIFFV_QUAT_x, FIFFV_HPI_G. */
    float cal = 1.0F;
    float gain = 1.0F;
};

/** The server's description of its channels. */
struct BabyMegInfo
{
    double rate = 0.0;
    std::size_t samples = 0; /**< Samples per data package. */
    std::vector<BabyMegChannel> channels;
};

/**
 * @param[in] text   The body of the server's "INFO" message.
 *
 * @return The description, or DeviceIo naming what does not parse.
 */
AcqExpected<BabyMegInfo> parse_babymeg_info(std::string_view text);

/**
 * @param[in] info   A description.
 *
 * @return The body of an "INFO" message with @p info, as the server sends it.
 */
std::string babymeg_info_text(const BabyMegInfo& info);

/** The server as the backend sees it. */
class BabyMegTransport
{
public:
    virtual ~BabyMegTransport() = default;

    /** @return The server, e.g. "localhost:6340". */
    [[nodiscard]] virtual std::string description() const = 0;

    /**
     * @param[in] timeout   Maximum wait for the answer.
     *
     * @return The body of the server's "INFO" answer, or the error of asking for it.
     */
    [[nodiscard]] virtual AcqExpected<std::string> info(std::chrono::milliseconds timeout) = 0;

    /**
     * Asks for data packages; next() returns them.
     *
     * @return An error if the server cannot be reached.
     */
    virtual AcqError start() = 0;

    /**
     * @param[in] timeout   Maximum wait.
     *
     * @return The body of the next "DATR" package, valid until the next call, then asks for the following one;
     *         Timeout, EndOfStream when the server quits or a simulated session ends, or DeviceIo.
     */
    [[nodiscard]] virtual AcqExpected<std::span<const std::byte>> next(std::chrono::milliseconds timeout) = 0;

    /** Says goodbye to the server ("QUIT", "QREL") and disconnects. */
    virtual void stop() = 0;
};

/** Settings of the simulated server. */
struct BabyMegSimulation
{
    std::size_t meg = 270;     /**< Inner layer magnetometers. */
    std::size_t ref = 114;     /**< Outer layer and reference magnetometers. */
    double rate = 1000.0;      /**< Hz. */
    std::size_t samples = 100; /**< Samples per data package. */
    std::uint64_t seed = 0;
    bool realtime = true;
    double duration = 0.0; /**< Seconds; 0 runs until stopped. */
};

/**
 * @param[in] simulation   Channels, rate and pacing.
 *
 * @return A server that sends a synthetic:// session's MEG and its trigger lines TRG001 to TRG008.
 */
std::unique_ptr<BabyMegTransport> make_babymeg_simulator(const BabyMegSimulation& simulation);

/**
 * @param[in] host           Host of the server.
 * @param[in] data_port      Its data port.
 * @param[in] command_port   Its command port.
 * @param[in] timeout        Maximum wait for the connection.
 *
 * @return A client of the server, or DeviceNotFound.
 */
AcqExpected<std::unique_ptr<BabyMegTransport>> connect_babymeg(const std::string& host, std::uint16_t data_port, std::uint16_t command_port,
                                                               std::chrono::milliseconds timeout);

} // namespace mne::detail

#endif // BABYMEG_TRANSPORT_H
