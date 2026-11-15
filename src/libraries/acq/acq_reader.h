//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_reader.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    AcqReader: one consumer of a stream with its own position, mode and exact loss count.
 *
 * Input streams: any number of readers (recorder, display, closed-loop controller) consume the ring
 * independently; the producer never waits for them. Output streams: the device backend consumes
 * through one gating reader, which the writer waits for; further non-gating readers monitor what was
 * sent (e.g. to record the delivered stimulus).
 *
 * Cost model (throughput does not degrade with the number of readers): each reader's position lives
 * on its own cache line and is written only by that reader; the producer publishes one position per
 * commit and touches no per-reader state; a reader computes its loss from positions on acquire();
 * a blocked reader is woken by std::atomic::notify only when it announced that it waits.
 * After start(), acquire() and release() perform no heap allocation and take no lock.
 *
 * Zero-copy loop:
 * @code
 * auto reader = stream.reader(mne::AcqReader::ReadMode::Lossless);
 * while (running) {
 *     auto view = reader->lease(256, std::chrono::milliseconds(100));
 *     if (!view) { if (view.error() == mne::AcqErrorCode::Timeout) continue; break; }
 *     process(**view);                                  // parts point into the ring
 *     if (!view->release().ok()) discardLastResult(); // the producer overtook the view
 * }
 * @endcode
 */

#ifndef ACQ_READER_H
#define ACQ_READER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"
#include "acq_block_view.h"
#include "acq_convert.h"
#include "acq_error.h"

#include <chrono>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

namespace detail
{
class AcqStreamCore;
}

class AcqStream;
class AcqViewLease;

//=============================================================================================================
/**
 * How a waiting reader or writer is woken.
 */
enum class AcqWait : std::uint8_t
{
    Block = 0,        /**< std::atomic::wait; no CPU while idle. */
    Spin = 1,         /**< Busy-poll; lowest latency, occupies a core. */
    SpinThenBlock = 2 /**< Busy-poll for spinFor, then block. */
};

//=============================================================================================================
/**
 * Per-reader options.
 *
 * @snippet ex_acq_performance/main.cpp acq_stream_setup
 */
struct AcqReaderOptions
{
    AcqWait wait = AcqWait::Block;                                    /**< Ignored for LowLatency, which spins. */
    std::chrono::nanoseconds spinFor = std::chrono::microseconds(50); /**< Spin window of SpinThenBlock. */
    bool startAtOldest = false;                                       /**< Start at the oldest retained sample instead of the newest. */
    bool gating = false;                                              /**< The writer never overwrites what this reader has not consumed: the device side of an output stream, or backpressure on an input stream (replay, conversion; a live device cannot wait). One per stream. */
};

//=============================================================================================================
/**
 * Counters of one reader; exact.
 *
 * @snippet ex_acq_performance/main.cpp acq_stream_stats
 */
struct AcqReaderStats
{
    std::uint64_t position = 0;     /**< Next sample index this reader will deliver. */
    std::uint64_t delivered = 0;    /**< Samples delivered in views. */
    std::uint64_t lost = 0;         /**< Samples overwritten before this reader could acquire them. */
    std::uint64_t skipped = 0;      /**< Samples skipped on purpose by a Latest reader. */
    std::uint64_t idle = 0;         /**< Gating reader: samples played as underrun fill (consumeIdle). */
    std::uint64_t overruns = 0;     /**< Views invalidated while held (release() returned Overrun). */
    std::uint64_t views = 0;        /**< Views acquired. */
    std::uint64_t eventsLost = 0;   /**< Events this reader did not receive: overwritten, attached to lost or overrun
                                         samples, or pushed after it had acquired their sample. Lossless readers:
                                         events received + eventsLost = events published. */
    std::uint64_t segmentsLost = 0; /**< Segment headers not received, likewise. */
};

//=============================================================================================================
/**
 * Result of the copying read() calls.
 */
struct AcqReadResult
{
    std::uint64_t firstSample = 0; /**< Sample index of column 0. */
    std::size_t samples = 0;       /**< Columns written. */
    std::uint64_t lost = 0;        /**< Samples lost before column 0. */
    std::uint32_t infoVersion = 0; /**< AcqStreamInfo::version of the samples. */
};

ACQ_STD_MEMBERS_BEGIN
//=============================================================================================================
/**
 * A consumer of one AcqStream. Move-only; destroying it frees its reader slot.
 *
 * @snippet ex_acq_performance/main.cpp acq_reader_acquire
 */
class ACQSHARED_EXPORT AcqReader
{
public:
    /**
     * Consumption mode.
     */
    enum class ReadMode : std::uint8_t
    {
        Lossless = 0,  /**< Every sample in order; an overrun is reported with its exact count and the reader resumes at the oldest retained sample. */
        Latest = 1,    /**< The newest samples; older ones are skipped (counted, not lost). For displays. */
        LowLatency = 2 /**< Like Lossless, woken by busy-poll for the shortest commit-to-wake latency. */
    };

    AcqReader() noexcept;
    ~AcqReader();
    AcqReader(AcqReader&& other) noexcept;
    AcqReader& operator=(AcqReader&& other) noexcept;
    AcqReader(const AcqReader&) = delete;
    AcqReader& operator=(const AcqReader&) = delete;

    /** @return true when attached to a stream. */
    [[nodiscard]] bool isValid() const noexcept;

    /** @return The consumption mode. */
    [[nodiscard]] ReadMode mode() const noexcept;

    /** @return Channels of the stream. */
    [[nodiscard]] std::size_t channelCount() const noexcept;

    /** @return The stream this reader consumes. */
    [[nodiscard]] AcqStream stream() const noexcept;

    //=========================================================================================================
    /**
     * Waits until at least @p minSamples are available (Latest: the newest @p minSamples), then returns a
     * view of up to @p maxSamples. Larger blocks amortise the per-view cost; pass maxSamples = 0 to take
     * everything available. One view per reader at a time.
     *
     * @param[in] minSamples   Samples per channel required; 0 returns immediately (device polling).
     * @param[in] timeout      Maximum wait.
     * @param[in] maxSamples   Upper bound of the view, 0 for everything available.
     *
     * @return The view, or Timeout, EndOfStream, DeviceStopped, InvalidState (a view is already held).
     */
    [[nodiscard]] AcqExpected<AcqBlockView> acquire(std::size_t minSamples, std::chrono::nanoseconds timeout,
                                                    std::size_t maxSamples = 0) noexcept;

    //=========================================================================================================
    /**
     * Returns @p view to the ring and advances the reader; clears @p view.
     *
     * @param[in, out] view   A view returned by this reader's acquire().
     *
     * @return Success, or Overrun when the producer overwrote part of the view while it was held; its
     *         samples count as lost and anything computed from them must be discarded.
     */
    AcqError release(AcqBlockView& view) noexcept;

    //=========================================================================================================
    /**
     * acquire() whose view is released when the returned lease goes out of scope, so no exit path can
     * leave the reader blocked. Call AcqViewLease::release() to learn whether the view was overrun.
     *
     * @param[in] minSamples   As for acquire().
     * @param[in] timeout      As for acquire().
     * @param[in] maxSamples   As for acquire().
     *
     * @return The lease, or the error of acquire().
     */
    [[nodiscard]] AcqExpected<AcqViewLease> lease(std::size_t minSamples, std::chrono::nanoseconds timeout,
                                                  std::size_t maxSamples = 0) noexcept;

    //=========================================================================================================
    /**
     * Gating reader of an output stream: the device has emitted @p samples samples of underrun fill
     * because the writer had not committed them. Advances this reader; the writer learns the gap from
     * AcqWriteView::underrun().
     *
     * @param[in] samples   Samples played as fill.
     *
     * @return WrongDirection unless this is the gating reader of an output stream.
     */
    AcqError consumeIdle(std::size_t samples) noexcept;

    //=========================================================================================================
    /**
     * Copying convenience: waits for @p out.cols samples, decodes them into @p out and releases.
     *
     * @param[out] out      Caller-owned destination, rows = channelCount().
     * @param[in] timeout   Maximum wait.
     * @param[in] scaling   Raw or physical values.
     *
     * @return Position and loss of the block, ShapeMismatch (rows != channelCount()), DeviceMemory (the
     *         ring is in GPU memory; returned at once, nothing is consumed), or the error of
     *         acquire()/decode()/release().
     */
    [[nodiscard]] AcqExpected<AcqReadResult> read(const AcqMatrixRef<float>& out, std::chrono::nanoseconds timeout,
                                                  AcqScaling scaling = AcqScaling::Physical) noexcept;

    /** @copydoc read(const AcqMatrixRef<float>&, std::chrono::nanoseconds, AcqScaling) */
    [[nodiscard]] AcqExpected<AcqReadResult> read(const AcqMatrixRef<double>& out, std::chrono::nanoseconds timeout,
                                                  AcqScaling scaling = AcqScaling::Physical) noexcept;

    /** @copydoc read(const AcqMatrixRef<float>&, std::chrono::nanoseconds, AcqScaling) */
    [[nodiscard]] AcqExpected<AcqReadResult> read(const AcqMatrixRef<std::complex<float>>& out,
                                                  std::chrono::nanoseconds timeout,
                                                  AcqScaling scaling = AcqScaling::Physical) noexcept;

    /** @return Samples committed and not yet acquired by this reader (capped at the ring capacity). */
    [[nodiscard]] std::size_t available() const noexcept;

    /** @return This reader's counters. */
    [[nodiscard]] AcqReaderStats stats() const noexcept;

private:
    friend class AcqStream;
    AcqReader(std::shared_ptr<detail::AcqStreamCore> core, std::uint32_t slot) noexcept;

    std::shared_ptr<detail::AcqStreamCore> m_core;
    std::uint32_t m_slot = 0;
};

//=============================================================================================================
/**
 * A view held from AcqReader::lease(); returns it to the ring when destroyed. Move-only. The reader that
 * issued it must outlive it (moving the reader is fine).
 *
 * @snippet ex_acq_oddball/main.cpp acq_epoch_stream
 */
class ACQSHARED_EXPORT AcqViewLease
{
public:
    AcqViewLease() noexcept = default;
    ~AcqViewLease();
    AcqViewLease(AcqViewLease&& other) noexcept;
    AcqViewLease& operator=(AcqViewLease&& other) noexcept;
    AcqViewLease(const AcqViewLease&) = delete;
    AcqViewLease& operator=(const AcqViewLease&) = delete;

    /** @return true until released. */
    [[nodiscard]] bool isHeld() const noexcept
    {
        return m_core != nullptr;
    }

    /** @return The held view; empty after release(). */
    [[nodiscard]] const AcqBlockView& operator*() const noexcept
    {
        return m_view;
    }

    /** @return The held view; empty after release(). */
    [[nodiscard]] const AcqBlockView* operator->() const noexcept
    {
        return &m_view;
    }

    //=========================================================================================================
    /**
     * Returns the view now. The destructor does the same and leaves an overrun only in AcqReaderStats.
     *
     * @return As AcqReader::release(); InvalidState if not held.
     */
    AcqError release() noexcept;

private:
    friend class AcqReader;
    AcqViewLease(detail::AcqStreamCore* core, std::uint32_t slot, const AcqBlockView& view) noexcept;

    detail::AcqStreamCore* m_core = nullptr;
    std::uint32_t m_slot = 0;
    AcqBlockView m_view;
};
ACQ_STD_MEMBERS_END

} // namespace mne

#endif // ACQ_READER_H
