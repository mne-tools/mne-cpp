//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_reader.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqReader definitions: zero-copy acquire/release with exact loss accounting.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_reader.h"

#include "acq_convert.h"
#include "acq_stream.h"
#include "detail/acq_stream_core.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <optional>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

namespace
{
using Clock = std::chrono::steady_clock;

constexpr std::chrono::nanoseconds kMaxTimeout = std::chrono::hours(24 * 365);

struct RecordWindow
{
    std::uint64_t first = 0;
    std::uint64_t end = 0;
};

// Records of @p ring with a sample index in [from, to), starting at @p cursor. Counts as lost the records
// overwritten before the scan and, if @p countSkipped, those skipped because their sample lies before @p from.
template<typename T, typename SampleOf>
RecordWindow selectRecords(const RecordRing<T>& ring, std::uint64_t cursor, std::uint64_t from, std::uint64_t to,
                           std::atomic<std::uint64_t>& lostCounter, bool countSkipped, SampleOf sampleOf) noexcept
{
    while (true) {
        const std::uint64_t start = std::max(cursor, ring.oldest());
        const std::uint64_t published = ring.published.load(std::memory_order_acquire);
        RecordWindow window{start, start};
        while (window.first < published && sampleOf(ring.records[window.first & ring.mask]) < from) {
            ++window.first;
        }
        window.end = window.first;
        while (window.end < published && sampleOf(ring.records[window.end & ring.mask]) < to) {
            ++window.end;
        }
        // A slot the scan read may have been overwritten meanwhile, which can hide records; scan again.
        std::atomic_thread_fence(std::memory_order_acquire);
        if (ring.oldest() > start) {
            continue;
        }
        const std::uint64_t lost = (start - cursor) + (countSkipped ? window.first - start : 0);
        if (lost > 0) {
            addOwned(lostCounter, lost);
        }
        return window;
    }
}

template<typename T>
AcqSpanPair<const T> recordSpans(const std::vector<T>& records, std::uint64_t mask, RecordWindow window) noexcept
{
    const std::uint64_t n = window.end - window.first;
    if (n == 0) {
        return {};
    }
    const std::uint64_t index = window.first & mask;
    const std::uint64_t head = std::min<std::uint64_t>(n, mask + 1 - index);
    return {std::span<const T>(records.data() + index, static_cast<std::size_t>(head)),
            std::span<const T>(records.data(), static_cast<std::size_t>(n - head))};
}

AcqSpanPair<const std::byte> payloadSpans(const RecordRing<AcqSegmentHeader>& ring, RecordWindow window) noexcept
{
    const std::uint64_t n = window.end - window.first;
    if (n == 0 || ring.payloadBytes == 0) {
        return {};
    }
    const std::uint64_t index = window.first & ring.mask;
    const std::uint64_t head = std::min<std::uint64_t>(n, ring.capacity() - index);
    const std::size_t pb = ring.payloadBytes;
    return {std::span<const std::byte>(ring.payloads.data() + index * pb, static_cast<std::size_t>(head) * pb),
            std::span<const std::byte>(ring.payloads.data(), static_cast<std::size_t>(n - head) * pb)};
}

template<typename T>
bool recordsOverwritten(const RecordRing<T>& ring, RecordWindow window) noexcept
{
    if (window.end == window.first) {
        return false;
    }
    const std::uint64_t r = ring.reserved.load(std::memory_order_relaxed);
    return r > ring.capacity() && r - ring.capacity() > window.first;
}

/**
 * Gating reader: decides for every output commit visible up to @p written whether it is played. A commit
 * whose scheduled start the device has already passed (played as fill) is dropped whole and counted late.
 */
void validateCommits(AcqStreamCore& c, ReaderSlot& slot, std::uint64_t pos, std::uint64_t written) noexcept
{
    const std::uint64_t published = c.outputCommits.published.load(std::memory_order_acquire);
    std::uint64_t i = slot.commitCursor;
    for (; i < published; ++i) {
        const OutputCommit& commit = c.outputCommits.records[i & c.outputCommits.mask];
        if (commit.end > written) {
            break;
        }
        if (commit.start < pos) {
            slot.dropEnd = std::max(slot.dropEnd, commit.end);
            c.lateCommits.fetch_add(1, std::memory_order_relaxed);
        }
    }
    if (i != slot.commitCursor) {
        slot.commitCursor = i;
        c.commitsValidated.store(i, std::memory_order_seq_cst);
        c.notifyWriter();
    }
}

void detach(AcqStreamCore& core, std::uint32_t index) noexcept
{
    ReaderSlot& slot = core.slots[index];
    slot.holding = false;
    if (slot.gating) {
        core.gatingSlot.store(-1, std::memory_order_release);
        core.deviceGone.store(true, std::memory_order_seq_cst);
        core.notifyWriter();
    }
    slot.inUse.store(false, std::memory_order_release);
}
} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AcqReader::AcqReader() noexcept = default;

AcqReader::AcqReader(std::shared_ptr<AcqStreamCore> core, std::uint32_t slot) noexcept
: m_core(std::move(core))
, m_slot(slot)
{
}

AcqReader::~AcqReader()
{
    if (m_core) {
        detach(*m_core, m_slot);
    }
}

AcqReader::AcqReader(AcqReader&& other) noexcept
: m_core(std::move(other.m_core))
, m_slot(other.m_slot)
{
}

AcqReader& AcqReader::operator=(AcqReader&& other) noexcept
{
    if (this != &other) {
        if (m_core) {
            detach(*m_core, m_slot);
        }
        m_core = std::move(other.m_core);
        m_slot = other.m_slot;
    }
    return *this;
}

bool AcqReader::isValid() const noexcept
{
    return m_core != nullptr;
}

AcqReader::ReadMode AcqReader::mode() const noexcept
{
    return m_core ? m_core->slots[m_slot].mode : ReadMode::Lossless;
}

std::size_t AcqReader::channelCount() const noexcept
{
    return m_core ? m_core->channels : 0;
}

AcqStream AcqReader::stream() const noexcept
{
    return AcqStream(m_core);
}

AcqExpected<AcqBlockView> AcqReader::acquire(std::size_t minSamples, std::chrono::nanoseconds timeout,
                                             std::size_t maxSamples) noexcept
{
    if (!m_core) {
        return AcqErrorCode::InvalidState;
    }
    AcqStreamCore& c = *m_core;
    ReaderSlot& slot = c.slots[m_slot];
    if (slot.holding) {
        return AcqErrorCode::InvalidState;
    }
    if (minSamples > c.capacity) {
        return AcqErrorCode::InvalidParameter;
    }
    const std::uint64_t maxCount = maxSamples == 0 ? c.capacity : std::min<std::uint64_t>(maxSamples, c.capacity);
    const bool latest = slot.mode == ReadMode::Latest;
    const AcqWait wait = slot.mode == ReadMode::LowLatency ? AcqWait::Spin : slot.options.wait;
    // The clock is read only if the samples are not there yet; reading it costs as much as the rest of acquire().
    std::optional<Clock::time_point> deadline;

    std::uint64_t pos = 0;
    std::uint64_t written = 0;
    std::uint64_t oldest = 0;
    while (true) {
        pos = slot.position.load(std::memory_order_relaxed);
        written = c.committed.load(std::memory_order_acquire);
        const std::uint64_t r = c.reserved.load(std::memory_order_acquire);
        oldest = r > c.capacity ? r - c.capacity : 0;
        if (!latest && pos < oldest) {
            const std::uint64_t lostNow = oldest - pos;
            slot.pendingLost += lostNow;
            addOwned(slot.lost, lostNow);
            pos = oldest;
            slot.position.store(pos, std::memory_order_release);
        }
        std::uint64_t available = written > pos ? written - pos : 0;
        const bool finished = c.finished.load(std::memory_order_acquire);
        if (finished) {
            written = c.committed.load(std::memory_order_acquire);
            available = written > pos ? written - pos : 0;
        }
        if (slot.gating && c.direction == AcqDirection::Output) {
            validateCommits(c, slot, pos, written);
            if (pos < slot.dropEnd) {
                // Nothing playable until the device has played fill past the dropped commit.
                if (minSamples != 0) {
                    return AcqErrorCode::Timeout;
                }
                written = pos;
                break;
            }
        }
        if (minSamples == 0 || available >= minSamples || (finished && available > 0)) {
            break;
        }
        if (finished) {
            return AcqErrorCode::EndOfStream;
        }
        if (!deadline) {
            if (timeout <= std::chrono::nanoseconds(0)) {
                return AcqErrorCode::Timeout;
            }
            deadline = Clock::now() + std::min(timeout, kMaxTimeout);
        } else if (Clock::now() >= *deadline) {
            return AcqErrorCode::Timeout;
        }
        const std::uint64_t from = pos;
        auto ready = [&c, from, minSamples] {
            const std::uint64_t w = c.committed.load(std::memory_order_seq_cst);
            return (w > from && w - from >= minSamples) || c.finished.load(std::memory_order_seq_cst);
        };
        waitFor(ready, wait, slot.options.spinFor, *deadline, slot.waiting, slot.wake, &c.waiters);
    }

    const std::uint64_t available = written > pos ? written - pos : 0;
    std::uint64_t start = pos;
    std::uint64_t count = std::min(available, maxCount);
    std::uint64_t skipped = 0;
    if (latest) {
        count = maxSamples != 0 ? std::min<std::uint64_t>(available, maxCount)
                                : (minSamples != 0 ? std::min<std::uint64_t>(available, minSamples) : available);
        count = std::min(count, written > oldest ? written - oldest : 0);
        start = written - count;
        skipped = start - pos;
        slot.position.store(start, std::memory_order_release);
    }

    const std::size_t versionIndex = c.versionAt(start);
    if (versionIndex + 1 < c.versionCount.load(std::memory_order_acquire)) {
        const std::uint64_t nextFrom = c.versions[versionIndex + 1]->validFrom;
        if (nextFrom < start + count) {
            count = nextFrom - start;
        }
    }

    const RecordWindow events = selectRecords(c.events, slot.eventCursor, start, start + count, slot.eventsLost,
                                              !latest, [](const AcqEvent& e) { return loadRecordKey(e); });
    const RecordWindow segments = selectRecords(c.segments, slot.segmentCursor, start, start + count,
                                                slot.segmentsLost, !latest,
                                                [](const AcqSegmentHeader& h) { return loadRecordKey(h); });

    c.clock.refresh(slot.clock, slot.clockSeq);
    c.timebase.refresh(slot.timebase, slot.timebaseSeq);
    AcqBlockView view;
    ViewAccess::setSamples(view, c, start, count, *c.versions[versionIndex], slot.clock, slot.timebase);
    ViewAccess::setRead(view, slot.pendingLost, skipped, recordSpans(c.events.records, c.events.mask, events),
                        recordSpans(c.segments.records, c.segments.mask, segments), payloadSpans(c.segments, segments),
                        c.segments.payloadBytes);
    slot.ticket = (static_cast<std::uint64_t>(m_slot) << 48U) | (++slot.generation & 0xFFFFFFFFFFFFULL);
    ViewAccess::setTicket(view, slot.ticket);
    slot.holding = true;
    slot.viewFirst = start;
    slot.viewCount = count;
    slot.eventFirst = events.first;
    slot.eventEnd = events.end;
    slot.segmentFirst = segments.first;
    slot.segmentEnd = segments.end;
    slot.pendingLost = 0;
    addOwned(slot.views, 1);
    if (skipped > 0) {
        addOwned(slot.skipped, skipped);
    }
    return view;
}

AcqError AcqReader::release(AcqBlockView& view) noexcept
{
    if (!m_core) {
        return AcqErrorCode::InvalidState;
    }
    return releaseView(*m_core, m_slot, view);
}

AcqExpected<AcqViewLease> AcqReader::lease(std::size_t minSamples, std::chrono::nanoseconds timeout,
                                           std::size_t maxSamples) noexcept
{
    auto view = acquire(minSamples, timeout, maxSamples);
    if (!view) {
        return view.error();
    }
    return AcqViewLease(m_core.get(), m_slot, *view);
}

AcqError mne::detail::releaseView(AcqStreamCore& c, std::uint32_t index, AcqBlockView& view) noexcept
{
    ReaderSlot& slot = c.slots[index];
    if (!slot.holding || ViewAccess::ticket(view) != slot.ticket) {
        return AcqErrorCode::InvalidState;
    }
    std::atomic_thread_fence(std::memory_order_acquire);
    const std::uint64_t r = c.reserved.load(std::memory_order_relaxed);
    const bool overwritten = (r > c.capacity && r - c.capacity > slot.viewFirst) ||
        recordsOverwritten(c.events, {slot.eventFirst, slot.eventEnd}) ||
        recordsOverwritten(c.segments, {slot.segmentFirst, slot.segmentEnd});
    if (overwritten) {
        addOwned(slot.lost, slot.viewCount);
        addOwned(slot.eventsLost, slot.eventEnd - slot.eventFirst);
        addOwned(slot.segmentsLost, slot.segmentEnd - slot.segmentFirst);
        addOwned(slot.overruns, 1);
    } else {
        addOwned(slot.delivered, slot.viewCount);
    }
    slot.eventCursor = slot.eventEnd;
    slot.segmentCursor = slot.segmentEnd;
    const std::uint64_t end = slot.viewFirst + slot.viewCount;
    slot.holding = false;
    ViewAccess::clear(view);
    if (slot.gating) {
        slot.position.store(end, std::memory_order_release);
        c.gatePosition.store(end, std::memory_order_seq_cst);
        c.notifyWriter();
    } else {
        slot.position.store(end, std::memory_order_release);
    }
    return overwritten ? AcqError(AcqErrorCode::Overrun) : AcqError();
}

//=============================================================================================================
// DEFINE MEMBER METHODS AcqViewLease
//=============================================================================================================

AcqViewLease::AcqViewLease(AcqStreamCore* core, std::uint32_t slot, const AcqBlockView& view) noexcept
: m_core(core)
, m_slot(slot)
, m_view(view)
{
}

AcqViewLease::~AcqViewLease()
{
    static_cast<void>(release()); // an overrun stays counted in AcqReaderStats
}

AcqViewLease::AcqViewLease(AcqViewLease&& other) noexcept
: m_core(std::exchange(other.m_core, nullptr))
, m_slot(other.m_slot)
, m_view(other.m_view)
{
}

AcqViewLease& AcqViewLease::operator=(AcqViewLease&& other) noexcept
{
    if (this != &other) {
        static_cast<void>(release());
        m_core = std::exchange(other.m_core, nullptr);
        m_slot = other.m_slot;
        m_view = other.m_view;
    }
    return *this;
}

AcqError AcqViewLease::release() noexcept
{
    if (m_core == nullptr) {
        return AcqErrorCode::InvalidState;
    }
    return releaseView(*std::exchange(m_core, nullptr), m_slot, m_view);
}

//=============================================================================================================

AcqError AcqReader::consumeIdle(std::size_t samples) noexcept
{
    if (!m_core) {
        return AcqErrorCode::InvalidState;
    }
    AcqStreamCore& c = *m_core;
    ReaderSlot& slot = c.slots[m_slot];
    if (!slot.gating || c.direction != AcqDirection::Output) {
        return AcqErrorCode::WrongDirection;
    }
    if (slot.holding) {
        return AcqErrorCode::InvalidState;
    }
    const std::uint64_t end = slot.position.load(std::memory_order_relaxed) + samples;
    slot.idle.fetch_add(samples, std::memory_order_relaxed);
    c.idleTotal.fetch_add(samples, std::memory_order_acq_rel);
    slot.position.store(end, std::memory_order_release);
    c.gatePosition.store(end, std::memory_order_seq_cst);
    c.notifyWriter();
    return {};
}

namespace
{

// Shared body of the read() overloads.
template<typename T>
AcqExpected<AcqReadResult> readInto(AcqReader& reader, const AcqStreamCore* core, const AcqMatrixRef<T>& out,
                                    std::chrono::nanoseconds timeout, AcqScaling scaling) noexcept
{
    if (core == nullptr) {
        return AcqErrorCode::InvalidState;
    }
    if (out.rows != core->channels) {
        return AcqErrorCode::ShapeMismatch;
    }
    if (!core->memory.hostAccessible()) {
        return AcqErrorCode::DeviceMemory;
    }
    auto view = reader.acquire(out.cols, timeout, out.cols);
    if (!view) {
        return view.error();
    }
    AcqConvertOptions options;
    options.scaling = scaling;
    const AcqError decoded = decode(*view, out, options);
    const AcqReadResult result{view->firstSample(), view->sampleCount(), view->lost(), view->infoVersion()};
    const AcqError released = reader.release(*view);
    if (!decoded.ok()) {
        return decoded;
    }
    if (!released.ok()) {
        return released;
    }
    return result;
}

} // namespace

AcqExpected<AcqReadResult> AcqReader::read(const AcqMatrixRef<float>& out, std::chrono::nanoseconds timeout,
                                           AcqScaling scaling) noexcept
{
    return readInto(*this, m_core.get(), out, timeout, scaling);
}

AcqExpected<AcqReadResult> AcqReader::read(const AcqMatrixRef<double>& out, std::chrono::nanoseconds timeout,
                                           AcqScaling scaling) noexcept
{
    return readInto(*this, m_core.get(), out, timeout, scaling);
}

AcqExpected<AcqReadResult> AcqReader::read(const AcqMatrixRef<std::complex<float>>& out,
                                           std::chrono::nanoseconds timeout, AcqScaling scaling) noexcept
{
    return readInto(*this, m_core.get(), out, timeout, scaling);
}

std::size_t AcqReader::available() const noexcept
{
    if (!m_core) {
        return 0;
    }
    const std::uint64_t written = m_core->committed.load(std::memory_order_acquire);
    const std::uint64_t pos = m_core->slots[m_slot].position.load(std::memory_order_acquire);
    return static_cast<std::size_t>(written > pos ? std::min(written - pos, m_core->capacity) : 0);
}

AcqReaderStats AcqReader::stats() const noexcept
{
    AcqReaderStats s;
    if (!m_core) {
        return s;
    }
    const ReaderSlot& slot = m_core->slots[m_slot];
    s.position = slot.position.load(std::memory_order_acquire);
    s.delivered = slot.delivered.load(std::memory_order_relaxed);
    s.lost = slot.lost.load(std::memory_order_relaxed);
    s.skipped = slot.skipped.load(std::memory_order_relaxed);
    s.idle = slot.idle.load(std::memory_order_relaxed);
    s.overruns = slot.overruns.load(std::memory_order_relaxed);
    s.views = slot.views.load(std::memory_order_relaxed);
    s.eventsLost = slot.eventsLost.load(std::memory_order_relaxed);
    s.segmentsLost = slot.segmentsLost.load(std::memory_order_relaxed);
    return s;
}
