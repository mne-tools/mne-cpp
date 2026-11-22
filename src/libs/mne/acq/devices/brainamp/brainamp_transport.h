//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     brainamp_transport.h
 * @author   Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    The BrainAmp driver interface as the backend uses it, with a Windows driver and a simulator behind it.
 *
 * The amplifier samples at 5 kHz. A read returns one block of points, each point the 16-bit values of the
 * set-up channels in steps of the resolution, followed by the 16-bit marker word of the trigger input.
 */

#ifndef BRAINAMP_TRANSPORT_H
#define BRAINAMP_TRANSPORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../acq_error.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
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

/** Sampling rate of every BrainAmp in Hz. */
inline constexpr double kBrainAmpRate = 5000.0;

/** Channels per amplifier; up to four amplifiers chain on one adapter. */
inline constexpr std::size_t kBrainAmpChannelsPerAmplifier = 32;

/** Amplifier models, as the driver reports them per adapter slot. */
enum class BrainAmpAmplifier : std::uint16_t
{
    None = 0,
    Standard = 1,
    Mr = 2,
    DcMrPlus = 3,
    ExG8 = 4,
    ExG16 = 5
};

/** Step of one bit of a channel, as the driver's resolution codes. */
enum class BrainAmpResolution : std::uint8_t
{
    Nv100 = 0, /**< 100 nV, +-3.28 mV. */
    Nv500 = 1, /**< 500 nV, +-16.4 mV. */
    Uv10 = 2,  /**< 10 uV, +-328 mV. */
    Uv152 = 3  /**< 152.6 uV, +-5 V. */
};

/**
 * @param[in] resolution   The amplifier's resolution setting.
 *
 * @return Volts per bit.
 */
constexpr double brainAmpVoltsPerBit(BrainAmpResolution resolution) noexcept
{
    switch (resolution) {
        case BrainAmpResolution::Nv100:
            return 100e-9;
        case BrainAmpResolution::Nv500:
            return 500e-9;
        case BrainAmpResolution::Uv10:
            return 10e-6;
        case BrainAmpResolution::Uv152:
            return 152.6e-6;
    }
    return 100e-9;
}

/** What the backend sends before starting. */
struct BrainAmpSetup
{
    std::vector<std::uint8_t> channels; /**< Physical channels, amplifier * 32 + input. */
    std::uint32_t points = 50;          /**< Points per read. */
    BrainAmpResolution resolution = BrainAmpResolution::Nv100;
    bool lowPass250Hz = false; /**< Hardware low pass at 250 Hz instead of 1 kHz. */
    bool dcCoupling = false;   /**< DC instead of AC coupling. */
};

/** The driver as seen by the backend. */
class BrainAmpTransport
{
public:
    virtual ~BrainAmpTransport() = default;

    /** @return The adapter and driver version, e.g. "USB adapter, driver 1.08.0001". */
    [[nodiscard]] virtual std::string description() const = 0;

    /** @return The model in each of the four adapter slots. */
    [[nodiscard]] virtual std::array<BrainAmpAmplifier, 4> amplifiers() = 0;

    /**
     * Sends the setup. Before start().
     *
     * @param[in] setup   Channels, block length and amplifier settings.
     *
     * @return An error if the driver refuses the setup.
     */
    virtual AcqError setup(const BrainAmpSetup& setup) = 0;

    virtual AcqError start() = 0;
    virtual void stop() = 0;

    /**
     * Reads one block without waiting.
     *
     * @param[out] block   setup.points * (channels + 1) values.
     *
     * @return true if a block was read, false if none is complete yet; EndOfStream when a simulated session
     *         ends, DeviceIo when the amplifier reports an error.
     */
    [[nodiscard]] virtual AcqExpected<bool> read(std::span<std::int16_t> block) = 0;
};

/** Settings of the simulated adapter. */
struct BrainAmpSimulation
{
    std::size_t amplifiers = 1;
    std::uint64_t seed = 0;
    bool realtime = true;
    double duration = 0.0; /**< Seconds; 0 runs until stopped. */
    double gain = 1.0;     /**< Amplitude factor of the simulated signal. */
};

/**
 * @param[in] simulation   Amplifiers, seed and pacing of the simulated adapter.
 *
 * @return A simulated adapter that digitises a synthetic:// session.
 */
std::unique_ptr<BrainAmpTransport> makeBrainAmpSimulator(const BrainAmpSimulation& simulation);

/** @return The Windows driver of a connected adapter, or BackendUnavailable / DeviceNotFound. */
AcqExpected<std::unique_ptr<BrainAmpTransport>> openBrainAmpDriver();

} // namespace mne::detail

#endif // BRAINAMP_TRANSPORT_H
