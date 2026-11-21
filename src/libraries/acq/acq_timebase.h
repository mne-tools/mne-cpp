//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_timebase.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqTimebase: sample-exact relation between streams that share a hardware clock (clock domain).
 *
 * AcqClockModel relates a stream to host time with microsecond-class uncertainty; enough for markers
 * across devices, not for MR (RF transmit, T/R switch, gradients, receive) or closed-loop stimulation,
 * which must line up to the sample. Streams whose sample clocks derive from one oscillator form a clock
 * domain: every stream has an integer number of domain ticks per sample and an origin tick, so mapping
 * between them is integer arithmetic without host time and without drift.
 *
 * Domains follow the hardware, as on Spectrum cards, where every timing quantity is an integer number
 * of clock cycles in a fixed step (sample rates from a PLL or clock divider of one reference; trigger
 * delay, gate length, pre/post trigger and pulse generator periods in steps of N samples):
 * - The domain tick is the common clock all sample clocks are integer divisions of (e.g. the 10 MHz
 *   reference multiplied by the PLL; for an 80 MS/s ADC and a 1.25 GS/s DAC, 10 GHz: 125 and 8 ticks).
 * - The origin tick is the domain tick at which sample 0 crosses the connector, so known converter and
 *   trigger latencies are part of it.
 * - sampleAlignment is the step in which a stream can place things (Spectrum SPC_GATE_LEN_ALIGNMENT,
 *   SPC_TRIG_AVAILDELAY_STEP, ...). Exact mapping only succeeds on that grid; an off-grid instant is a
 *   configuration error reported as NotOnGrid, not hidden by interpolation.
 *
 * Domain membership (who shares a clock):
 * - One device: all its streams are one domain, named by the backend "scheme:serial"; FrequencyLocked
 *   from opening, PhaseLocked once the backend knows the per-stream latencies.
 * - Devices synchronised by hardware the backend can query (Spectrum star-hub): the hub's domain,
 *   PhaseLocked by the common start.
 * - Devices wired by hand (clock output of one card into the external clock input of another, a shared
 *   10 MHz reference, an amplifier clocked from the stimulator): the user declares it with the URI
 *   parameter clock_from=ID (the device id of the clock master), or clock_domain=NAME for an external reference
 *   that is not itself a device. The declaration only yields Declared; verify() promotes it to PhaseLocked
 *   after loop-back sync markers have shown zero drift, so a wiring mistake can never produce a wrong
 *   sample.
 * - Everything else: domain 0 (Free); only AcqClockModel relations apply. No configuration for EEG users.
 *
 * Sample-exact playout of an RF pulse 2 ms after an event detected in a receive stream:
 * @code
 * const mne::AcqTimebase& rxTb = view.timebase();                // from the input view
 * const mne::AcqTimebase txTb = tx.timebase();                    // output stream of the same domain
 * const auto twoMs = static_cast<std::int64_t>(rxTb.ticksPerSecond / 500);
 * auto at = mne::mapSampleExact(rxTb, txTb, eventSample, twoMs, mne::AcqRounding::Exact);
 * if (at && *at >= writer.earliestWritableSample()) {
 *     auto out = writer.beginWriteAt(*at, pulseSamples, std::chrono::microseconds(200));
 *     // encode the pulse and the T/R switch bit into *out, then writer.commit(pulseSamples)
 * }
 * @endcode
 */

#ifndef ACQ_TIMEBASE_H
#define ACQ_TIMEBASE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"
#include "acq_error.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <span>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * How far a stream's timebase is known relative to its domain.
 */
enum class AcqLockState : std::uint8_t
{
    Free = 0,            /**< No common clock, or the reference was lost; no exact mapping. */
    Declared = 1,        /**< Common clock asserted by the user (manual wiring), not yet verified. */
    FrequencyLocked = 2, /**< Common clock known to the backend; origin offset not yet known. */
    PhaseLocked = 3      /**< Common clock and known origin; mapping is sample-exact. */
};

//=============================================================================================================
/**
 * Position of a stream's samples on its domain's tick axis. Trivially copyable; published by the
 * backend or by verify() through AcqStream::setTimebase() and copied into every view.
 *
 * @snippet ex_acq_performance/main.cpp acq_timebase_setup
 * @snippet ex_acq_performance/main.cpp acq_writer_schedule
 */
struct AcqTimebase
{
    std::uint64_t domain = 0;                 /**< acqClockDomainId(name); 0 = free-running. */
    std::uint64_t ticksPerSecond = 0;         /**< Domain tick rate in Hz (integer, as hardware clocks are). */
    std::uint64_t ticksPerSample = 1;         /**< Integer by construction of the domain tick. */
    std::uint32_t sampleAlignment = 1;        /**< Step in samples on which this stream can schedule. */
    std::int64_t originTick = 0;              /**< Domain tick at which sample 0 crosses the connector. */
    std::uint64_t originUncertaintyTicks = 0; /**< 0 for a hardware start; marker residual otherwise. */
    AcqLockState lock = AcqLockState::Free;   /**< See AcqLockState. */

    /**
     * @param[in] sample   Sample index of this stream.
     *
     * @return Domain tick of that sample.
     */
    [[nodiscard]] constexpr std::int64_t tickOf(std::uint64_t sample) const noexcept
    {
        return originTick + static_cast<std::int64_t>(sample * ticksPerSample);
    }
};

//=============================================================================================================
/**
 * @param[in] name   Domain name, e.g. "spectrum:12345" or "scanner-10MHz".
 *
 * @return The stable domain id (FNV-1a 64, never 0).
 */
[[nodiscard]] constexpr std::uint64_t acqClockDomainId(std::string_view name) noexcept
{
    std::uint64_t h = 14695981039346656037ULL;
    for (const char c : name) {
        h = (h ^ static_cast<std::uint8_t>(c)) * 1099511628211ULL;
    }
    return h == 0 ? 1 : h;
}

//=============================================================================================================
/**
 * What mapSampleExact() does with an instant that is not on the target's schedulable grid.
 */
enum class AcqRounding : std::uint8_t
{
    Exact = 0, /**< Return NotOnGrid. Default for scheduling. */
    Floor = 1, /**< Last grid point at or before the instant. */
    Ceil = 2   /**< First grid point at or after the instant. */
};

//=============================================================================================================
/**
 * Maps @p sample of stream @p from, shifted by @p offsetTicks, to the stream @p to of the same domain.
 * Integer arithmetic only, allocation-free; usable on the data path.
 *
 * @param[in] from          Timebase of the source stream.
 * @param[in] to            Timebase of the target stream.
 * @param[in] sample        Sample index in @p from.
 * @param[in] offsetTicks   Delay added to the instant of @p sample, in domain ticks (may be negative).
 * @param[in] rounding      Handling of instants off the grid of to.ticksPerSample * to.sampleAlignment.
 *
 * @return The sample index in @p to; NotSynchronized unless both are PhaseLocked in the same domain;
 *         BeforeStart before sample 0 of @p to; NotOnGrid for an off-grid instant with Exact.
 */
[[nodiscard]] inline AcqExpected<std::uint64_t> mapSampleExact(const AcqTimebase& from, const AcqTimebase& to,
                                                               std::uint64_t sample, std::int64_t offsetTicks = 0,
                                                               AcqRounding rounding = AcqRounding::Exact) noexcept
{
    if (from.domain == 0 || from.domain != to.domain || from.lock != AcqLockState::PhaseLocked ||
        to.lock != AcqLockState::PhaseLocked || to.ticksPerSample == 0 || to.sampleAlignment == 0) {
        return AcqErrorCode::NotSynchronized;
    }
    const std::int64_t delta = from.tickOf(sample) + offsetTicks - to.originTick;
    if (delta < 0) {
        return AcqErrorCode::BeforeStart;
    }
    const std::uint64_t grid = to.ticksPerSample * to.sampleAlignment;
    const auto ticks = static_cast<std::uint64_t>(delta);
    std::uint64_t steps = ticks / grid;
    if (ticks % grid != 0) {
        if (rounding == AcqRounding::Exact) {
            return AcqErrorCode::NotOnGrid;
        }
        steps += rounding == AcqRounding::Ceil ? 1U : 0U;
    }
    return steps * to.sampleAlignment;
}

//=============================================================================================================
/**
 * Half-open range of sample indices [first, end).
 */
struct AcqSampleRange
{
    std::uint64_t first = 0; /**< First sample. */
    std::uint64_t end = 0;   /**< One past the last sample; equals first for an empty range. */
};

//=============================================================================================================
/**
 * The samples of @p to whose instants lie in the time span of samples [@p first, @p end) of @p from, e.g.
 * the ECoG or fNIRS samples of an EEG epoch. Integer arithmetic only; adjacent windows map to adjacent
 * ranges, so a recording cut into windows loses and repeats no sample. Every sample of @p to counts,
 * whatever its sampleAlignment; samples before sample 0 of @p to do not exist and are left out.
 *
 * @param[in] from    Timebase of the stream the window is given in.
 * @param[in] to      Timebase of the stream to read.
 * @param[in] first   First sample of the window in @p from.
 * @param[in] end     One past the last sample of the window in @p from.
 *
 * @return The range in @p to, NotSynchronized unless both are PhaseLocked in the same domain,
 *         InvalidParameter if @p end < @p first.
 */
[[nodiscard]] inline AcqExpected<AcqSampleRange> mapRangeExact(const AcqTimebase& from, const AcqTimebase& to,
                                                               std::uint64_t first, std::uint64_t end) noexcept
{
    if (from.domain == 0 || from.domain != to.domain || from.lock != AcqLockState::PhaseLocked ||
        to.lock != AcqLockState::PhaseLocked || to.ticksPerSample == 0) {
        return AcqErrorCode::NotSynchronized;
    }
    if (end < first) {
        return AcqErrorCode::InvalidParameter;
    }
    // First sample of to at or after the instant of sample s of from.
    const auto atOrAfter = [&](std::uint64_t s) -> std::uint64_t {
        const std::int64_t delta = from.tickOf(s) - to.originTick;
        if (delta <= 0) {
            return 0;
        }
        const auto ticks = static_cast<std::uint64_t>(delta);
        return ticks / to.ticksPerSample + (ticks % to.ticksPerSample != 0 ? 1U : 0U);
    };
    return AcqSampleRange{atOrAfter(first), atOrAfter(end)};
}

//=============================================================================================================
/**
 * One loop-back observation: the target stream emitted (output) or recorded (input) a sync edge at
 * @c targetSample, and the reference stream recorded the same edge at @c referenceSample.
 */
struct AcqSyncMarker
{
    std::uint64_t targetSample = 0;    /**< Sample of the edge in the target stream. */
    std::uint64_t referenceSample = 0; /**< Sample of the edge in the reference stream. */
    std::int64_t subTicks = 0;         /**< Edge position inside the reference sample, in domain ticks. */
};

//=============================================================================================================
/**
 * Determines the origin of @p target from loop-back markers against @p reference and verifies the
 * declared clock relation: every marker must imply the same origin within @p toleranceTicks, so drift
 * (a missing or wrong clock connection) is detected instead of producing wrong samples.
 *
 * @param[in] target           Timebase to verify; same domain as @p reference, Declared or FrequencyLocked.
 * @param[in] markers          Observations; at least 1 for FrequencyLocked, at least 2 spanning
 *                             minSpanSamples for Declared.
 * @param[in] reference        PhaseLocked timebase of the domain.
 * @param[in] toleranceTicks   Allowed spread of the implied origins (edge detection jitter).
 * @param[in] minSpanSamples   Minimum distance in target samples between first and last marker for Declared.
 *
 * @return The PhaseLocked target timebase (originUncertaintyTicks = observed spread), or NotSynchronized
 *         when the domains differ, the reference is not PhaseLocked, too few markers were given, or the
 *         implied origins drift by more than @p toleranceTicks.
 */
[[nodiscard]] ACQSHARED_EXPORT AcqExpected<AcqTimebase> verify(const AcqTimebase& target,
                                                               std::span<const AcqSyncMarker> markers,
                                                               const AcqTimebase& reference,
                                                               std::uint64_t toleranceTicks,
                                                               std::uint64_t minSpanSamples = 1'000'000) noexcept;

} // namespace mne

#endif // ACQ_TIMEBASE_H
