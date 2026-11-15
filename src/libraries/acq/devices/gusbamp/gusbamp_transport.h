//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     gusbamp_transport.h
 * @author   Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     November, 2015
 * @brief    The g.USBamp API as the backend uses it, with a Windows driver and a simulator behind it.
 *
 * Up to four g.USBamps run synchronised, the first one as master. Every unit delivers its transfers
 * separately: a 38-byte header, then the scans, each the float values in microvolts of the set-up inputs and,
 * if enabled, the trigger line.
 */

#ifndef GUSBAMP_TRANSPORT_H
#define GUSBAMP_TRANSPORT_H

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

/** Bytes before the samples of every transfer. */
inline constexpr std::size_t kGusbampHeaderBytes = 38;

/** Inputs of one unit. */
inline constexpr std::size_t kGusbampInputs = 16;

/** Units that run synchronised. */
inline constexpr std::size_t kGusbampMaxUnits = 4;

/** What the backend sends before starting. */
struct GusbampSetup
{
    std::vector<std::uint8_t> inputs; /**< Inputs 1..16 of every unit, ascending. */
    std::uint16_t rate = 1200;        /**< Sampling rate in Hz, one the amplifier supports. */
    std::uint16_t scans = 128;        /**< Scans per transfer. */
    bool trigger = false;             /**< The master's trigger line follows its inputs. */

    /**
     * @param[in] unit   Index of the unit, 0 for the master.
     *
     * @return Values per scan of the unit.
     */
    [[nodiscard]] std::size_t width(std::size_t unit) const noexcept
    {
        return inputs.size() + (trigger && unit == 0 ? 1 : 0);
    }

    /**
     * @param[in] unit   Index of the unit, 0 for the master.
     *
     * @return Bytes of one transfer of the unit.
     */
    [[nodiscard]] std::size_t transfer_bytes(std::size_t unit) const noexcept
    {
        return kGusbampHeaderBytes + std::size_t{scans} * width(unit) * sizeof(float);
    }
};

/** The g.USBamp API as seen by the backend. */
class GusbampTransport
{
public:
    virtual ~GusbampTransport() = default;

    /** @return The serial numbers of the opened units, master first. */
    [[nodiscard]] virtual std::vector<std::string> serials() const = 0;

    /** @return The units, e.g. "UB-2015.05.16 (master), UB-2015.05.17". */
    [[nodiscard]] virtual std::string description() const = 0;

    /**
     * Sets up the units and starts the slaves, then the master.
     *
     * @param[in] setup   Inputs, rate and transfer size.
     *
     * @return An error if a unit refuses the setup or does not start.
     */
    virtual AcqError start(const GusbampSetup& setup) = 0;

    virtual void stop() = 0;

    /**
     * Waits for the next transfer of every unit.
     *
     * @param[in] timeout   Maximum wait.
     *
     * @return One transfer per unit, master first, each setup.transfer_bytes(unit) long; valid until the next
     *         read() or stop(). Timeout if a transfer is not complete in time, EndOfStream when a simulated
     *         session ends, DeviceIo when a unit fails or a transfer comes up short.
     */
    [[nodiscard]] virtual AcqExpected<std::span<const std::span<const std::byte>>> read(std::chrono::milliseconds timeout) = 0;

    /**
     * Measures the electrode impedances. Not while started.
     *
     * @param[in] inputs   Inputs 1..16 to measure on every unit.
     *
     * @return Ohms per unit and input, unit after unit.
     */
    [[nodiscard]] virtual AcqExpected<std::vector<double>> impedances(std::span<const std::uint8_t> inputs) = 0;
};

/** Settings of the simulated units. */
struct GusbampSimulation
{
    std::uint64_t seed = 0;
    bool realtime = true;
    double duration = 0.0; /**< Seconds; 0 runs until stopped. */
    double gain = 1.0;     /**< Amplitude factor of the simulated signal. */
};

/**
 * @param[in] serials      Serial numbers of the simulated units, master first.
 * @param[in] simulation   Seed and pacing.
 *
 * @return Simulated units that digitise a synthetic:// session.
 */
std::unique_ptr<GusbampTransport> make_gusbamp_simulator(std::vector<std::string> serials, const GusbampSimulation& simulation);

/**
 * @param[in] serials   Serial numbers of the units, master first; empty for the first connected unit.
 *
 * @return The opened units, or BackendUnavailable / DeviceNotFound.
 */
AcqExpected<std::unique_ptr<GusbampTransport>> open_gusbamp_driver(std::span<const std::string> serials);

/** @return The serial numbers of the connected units, or BackendUnavailable. */
AcqExpected<std::vector<std::string>> list_gusbamp_units();

} // namespace mne::detail

#endif // GUSBAMP_TRANSPORT_H
