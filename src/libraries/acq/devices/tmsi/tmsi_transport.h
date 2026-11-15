//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     tmsi_transport.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     September, 2013
 * @brief    The TMSi SDK interface as the backend uses it, with a Windows driver and a simulator behind it.
 *
 * A TMSi Refa or Porti delivers every channel as a 32-bit integer; the physical value is
 * (raw * gain + offset) * 10^exponent in the channel's unit. A read returns whole samples of all device
 * channels, interleaved, as many as the SDK has buffered.
 */

#ifndef TMSI_TRANSPORT_H
#define TMSI_TRANSPORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../acq_error.h"

#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** Channel types, as the SDK's signal format reports them. */
enum class TmsiChannelType : std::uint32_t
{
    Unknown = 0,
    Exg = 1, /**< Unipolar EEG input. */
    Bip = 2, /**< Bipolar input. */
    Aux = 3, /**< Auxiliary input. */
    Dig = 4, /**< Digital inputs, one bit each. */
    Time = 5,
    Leak = 6,
    Pressure = 7,
    Envelope = 8,
    Marker = 9,
    Saw = 10, /**< Test ramp of the amplifier. */
    SaO2 = 11
};

/** Units, as the SDK's signal format reports them. */
enum class TmsiUnit : std::uint32_t
{
    Unknown = 0,
    Volt = 1,
    Percent = 2,
    Bpm = 3,
    Bar = 4,
    Psi = 5,
    MH2O = 6,
    MHg = 7,
    Bit = 8
};

/** One device channel. */
struct TmsiChannel
{
    std::string name; /**< The SDK's channel name. */
    TmsiChannelType type = TmsiChannelType::Unknown;
    TmsiUnit unit = TmsiUnit::Unknown;
    double gain = 1.0;   /**< Physical = (raw * gain + offset) * 10^exponent. */
    double offset = 0.0; /**< See gain. */
    int exponent = 0;    /**< See gain. */
};

/** Raw value of an analog channel whose input is out of range. */
inline constexpr std::int32_t kTmsiOverflow = std::numeric_limits<std::int32_t>::min();

/** What the backend sends before starting. */
struct TmsiSetup
{
    double rate = 2048.0;       /**< Requested sampling rate in Hz. */
    bool commonAverage = false; /**< Reference the ExG channels to their average in the amplifier. */
};

/** The SDK as seen by the backend. */
class TmsiTransport
{
public:
    virtual ~TmsiTransport() = default;

    /** @return The device and its serial number, e.g. "Refa 1234567". */
    [[nodiscard]] virtual std::string description() const = 0;

    /** @return The device channels, in the order of a sample. */
    [[nodiscard]] virtual std::vector<TmsiChannel> channels() const = 0;

    /**
     * Sends the setup. Before start().
     *
     * @param[in] setup   Sampling rate and reference.
     *
     * @return The rate the amplifier runs at (the supported rate nearest the requested one), or the error.
     */
    virtual AcqExpected<double> setup(const TmsiSetup& setup) = 0;

    virtual AcqError start() = 0;
    virtual void stop() = 0;

    /**
     * Reads the buffered samples without waiting.
     *
     * @param[out] frames   Room for whole samples of all channels.
     *
     * @return Samples read, 0 if none are buffered yet; EndOfStream when a simulated session ends, DeviceIo
     *         when the amplifier reports an error.
     */
    [[nodiscard]] virtual AcqExpected<std::size_t> read(std::span<std::int32_t> frames) = 0;

    /**
     * Measures the electrode impedances. Not while started.
     *
     * @param[in] timeout   Maximum duration of the measurement.
     *
     * @return Ohms per ExG channel, in channel order.
     */
    [[nodiscard]] virtual AcqExpected<std::vector<double>> impedances(std::chrono::milliseconds timeout) = 0;
};

/** Settings of the simulated amplifier. */
struct TmsiSimulation
{
    std::size_t exg = 64; /**< ExG inputs; four bipolar and four auxiliary inputs, the digital word and the test ramp follow. */
    std::uint64_t seed = 0;
    bool realtime = true;
    double duration = 0.0; /**< Seconds; 0 runs until stopped. */
    double gain = 1.0;     /**< Amplitude factor of the simulated signal. */
};

/**
 * @param[in] simulation   Inputs, seed and pacing of the simulated amplifier.
 *
 * @return A simulated amplifier that digitises a synthetic:// session.
 */
std::unique_ptr<TmsiTransport> makeTmsiSimulator(const TmsiSimulation& simulation);

/** @return The SDK connection to the first connected amplifier, or BackendUnavailable / DeviceNotFound. */
AcqExpected<std::unique_ptr<TmsiTransport>> openTmsiDriver();

} // namespace mne::detail

#endif // TMSI_TRANSPORT_H
