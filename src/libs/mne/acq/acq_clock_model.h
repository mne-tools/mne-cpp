//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_clock_model.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqClockModel: relation between the sample index of a stream and host time.
 *
 * The sample index is the time base of every stream, input or output. Host time is
 * derived from it by a linear model anchored at one sample, with an estimated rate correction, the
 * device pipeline delay (ADC group delay, DAC/RF output latency) and a stated uncertainty. Closed-loop
 * code maps an input sample to the output sample that is emitted at the same physical time with
 * mapSample().
 */

#ifndef ACQ_CLOCK_MODEL_H
#define ACQ_CLOCK_MODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdint>
#include <limits>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * What the anchor of the clock model is derived from.
 */
enum class AcqClockSource : std::uint8_t
{
    Nominal = 0, /**< Only the nominal rate is known; host times are extrapolated from start(). */
    Host = 1,    /**< Host receive times of blocks, smoothed. */
    Device = 2,  /**< Timestamps sent by the device (BrainFlow timestamp row, vendor counters). */
    Lsl = 3,     /**< LSL timestamps with time_correction applied. */
    External = 4 /**< External reference (10 MHz clock, PTP, sequence trigger). */
};

//=============================================================================================================
/**
 * Linear sample-index <-> host-time model. Trivially copyable; published by the producer through a
 * sequence counter (no lock) and copied into every view.
 *
 * physicalNs(s) = anchorHostNs + (s - anchorSample) * 1e9 / (sfreq * rateCorrection) - pipelineDelayNs
 * for input streams (the sample was taken earlier than it arrived) and + pipelineDelayNs for output
 * streams (the sample is emitted later than it is handed over). Times are std::chrono::steady_clock ns.
 */
struct AcqClockModel
{
    double sfreq = 0.0;                                             /**< Nominal sampling frequency in Hz. */
    double rateCorrection = 1.0;                                    /**< Estimated actual / nominal rate. */
    double uncertaintyNs = std::numeric_limits<double>::infinity(); /**< One-sigma uncertainty of physicalNs(). */
    std::uint64_t anchorSample = 0;                                 /**< Sample index of the anchor. */
    std::int64_t anchorHostNs = 0;                                  /**< Host time at which the anchor sample crossed the host boundary. */
    std::int64_t pipelineDelayNs = 0;                               /**< Fixed device delay between the physical event and the host boundary. */
    std::int64_t utcOffsetNs = 0;                                   /**< UTC minus steady_clock at the anchor; 0 if not known. */
    AcqClockSource source = AcqClockSource::Nominal;                /**< Origin of the anchor. */
    bool output = false;                                            /**< true for output streams (sign of pipelineDelayNs). */

    /**
     * @param[in] sample   Sample index. @pre sfreq > 0
     *
     * @return steady_clock time in ns at which the sample was taken (input) or emitted (output).
     */
    [[nodiscard]] std::int64_t physicalNs(std::uint64_t sample) const noexcept
    {
        const double ds = static_cast<double>(sample) - static_cast<double>(anchorSample);
        const auto t = anchorHostNs + static_cast<std::int64_t>(std::llround(ds * 1e9 / (sfreq * rateCorrection)));
        return output ? t + pipelineDelayNs : t - pipelineDelayNs;
    }

    /**
     * @param[in] ns   steady_clock time in ns. @pre sfreq > 0
     *
     * @return The sample taken or emitted closest to that time, clamped at 0.
     */
    [[nodiscard]] std::uint64_t sampleAt(std::int64_t ns) const noexcept
    {
        const std::int64_t boundaryNs = output ? ns - pipelineDelayNs : ns + pipelineDelayNs;
        const double s = static_cast<double>(anchorSample) +
            static_cast<double>(boundaryNs - anchorHostNs) * sfreq * rateCorrection / 1e9;
        return s <= 0.0 ? 0U : static_cast<std::uint64_t>(std::llround(s));
    }
};

//=============================================================================================================
/**
 * Maps a sample of one stream to the sample of another stream at the same physical time through host
 * time, so the result carries both clocks' uncertainty. Use it across clock domains (EEG -> TMS on a
 * different device); within one domain use the sample-exact mapSampleExact() of acq_timebase.h.
 *
 * @param[in] from     Clock model of the stream @p sample belongs to.
 * @param[in] to       Clock model of the target stream.
 * @param[in] sample   Sample index in @p from.
 *
 * @return Sample index in @p to.
 */
[[nodiscard]] inline std::uint64_t mapSample(const AcqClockModel& from, const AcqClockModel& to,
                                             std::uint64_t sample) noexcept
{
    return to.sampleAt(from.physicalNs(sample));
}

} // namespace mne

#endif // ACQ_CLOCK_MODEL_H
