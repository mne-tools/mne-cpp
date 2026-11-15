//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_stream.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Stream ring core, AcqStream and AcqWriter definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_stream.h"

#include "detail/acq_diagnostics.h"
#include "detail/acq_stream_core.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <new>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

namespace
{
std::atomic<std::uint64_t> g_controlLocks{0};

constexpr std::uint64_t kDefaultMaxRingBytes = 256ULL << 20;

std::uint64_t nextPow2(std::uint64_t v) noexcept
{
    return v <= 1 ? 1 : std::bit_ceil(v);
}

std::unique_ptr<InfoVersion> makeVersion(AcqStreamInfo info, std::uint32_t version, std::uint64_t validFrom)
{
    auto v = std::make_unique<InfoVersion>();
    info.version = version;
    info.validFromSample = validFrom;
    for (const AcqChannelInfo& ch : info.channels) {
        v->scales.push_back(ch.scale);
        v->offsets.push_back(ch.offset);
        v->rangeMin.push_back(ch.rangeMin);
        v->rangeMax.push_back(ch.rangeMax);
        v->digitalBits.push_back(ch.digitalBits);
    }
    v->validFrom = validFrom;
    v->info = std::make_shared<const AcqStreamInfo>(std::move(info));
    return v;
}

template<typename T>
void initRecords(RecordRing<T>& ring, std::size_t capacity, std::size_t payloadBytes)
{
    const std::uint64_t cap = nextPow2(std::max<std::uint64_t>(capacity, 2));
    ring.records.assign(cap, T{});
    ring.payloadBytes = payloadBytes;
    ring.payloads.assign(cap * payloadBytes, std::byte{0});
    ring.mask = cap - 1;
}

template<typename T>
void pushRecord(RecordRing<T>& ring, const T& record, std::span<const std::byte> payload, bool publish) noexcept
{
    const std::uint64_t index = ring.pushed;
    ring.reserved.store(index + 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    const std::uint64_t slot = index & ring.mask;
    storeRecord(ring.records[slot], record);
    if (ring.payloadBytes > 0) {
        std::byte* dst = ring.payloads.data() + slot * ring.payloadBytes;
        const std::size_t n = std::min(payload.size(), ring.payloadBytes);
        if (n > 0) { // an empty span's data() may be null, which memcpy must never receive
            std::memcpy(dst, payload.data(), n);
        }
        std::memset(dst + n, 0, ring.payloadBytes - n);
    }
    ring.pushed = index + 1;
    if (publish) {
        ring.published.store(ring.pushed, std::memory_order_release);
    }
}
} // namespace

//=============================================================================================================
// DEFINE DETAIL METHODS
//=============================================================================================================

std::uint64_t mne::detail::controlLockCount() noexcept
{
    return g_controlLocks.load(std::memory_order_relaxed);
}

//=============================================================================================================

void ControlMutex::lock()
{
    g_controlLocks.fetch_add(1, std::memory_order_relaxed);
    m_mutex.lock();
}

//=============================================================================================================

void mne::detail::cpuRelax() noexcept
{
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    _mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __asm__ __volatile__("yield");
#endif
}

//=============================================================================================================

AcqExpected<std::shared_ptr<AcqStreamCore>> AcqStreamCore::create(AcqStreamInfo info, const AcqStreamOptions& options)
{
    const std::size_t bps = mne::bytesPerSample(info.format);
    if (bps == 0 || (info.layout != AcqLayout::Interleaved && info.layout != AcqLayout::Planar)) {
        return AcqErrorCode::UnsupportedFormat;
    }
    if (!(info.sfreq >= 0.0) || !std::isfinite(info.sfreq) || options.maxReaders == 0 || options.maxReaders > 4096 ||
        options.rowAlignment == 0 || !std::has_single_bit(options.rowAlignment) || options.alignment < 64 ||
        !std::has_single_bit(options.alignment)) {
        return AcqErrorCode::InvalidParameter;
    }

    std::shared_ptr<AcqStreamCore> core(new AcqStreamCore());
    core->direction = info.direction;
    core->format = info.format;
    core->layout = info.layout;
    core->underrunFill = info.underrunFill;
    core->channels = info.channels.size();
    core->bytesPerSample = bps;
    core->frameBytes = core->channels * bps;

    if (options.external.data != nullptr) {
        const AcqExternalBuffer& ext = options.external;
        if (reinterpret_cast<std::uintptr_t>(ext.data) % options.alignment != 0 || core->channels == 0 ||
            (ext.memory.kind != AcqMemoryKind::Host && ext.memory.device < 0)) {
            return AcqErrorCode::InvalidBuffer;
        }
        std::uint64_t capacity = 0;
        if (info.layout == AcqLayout::Interleaved) {
            capacity = ext.bytes / core->frameBytes;
            if (capacity * core->frameBytes != ext.bytes) {
                return AcqErrorCode::InvalidBuffer;
            }
        } else {
            core->rowStride = ext.channelStride != 0 ? ext.channelStride
                                                     : static_cast<std::ptrdiff_t>(ext.bytes / core->channels);
            capacity = options.capacitySamples != 0 ? options.capacitySamples
                                                    : static_cast<std::uint64_t>(core->rowStride) / bps;
            if (capacity * bps > static_cast<std::uint64_t>(core->rowStride) ||
                static_cast<std::uint64_t>(core->rowStride) * core->channels > ext.bytes) {
                return AcqErrorCode::InvalidBuffer;
            }
        }
        if (capacity < 2 || !std::has_single_bit(capacity)) {
            return AcqErrorCode::InvalidBuffer;
        }
        if (options.capacitySamples != 0 && options.capacitySamples != capacity) {
            return AcqErrorCode::InvalidParameter;
        }
        core->capacity = capacity;
        core->base = ext.data;
        core->memory = ext.memory;
        core->bufferBytes = ext.bytes;
        core->releaseExternal = ext.release;
    } else {
        std::uint64_t capacity = 0;
        if (options.capacitySamples != 0) {
            capacity = nextPow2(std::max<std::uint64_t>(options.capacitySamples, 2));
        } else {
            capacity = nextPow2(std::max<std::uint64_t>(65536, static_cast<std::uint64_t>(std::ceil(2.0 * info.sfreq))));
            while (capacity > 1024 && capacity * core->frameBytes > kDefaultMaxRingBytes) {
                capacity /= 2;
            }
        }
        core->capacity = capacity;
        if (info.layout == AcqLayout::Planar) {
            const std::uint64_t row = capacity * bps;
            const std::uint64_t align = options.rowAlignment;
            core->rowStride = static_cast<std::ptrdiff_t>((row + align - 1) / align * align);
            core->bufferBytes = static_cast<std::size_t>(core->rowStride) * core->channels;
        } else {
            core->bufferBytes = static_cast<std::size_t>(capacity * core->frameBytes);
        }
        if (core->bufferBytes > 0) {
            // Only interleaved rings mirror: a planar ring would need every channel row mirrored on its own.
            core->ownedMemory = allocateRingMemory(core->bufferBytes, options.alignment, options.hugePages,
                                                   options.numaNode,
                                                   options.mirror && info.layout == AcqLayout::Interleaved);
            if (core->ownedMemory.data == nullptr) {
                return AcqErrorCode::OutOfMemory;
            }
            core->base = core->ownedMemory.data;
            if (options.prefault) {
                // Touch the mirror too, so its page-table entries exist before start().
                std::memset(core->base, 0, core->ownedMemory.bytes * (core->ownedMemory.mirrored ? 2 : 1));
            }
            if (options.lockMemory) {
                lockRingMemory(core->ownedMemory);
            }
        }
    }
    core->mask = core->capacity - 1;

    initRecords(core->events, options.eventCapacity, 0);
    if (core->direction == AcqDirection::Output) {
        initRecords(core->outputCommits, kOutputCommitLog, 0);
    }
    if (info.segmentation.mode == AcqSegmentation::Mode::Triggered) {
        const std::uint64_t perSegment = std::max<std::uint64_t>(info.segmentation.samplesPerSegment, 1);
        const std::size_t count = options.segmentCapacity != 0
            ? options.segmentCapacity
            : static_cast<std::size_t>(std::max<std::uint64_t>(64, core->capacity / perSegment));
        initRecords(core->segments, count, info.segmentation.headerBytes);
    } else {
        initRecords(core->segments, 2, 0);
    }

    AcqClockModel clock;
    clock.sfreq = info.sfreq;
    clock.output = info.direction == AcqDirection::Output;
    core->clock.store(clock);

    core->maxReaders = options.maxReaders;
    core->slots = std::make_unique<ReaderSlot[]>(options.maxReaders);
    core->versions[0] = makeVersion(std::move(info), 0, 0);
    core->versionCount.store(1, std::memory_order_release);
    return core;
}

//=============================================================================================================

AcqStreamCore::~AcqStreamCore()
{
    if (ownedMemory.data != nullptr) {
        releaseRingMemory(ownedMemory);
    } else if (releaseExternal) {
        releaseExternal();
    }
}

//=============================================================================================================

std::size_t AcqStreamCore::versionAt(std::uint64_t sample) const noexcept
{
    const std::size_t count = versionCount.load(std::memory_order_acquire);
    std::size_t index = count - 1;
    while (index > 0 && versions[index]->validFrom > sample) {
        --index;
    }
    return index;
}

//=============================================================================================================

std::byte* AcqStreamCore::sampleAddress(std::uint64_t sample) const noexcept
{
    if (base == nullptr) {
        return nullptr;
    }
    const std::uint64_t index = sample & mask;
    return layout == AcqLayout::Interleaved ? base + index * frameBytes : base + index * bytesPerSample;
}

//=============================================================================================================

void AcqStreamCore::reserveUpTo(std::uint64_t end) noexcept
{
    if (end > reserved.load(std::memory_order_relaxed)) {
        reserved.store(end, std::memory_order_relaxed);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

//=============================================================================================================

void AcqStreamCore::fillRange(std::uint64_t from, std::uint64_t to) noexcept
{
    if (to <= from || base == nullptr) {
        return;
    }
    if (to - from > capacity) {
        from = to - capacity;
    }
    reserveUpTo(to);
    if (!memory.hostAccessible()) {
        return; // the device emits its fill itself
    }
    std::array<AcqWritePart, 2> parts;
    const std::size_t n = buildParts(from, to - from, parts);
    const bool hold = underrunFill == AcqUnderrunFill::HoldLast && from > 0;
    std::array<std::byte, 16> zero{};
    for (std::size_t p = 0; p < n; ++p) {
        for (std::size_t c = 0; c < channels; ++c) {
            const std::byte* src = zero.data();
            if (hold) {
                const std::uint64_t last = (from - 1) & mask;
                src = layout == AcqLayout::Interleaved ? base + last * frameBytes + c * bytesPerSample
                                                       : base + static_cast<std::ptrdiff_t>(c) * rowStride +
                        static_cast<std::ptrdiff_t>(last * bytesPerSample);
            }
            for (std::size_t s = 0; s < parts[p].samples; ++s) {
                std::byte* dst = parts[p].at(c, s);
                if (dst != src) {
                    std::memcpy(dst, src, bytesPerSample);
                }
            }
        }
    }
}

//=============================================================================================================

void AcqStreamCore::notifyReaders() noexcept
{
    if (waiters.load(std::memory_order_seq_cst) == 0) {
        return;
    }
    for (std::size_t i = 0; i < maxReaders; ++i) {
        ReaderSlot& slot = slots[i];
        if (slot.waiting.load(std::memory_order_seq_cst) && slot.waiting.exchange(false, std::memory_order_acq_rel)) {
            slot.wake.release();
        }
    }
}

//=============================================================================================================

void AcqStreamCore::notifyWriter() noexcept
{
    if (writerWaiting.load(std::memory_order_seq_cst) && writerWaiting.exchange(false, std::memory_order_acq_rel)) {
        writerWake.release();
    }
}

//=============================================================================================================
// DEFINE MEMBER METHODS AcqStream
//=============================================================================================================

AcqStream::AcqStream() noexcept = default;

AcqStream::AcqStream(std::shared_ptr<AcqStreamCore> core) noexcept
: m_core(std::move(core))
{
}

bool AcqStream::isValid() const noexcept
{
    return m_core != nullptr;
}

AcqDirection AcqStream::direction() const noexcept
{
    return m_core ? m_core->direction : AcqDirection::Input;
}

std::shared_ptr<const AcqStreamInfo> AcqStream::info() const
{
    if (!m_core) {
        return {};
    }
    return m_core->versions[m_core->versionCount.load(std::memory_order_acquire) - 1]->info;
}

std::shared_ptr<const AcqStreamInfo> AcqStream::infoAt(std::uint64_t sample) const
{
    return m_core ? m_core->versions[m_core->versionAt(sample)]->info : nullptr;
}

AcqClockModel AcqStream::clock() const noexcept
{
    return m_core ? m_core->clock.load() : AcqClockModel();
}

AcqMemoryLocation AcqStream::memory() const noexcept
{
    return m_core ? m_core->memory : AcqMemoryLocation{};
}

//=============================================================================================================

AcqTimebase AcqStream::timebase() const noexcept
{
    return m_core ? m_core->timebase.load() : AcqTimebase();
}

void AcqStream::setTimebase(const AcqTimebase& timebase) noexcept
{
    if (m_core) {
        std::lock_guard<ControlMutex> lock(m_core->controlMutex);
        m_core->timebase.store(timebase);
    }
}

AcqStreamStats AcqStream::stats() const noexcept
{
    AcqStreamStats s;
    if (!m_core) {
        return s;
    }
    const AcqStreamCore& c = *m_core;
    s.committed = c.committed.load(std::memory_order_acquire);
    s.commits = c.commits.load(std::memory_order_relaxed);
    s.events = c.events.reserved.load(std::memory_order_relaxed);
    s.segments = c.segments.reserved.load(std::memory_order_relaxed);
    s.underrun = c.underrun.load(std::memory_order_relaxed);
    s.underruns = c.underruns.load(std::memory_order_relaxed);
    s.writerWaits = c.writerWaits.load(std::memory_order_relaxed);
    s.lateEvents = c.lateEvents.load(std::memory_order_relaxed);
    s.lateCommits = c.lateCommits.load(std::memory_order_relaxed);
    s.capacitySamples = static_cast<std::size_t>(c.capacity);
    for (std::size_t i = 0; i < c.maxReaders; ++i) {
        s.activeReaders += c.slots[i].inUse.load(std::memory_order_acquire) ? 1U : 0U;
    }
    s.memoryLocked = c.ownedMemory.locked;
    s.mirrored = c.ownedMemory.mirrored;
    s.adoptedBuffer = c.ownedMemory.data == nullptr && c.base != nullptr;
    s.hugePages = c.ownedMemory.hugePages;
    s.numaNode = c.ownedMemory.numaNode;
    return s;
}

AcqExpected<AcqReader> AcqStream::reader(AcqReader::ReadMode mode, const AcqReaderOptions& options) const
{
    if (!m_core) {
        return AcqErrorCode::InvalidState;
    }
    AcqStreamCore& c = *m_core;
    std::lock_guard<ControlMutex> lock(c.controlMutex);
    if (options.gating && c.gatingSlot.load(std::memory_order_acquire) >= 0) {
        return AcqErrorCode::InvalidState;
    }
    for (std::size_t i = 0; i < c.maxReaders; ++i) {
        ReaderSlot& slot = c.slots[i];
        if (slot.inUse.load(std::memory_order_acquire)) {
            continue;
        }
        slot.mode = mode;
        slot.options = options;
        slot.gating = options.gating;
        slot.holding = false;
        slot.pendingLost = 0;
        slot.clockSeq = SeqlockValue<AcqClockModel>::kSeqNone;
        slot.timebaseSeq = SeqlockValue<AcqTimebase>::kSeqNone;
        slot.waiting.store(false, std::memory_order_relaxed);
        std::uint64_t start = c.committed.load(std::memory_order_acquire);
        if (options.gating && c.direction == AcqDirection::Output) {
            start = c.gatePosition.load(std::memory_order_acquire);
        } else if (options.startAtOldest) {
            const std::uint64_t r = c.reserved.load(std::memory_order_acquire);
            start = r > c.capacity ? r - c.capacity : 0;
        }
        slot.position.store(start, std::memory_order_relaxed);
        slot.eventCursor = c.events.oldest();
        slot.segmentCursor = c.segments.oldest();
        for (auto* counter : {&slot.delivered, &slot.lost, &slot.skipped, &slot.idle, &slot.overruns, &slot.views,
                              &slot.eventsLost, &slot.segmentsLost}) {
            counter->store(0, std::memory_order_relaxed);
        }
        if (options.gating) {
            slot.commitCursor = c.outputCommits.published.load(std::memory_order_acquire);
            slot.dropEnd = 0;
            c.commitsValidated.store(slot.commitCursor, std::memory_order_seq_cst);
            c.gatePosition.store(start, std::memory_order_seq_cst);
            c.deviceGone.store(false, std::memory_order_seq_cst);
            c.gatingSlot.store(static_cast<int>(i), std::memory_order_seq_cst);
        }
        slot.inUse.store(true, std::memory_order_release);
        return AcqReader(m_core, static_cast<std::uint32_t>(i));
    }
    return AcqErrorCode::TooManyReaders;
}

AcqExpected<AcqWriter> AcqStream::writer(AcqWait wait) const
{
    if (!m_core) {
        return AcqErrorCode::InvalidState;
    }
    if (m_core->direction != AcqDirection::Output) {
        return AcqErrorCode::WrongDirection;
    }
    if (m_core->writerTaken.exchange(true, std::memory_order_acq_rel)) {
        return AcqErrorCode::WriterInUse;
    }
    m_core->writerWait = wait;
    return AcqWriter(m_core);
}

//=============================================================================================================
// DEFINE MEMBER METHODS AcqWriter
//=============================================================================================================

AcqExpected<AcqWriter> AcqWriter::create(AcqStreamInfo info, const AcqStreamOptions& options)
{
    auto core = AcqStreamCore::create(std::move(info), options);
    if (!core) {
        return core.error();
    }
    (*core)->writerTaken.store(true, std::memory_order_release);
    return AcqWriter(std::move(*core));
}

AcqWriter::AcqWriter() noexcept = default;

AcqWriter::AcqWriter(std::shared_ptr<AcqStreamCore> core) noexcept
: m_core(std::move(core))
{
}

AcqWriter::~AcqWriter()
{
    finish();
}

AcqWriter::AcqWriter(AcqWriter&& other) noexcept = default;

AcqWriter& AcqWriter::operator=(AcqWriter&& other) noexcept
{
    if (this != &other) {
        finish();
        m_core = std::move(other.m_core);
    }
    return *this;
}

AcqStream AcqWriter::stream() const noexcept
{
    return AcqStream(m_core);
}

std::uint64_t AcqWriter::writePosition() const noexcept
{
    return m_core ? m_core->writePos : 0;
}

std::uint64_t AcqWriter::devicePosition() const noexcept
{
    if (!m_core) {
        return 0;
    }
    return m_core->direction == AcqDirection::Output ? m_core->gatePosition.load(std::memory_order_acquire)
                                                     : m_core->writePos;
}

std::size_t AcqWriter::space() const noexcept
{
    if (!m_core) {
        return 0;
    }
    const AcqStreamCore& c = *m_core;
    if (!c.gated()) {
        return static_cast<std::size_t>(c.capacity);
    }
    const std::uint64_t gate = c.gatePosition.load(std::memory_order_acquire);
    return c.writePos > gate ? static_cast<std::size_t>(c.capacity - (c.writePos - gate))
                             : static_cast<std::size_t>(c.capacity);
}

std::uint64_t AcqWriter::earliestWritableSample() const noexcept
{
    if (!m_core) {
        return 0;
    }
    return std::max(m_core->writePos, devicePosition());
}

namespace
{
// Output: moves the write position past samples the device already emitted as fill and accounts the underrun.
std::uint64_t catchUpWithDevice(AcqStreamCore& c) noexcept
{
    const std::uint64_t idle = c.idleTotal.load(std::memory_order_acquire);
    const std::uint64_t underrun = idle - c.idleSeen;
    c.idleSeen = idle;
    if (underrun > 0) {
        c.underrun.fetch_add(underrun, std::memory_order_relaxed);
        c.underruns.fetch_add(1, std::memory_order_relaxed);
    }
    const std::uint64_t gate = c.gatePosition.load(std::memory_order_acquire);
    if (gate > c.writePos) {
        c.fillRange(c.writePos, gate);
        c.writePos = gate;
        c.committed.store(gate, std::memory_order_seq_cst);
        c.notifyReaders();
    }
    return underrun;
}

AcqError waitForGate(AcqStreamCore& c, std::uint64_t end, std::chrono::nanoseconds timeout) noexcept
{
    auto fits = [&c, end] {
        const std::uint64_t gate = c.gatePosition.load(std::memory_order_seq_cst);
        const bool logSpace = c.outputCommits.pushed - c.commitsValidated.load(std::memory_order_seq_cst) <
            c.outputCommits.capacity();
        return logSpace && (gate >= end || end - gate <= c.capacity);
    };
    if (fits()) {
        return {};
    }
    if (c.deviceGone.load(std::memory_order_acquire)) {
        return AcqErrorCode::EndOfStream;
    }
    c.writerWaits.fetch_add(1, std::memory_order_relaxed);
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    auto ready = [&] {
        return fits() || c.deviceGone.load(std::memory_order_acquire);
    };
    if (!waitFor(ready, c.writerWait, std::chrono::microseconds(50), deadline, c.writerWaiting, c.writerWake,
                 nullptr)) {
        return AcqErrorCode::Timeout;
    }
    return fits() ? AcqError() : AcqError(AcqErrorCode::EndOfStream);
}

// Input: waits while a gating reader has not consumed what reserving up to @p end would overwrite.
AcqError waitForInputSpace(AcqStreamCore& c, std::uint64_t end, std::chrono::nanoseconds timeout) noexcept
{
    if (!c.gated()) {
        return {};
    }
    const AcqError e = waitForGate(c, end, timeout);
    return e.code() == AcqErrorCode::EndOfStream ? AcqError() : e; // the gating reader left: nothing to wait for
}

AcqWriteView makeWriteView(AcqStreamCore& c, std::uint64_t first, std::uint64_t count, std::uint64_t underrun) noexcept
{
    c.reserveUpTo(first + count);
    c.pendingFirst = first;
    c.pendingCount = count;
    AcqWriteView view;
    ViewAccess::setSamples(view, c, first, count, *c.versions[c.versionAt(first)], c.clock.load(), c.timebase.load());
    ViewAccess::setUnderrun(view, underrun);
    return view;
}
} // namespace

AcqExpected<AcqWriteView> AcqWriter::beginWrite(std::size_t samples, std::chrono::nanoseconds timeout) noexcept
{
    if (!m_core) {
        return AcqErrorCode::InvalidState;
    }
    AcqStreamCore& c = *m_core;
    if (samples > c.capacity) {
        return AcqErrorCode::InvalidParameter;
    }
    std::uint64_t underrun = 0;
    if (c.direction == AcqDirection::Output) {
        underrun = catchUpWithDevice(c);
        if (AcqError e = waitForGate(c, c.writePos + samples, timeout); !e.ok()) {
            return e;
        }
    } else if (AcqError e = waitForInputSpace(c, c.writePos + samples, timeout); !e.ok()) {
        return e;
    }
    return makeWriteView(c, c.writePos, samples, underrun);
}

AcqError AcqWriter::waitForSpace(std::size_t samples, std::chrono::nanoseconds timeout) noexcept
{
    if (!m_core) {
        return AcqErrorCode::InvalidState;
    }
    AcqStreamCore& c = *m_core;
    if (samples > c.capacity) {
        return AcqErrorCode::InvalidParameter;
    }
    if (c.direction == AcqDirection::Output) {
        return waitForGate(c, std::max(c.writePos, c.gatePosition.load(std::memory_order_acquire)) + samples,
                           timeout);
    }
    return waitForInputSpace(c, c.writePos + samples, timeout);
}

AcqExpected<AcqWriteView> AcqWriter::beginWriteAt(std::uint64_t firstSample, std::size_t samples,
                                                  std::chrono::nanoseconds timeout) noexcept
{
    if (!m_core) {
        return AcqErrorCode::InvalidState;
    }
    AcqStreamCore& c = *m_core;
    if (c.direction != AcqDirection::Output) {
        return AcqErrorCode::WrongDirection;
    }
    const std::uint32_t alignment = c.timebase.load().sampleAlignment;
    if (alignment > 1 && firstSample % alignment != 0) {
        return AcqErrorCode::NotOnGrid;
    }
    if (firstSample < c.gatePosition.load(std::memory_order_acquire)) {
        return AcqErrorCode::Late;
    }
    // The device may have played fill far past writePos; the range check refers to the caught-up position.
    const std::uint64_t underrun = catchUpWithDevice(c);
    if (firstSample < c.writePos || firstSample + samples - c.writePos > c.capacity) {
        return AcqErrorCode::InvalidParameter;
    }
    if (AcqError e = waitForGate(c, firstSample + samples, timeout); !e.ok()) {
        return e;
    }
    c.fillRange(c.writePos, firstSample);
    return makeWriteView(c, firstSample, samples, underrun);
}

AcqError AcqWriter::grant(std::size_t samples) noexcept
{
    if (!m_core) {
        return AcqErrorCode::InvalidState;
    }
    AcqStreamCore& c = *m_core;
    if (c.direction != AcqDirection::Input) {
        return AcqErrorCode::WrongDirection;
    }
    const std::uint64_t end = std::max(c.grantedEnd, c.writePos) + samples;
    if (end - c.writePos > c.capacity) {
        return AcqErrorCode::InvalidParameter;
    }
    if (AcqError e = waitForInputSpace(c, end, std::chrono::nanoseconds(0)); !e.ok()) {
        return e;
    }
    c.grantedEnd = end;
    c.reserveUpTo(end);
    return {};
}

void AcqWriter::commit(std::size_t samples) noexcept
{
    if (!m_core) {
        return;
    }
    AcqStreamCore& c = *m_core;
    if (c.pendingCount == 0 && c.grantedEnd > c.writePos) {
        c.pendingFirst = c.writePos;
        c.pendingCount = c.grantedEnd - c.writePos;
    }
    const std::uint64_t n = std::min<std::uint64_t>(samples, c.pendingCount);
    const std::uint64_t end = c.pendingFirst + n;
    c.pendingCount = 0;
    if (end <= c.writePos && n == 0) {
        return;
    }
    c.writePos = std::max(c.writePos, end);
    if (c.direction == AcqDirection::Output && n > 0) {
        // Published before the samples, so the device sees the scheduled start of every block it can play.
        pushRecord(c.outputCommits, OutputCommit{c.pendingFirst, end}, {}, true);
    }
    if (c.direction == AcqDirection::Input) {
        c.events.published.store(c.events.pushed, std::memory_order_release);
        c.segments.published.store(c.segments.pushed, std::memory_order_release);
    }
    c.committed.store(c.writePos, std::memory_order_seq_cst);
    addOwned(c.commits, 1);
    c.notifyReaders();
}

AcqError AcqWriter::pushEvent(const AcqEvent& event) noexcept
{
    if (!m_core) {
        return AcqErrorCode::InvalidState;
    }
    AcqStreamCore& c = *m_core;
    const bool output = c.direction == AcqDirection::Output;
    if (output && event.sample < c.gatePosition.load(std::memory_order_acquire)) {
        c.lateEvents.fetch_add(1, std::memory_order_relaxed);
        return AcqErrorCode::Late;
    }
    if (c.events.pushed > 0 && event.sample < c.lastEventSample) {
        return AcqErrorCode::InvalidParameter;
    }
    c.lastEventSample = event.sample;
    pushRecord(c.events, event, {}, output);
    return {};
}

AcqError AcqWriter::pushSegment(const AcqSegmentHeader& header, std::span<const std::byte> payload) noexcept
{
    if (!m_core) {
        return AcqErrorCode::InvalidState;
    }
    AcqStreamCore& c = *m_core;
    const bool output = c.direction == AcqDirection::Output;
    if (output && header.firstSample < c.gatePosition.load(std::memory_order_acquire)) {
        c.lateEvents.fetch_add(1, std::memory_order_relaxed);
        return AcqErrorCode::Late;
    }
    if (c.segments.pushed > 0 && header.firstSample < c.lastSegmentSample) {
        return AcqErrorCode::InvalidParameter;
    }
    c.lastSegmentSample = header.firstSample;
    pushRecord(c.segments, header, payload, output);
    return {};
}

void AcqWriter::setClock(const AcqClockModel& clock) noexcept
{
    if (m_core) {
        m_core->clock.store(clock);
    }
}

AcqError AcqWriter::updateInfo(AcqStreamInfo info, std::uint64_t fromSample)
{
    if (!m_core) {
        return AcqErrorCode::InvalidState;
    }
    AcqStreamCore& c = *m_core;
    std::lock_guard<ControlMutex> lock(c.controlMutex);
    const std::size_t count = c.versionCount.load(std::memory_order_acquire);
    const AcqStreamInfo& current = *c.versions[count - 1]->info;
    if (fromSample < c.writePos || fromSample < c.versions[count - 1]->validFrom ||
        info.channels.size() != current.channels.size() || info.format != current.format ||
        info.layout != current.layout || info.direction != current.direction) {
        return AcqError(AcqErrorCode::InvalidParameter, "updateInfo may not change the ring geometry or the past");
    }
    if (count == kMaxInfoVersions) {
        return AcqError(AcqErrorCode::InvalidState, "too many metadata versions");
    }
    c.versions[count] = makeVersion(std::move(info), current.version + 1, fromSample);
    c.versionCount.store(count + 1, std::memory_order_release);
    return {};
}

void AcqWriter::finish() noexcept
{
    if (!m_core || m_core->finished.exchange(true, std::memory_order_seq_cst)) {
        return;
    }
    m_core->events.published.store(m_core->events.pushed, std::memory_order_release);
    m_core->segments.published.store(m_core->segments.pushed, std::memory_order_release);
    m_core->notifyReaders();
}
