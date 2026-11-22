//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_stream_core.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Internal shared state of one stream ring; not installed.
 *
 * Concurrency protocol:
 * - The single producer publishes samples with a seq_cst store of @c committed. Before it writes into the
 *   ring it announces the highest sample index it may touch in @c reserved, followed by a full fence, so a
 *   reader that validates a view by loading @c reserved after an acquire fence detects every overwrite.
 * - Each reader slot is written only by its reader. The producer touches reader slots only to wake readers
 *   that announced a wait (@c waiters > 0) and, for output streams, reads the gating reader's position.
 * - Records (events, segment headers) use the same reserve/publish scheme as samples.
 * - Blocking uses one binary semaphore per waiter and a Dekker handshake on seq_cst atomics; nothing on the
 *   data path takes a mutex or allocates.
 */

#ifndef ACQ_STREAM_CORE_H
#define ACQ_STREAM_CORE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../acq_reader.h"
#include "../acq_stream.h"
#include "acq_ring_memory.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <semaphore>
#include <type_traits>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4324) // padding from cache-line alignment is intended
#endif

namespace mne::detail
{

inline constexpr std::size_t kCacheLine = 64;
inline constexpr std::size_t kMaxInfoVersions = 1024;

//=============================================================================================================
/**
 * Mutex for control-path state; every lock is counted so tests can prove the data path takes none.
 */
class ControlMutex
{
public:
    void lock();
    void unlock() noexcept
    {
        m_mutex.unlock();
    }

private:
    std::mutex m_mutex;
};

//=============================================================================================================
/**
 * Adds @p n to a counter that only the calling thread writes (others only read it): a plain load and store
 * instead of an atomic read-modify-write, which costs several times as much on the data path.
 */
inline void addOwned(std::atomic<std::uint64_t>& counter, std::uint64_t n) noexcept
{
    counter.store(counter.load(std::memory_order_relaxed) + n, std::memory_order_relaxed);
}

//=============================================================================================================
/**
 * Copies @p src into the record slot @p dst as relaxed 8-byte atomic stores. A lagging reader may scan a slot
 * the producer is overwriting (the reader detects that afterwards and scans again); the accesses must still not
 * be a data race.
 */
template<typename T>
void storeRecord(T& dst, const T& src) noexcept
{
    static_assert(std::is_trivially_copyable_v<T> && sizeof(T) % 8 == 0 && alignof(T) >= 8);
    std::array<std::uint64_t, sizeof(T) / 8> words{};
    std::memcpy(words.data(), &src, sizeof(T));
    auto* raw = reinterpret_cast<std::uint64_t*>(&dst);
    for (std::size_t k = 0; k < words.size(); ++k) {
        std::atomic_ref<std::uint64_t>(raw[k]).store(words[k], std::memory_order_relaxed);
    }
}

/** @return The sample index of a record (its first 8 bytes), read as a relaxed atomic. */
template<typename T>
[[nodiscard]] std::uint64_t loadRecordKey(const T& record) noexcept
{
    static_assert(alignof(T) >= 8 && (offsetof(AcqEvent, sample) == 0) && (offsetof(AcqSegmentHeader, firstSample) == 0));
    auto* raw = reinterpret_cast<std::uint64_t*>(const_cast<T*>(&record));
    return std::atomic_ref<std::uint64_t>(*raw).load(std::memory_order_relaxed);
}

//=============================================================================================================
/**
 * Lock-free single-writer value with a sequence counter, stored in atomic words so readers never race.
 */
template<typename T>
class SeqlockValue
{
    static_assert(std::is_trivially_copyable_v<T>);
    static constexpr std::size_t kWords = (sizeof(T) + 7) / 8;

public:
    static constexpr std::uint64_t kSeqNone = ~std::uint64_t{0}; /**< Never reached by the sequence. */

    explicit SeqlockValue(const T& value = T()) noexcept
    {
        store(value);
    }

    void store(const T& value) noexcept
    {
        std::array<std::uint64_t, kWords> words{};
        std::memcpy(words.data(), &value, sizeof(T));
        const std::uint64_t seq = m_seq.load(std::memory_order_relaxed);
        m_seq.store(seq + 1, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);
        for (std::size_t i = 0; i < kWords; ++i) {
            m_words[i].store(words[i], std::memory_order_relaxed);
        }
        m_seq.store(seq + 2, std::memory_order_release);
    }

    [[nodiscard]] T load() const noexcept
    {
        T value;
        std::uint64_t seen = kSeqNone;
        refresh(value, seen);
        return value;
    }

    /**
     * Updates @p cache if the value changed since sequence @p seen (start with kSeqNone).
     * The 64-bit sequence cannot wrap, so an unchanged sequence proves an unchanged value.
     */
    void refresh(T& cache, std::uint64_t& seen) const noexcept
    {
        if (m_seq.load(std::memory_order_acquire) == seen) {
            return;
        }
        std::array<std::uint64_t, kWords> words{};
        std::uint64_t before = 0;
        std::uint64_t after = 0;
        do {
            before = m_seq.load(std::memory_order_acquire);
            for (std::size_t i = 0; i < kWords; ++i) {
                words[i] = m_words[i].load(std::memory_order_relaxed);
            }
            std::atomic_thread_fence(std::memory_order_acquire);
            after = m_seq.load(std::memory_order_relaxed);
        } while (before != after || (before & 1U) != 0);
        std::memcpy(static_cast<void*>(&cache), words.data(), sizeof(T)); // T is trivially copyable (asserted)
        seen = before;
    }

private:
    std::atomic<std::uint64_t> m_seq{0};
    std::array<std::atomic<std::uint64_t>, kWords> m_words{};
};

//=============================================================================================================
/**
 * Ring of fixed-size records (events, segment headers) with an optional fixed-size payload per record.
 */
template<typename T>
struct RecordRing
{
    std::vector<T> records;
    std::vector<std::byte> payloads;
    std::size_t payloadBytes = 0;
    std::uint64_t mask = 0;
    alignas(kCacheLine) std::atomic<std::uint64_t> reserved{0};
    std::atomic<std::uint64_t> published{0};
    std::uint64_t pushed = 0; /**< Producer-local count of written records. */

    [[nodiscard]] std::uint64_t capacity() const noexcept
    {
        return mask + 1;
    }

    [[nodiscard]] std::uint64_t oldest() const noexcept
    {
        const std::uint64_t r = reserved.load(std::memory_order_acquire);
        return r > capacity() ? r - capacity() : 0;
    }
};

//=============================================================================================================
/**
 * One output commit. The device plays it only if it had not passed @c start when it first saw the commit;
 * otherwise the whole commit is dropped, so a late block is never played truncated and shifted.
 */
struct OutputCommit
{
    std::uint64_t start = 0;
    std::uint64_t end = 0;
};

/** Output commits the writer may have ahead of the device's validation before beginWrite() waits. */
constexpr std::size_t kOutputCommitLog = 1024;

//=============================================================================================================
/**
 * One metadata version with the per-channel arrays views point into.
 */
struct InfoVersion
{
    std::shared_ptr<const AcqStreamInfo> info;
    std::vector<double> scales;
    std::vector<double> offsets;
    std::vector<double> rangeMin;
    std::vector<double> rangeMax;
    std::vector<std::uint8_t> digitalBits;
    std::uint64_t validFrom = 0;
};

//=============================================================================================================
/**
 * State of one reader. Plain members are touched only by the owning reader; atomics may be read anywhere.
 */
struct alignas(kCacheLine) ReaderSlot
{
    std::atomic<bool> inUse{false};
    std::atomic<std::uint64_t> position{0};
    std::atomic<bool> waiting{false};
    std::binary_semaphore wake{0};

    AcqReader::ReadMode mode = AcqReader::ReadMode::Lossless;
    AcqReaderOptions options;
    bool gating = false;
    bool holding = false;
    std::uint64_t ticket = 0;
    std::uint64_t generation = 0;
    std::uint64_t viewFirst = 0;
    std::uint64_t viewCount = 0;
    std::uint64_t pendingLost = 0;
    std::uint64_t eventCursor = 0;
    std::uint64_t eventFirst = 0;
    std::uint64_t eventEnd = 0;
    std::uint64_t segmentCursor = 0;
    std::uint64_t commitCursor = 0; /**< Gating reader: next output commit to validate. */
    std::uint64_t dropEnd = 0;      /**< Gating reader: end of the last commit dropped as late. */
    std::uint64_t segmentFirst = 0;
    std::uint64_t segmentEnd = 0;
    AcqClockModel clock;  /**< Last clock seen; refreshed when clockSeq changes. */
    AcqTimebase timebase; /**< Last timebase seen; refreshed when timebaseSeq changes. */
    std::uint64_t clockSeq = SeqlockValue<AcqClockModel>::kSeqNone;
    std::uint64_t timebaseSeq = SeqlockValue<AcqTimebase>::kSeqNone;

    std::atomic<std::uint64_t> delivered{0};
    std::atomic<std::uint64_t> lost{0};
    std::atomic<std::uint64_t> skipped{0};
    std::atomic<std::uint64_t> idle{0};
    std::atomic<std::uint64_t> overruns{0};
    std::atomic<std::uint64_t> views{0};
    std::atomic<std::uint64_t> eventsLost{0};
    std::atomic<std::uint64_t> segmentsLost{0};
};

//=============================================================================================================
/**
 * Shared state of one stream ring.
 */
class AcqStreamCore
{
public:
    [[nodiscard]] static AcqExpected<std::shared_ptr<AcqStreamCore>> create(AcqStreamInfo info,
                                                                            const AcqStreamOptions& options);
    ~AcqStreamCore();
    AcqStreamCore(const AcqStreamCore&) = delete;
    AcqStreamCore& operator=(const AcqStreamCore&) = delete;

    // Geometry (fixed after create)
    AcqDirection direction = AcqDirection::Input;
    AcqSampleFormat format = AcqSampleFormat::Float32;
    AcqLayout layout = AcqLayout::Interleaved;
    AcqUnderrunFill underrunFill = AcqUnderrunFill::Zero;
    std::size_t channels = 0;
    std::size_t bytesPerSample = 0;
    std::size_t frameBytes = 0;
    std::uint64_t capacity = 0;
    std::uint64_t mask = 0;
    std::ptrdiff_t rowStride = 0;
    std::byte* base = nullptr;
    AcqMemoryLocation memory;
    RingMemory ownedMemory; // empty for an adopted buffer
    std::size_t bufferBytes = 0;
    std::function<void()> releaseExternal;

    // Producer
    alignas(kCacheLine) std::atomic<std::uint64_t> committed{0};
    alignas(kCacheLine) std::atomic<std::uint64_t> reserved{0};
    std::atomic<bool> finished{false};
    std::atomic<std::uint32_t> waiters{0};
    std::uint64_t writePos = 0;     /**< Producer-local next sample index. */
    std::uint64_t pendingFirst = 0; /**< Producer-local start of the reserved view. */
    std::uint64_t pendingCount = 0; /**< Producer-local size of the reserved view. */
    std::uint64_t grantedEnd = 0;   /**< Producer-local end of the space granted to a DMA engine. */
    std::uint64_t idleSeen = 0;     /**< Producer-local snapshot of idleTotal. */
    std::uint64_t lastEventSample = 0;
    std::uint64_t lastSegmentSample = 0;
    std::atomic<std::uint64_t> commits{0};

    // Output direction
    alignas(kCacheLine) std::atomic<std::uint64_t> gatePosition{0};
    std::atomic<std::uint64_t> idleTotal{0};
    std::atomic<int> gatingSlot{-1};
    std::atomic<bool> deviceGone{false};
    std::atomic<bool> writerTaken{false};
    std::atomic<bool> writerWaiting{false};
    std::binary_semaphore writerWake{0};
    AcqWait writerWait = AcqWait::Block;
    std::atomic<std::uint64_t> underrun{0};
    std::atomic<std::uint64_t> underruns{0};
    std::atomic<std::uint64_t> writerWaits{0};
    std::atomic<std::uint64_t> lateEvents{0};
    std::atomic<std::uint64_t> lateCommits{0};
    std::atomic<std::uint64_t> commitsValidated{0}; /**< Output commits the gating reader has validated. */

    // Records
    RecordRing<OutputCommit> outputCommits; /**< Output: scheduled start and end of every commit. */
    RecordRing<AcqEvent> events;
    RecordRing<AcqSegmentHeader> segments;

    // Metadata
    std::array<std::unique_ptr<InfoVersion>, kMaxInfoVersions> versions;
    std::atomic<std::size_t> versionCount{0};
    SeqlockValue<AcqClockModel> clock;
    SeqlockValue<AcqTimebase> timebase;
    ControlMutex controlMutex;

    // Readers
    std::unique_ptr<ReaderSlot[]> slots;
    std::size_t maxReaders = 0;

    /** @return Whether the writer must wait for a gating reader (always on output streams). */
    [[nodiscard]] bool gated() const noexcept
    {
        return direction == AcqDirection::Output || gatingSlot.load(std::memory_order_seq_cst) >= 0;
    }

    /** @return Index of the info version valid at @p sample. */
    [[nodiscard]] std::size_t versionAt(std::uint64_t sample) const noexcept;

    /** @return Pointer to (channel 0, sample @p sample) and the number of samples until the ring end. */
    [[nodiscard]] std::byte* sampleAddress(std::uint64_t sample) const noexcept;

    /** Fills [@p from, @p to) with the underrun fill (output) after announcing the reservation. */
    void fillRange(std::uint64_t from, std::uint64_t to) noexcept;

    /** Wakes readers that announced a wait. */
    void notifyReaders() noexcept;

    /** Wakes the writer if it announced a wait. */
    void notifyWriter() noexcept;

    /** Reserves ring space up to sample @p end (exclusive) before the producer writes. */
    void reserveUpTo(std::uint64_t end) noexcept;

    /** Builds the one or two parts of [@p first, @p first + @p count). */
    template<typename Byte>
    std::size_t buildParts(std::uint64_t first, std::uint64_t count, std::array<AcqPart<Byte>, 2>& parts) const noexcept
    {
        parts = {};
        if (count == 0) {
            return 0;
        }
        const std::uint64_t index = first & mask;
        const std::uint64_t head = ownedMemory.mirrored ? count : std::min<std::uint64_t>(count, capacity - index);
        const std::ptrdiff_t sampleStride = layout == AcqLayout::Interleaved
            ? static_cast<std::ptrdiff_t>(frameBytes)
            : static_cast<std::ptrdiff_t>(bytesPerSample);
        const std::ptrdiff_t channelStride =
            layout == AcqLayout::Interleaved ? static_cast<std::ptrdiff_t>(bytesPerSample) : rowStride;
        parts[0] = {sampleAddress(first), static_cast<std::size_t>(head), sampleStride, channelStride};
        if (head == count) {
            return 1;
        }
        parts[1] = {base, static_cast<std::size_t>(count - head), sampleStride, channelStride};
        return 2;
    }

private:
    AcqStreamCore() = default;
};

//=============================================================================================================
/**
 * Fills the private members of views.
 */
struct ViewAccess
{
    template<typename Byte>
    static void setSamples(AcqViewBase<Byte>& view, const AcqStreamCore& core, std::uint64_t first,
                           std::uint64_t count, const InfoVersion& version, const AcqClockModel& clock,
                           const AcqTimebase& timebase) noexcept
    {
        view.m_partCount = core.buildParts(first, count, view.m_parts);
        view.m_firstSample = first;
        view.m_sampleCount = static_cast<std::size_t>(count);
        view.m_channelCount = core.channels;
        view.m_format = core.format;
        view.m_infoVersion = version.info->version;
        view.m_scales = version.scales;
        view.m_offsets = version.offsets;
        view.m_rangeMin = version.rangeMin;
        view.m_rangeMax = version.rangeMax;
        view.m_digitalBits = version.digitalBits;
        view.m_clock = clock;
        view.m_timebase = timebase;
        view.m_memory = core.memory;
    }

    /** Empties a released view: no samples, events or segments, and no longer accepted by release(). */
    static void clear(AcqBlockView& view) noexcept
    {
        view.m_partCount = 0;
        view.m_sampleCount = 0;
        view.m_events = {};
        view.m_segments = {};
        view.m_segmentPayloads = {};
        view.m_ticket = 0;
    }

    static void setUnderrun(AcqWriteView& view, std::uint64_t underrun) noexcept
    {
        view.m_underrun = underrun;
    }

    static void setTicket(AcqBlockView& view, std::uint64_t ticket) noexcept
    {
        view.m_ticket = ticket;
    }

    [[nodiscard]] static std::uint64_t ticket(const AcqBlockView& view) noexcept
    {
        return view.m_ticket;
    }

    static void setRead(AcqBlockView& view, std::uint64_t lost, std::uint64_t skipped,
                        AcqSpanPair<const AcqEvent> events, AcqSpanPair<const AcqSegmentHeader> segments,
                        AcqSpanPair<const std::byte> payloads, std::size_t payloadBytes) noexcept
    {
        view.m_lost = lost;
        view.m_skipped = skipped;
        view.m_events = events;
        view.m_segments = segments;
        view.m_segmentPayloads = payloads;
        view.m_segmentPayloadBytes = payloadBytes;
    }
};

//=============================================================================================================
/**
 * Pause instruction for busy-wait loops.
 */
void cpuRelax() noexcept;

/** AcqReader::release() for reader slot @p slot of @p core; shared with AcqViewLease. */
AcqError releaseView(AcqStreamCore& core, std::uint32_t slot, AcqBlockView& view) noexcept;

//=============================================================================================================
/**
 * Waits on @p semaphore according to @p wait until @p ready() or @p deadline; Dekker handshake with the
 * notifier through @p flag (announce) and @p counter (optional count of waiters).
 *
 * @return true if @p ready() holds on return.
 */
template<typename Ready>
bool waitFor(Ready&& ready, AcqWait wait, std::chrono::nanoseconds spinFor,
             std::chrono::steady_clock::time_point deadline, std::atomic<bool>& flag, std::binary_semaphore& semaphore,
             std::atomic<std::uint32_t>* counter)
{
    using Clock = std::chrono::steady_clock;
    if (wait != AcqWait::Block) {
        const auto spinEnd = wait == AcqWait::Spin ? deadline : std::min(deadline, Clock::now() + spinFor);
        for (std::uint32_t i = 0;; ++i) {
            if (ready()) {
                return true;
            }
            if ((i & 63U) == 0 && Clock::now() >= spinEnd) {
                break;
            }
            cpuRelax();
        }
        if (wait == AcqWait::Spin) {
            return ready();
        }
    }
    while (true) {
        flag.store(true, std::memory_order_seq_cst);
        if (counter != nullptr) {
            counter->fetch_add(1, std::memory_order_seq_cst);
        }
        bool woken = false;
        const bool readyNow = ready();
        if (!readyNow && Clock::now() < deadline) {
            woken = semaphore.try_acquire_until(deadline);
        }
        if (counter != nullptr) {
            counter->fetch_sub(1, std::memory_order_seq_cst);
        }
        if (!woken && !flag.exchange(false, std::memory_order_acq_rel)) {
            semaphore.acquire(); // a notifier took the flag; consume its token
        }
        if (readyNow || ready()) {
            return true;
        }
        if (Clock::now() >= deadline) {
            return false;
        }
    }
}

} // namespace mne::detail

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif // ACQ_STREAM_CORE_H
