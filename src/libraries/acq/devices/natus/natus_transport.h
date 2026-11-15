//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     natus_transport.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2018
 * @brief    The datagrams of the Natus streaming machine, from UDP or a simulator.
 *
 * The streaming machine (startDataTransmit, Natus SDK) sends a datagram per block: three floats, the second the
 * number of samples and the third the number of channels, then the samples as floats, sample after sample.
 */

#ifndef NATUS_TRANSPORT_H
#define NATUS_TRANSPORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../acq_error.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** Floats before the samples of a datagram. */
inline constexpr std::size_t kNatusHeaderFloats = 3;

/** Volts per unit of the streamed values. */
inline constexpr double kNatusScale = 1e-7;

/** The datagrams of the streaming machine. */
class NatusTransport
{
public:
    virtual ~NatusTransport() = default;

    /** @return The source, e.g. "UDP 0.0.0.0:50000". */
    [[nodiscard]] virtual std::string description() const = 0;

    /**
     * @param[out] buffer    Destination of the next datagram.
     * @param[in]  timeout   Maximum wait.
     *
     * @return Its size, Timeout, EndOfStream when a simulated session ends, or DeviceIo.
     */
    [[nodiscard]] virtual AcqExpected<std::size_t> receive(std::span<std::byte> buffer, std::chrono::milliseconds timeout) = 0;
};

/** Settings of the simulated streaming machine. */
struct NatusSimulation
{
    std::size_t channels = 46;
    double rate = 2048.0;   /**< Hz. */
    std::size_t block = 16; /**< Samples per datagram. */
    std::uint64_t seed = 0;
    bool realtime = true;
    double duration = 0.0; /**< Seconds; 0 runs until stopped. */
    double gain = 1.0;     /**< Amplitude factor of the simulated signal. */
};

/**
 * @param[in] simulation   Channels, rate and pacing.
 *
 * @return A streaming machine that sends a synthetic:// session's EEG.
 */
std::unique_ptr<NatusTransport> make_natus_simulator(const NatusSimulation& simulation);

/**
 * @param[in] address   Local IPv4 address to receive on.
 * @param[in] port      Local port.
 *
 * @return The receiver, or the error of binding the port.
 */
AcqExpected<std::unique_ptr<NatusTransport>> open_natus_receiver(const std::string& address, std::uint16_t port);

} // namespace mne::detail

#endif // NATUS_TRANSPORT_H
