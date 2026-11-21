//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     neuromag_transport.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    The Neuromag (MEGIN) acquisition server as the backend uses it, with a DACQ client and a simulator.
 *
 * The data acquisition of a Neuromag Vectorview or TRIUX system hands every client the FIFF tags of the running
 * measurement: at its start the measurement info (FIFFB_MEAS_INFO), then FIFF_DATA_BUFFER tags of 32-bit
 * integers, one per channel and sample, whose physical value is raw * cal * range.
 */

#ifndef NEUROMAG_TRANSPORT_H
#define NEUROMAG_TRANSPORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag_fiff.h"
#include "../../acq_error.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** The acquisition server tags as seen by the backend. */
class NeuromagTransport
{
public:
    virtual ~NeuromagTransport() = default;

    /** @return The server, e.g. "DACQ on 127.0.0.1". */
    [[nodiscard]] virtual std::string description() const = 0;

    /**
     * Connects as a data client and, if so set up, starts a measurement.
     *
     * @return An error if the server cannot be reached or refuses.
     */
    virtual AcqError start() = 0;

    /** Stops the measurement it started, if any, and disconnects. */
    virtual void stop() = 0;

    /**
     * @param[in] timeout   Maximum wait.
     *
     * @return The next tag, valid until the next call, or Timeout, EndOfStream when a simulated session ends,
     *         DeviceIo when the server fails.
     */
    [[nodiscard]] virtual AcqExpected<const NeuromagTag*> next(std::chrono::milliseconds timeout) = 0;
};

/** Where the acquisition server is and how to use it. */
struct NeuromagDacq
{
    std::string root = "/neuro/dacq"; /**< Directory with the server's sockets/, shmem/ and raw/. */
    std::string host = "127.0.0.1";   /**< Host of the collector, which controls the measurement. */
    int port = 11122;                 /**< Port of the collector. */
    int client = 13014;               /**< Client id at the data server, above 10000 for shared memory. */
    int buffer = 0;                   /**< Samples per data buffer to set; 0 keeps the server's setting. */
    bool autostart = true;            /**< Start and stop measurements through the collector. */
};

/** Settings of the simulated system. */
struct NeuromagSimulation
{
    std::size_t meg = 306; /**< Magnetometer channels. */
    std::size_t eeg = 0;
    double rate = 1000.0;     /**< Sampling rate in Hz. */
    std::size_t buffer = 100; /**< Samples per data buffer. */
    std::uint64_t seed = 0;
    bool realtime = true;
    double duration = 0.0; /**< Seconds; 0 runs until stopped. */
    double gain = 1.0;     /**< Amplitude factor of the simulated signal. */
};

/**
 * @param[in] simulation   Channels, rate and pacing of the simulated system.
 *
 * @return A simulated acquisition server that digitises a synthetic:// session.
 */
std::unique_ptr<NeuromagTransport> make_neuromag_simulator(const NeuromagSimulation& simulation);

/**
 * @param[in] dacq   Where the server is and how to use it.
 *
 * @return A client of the acquisition server on this machine, or PlatformUnsupported where it cannot run.
 */
AcqExpected<std::unique_ptr<NeuromagTransport>> make_neuromag_client(const NeuromagDacq& dacq);

} // namespace mne::detail

#endif // NEUROMAG_TRANSPORT_H
