//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_block_view.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Zero-copy views into a stream ring: AcqBlockView (read) and AcqWriteView (write).
 *
 * A view addresses committed (read) or reserved (write) samples in place: one contiguous part, or two
 * when the block wraps around the end of a ring that is not mirror-mapped. Views own nothing, do not
 * allocate and are valid until released (AcqReader::release) or committed (AcqWriter::commit).
 *
 * Element (channel c, sample s) of a part is at data + c * channelStride + s * sampleStride, so
 * interleaved and planar rings, and any channel subset, are handled by the same code.
 */

#ifndef ACQ_BLOCK_VIEW_H
#define ACQ_BLOCK_VIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"
#include "acq_clock_model.h"
#include "acq_memory.h"
#include "acq_sample_format.h"
#include "acq_timebase.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

namespace detail
{
struct ViewAccess;
}

//=============================================================================================================
/**
 * An event at a sample index: input trigger or marker, or a scheduled output trigger (TMS pulse,
 * stimulus onset, RF gate). Fixed size, stored in a ring next to the samples; text labels are
 * resolved through AcqStreamInfo::eventLabels.
 *
 * @snippet ex_acq_performance/main.cpp acq_reader_block
 */
struct AcqEvent
{
    std::uint64_t sample = 0;   /**< Sample index of the onset. */
    std::uint32_t duration = 0; /**< Duration in samples, 0 for an instantaneous event. */
    std::int32_t code = 0;      /**< Event code (trigger value, marker id, output line mask). */
};

//=============================================================================================================
/**
 * Generic header of one segment of a segmented stream (MR readout, RF pulse, triggered epoch). The
 * device-specific payload of AcqSegmentation::headerBytes travels next to it.
 */
struct AcqSegmentHeader
{
    std::uint64_t firstSample = 0; /**< Sample index of the first sample of the segment. */
    std::uint32_t sampleCount = 0; /**< Samples per channel in the segment. */
    std::uint32_t flags = 0;       /**< Device-defined flags (e.g. last readout of a slice). */
    std::int64_t deviceTime = 0;   /**< Device time stamp of the trigger, device units. */
    std::uint64_t counter = 0;     /**< Device segment counter; gaps reveal dropped segments. */
};

//=============================================================================================================
/**
 * Up to two contiguous spans forming one logical sequence (the second is empty without wrap-around).
 *
 * @tparam T   Element type.
 *
 * @snippet ex_acq_performance/main.cpp acq_reader_block
 */
template<typename T>
struct AcqSpanPair
{
    std::span<T> first;  /**< Elements up to the end of the ring. */
    std::span<T> second; /**< Elements from the start of the ring; empty without wrap. */

    /** @return Total number of elements. */
    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return first.size() + second.size();
    }

    /** @return true when there are no elements. */
    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

    /**
     * @param[in] i   Index into the logical sequence. @pre i < size()
     *
     * @return The element.
     */
    constexpr T& operator[](std::size_t i) const noexcept
    {
        return i < first.size() ? first[i] : second[i - first.size()];
    }
};

//=============================================================================================================
/**
 * One contiguous run of samples in a ring.
 *
 * @tparam Byte   std::byte for writable runs, const std::byte for read-only runs.
 *
 * @snippet ex_acq_performance/main.cpp acq_reader_block
 */
template<typename Byte>
struct AcqPart
{
    Byte* data = nullptr;             /**< Channel 0, sample 0 of the run. */
    std::size_t samples = 0;          /**< Samples per channel in the run. */
    std::ptrdiff_t sampleStride = 0;  /**< Bytes between consecutive samples of one channel. */
    std::ptrdiff_t channelStride = 0; /**< Bytes between consecutive channels of one sample. */

    /**
     * @param[in] c   Channel.
     * @param[in] s   Sample within the run.
     *
     * @return Pointer to the element.
     */
    [[nodiscard]] Byte* at(std::size_t c, std::size_t s) const noexcept
    {
        return data + static_cast<std::ptrdiff_t>(c) * channelStride + static_cast<std::ptrdiff_t>(s) * sampleStride;
    }
};

using AcqBlockPart = AcqPart<const std::byte>; /**< Read-only run. */
using AcqWritePart = AcqPart<std::byte>;       /**< Writable run. */

//=============================================================================================================
/**
 * Fields shared by read and write views.
 *
 * @snippet ex_acq_performance/main.cpp acq_reader_block
 */
template<typename Byte>
class AcqViewBase
{
public:
    /** @return Sample index of the first sample in the view. */
    [[nodiscard]] std::uint64_t firstSample() const noexcept
    {
        return m_firstSample;
    }

    /** @return Samples per channel (sum over the parts). */
    [[nodiscard]] std::size_t sampleCount() const noexcept
    {
        return m_sampleCount;
    }

    /** @return Number of channels. */
    [[nodiscard]] std::size_t channelCount() const noexcept
    {
        return m_channelCount;
    }

    /** @return Native sample format. */
    [[nodiscard]] AcqSampleFormat format() const noexcept
    {
        return m_format;
    }

    /** @return AcqStreamInfo::version that describes the samples. */
    [[nodiscard]] std::uint32_t infoVersion() const noexcept
    {
        return m_infoVersion;
    }

    /** @return 0 (no samples), 1, or 2 when the view wraps around the end of the ring. */
    [[nodiscard]] std::size_t partCount() const noexcept
    {
        return m_partCount;
    }

    /**
     * @param[in] i   Part index. @pre i < partCount()
     *
     * @return The part.
     */
    [[nodiscard]] const AcqPart<Byte>& part(std::size_t i) const noexcept
    {
        assert(i < m_partCount);
        return m_parts[i];
    }

    /** @return Per-channel scale of infoVersion(); valid as long as the view. */
    [[nodiscard]] std::span<const double> scales() const noexcept
    {
        return m_scales;
    }

    /** @return Per-channel offset of infoVersion(); valid as long as the view. */
    [[nodiscard]] std::span<const double> offsets() const noexcept
    {
        return m_offsets;
    }

    /** @return Per-channel physical lower limit (NaN if unknown); the encoder saturates to it. */
    [[nodiscard]] std::span<const double> rangeMin() const noexcept
    {
        return m_rangeMin;
    }

    /** @return Per-channel physical upper limit (NaN if unknown). */
    [[nodiscard]] std::span<const double> rangeMax() const noexcept
    {
        return m_rangeMax;
    }

    /** @return Per-channel AcqChannelInfo::digitalBits of infoVersion(). */
    [[nodiscard]] std::span<const std::uint8_t> digitalBits() const noexcept
    {
        return m_digitalBits;
    }

    /** @return Clock model valid for this view. */
    [[nodiscard]] const AcqClockModel& clock() const noexcept
    {
        return m_clock;
    }

    /** @return Hardware timebase valid for this view (sample-exact mapping within a clock domain). */
    [[nodiscard]] const AcqTimebase& timebase() const noexcept
    {
        return m_timebase;
    }

    /** @return Where the samples live; parts of Device memory must not be dereferenced on the CPU. */
    [[nodiscard]] AcqMemoryLocation memory() const noexcept
    {
        return m_memory;
    }

protected:
    friend struct detail::ViewAccess;

    std::array<AcqPart<Byte>, 2> m_parts{};
    std::size_t m_partCount = 0;
    std::uint64_t m_firstSample = 0;
    std::size_t m_sampleCount = 0;
    std::size_t m_channelCount = 0;
    AcqSampleFormat m_format = AcqSampleFormat::Float32;
    std::uint32_t m_infoVersion = 0;
    std::span<const double> m_scales;
    std::span<const double> m_offsets;
    std::span<const double> m_rangeMin;
    std::span<const double> m_rangeMax;
    std::span<const std::uint8_t> m_digitalBits;
    AcqClockModel m_clock;
    AcqTimebase m_timebase;
    AcqMemoryLocation m_memory;
    std::uint64_t m_ticket = 0; /**< Owner-private validation token. */
};

//=============================================================================================================
/**
 * Read view returned by AcqReader::acquire().
 *
 * @snippet ex_acq_performance/main.cpp acq_reader_block
 */
class AcqBlockView : public AcqViewBase<const std::byte>
{
public:
    /** @return true when the view holds no samples, events or segments. */
    [[nodiscard]] bool empty() const noexcept
    {
        return m_sampleCount == 0 && m_events.empty() && m_segments.empty();
    }

    /** @return Samples lost to overrun between the previous view of this reader and this one. */
    [[nodiscard]] std::uint64_t lost() const noexcept
    {
        return m_lost;
    }

    /** @return Samples skipped on purpose by a Latest reader before this view (not a loss). */
    [[nodiscard]] std::uint64_t skipped() const noexcept
    {
        return m_skipped;
    }

    /** @return Events with sample index in [firstSample(), firstSample() + sampleCount()). */
    [[nodiscard]] const AcqSpanPair<const AcqEvent>& events() const noexcept
    {
        return m_events;
    }

    /** @return Headers of the segments that start inside this view. */
    [[nodiscard]] const AcqSpanPair<const AcqSegmentHeader>& segments() const noexcept
    {
        return m_segments;
    }

    /**
     * @param[in] i   Segment index. @pre i < segments().size()
     *
     * @return The device-specific header payload of the segment.
     */
    [[nodiscard]] std::span<const std::byte> segmentPayload(std::size_t i) const noexcept
    {
        const std::size_t offset = i * m_segmentPayloadBytes;
        return offset < m_segmentPayloads.first.size()
            ? m_segmentPayloads.first.subspan(offset, m_segmentPayloadBytes)
            : m_segmentPayloads.second.subspan(offset - m_segmentPayloads.first.size(), m_segmentPayloadBytes);
    }

private:
    friend struct detail::ViewAccess;

    std::uint64_t m_lost = 0;
    std::uint64_t m_skipped = 0;
    AcqSpanPair<const AcqEvent> m_events;
    AcqSpanPair<const AcqSegmentHeader> m_segments;
    AcqSpanPair<const std::byte> m_segmentPayloads;
    std::size_t m_segmentPayloadBytes = 0;
};

//=============================================================================================================
/**
 * Write view returned by AcqWriter::beginWrite(): reserved ring space for the next samples.
 *
 * @snippet ex_acq_performance/main.cpp acq_writer_commit
 */
class AcqWriteView : public AcqViewBase<std::byte>
{
public:
    /**
     * @return Output streams: samples the device emitted as AcqUnderrunFill since the previous
     *         beginWrite() because they were not committed in time. firstSample() already accounts for
     *         them. Always 0 for input streams.
     */
    [[nodiscard]] std::uint64_t underrun() const noexcept
    {
        return m_underrun;
    }

private:
    friend struct detail::ViewAccess;

    std::uint64_t m_underrun = 0;
};

} // namespace mne

#endif // ACQ_BLOCK_VIEW_H
