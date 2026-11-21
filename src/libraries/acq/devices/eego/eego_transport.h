//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eego_transport.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    The eego SDK as the backend uses it, with the SDK library and a simulator behind it.
 *
 * An ANT Neuro eego amplifier delivers doubles in volts: its referential and bipolar inputs, then the trigger
 * word and a sample counter. A read returns whole samples of all stream channels, interleaved.
 */

#ifndef EEGO_TRANSPORT_H
#define EEGO_TRANSPORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../acq_error.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
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

/** Channel types, as the SDK reports them. */
enum class EegoChannelType : int
{
    Reference = 0, /**< Referential EEG input. */
    Bipolar = 1,
    Accelerometer = 2,
    Gyroscope = 3,
    Magnetometer = 4,
    Trigger = 5,
    SampleCounter = 6,
    ImpedanceReference = 7,
    ImpedanceGround = 8
};

/** One channel of the amplifier or a stream. */
struct EegoChannel
{
    int index = 0; /**< The SDK's channel index. */
    EegoChannelType type = EegoChannelType::Reference;

    bool operator==(const EegoChannel&) const = default;
};

/** What the backend sends to start a stream. */
struct EegoSetup
{
    int rate = 512;             /**< Sampling rate in Hz, one of the amplifier's. */
    double reference_range = 1; /**< Input range of the referential inputs in volts, one of the amplifier's. */
    double bipolar_range = 4;   /**< Input range of the bipolar inputs in volts, one of the amplifier's. */
};

/** The SDK as seen by the backend: one amplifier. */
class EegoTransport
{
public:
    virtual ~EegoTransport() = default;

    /** @return The amplifier type and serial number, e.g. "EE225 000123". */
    [[nodiscard]] virtual std::string description() const = 0;

    /** @return The channels of a stream: the amplifier's inputs, then the trigger word and the sample counter. */
    [[nodiscard]] virtual std::vector<EegoChannel> channels() const = 0;

    /** @return The supported sampling rates in Hz. */
    [[nodiscard]] virtual std::vector<int> rates() const = 0;

    /** @return The supported ranges of the referential inputs in volts. */
    [[nodiscard]] virtual std::vector<double> reference_ranges() const = 0;

    /** @return The supported ranges of the bipolar inputs in volts. */
    [[nodiscard]] virtual std::vector<double> bipolar_ranges() const = 0;

    /**
     * Opens the EEG stream.
     *
     * @param[in] setup   Rate and input ranges.
     *
     * @return An error if the amplifier refuses the setup.
     */
    virtual AcqError start(const EegoSetup& setup) = 0;

    virtual void stop() = 0;

    /**
     * Reads the buffered samples.
     *
     * @return Whole samples of channels(), interleaved, valid until the next read() or stop(); empty if none is
     *         buffered yet. EndOfStream when a simulated session ends, DeviceIo when the amplifier fails.
     */
    [[nodiscard]] virtual AcqExpected<std::span<const double>> read() = 0;

    /**
     * Measures the impedances of the referential inputs. Not while started.
     *
     * @param[in] timeout   Maximum duration of the measurement.
     *
     * @return Ohms per referential input, in channel order.
     */
    [[nodiscard]] virtual AcqExpected<std::vector<double>> impedances(std::chrono::milliseconds timeout) = 0;
};

/** Settings of the simulated amplifier. */
struct EegoSimulation
{
    std::size_t reference = 64; /**< Referential inputs. */
    std::size_t bipolar = 24;   /**< Bipolar inputs. */
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
std::unique_ptr<EegoTransport> make_eego_simulator(const EegoSimulation& simulation);

/**
 * Loads the eego SDK and opens an amplifier.
 *
 * @param[in] library   Paths or names of the SDK library to try, in order.
 * @param[in] serial    Serial number of the amplifier; empty for the first connected one.
 *
 * @return The amplifier, or BackendUnavailable (no SDK) / DeviceNotFound (no amplifier).
 */
AcqExpected<std::unique_ptr<EegoTransport>> open_eego_driver(const std::vector<std::string>& library, const std::string& serial);

/**
 * @param[in] library   Paths or names of the SDK library to try, in order.
 *
 * @return The serial numbers of the connected amplifiers, or BackendUnavailable.
 */
AcqExpected<std::vector<std::string>> list_eego_amplifiers(const std::vector<std::string>& library);

} // namespace mne::detail

#endif // EEGO_TRANSPORT_H
