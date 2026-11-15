//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     ftbuffer_transport.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2020
 * @brief    The FieldTrip buffer protocol: requests and their answers, over TCP or from a simulator.
 *
 * A request is a message header (version 1, command, payload size) and the payload; the answer has the same
 * form. Numbers are in the byte order of the server's machine, little-endian on every host this runs on.
 */

#ifndef FTBUFFER_TRANSPORT_H
#define FTBUFFER_TRANSPORT_H

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
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** Commands and constants of the FieldTrip buffer protocol. */
namespace ft
{
inline constexpr std::uint16_t kVersion = 1;
inline constexpr std::uint16_t kGetHdr = 0x201;
inline constexpr std::uint16_t kGetDat = 0x202;
inline constexpr std::uint16_t kGetEvt = 0x203;
inline constexpr std::uint16_t kGetOk = 0x204;
inline constexpr std::uint16_t kGetErr = 0x205;
inline constexpr std::uint16_t kWaitDat = 0x402;
inline constexpr std::uint16_t kWaitOk = 0x404;
inline constexpr std::uint16_t kWaitErr = 0x405;

inline constexpr std::uint32_t kChunkChannelNames = 1;
inline constexpr std::uint32_t kChunkNeuromagHeader = 8;
inline constexpr std::uint32_t kChunkNeuromagIsotrak = 9;

inline constexpr std::uint32_t kTypeChar = 0;
inline constexpr std::uint32_t kTypeUint8 = 1;
inline constexpr std::uint32_t kTypeUint16 = 2;
inline constexpr std::uint32_t kTypeUint32 = 3;
inline constexpr std::uint32_t kTypeUint64 = 4;
inline constexpr std::uint32_t kTypeInt8 = 5;
inline constexpr std::uint32_t kTypeInt16 = 6;
inline constexpr std::uint32_t kTypeInt32 = 7;
inline constexpr std::uint32_t kTypeInt64 = 8;
inline constexpr std::uint32_t kTypeFloat32 = 9;
inline constexpr std::uint32_t kTypeFloat64 = 10;

inline constexpr std::size_t kHeaderDefBytes = 24; /**< nchans, nsamples, nevents, fsample, data_type, bufsize. */
inline constexpr std::size_t kDataDefBytes = 16;   /**< nchans, nsamples, data_type, bufsize. */
inline constexpr std::size_t kEventDefBytes = 32;  /**< 4 types and counts, sample, offset, duration, bufsize. */
} // namespace ft

/** The answer to a request. */
struct FtMessage
{
    std::uint16_t command = 0;
    std::vector<std::byte> payload;
};

/** A FieldTrip buffer server as the backend sees it. */
class FtBufferTransport
{
public:
    virtual ~FtBufferTransport() = default;

    /** @return The server, e.g. "localhost:1972". */
    [[nodiscard]] virtual std::string description() const = 0;

    /**
     * Sends a request and waits for its answer.
     *
     * @param[in] command   Request command, e.g. ft::kGetHdr.
     * @param[in] payload   Request payload.
     * @param[in] timeout   Maximum wait for the answer, beyond any wait the request itself asks for.
     *
     * @return The answer, Timeout, or DeviceIo if the connection failed.
     */
    [[nodiscard]] virtual AcqExpected<FtMessage> request(std::uint16_t command, std::span<const std::byte> payload,
                                                         std::chrono::milliseconds timeout) = 0;
};

/** Settings of the simulated buffer. */
struct FtBufferSimulation
{
    std::size_t meg = 306;  /**< Magnetometers of the relayed Neuromag system. */
    std::size_t eeg = 0;    /**< EEG channels of the relayed Neuromag system. */
    double rate = 1000.0;   /**< Hz. */
    std::size_t block = 50; /**< Samples per data put. */
    std::uint64_t seed = 0;
    bool realtime = true;
    double duration = 0.0; /**< Seconds; 0 runs until stopped. */
};

/**
 * @param[in] simulation   The relayed system and pacing.
 *
 * @return A buffer that neuromag2ft fills from a simulated Neuromag acquisition: channel names and measurement
 *         info as header chunks, then float samples in physical units.
 */
std::unique_ptr<FtBufferTransport> make_ftbuffer_simulator(const FtBufferSimulation& simulation);

/**
 * @param[in] host      Host of the buffer server.
 * @param[in] port      Its port.
 * @param[in] timeout   Maximum wait for the connection.
 *
 * @return A connection to the server, or DeviceNotFound.
 */
AcqExpected<std::unique_ptr<FtBufferTransport>> connect_ftbuffer(const std::string& host, std::uint16_t port, std::chrono::milliseconds timeout);

} // namespace mne::detail

#endif // FTBUFFER_TRANSPORT_H
