//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     polhemus_transport.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May, 2026
 * @brief    The byte stream of a Polhemus Fastrak / FastSCAN, from a serial port or a simulator.
 *
 * The backend sends the digitizer's ASCII commands ("C\r" continuous output, "P\r" stop, "l1,1\r",
 * "O1,2,4,1\r", "H1,0,0,1\r") and reads its records, one line per station and update.
 */

#ifndef POLHEMUS_TRANSPORT_H
#define POLHEMUS_TRANSPORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fastrak_record.h"

#include "../../acq_error.h"

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

/** A digitizer's command and record stream. */
class PolhemusTransport
{
public:
    virtual ~PolhemusTransport() = default;

    /** @return The digitizer, e.g. "Fastrak on /dev/cu.usbserial-1". */
    [[nodiscard]] virtual std::string description() const = 0;

    /**
     * @param[in] command   One or more commands, each ending in a carriage return.
     *
     * @return DeviceIo if the digitizer went away.
     */
    virtual AcqError write(std::string_view command) = 0;

    /**
     * @param[out] buffer    Destination of the record bytes.
     * @param[in]  timeout   Maximum wait.
     *
     * @return The bytes read, Timeout if none arrived, EndOfStream when a simulated session ends, or DeviceIo.
     */
    [[nodiscard]] virtual AcqExpected<std::size_t> read(std::span<char> buffer, std::chrono::milliseconds timeout) = 0;
};

/** Settings of the simulated digitizer. */
struct PolhemusSimulation
{
    std::vector<int> stations{1, 2};           /**< Stations 1..4 with a sensor; 1 is the stylus, 2 the tracker. */
    FastrakUnits units = FastrakUnits::Inches; /**< Unit of the simulated records. */
    double rate = 120.0;                       /**< Records per second, shared by the stations. */
    std::uint64_t seed = 0;                    /**< Phase of the simulated sweep. */
    bool realtime = true;                      /**< Deliver records at the digitizer's pace. */
    double duration = 0.0;                     /**< Seconds; 0 runs until stopped. */
    double button = 0.0;                       /**< Seconds between stylus button presses; 0 never. */
};

/**
 * @param[in] simulation   Stations, units and pacing.
 *
 * @return A digitizer that sweeps the stylus over a 10 cm sphere around a tracker and, like a Fastrak, stops
 *         its continuous output while the stylus button is pressed.
 */
std::unique_ptr<PolhemusTransport> make_polhemus_simulator(const PolhemusSimulation& simulation);

} // namespace mne::detail

#endif // POLHEMUS_TRANSPORT_H
