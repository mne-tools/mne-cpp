//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_stream.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqStream (shared handle), AcqWriter (the single producer) and the ring options of a stream.
 *
 * One stream = one rate, one native format, one lock-free single-producer / multi-reader ring
 *. The ring is a power of two in samples, page-aligned, optionally mirror-mapped
 * (the ring's pages mapped twice back to back, so every view is one contiguous part), memory-locked
 * and pre-faulted, or an adopted external buffer such as a device DMA region.
 *
 * Input stream: the backend owns the writer; commit() is one release store and never waits; samples
 * are overwritten whether or not readers consumed them, and slow readers learn the exact loss.
 * Output stream: the application takes the writer with AcqStream::writer(); beginWrite() waits for
 * space freed by the device's gating reader, so unplayed samples are never overwritten. A block whose
 * scheduled start the device has already played as fill is dropped whole and counted in
 * AcqStreamStats::lateCommits; output is never played truncated or shifted in time.
 *
 * Closed-loop output, e.g. a TMS trigger 20 ms after a detected input event and a stimulus waveform:
 * @code
 * auto stim = source->findStream("stim");                          // output stream
 * auto writer = stim.writer();                                      // exclusive
 * const auto at = mne::mapSample(eeg.clock(), stim.clock(), peakSample) + 20 * stimRate / 1000;
 * writer->pushEvent({.sample = at, .duration = 0, .code = 1});      // device fires at exactly 'at'
 * auto view = writer->beginWrite(pulse.cols, std::chrono::milliseconds(1));
 * mne::encode(pulse, *view);                                     // physical -> DAC counts, saturating
 * writer->commit(view->sampleCount());
 * @endcode
 */

#ifndef ACQ_STREAM_H
#define ACQ_STREAM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"
#include "acq_block_view.h"
#include "acq_clock_model.h"
#include "acq_error.h"
#include "acq_memory.h"
#include "acq_reader.h"
#include "acq_stream_info.h"
#include "acq_timebase.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

namespace detail
{
class AcqStreamCore;
}

class AcqWriter;

//=============================================================================================================
/**
 * An externally owned buffer adopted as the ring (zero-copy DMA), e.g. a card's DMA buffer or GPU memory
 * written by GPUDirect RDMA. Its capacity in samples must be a power of two and its start aligned to
 * AcqStreamOptions::alignment; @c release is called once the ring no longer uses it.
 */
struct AcqExternalBuffer
{
    std::byte* data = nullptr;        /**< Start of the buffer. */
    std::size_t bytes = 0;            /**< Size in bytes. */
    std::ptrdiff_t channelStride = 0; /**< Planar buffers: bytes between channel rows; 0 = capacity * bytesPerSample. */
    AcqMemoryLocation memory;         /**< Where the buffer lives; Device memory is never dereferenced by the library. */
    std::function<void()> release;    /**< Called once after the ring no longer uses the buffer; may be empty. */
};

//=============================================================================================================
/**
 * Ring and real-time options of a stream; fixed at creation (control path).
 */
struct AcqStreamOptions
{
    std::size_t capacitySamples = 0;  /**< Rounded up to a power of two; 0 = max(65536, 2 s of data), halved while above 256 MiB. */
    std::size_t maxReaders = 8;       /**< Reader slots, allocated at creation. */
    std::size_t eventCapacity = 4096; /**< Event ring entries (power of two). */
    std::size_t segmentCapacity = 0;  /**< Segment header entries; 0 = derived from capacity for segmented streams. */
    std::size_t rowAlignment = 64;    /**< Planar rings: byte alignment of every channel row (SIMD). */
    std::size_t alignment = 4096;     /**< Ring start alignment, power of two >= 64 (e.g. 65536 for GPU pages). */
    bool mirror = true;               /**< Map an owned interleaved ring twice back to back so every view is one part;
                                           needs a size in whole pages (64 KiB on Windows), not with huge pages; in stats. */
    bool lockMemory = true;           /**< mlock / VirtualLock; best effort, reported in stats. */
    bool prefault = true;             /**< Touch every page at creation, so start() never page-faults. */
    bool hugePages = false;           /**< 2 MiB pages (Linux transparent huge pages); best effort, reported in stats. */
    int numaNode = -1;                /**< NUMA node of the ring, e.g. the card's; -1 = local; best effort, in stats. */
    AcqExternalBuffer external;       /**< Adopt this buffer instead of allocating one. */
};

//=============================================================================================================
/**
 * Producer-side counters of a stream.
 *
 * @snippet ex_acq_performance/main.cpp acq_stream_stats
 */
struct AcqStreamStats
{
    std::uint64_t committed = 0;     /**< Samples committed since creation (= next write position). */
    std::uint64_t commits = 0;       /**< commit() calls. */
    std::uint64_t events = 0;        /**< Events pushed. */
    std::uint64_t segments = 0;      /**< Segment headers pushed. */
    std::uint64_t underrun = 0;      /**< Output: samples emitted as underrun fill. */
    std::uint64_t underruns = 0;     /**< Output: underrun episodes. */
    std::uint64_t writerWaits = 0;   /**< Output: beginWrite() calls that had to wait for space. */
    std::uint64_t lateEvents = 0;    /**< Output: events or segments rejected as Late. */
    std::uint64_t lateCommits = 0;   /**< Output: committed blocks the device had already passed; not played. */
    std::size_t capacitySamples = 0; /**< Effective ring capacity. */
    std::size_t activeReaders = 0;   /**< Readers attached right now. */
    bool memoryLocked = false;       /**< Whether lockMemory succeeded. */
    bool mirrored = false;           /**< Whether the ring is mirror-mapped. */
    bool adoptedBuffer = false;      /**< Whether the ring is an adopted external buffer. */
    bool hugePages = false;          /**< Whether the ring is backed by huge pages. */
    int numaNode = -1;               /**< NUMA node the ring is bound to, -1 if not bound. */
};

ACQ_STD_MEMBERS_BEGIN
//=============================================================================================================
/**
 * Shared handle of a stream. Cheap to copy; keeps the ring alive, so readers stay valid after the
 * device is closed (they then return EndOfStream).
 *
 * @snippet ex_acq_performance/main.cpp acq_stream_setup
 */
class ACQSHARED_EXPORT AcqStream
{
public:
    AcqStream() noexcept;

    /** @return true when attached to a ring. */
    [[nodiscard]] bool isValid() const noexcept;

    /** @return Input or output. */
    [[nodiscard]] AcqDirection direction() const noexcept;

    /** @return The current metadata version (control path). */
    [[nodiscard]] std::shared_ptr<const AcqStreamInfo> info() const;

    /**
     * @param[in] sample   Sample index.
     *
     * @return The metadata version valid at that sample, or the oldest retained one.
     */
    [[nodiscard]] std::shared_ptr<const AcqStreamInfo> infoAt(std::uint64_t sample) const;

    /** @return The current clock model. */
    [[nodiscard]] AcqClockModel clock() const noexcept;

    /** @return Where the samples live; views of Device memory must not be dereferenced on the CPU. */
    [[nodiscard]] AcqMemoryLocation memory() const noexcept;

    /** @return The current hardware timebase (domain, ticks per sample, origin, lock state). */
    [[nodiscard]] AcqTimebase timebase() const noexcept;

    /**
     * Publishes a new timebase, e.g. the result of verify() or a reference loss reported by the backend.
     * Control path; serialised internally, readers see it through a sequence counter without a lock.
     *
     * @param[in] timebase   The new timebase.
     */
    void setTimebase(const AcqTimebase& timebase) noexcept;

    /** @return Producer-side counters. */
    [[nodiscard]] AcqStreamStats stats() const noexcept;

    //=========================================================================================================
    /**
     * Attaches a reader (control path; before or after start()).
     *
     * @param[in] mode      Consumption mode.
     * @param[in] options   Wake-up strategy, start position, gating.
     *
     * @return The reader, or TooManyReaders, InvalidState (a gating reader already exists).
     */
    [[nodiscard]] AcqExpected<AcqReader> reader(AcqReader::ReadMode mode, const AcqReaderOptions& options = {}) const;

    //=========================================================================================================
    /**
     * Takes the writer of an output stream (control path).
     *
     * @param[in] wait   How beginWrite() waits for space.
     *
     * @return The writer, or WrongDirection (input stream), WriterInUse.
     */
    [[nodiscard]] AcqExpected<AcqWriter> writer(AcqWait wait = AcqWait::Block) const;

private:
    friend class AcqBackend;
    friend class AcqReader;
    friend class AcqWriter;
    explicit AcqStream(std::shared_ptr<detail::AcqStreamCore> core) noexcept;

    std::shared_ptr<detail::AcqStreamCore> m_core;
};
ACQ_STD_MEMBERS_END

ACQ_STD_MEMBERS_BEGIN
//=============================================================================================================
/**
 * The single producer of a ring: the backend for an input stream, the application for an output
 * stream. Move-only. All members except create() and updateInfo() are allocation-free and lock-free;
 * on input streams without a gating reader they are also wait-free.
 *
 * Producing an input block, scheduling an output pulse at an exact sample, and driving a card that writes
 * into the ring by DMA:
 * @snippet ex_acq_performance/main.cpp acq_writer_commit
 * @snippet ex_acq_performance/main.cpp acq_writer_schedule
 * @snippet ex_acq_performance/main.cpp acq_dma_grant
 */
class ACQSHARED_EXPORT AcqWriter
{
public:
    //=========================================================================================================
    /**
     * Creates a standalone ring with its writer (backends, DMA emulation, application-internal pipelines).
     *
     * @param[in] info      Stream description (version 0), including the direction.
     * @param[in] options   Ring and real-time options.
     *
     * @return The writer, or InvalidParameter, InvalidBuffer, OutOfMemory, PlatformUnsupported.
     */
    [[nodiscard]] static AcqExpected<AcqWriter> create(AcqStreamInfo info, const AcqStreamOptions& options = {});

    AcqWriter() noexcept;
    ~AcqWriter(); /**< Equals finish(). */
    AcqWriter(AcqWriter&& other) noexcept;
    AcqWriter& operator=(AcqWriter&& other) noexcept;
    AcqWriter(const AcqWriter&) = delete;
    AcqWriter& operator=(const AcqWriter&) = delete;

    /** @return The shared handle of this ring. */
    [[nodiscard]] AcqStream stream() const noexcept;

    /** @return Next sample index to be written. */
    [[nodiscard]] std::uint64_t writePosition() const noexcept;

    /** @return Output streams: the sample index the device consumes next; input streams: writePosition(). */
    [[nodiscard]] std::uint64_t devicePosition() const noexcept;

    /** @return Samples that can be reserved without waiting (input streams without a gating reader: the capacity). */
    [[nodiscard]] std::size_t space() const noexcept;

    /**
     * @return Output streams: the first sample that can still be written in time, i.e. devicePosition()
     *         plus the samples the device has already taken into its FIFO/DMA queue. A schedule earlier
     *         than this returns Late. Input streams: writePosition().
     */
    [[nodiscard]] std::uint64_t earliestWritableSample() const noexcept;

    //=========================================================================================================
    /**
     * Reserves the next @p samples samples, waiting up to @p timeout for the gating reader to free space
     * (input streams without one never wait). Output streams: if the device has already passed
     * writePosition(), the view starts at the device position and reports the gap in
     * AcqWriteView::underrun().
     *
     * @param[in] samples   Samples per channel. @pre samples <= capacity
     * @param[in] timeout   Maximum wait for space.
     *
     * @return The writable view, or Timeout, InvalidParameter (more than the capacity), EndOfStream (the
     *         gating reader of an output stream was destroyed).
     */
    [[nodiscard]] AcqExpected<AcqWriteView> beginWrite(std::size_t samples,
                                                       std::chrono::nanoseconds timeout = {}) noexcept;

    //=========================================================================================================
    /**
     * Waits until beginWrite(@p samples) would not wait. A producer of several streams calls this for each
     * before reserving any, so a gated stream never holds back the others' reservations.
     *
     * @param[in] samples   Samples per channel. @pre samples <= capacity
     * @param[in] timeout   Maximum wait.
     *
     * @return Success, Timeout, InvalidParameter, or EndOfStream as beginWrite().
     */
    AcqError waitForSpace(std::size_t samples, std::chrono::nanoseconds timeout) noexcept;

    //=========================================================================================================
    /**
     * Output streams: reserves @p samples samples starting exactly at @p firstSample, for sample-precise
     * playout (an RF pulse at the sample mapped from a receive event). Samples between writePosition()
     * and @p firstSample are filled with the stream's AcqUnderrunFill on commit; they are scheduled
     * silence, not underrun.
     *
     * @param[in] firstSample   Absolute sample index. @pre firstSample >= writePosition()
     * @param[in] samples       Samples per channel. @pre firstSample + samples - writePosition() <= capacity
     * @param[in] timeout       Maximum wait for space.
     *
     * @return The view at @p firstSample, or Late (already played; takes precedence), Timeout,
     *         InvalidParameter (before writePosition()), NotOnGrid (not a multiple of
     *         timebase().sampleAlignment), WrongDirection (input stream).
     */
    [[nodiscard]] AcqExpected<AcqWriteView> beginWriteAt(std::uint64_t firstSample, std::size_t samples,
                                                         std::chrono::nanoseconds timeout = {}) noexcept;

    /**
     * Input streams written by a device (DMA): allows the device to write the next @p samples samples
     * after those already granted, directly into the ring. From now on the device may overwrite
     * anything older than the granted end minus the capacity, so readers count those samples as lost;
     * commit() publishes granted samples as the device reports them written. Keep several blocks granted
     * so the device never stalls; do not also beginWrite() the granted samples.
     *
     * @param[in] samples   Samples per channel.
     *
     * @return InvalidParameter if more than the capacity would be granted beyond writePosition(), Timeout
     *         if a gating reader has not yet consumed the space, WrongDirection for output streams (there
     *         the device is the gating reader).
     */
    AcqError grant(std::size_t samples) noexcept;

    /**
     * Publishes the first samples of the last view (one release store); without a view, the next granted
     * samples.
     *
     * @param[in] samples   Samples per channel. @pre samples <= reserved or granted
     */
    void commit(std::size_t samples) noexcept;

    /**
     * Attaches @p event (absolute sample index, not decreasing), published with the next commit (input) or
     * immediately (output; scheduled events may lie ahead of writePosition()). Old events are overwritten
     * like samples; lagging readers count them in AcqReaderStats::eventsLost.
     *
     * @param[in] event   The event.
     *
     * @return Late for an output event before devicePosition(), InvalidParameter for a sample index below the
     *         previous event's; otherwise success.
     */
    AcqError pushEvent(const AcqEvent& event) noexcept;

    /**
     * Starts a segment at header.firstSample with its device-specific @p payload (truncated or zero-padded
     * to AcqSegmentation::headerBytes); published like events.
     *
     * @param[in] header    Generic segment header.
     * @param[in] payload   Device-specific header payload.
     *
     * @return Late for an output segment before devicePosition(), InvalidParameter for a decreasing
     *         firstSample; otherwise success.
     */
    AcqError pushSegment(const AcqSegmentHeader& header, std::span<const std::byte> payload) noexcept;

    /**
     * Publishes a new clock model (sequence counter, no lock).
     *
     * @param[in] clock   The clock model.
     */
    void setClock(const AcqClockModel& clock) noexcept;

    /**
     * Publishes a new metadata version, current + 1 (allocates; rare changes only).
     *
     * @param[in] info         New description; channel count, format, layout and direction must not change.
     * @param[in] fromSample   First sample it describes. @pre fromSample >= writePosition()
     *
     * @return InvalidParameter for a geometry change or a past sample, InvalidState after too many versions.
     */
    AcqError updateInfo(AcqStreamInfo info, std::uint64_t fromSample);

    /** Marks the end of the stream; readers drain and then receive EndOfStream. Idempotent. */
    void finish() noexcept;

private:
    friend class AcqStream;
    explicit AcqWriter(std::shared_ptr<detail::AcqStreamCore> core) noexcept;

    std::shared_ptr<detail::AcqStreamCore> m_core;
};
ACQ_STD_MEMBERS_END

} // namespace mne

#endif // ACQ_STREAM_H
