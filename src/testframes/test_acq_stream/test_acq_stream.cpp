//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_acq_stream.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests for the lock-free stream ring: AcqWriter, AcqStream, AcqReader and the views, in both directions.
 *
 * Samples carry their own index (value(s, c)), so every read checks position and content. A replaced global
 * operator new counts heap allocations to prove the data path allocates nothing after set-up.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq_stream.h>
#include <acq/detail/acq_diagnostics.h>

#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <malloc.h>
#include <windows.h>
#else
#include <sys/mman.h>
#if defined(__linux__)
#include <sys/syscall.h>
#include <unistd.h>
#endif
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

//=============================================================================================================
// COUNTING ALLOCATOR
//=============================================================================================================

namespace
{
std::atomic<bool> g_countAllocations{false};
std::atomic<std::size_t> g_allocations{0};

void* countedAlloc(std::size_t size, std::size_t alignment)
{
    if (g_countAllocations.load(std::memory_order_relaxed)) {
        g_allocations.fetch_add(1, std::memory_order_relaxed);
    }
    size = size == 0 ? 1 : size;
#if defined(_WIN32)
    // Default-aligned blocks must come from malloc: the DLLs free what they did not allocate with free().
    void* p = alignment <= alignof(std::max_align_t) ? std::malloc(size) : _aligned_malloc(size, alignment);
#else
    void* p = nullptr;
    if (alignment <= alignof(std::max_align_t)) {
        p = std::malloc(size);
    } else {
        p = std::aligned_alloc(alignment, (size + alignment - 1) / alignment * alignment);
    }
#endif
    return p;
}

void* countedAllocOrThrow(std::size_t size, std::size_t alignment)
{
    void* p = countedAlloc(size, alignment);
    if (p == nullptr) {
        throw std::bad_alloc();
    }
    return p;
}

void countedFree(void* p) noexcept
{
    std::free(p);
}

void countedAlignedFree(void* p) noexcept
{
#if defined(_WIN32)
    _aligned_free(p);
#else
    std::free(p);
#endif
}
} // namespace

void* operator new(std::size_t size)
{
    return countedAllocOrThrow(size, alignof(std::max_align_t));
}
void* operator new[](std::size_t size)
{
    return countedAllocOrThrow(size, alignof(std::max_align_t));
}
void* operator new(std::size_t size, std::align_val_t al)
{
    return countedAllocOrThrow(size, static_cast<std::size_t>(al));
}
void* operator new[](std::size_t size, std::align_val_t al)
{
    return countedAllocOrThrow(size, static_cast<std::size_t>(al));
}
void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    return countedAlloc(size, alignof(std::max_align_t));
}
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
    return countedAlloc(size, alignof(std::max_align_t));
}
void* operator new(std::size_t size, std::align_val_t al, const std::nothrow_t&) noexcept
{
    return countedAlloc(size, static_cast<std::size_t>(al));
}
void* operator new[](std::size_t size, std::align_val_t al, const std::nothrow_t&) noexcept
{
    return countedAlloc(size, static_cast<std::size_t>(al));
}
void operator delete(void* p) noexcept
{
    countedFree(p);
}
void operator delete[](void* p) noexcept
{
    countedFree(p);
}
void operator delete(void* p, std::size_t) noexcept
{
    countedFree(p);
}
void operator delete[](void* p, std::size_t) noexcept
{
    countedFree(p);
}
void operator delete(void* p, std::align_val_t) noexcept
{
    countedAlignedFree(p);
}
void operator delete[](void* p, std::align_val_t) noexcept
{
    countedAlignedFree(p);
}
void operator delete(void* p, std::size_t, std::align_val_t) noexcept
{
    countedAlignedFree(p);
}
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept
{
    countedAlignedFree(p);
}
void operator delete(void* p, const std::nothrow_t&) noexcept
{
    countedFree(p);
}
void operator delete[](void* p, const std::nothrow_t&) noexcept
{
    countedFree(p);
}
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept
{
    countedAlignedFree(p);
}
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept
{
    countedAlignedFree(p);
}

//=============================================================================================================
// HELPERS
//=============================================================================================================

namespace
{
std::int32_t value(std::uint64_t sample, std::size_t channel)
{
    return static_cast<std::int32_t>((sample * 16 + channel) & 0x7FFFFFFFULL);
}

AcqStreamInfo makeInfo(std::size_t channels, AcqLayout layout = AcqLayout::Interleaved,
                       AcqDirection direction = AcqDirection::Input)
{
    AcqStreamInfo info;
    info.name = "test";
    info.direction = direction;
    info.format = AcqSampleFormat::Int32;
    info.layout = layout;
    info.sfreq = 1000.0;
    for (std::size_t c = 0; c < channels; ++c) {
        AcqChannelInfo ch;
        ch.name = "ch" + std::to_string(c);
        ch.kind = AcqChannelKind::Eeg;
        ch.unit = AcqUnit::Volt;
        info.channels.push_back(ch);
    }
    return info;
}

AcqStreamOptions options(std::size_t capacity, std::size_t maxReaders = 8)
{
    AcqStreamOptions o;
    o.capacitySamples = capacity;
    o.maxReaders = maxReaders;
    o.lockMemory = false;
    return o;
}

// Relaxed atomic element access: readers may check a view the producer is overwriting (release() then reports
// Overrun), and that check must not itself be a data race.
std::atomic_ref<std::int32_t> element(const std::byte* p)
{
    return std::atomic_ref<std::int32_t>(*reinterpret_cast<std::int32_t*>(const_cast<std::byte*>(p)));
}

void fill(const AcqWriteView& view)
{
    std::uint64_t sample = view.firstSample();
    for (std::size_t p = 0; p < view.partCount(); ++p) {
        const AcqWritePart& part = view.part(p);
        for (std::size_t s = 0; s < part.samples; ++s, ++sample) {
            for (std::size_t c = 0; c < view.channelCount(); ++c) {
                element(part.at(c, s)).store(value(sample, c), std::memory_order_relaxed);
            }
        }
    }
}

bool matches(const AcqBlockView& view)
{
    std::uint64_t sample = view.firstSample();
    for (std::size_t p = 0; p < view.partCount(); ++p) {
        const AcqBlockPart& part = view.part(p);
        for (std::size_t s = 0; s < part.samples; ++s, ++sample) {
            for (std::size_t c = 0; c < view.channelCount(); ++c) {
                const std::int32_t v = element(part.at(c, s)).load(std::memory_order_relaxed);
                if (v != value(sample, c)) {
                    return false;
                }
            }
        }
    }
    return true;
}

/** An address range any CPU access to faults, standing in for GPU memory the library must never touch. */
class NoAccessRegion
{
public:
    explicit NoAccessRegion(std::size_t bytes)
    : m_bytes(bytes)
    {
#if defined(_WIN32)
        m_data = static_cast<std::byte*>(VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS));
#else
        void* p = mmap(nullptr, bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        m_data = p == MAP_FAILED ? nullptr : static_cast<std::byte*>(p);
#endif
    }

    ~NoAccessRegion()
    {
#if defined(_WIN32)
        VirtualFree(m_data, 0, MEM_RELEASE);
#else
        munmap(m_data, m_bytes);
#endif
    }

    NoAccessRegion(const NoAccessRegion&) = delete;
    NoAccessRegion& operator=(const NoAccessRegion&) = delete;

    [[nodiscard]] std::byte* data() const noexcept
    {
        return m_data;
    }

private:
    std::byte* m_data = nullptr;
    std::size_t m_bytes = 0;
};

/** Page-aligned host memory for adopted buffers, released by the ring. */
AcqExternalBuffer hostBuffer(std::size_t bytes, std::size_t alignment = 4096)
{
    AcqExternalBuffer buffer;
    buffer.data = static_cast<std::byte*>(::operator new(bytes, std::align_val_t(alignment)));
    buffer.bytes = bytes;
    buffer.release = [data = buffer.data, alignment] {
        ::operator delete(data, std::align_val_t(alignment));
    };
    return buffer;
}

/**
 * A PCIe card's DMA engine: writes every granted sample into the adopted ring on its own thread and reports
 * its progress, like a card's "bytes available" register. The host never touches the samples.
 */
class DmaEngineSimulator
{
public:
    DmaEngineSimulator(std::byte* ring, std::size_t capacity, std::size_t channels, std::size_t block)
    : m_ring(ring)
    , m_capacity(capacity)
    , m_channels(channels)
    , m_block(block)
    {
        m_thread = std::thread([this] { run(); });
    }

    ~DmaEngineSimulator()
    {
        m_stop.store(true, std::memory_order_release);
        m_thread.join();
    }

    DmaEngineSimulator(const DmaEngineSimulator&) = delete;
    DmaEngineSimulator& operator=(const DmaEngineSimulator&) = delete;

    /** Host side: the card may write up to @p end. */
    void setGrantedEnd(std::uint64_t end) noexcept
    {
        m_granted.store(end, std::memory_order_release);
    }

    /** @return Samples the card has written so far. */
    [[nodiscard]] std::uint64_t written() const noexcept
    {
        return m_written.load(std::memory_order_acquire);
    }

private:
    void run()
    {
        std::uint64_t s = 0;
        while (!m_stop.load(std::memory_order_acquire)) {
            const std::uint64_t granted = m_granted.load(std::memory_order_acquire);
            if (s == granted) {
                std::this_thread::yield();
                continue;
            }
            const std::uint64_t end = std::min<std::uint64_t>(granted, s + m_block);
            for (; s < end; ++s) {
                std::byte* frame = m_ring + (s % m_capacity) * m_channels * sizeof(std::int32_t);
                for (std::size_t c = 0; c < m_channels; ++c) {
                    const std::int32_t v = value(s, c);
                    std::memcpy(frame + c * sizeof(v), &v, sizeof(v));
                }
            }
            m_written.store(s, std::memory_order_release);
        }
    }

    std::byte* m_ring;
    std::size_t m_capacity;
    std::size_t m_channels;
    std::size_t m_block;
    std::atomic<std::uint64_t> m_granted{0};
    std::atomic<std::uint64_t> m_written{0};
    std::atomic<bool> m_stop{false};
    std::thread m_thread;
};

/** Deterministic generator for randomised schedules (block sizes, pacing). */
struct Lcg
{
    std::uint64_t state;

    std::uint64_t next() noexcept
    {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return state >> 33U;
    }

    std::size_t below(std::size_t n) noexcept
    {
        return static_cast<std::size_t>(next() % n);
    }
};

constexpr std::uint64_t kEventEvery = 37;

std::int32_t eventCode(std::uint64_t sample)
{
    return static_cast<std::int32_t>((sample / kEventEvery) % 1000 + 1);
}

/** Whether every event of @p view is a pushed one of its sample range, in order and without duplicates. */
bool eventsValid(const AcqBlockView& view)
{
    const std::uint64_t end = view.firstSample() + view.sampleCount();
    std::uint64_t previous = 0;
    for (std::size_t i = 0; i < view.events().size(); ++i) {
        // The producer may be overwriting the event while the view is held; it stores records as 8-byte atomics.
        std::array<std::uint64_t, 2> words{};
        auto* raw = reinterpret_cast<std::uint64_t*>(const_cast<AcqEvent*>(&view.events()[i]));
        for (std::size_t k = 0; k < words.size(); ++k) {
            words[k] = std::atomic_ref<std::uint64_t>(raw[k]).load(std::memory_order_relaxed);
        }
        const auto event = std::bit_cast<AcqEvent>(words);
        if (event.sample < view.firstSample() || event.sample >= end || event.sample % kEventEvery != 0 ||
            event.code != eventCode(event.sample) || (i > 0 && event.sample <= previous)) {
            return false;
        }
        previous = event.sample;
    }
    return true;
}

bool produce(AcqWriter& writer, std::size_t samples)
{
    auto view = writer.beginWrite(samples);
    if (!view || view->sampleCount() != samples) {
        return false;
    }
    fill(*view);
    writer.commit(samples);
    return true;
}
} // namespace

//=============================================================================================================

class TestAcqStream : public QObject
{
    Q_OBJECT

private slots:
    void testCreateValidation();
    void testLosslessRoundTrip_data();
    void testLosslessRoundTrip();
    void testWrapAroundTwoParts_data();
    void testWrapAroundTwoParts();
    void testExactLossUnderStall();
    void testOverrunWhileHeld();
    void testInputBackpressure();
    void testLatestSkips();
    void testStartPositions();
    void testTimeoutAndEndOfStream();
    void testFinishWakesBlockedReader();
    void testLowLatencySpinWake();
    void testReaderSlotsAndMisuse();
    void testManyConcurrentReaders();
    void testSlowReaderLossAccounting();
    void testNoSilentLossOrCorruption_data();
    void testNoSilentLossOrCorruption();
    void testNoAllocationsOrLocksPerBlock();
    void testAdoptExternalBuffer();
    void testEvents();
    void testSegmentedComplexStream();
    void testClockAndTimebasePublished();
    void testInfoVersioning();
    void testOutputWriterWaitsForDevice();
    void testOutputUnderrun();
    void testOutputScheduledWrite();
    void testScheduledWriteAfterLongIdle();
    void testCommitOverPlayedFillIsDropped();
    void testOutputMonitorAndEvents();
    void testWriterHandles();
    void testInvalidHandles();
    void testExternalPlanarBuffer();
    void testDefaultCapacityLimitAndMemoryLock();
    void testHoldLastUnderrunFill();
    void testOutputDeviceGoneAndLimits();
    void testRecordOrdering();
    void testInfoVersionLimit();
    void testViewHelpers();
    void testDeviceMemoryNeverTouched();
    void testGrantDeviceOwnedWrites();
    void testDmaEngineSimulation();
    void testRingPlacement();
    void testMirroredRingSinglePart();
};

//=============================================================================================================

void TestAcqStream::testCreateValidation()
{
    auto rounded = AcqWriter::create(makeInfo(4), options(100));
    QVERIFY(rounded);
    QCOMPARE(rounded->stream().stats().capacitySamples, std::size_t(128));
    QCOMPARE(rounded->stream().direction(), AcqDirection::Input);
    QCOMPARE(makeInfo(4).frameBytes(), std::size_t(16));
    QCOMPARE(makeInfo(4).channelCount(), std::size_t(4));
    QVERIFY(rounded->stream().isValid());
    QVERIFY(!AcqStream().isValid());

    auto automatic = AcqWriter::create(makeInfo(4), options(0));
    QVERIFY(automatic);
    const std::size_t cap = automatic->stream().stats().capacitySamples;
    QVERIFY(cap >= 2000);
    QCOMPARE(cap & (cap - 1), std::size_t(0));

    QVERIFY(AcqWriter::create(makeInfo(4), options(64, 0)).error() == AcqErrorCode::InvalidParameter);
    AcqStreamInfo negative = makeInfo(4);
    negative.sfreq = -1.0;
    QVERIFY(AcqWriter::create(negative, options(64)).error() == AcqErrorCode::InvalidParameter);
    AcqStreamInfo badFormat = makeInfo(4);
    badFormat.format = static_cast<AcqSampleFormat>(99);
    QVERIFY(AcqWriter::create(badFormat, options(64)).error() == AcqErrorCode::UnsupportedFormat);

    auto eventsOnly = AcqWriter::create(makeInfo(0), options(64));
    QVERIFY(eventsOnly);
}

//=============================================================================================================

void TestAcqStream::testLosslessRoundTrip_data()
{
    QTest::addColumn<int>("layout");
    QTest::newRow("interleaved") << static_cast<int>(AcqLayout::Interleaved);
    QTest::newRow("planar") << static_cast<int>(AcqLayout::Planar);
}

void TestAcqStream::testLosslessRoundTrip()
{
    QFETCH(int, layout);
    auto writer = AcqWriter::create(makeInfo(5, static_cast<AcqLayout>(layout)), options(256));
    QVERIFY(writer);
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(reader);

    std::uint64_t next = 0;
    for (int block = 0; block < 20; ++block) {
        QVERIFY(produce(*writer, 37));
        auto view = reader->acquire(37, 0ns, 37);
        QVERIFY(view);
        QCOMPARE(view->firstSample(), next);
        QCOMPARE(view->sampleCount(), std::size_t(37));
        QCOMPARE(view->lost(), std::uint64_t(0));
        QVERIFY(matches(*view));
        QVERIFY(reader->release(*view).ok());
        next += 37;
    }
    const AcqReaderStats stats = reader->stats();
    QCOMPARE(stats.delivered, next);
    QCOMPARE(stats.position, next);
    QCOMPARE(stats.views, std::uint64_t(20));
    QCOMPARE(writer->stream().stats().committed, next);
    QCOMPARE(writer->stream().stats().commits, std::uint64_t(20));

    if (static_cast<AcqLayout>(layout) == AcqLayout::Planar) {
        QVERIFY(produce(*writer, 1));
        auto view = reader->acquire(1, 0ns);
        QVERIFY(view);
        QCOMPARE(view->part(0).channelStride % 64, std::ptrdiff_t(0));
        QCOMPARE(view->part(0).sampleStride, std::ptrdiff_t(4));
        QVERIFY(reader->release(*view).ok());
    }
}

//=============================================================================================================

void TestAcqStream::testWrapAroundTwoParts_data()
{
    testLosslessRoundTrip_data();
}

void TestAcqStream::testWrapAroundTwoParts()
{
    QFETCH(int, layout);
    AcqStreamOptions o = options(16);
    o.mirror = false;
    auto writer = AcqWriter::create(makeInfo(3, static_cast<AcqLayout>(layout)), o);
    QVERIFY(writer);
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(produce(*writer, 12));
    auto first = reader->acquire(12, 0ns);
    QVERIFY(first);
    QVERIFY(reader->release(*first).ok());

    auto wrapped = writer->beginWrite(8);
    QVERIFY(wrapped);
    QCOMPARE(wrapped->partCount(), std::size_t(2));
    fill(*wrapped);
    writer->commit(8);

    auto view = reader->acquire(8, 0ns);
    QVERIFY(view);
    QCOMPARE(view->firstSample(), std::uint64_t(12));
    QCOMPARE(view->partCount(), std::size_t(2));
    QCOMPARE(view->part(0).samples, std::size_t(4));
    QCOMPARE(view->part(1).samples, std::size_t(4));
    QVERIFY(matches(*view));
    QVERIFY(reader->release(*view).ok());
}

//=============================================================================================================

void TestAcqStream::testExactLossUnderStall()
{
    auto writer = AcqWriter::create(makeInfo(4), options(64));
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    for (int i = 0; i < 20; ++i) {
        QVERIFY(produce(*writer, 10));
    }
    QCOMPARE(reader->available(), std::size_t(64));
    auto view = reader->acquire(0, 0ns);
    QVERIFY(view);
    QCOMPARE(view->lost(), std::uint64_t(136));
    QCOMPARE(view->firstSample(), std::uint64_t(136));
    QCOMPARE(view->sampleCount(), std::size_t(64));
    QVERIFY(matches(*view));
    QVERIFY(reader->release(*view).ok());
    QCOMPARE(reader->stats().lost, std::uint64_t(136));
    QCOMPARE(reader->stats().delivered + reader->stats().lost, std::uint64_t(200));
}

//=============================================================================================================

void TestAcqStream::testOverrunWhileHeld()
{
    auto writer = AcqWriter::create(makeInfo(4), options(64));
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(produce(*writer, 64));
    auto view = reader->acquire(64, 0ns);
    QVERIFY(view);
    QVERIFY(produce(*writer, 10));
    QVERIFY(reader->release(*view) == AcqErrorCode::Overrun);
    QCOMPARE(reader->stats().overruns, std::uint64_t(1));
    QCOMPARE(reader->stats().lost, std::uint64_t(64));
    QCOMPARE(reader->stats().delivered, std::uint64_t(0));

    auto next = reader->acquire(0, 0ns);
    QVERIFY(next);
    QCOMPARE(next->firstSample(), std::uint64_t(64));
    QCOMPARE(next->lost(), std::uint64_t(0));
    QVERIFY(reader->release(*next).ok());

    // The same through leases: explicit release reports the overrun, scope exit releases, moves transfer.
    auto lease = reader->lease(0, 0ns);
    QVERIFY(lease && lease->isHeld() && (*lease)->sampleCount() == 0);
    QVERIFY(lease->release().ok());
    QVERIFY(!lease->isHeld() && lease->release() == AcqErrorCode::InvalidState);
    QVERIFY(produce(*writer, 64));
    auto overrun = reader->lease(64, 0ns);
    QVERIFY(overrun && (*overrun)->firstSample() == 74);
    QVERIFY(reader->lease(0, 0ns).error() == AcqErrorCode::InvalidState); // one view at a time
    QVERIFY(produce(*writer, 1));
    QVERIFY(overrun->release() == AcqErrorCode::Overrun);
    QCOMPARE(reader->stats().overruns, std::uint64_t(2));
    QCOMPARE(reader->stats().lost, std::uint64_t(128));
    {
        auto scoped = reader->lease(1, 0ns);
        QVERIFY(scoped && (*scoped)->sampleCount() == 1);
        AcqViewLease moved(std::move(*scoped));
        QVERIFY(!scoped->isHeld() && moved.isHeld());
        AcqReader movedReader = std::move(*reader);
        *reader = std::move(movedReader); // moving the reader keeps the lease valid
    }
    QCOMPARE(reader->stats().delivered, std::uint64_t(11));
    {
        QVERIFY(produce(*writer, 64));
        auto overrunOnExit = reader->lease(1, 0ns, 1);
        QVERIFY(overrunOnExit && (*overrunOnExit)->firstSample() == 139);
        AcqViewLease target;
        target = std::move(*overrunOnExit);
        QVERIFY(produce(*writer, 1));
    }
    QCOMPARE(reader->stats().delivered, std::uint64_t(11));
    QCOMPARE(reader->stats().overruns, std::uint64_t(3));
    QCOMPARE(reader->stats().lost, std::uint64_t(129));
    auto after = reader->acquire(0, 0ns);
    QVERIFY(after && after->firstSample() == 140);
    QVERIFY(reader->release(*after).ok());
}

//=============================================================================================================

void TestAcqStream::testInputBackpressure()
{
    auto writer = AcqWriter::create(makeInfo(3), options(64));
    AcqReaderOptions gating;
    gating.gating = true;
    auto gate = writer->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(gate);
    QVERIFY(writer->stream().reader(AcqReader::ReadMode::Lossless, gating).error() == AcqErrorCode::InvalidState);
    QVERIFY(gate->consumeIdle(1).code() == AcqErrorCode::WrongDirection);
    auto monitor = writer->stream().reader(AcqReader::ReadMode::Lossless);

    // The producer may fill the ring once, then waits for the gating reader instead of overwriting.
    QVERIFY(produce(*writer, 60));
    QCOMPARE(writer->space(), std::size_t(4));
    QVERIFY(writer->beginWrite(5, 0ns).error() == AcqErrorCode::Timeout);
    QVERIFY(writer->waitForSpace(5, 1ms).code() == AcqErrorCode::Timeout);
    QVERIFY(writer->grant(5).code() == AcqErrorCode::Timeout);
    QVERIFY(writer->waitForSpace(4, 0ns).ok());
    QVERIFY(produce(*writer, 4));
    auto view = gate->acquire(10, 0ns, 10);
    QVERIFY(view && view->firstSample() == 0);
    QVERIFY(writer->beginWrite(1, 0ns).error() == AcqErrorCode::Timeout); // held, not yet consumed
    QVERIFY(gate->release(*view).ok());
    QCOMPARE(writer->space(), std::size_t(10));
    QVERIFY(produce(*writer, 10));
    QCOMPARE(monitor->available(), std::size_t(64)); // a non-gating reader does not hold the producer back

    // Every sample arrives, in order and intact, however the producer and the consumer block.
    constexpr std::uint64_t kTotal = 200000;
    std::thread producer([&writer] {
        Lcg rng{7};
        while (writer->writePosition() < kTotal) {
            const auto n = std::min<std::size_t>(1 + rng.below(40), static_cast<std::size_t>(kTotal - writer->writePosition()));
            auto block = writer->beginWrite(n, 10s);
            if (!block) {
                return;
            }
            fill(*block);
            writer->commit(n);
        }
        writer->finish();
    });
    std::uint64_t next = 10;
    std::uint64_t corrupt = 0;
    Lcg rng{11};
    while (true) {
        auto lease = gate->lease(1, 10s, 1 + rng.below(50));
        if (!lease) {
            QVERIFY(lease.error() == AcqErrorCode::EndOfStream);
            break;
        }
        corrupt += (*lease)->firstSample() != next || (*lease)->lost() != 0 ? 1U : 0U;
        corrupt += matches(**lease) ? 0U : 1U;
        next = (*lease)->firstSample() + (*lease)->sampleCount();
        QVERIFY(lease->release().ok());
    }
    producer.join();
    QCOMPARE(corrupt, std::uint64_t(0));
    QCOMPARE(next, kTotal);
    QCOMPARE(gate->stats().lost, std::uint64_t(0));
    QCOMPARE(gate->stats().delivered, kTotal);
    auto late = monitor->acquire(1, 0ns); // the monitor never read, so it lost all but the last ring
    QVERIFY(late && late->lost() == kTotal - 64);
    QVERIFY(monitor->release(*late).ok());

    // A DMA grant respects the gating reader as well.
    auto dma = AcqWriter::create(makeInfo(1), options(64));
    auto dmaGate = dma->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(dma->grant(64).ok());
    dma->commit(64);
    QVERIFY(dma->grant(1).code() == AcqErrorCode::Timeout);
    auto first = dmaGate->acquire(1, 0ns, 1);
    QVERIFY(first && dmaGate->release(*first).ok());
    QVERIFY(dma->grant(1).ok());
    QVERIFY(dma->grant(1).code() == AcqErrorCode::Timeout);

    // Without its gating reader the producer no longer waits.
    auto free = AcqWriter::create(makeInfo(1), options(64));
    {
        auto onlyGate = free->stream().reader(AcqReader::ReadMode::Lossless, gating);
        QVERIFY(produce(*free, 64));
        std::thread release([&onlyGate] {
            std::this_thread::sleep_for(20ms);
            onlyGate = AcqReader();
        });
        QVERIFY(free->beginWrite(8, 10s));
        release.join();
    }
    free->commit(8);
    QVERIFY(produce(*free, 64));
}

//=============================================================================================================

void TestAcqStream::testLatestSkips()
{
    auto writer = AcqWriter::create(makeInfo(2), options(64));
    auto latest = writer->stream().reader(AcqReader::ReadMode::Latest);
    QVERIFY(produce(*writer, 50));
    auto view = latest->acquire(10, 0ns);
    QVERIFY(view);
    QCOMPARE(view->firstSample(), std::uint64_t(40));
    QCOMPARE(view->sampleCount(), std::size_t(10));
    QCOMPARE(view->skipped(), std::uint64_t(40));
    QCOMPARE(view->lost(), std::uint64_t(0));
    QVERIFY(matches(*view));
    QVERIFY(latest->release(*view).ok());

    for (int i = 0; i < 10; ++i) {
        QVERIFY(produce(*writer, 30));
    }
    auto again = latest->acquire(5, 0ns, 20);
    QVERIFY(again);
    QCOMPARE(again->firstSample(), std::uint64_t(330));
    QCOMPARE(again->sampleCount(), std::size_t(20));
    QCOMPARE(again->lost(), std::uint64_t(0));
    QVERIFY(latest->release(*again).ok());
    QCOMPARE(latest->stats().skipped, std::uint64_t(40 + 280));
    QCOMPARE(latest->stats().lost, std::uint64_t(0));
}

//=============================================================================================================

void TestAcqStream::testStartPositions()
{
    auto writer = AcqWriter::create(makeInfo(2), options(64));
    QVERIFY(produce(*writer, 60));
    QVERIFY(produce(*writer, 40));
    AcqReaderOptions oldest;
    oldest.startAtOldest = true;
    auto fromOldest = writer->stream().reader(AcqReader::ReadMode::Lossless, oldest);
    auto fromNewest = writer->stream().reader(AcqReader::ReadMode::Lossless);

    auto view = fromOldest->acquire(0, 0ns);
    QVERIFY(view);
    QCOMPARE(view->firstSample(), std::uint64_t(36));
    QCOMPARE(view->lost(), std::uint64_t(0));
    QVERIFY(fromOldest->release(*view).ok());

    QVERIFY(fromNewest->acquire(1, 0ns).error() == AcqErrorCode::Timeout);
    QVERIFY(produce(*writer, 1));
    auto fresh = fromNewest->acquire(1, 0ns);
    QVERIFY(fresh);
    QCOMPARE(fresh->firstSample(), std::uint64_t(100));
    QVERIFY(fromNewest->release(*fresh).ok());
}

//=============================================================================================================

void TestAcqStream::testTimeoutAndEndOfStream()
{
    auto writer = AcqWriter::create(makeInfo(2), options(64));
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    const auto start = std::chrono::steady_clock::now();
    QVERIFY(reader->acquire(1, 5ms).error() == AcqErrorCode::Timeout);
    QVERIFY(std::chrono::steady_clock::now() - start >= 5ms);
    QVERIFY(reader->acquire(65, 0ns).error() == AcqErrorCode::InvalidParameter);

    QVERIFY(produce(*writer, 3));
    writer->finish();
    writer->finish();
    auto rest = reader->acquire(10, 1s);
    QVERIFY(rest);
    QCOMPARE(rest->sampleCount(), std::size_t(3));
    QVERIFY(reader->release(*rest).ok());
    QVERIFY(reader->acquire(1, 1s).error() == AcqErrorCode::EndOfStream);
}

//=============================================================================================================

void TestAcqStream::testFinishWakesBlockedReader()
{
    auto writer = AcqWriter::create(makeInfo(2), options(64));
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    AcqErrorCode result = AcqErrorCode::None;
    std::thread consumer([&] { result = reader->acquire(1, 10s).error().code(); });
    std::this_thread::sleep_for(20ms);
    const auto start = std::chrono::steady_clock::now();
    writer->finish();
    consumer.join();
    QCOMPARE(result, AcqErrorCode::EndOfStream);
    QVERIFY(std::chrono::steady_clock::now() - start < 2s);
}

//=============================================================================================================

void TestAcqStream::testLowLatencySpinWake()
{
    auto writer = AcqWriter::create(makeInfo(2), options(64));
    auto reader = writer->stream().reader(AcqReader::ReadMode::LowLatency);
    QCOMPARE(reader->mode(), AcqReader::ReadMode::LowLatency);
    std::atomic<bool> ready{false};
    std::uint64_t first = 99;
    std::thread consumer([&] {
        ready = true;
        auto view = reader->acquire(1, 10s);
        if (view) {
            first = view->firstSample();
            reader->release(*view);
        }
    });
    while (!ready) {
        std::this_thread::yield();
    }
    std::this_thread::sleep_for(5ms);
    QVERIFY(produce(*writer, 1));
    consumer.join();
    QCOMPARE(first, std::uint64_t(0));
}

//=============================================================================================================

void TestAcqStream::testReaderSlotsAndMisuse()
{
    auto writer = AcqWriter::create(makeInfo(2), options(64, 2));
    auto a = writer->stream().reader(AcqReader::ReadMode::Lossless);
    std::optional<AcqReader> b;
    {
        auto tmp = writer->stream().reader(AcqReader::ReadMode::Latest);
        QVERIFY(tmp);
        b.emplace(std::move(*tmp));
    }
    QVERIFY(writer->stream().reader(AcqReader::ReadMode::Lossless).error() == AcqErrorCode::TooManyReaders);
    QCOMPARE(writer->stream().stats().activeReaders, std::size_t(2));
    b.reset();
    QCOMPARE(writer->stream().stats().activeReaders, std::size_t(1));
    auto c = writer->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(c);
    QCOMPARE(c->channelCount(), std::size_t(2));
    QVERIFY(c->stream().isValid());

    QVERIFY(c->consumeIdle(1).code() == AcqErrorCode::WrongDirection);

    QVERIFY(produce(*writer, 4));
    auto held = a->acquire(1, 0ns);
    QVERIFY(held);
    QVERIFY(a->acquire(1, 0ns).error() == AcqErrorCode::InvalidState);
    auto other = c->acquire(1, 0ns);
    QVERIFY(other);
    QVERIFY(a->release(*other).code() == AcqErrorCode::InvalidState);
    QVERIFY(a->release(*held).ok());
    QVERIFY(c->release(*other).ok());
    AcqBlockView empty;
    QVERIFY(a->release(empty).code() == AcqErrorCode::InvalidState);

    AcqReader moved = std::move(*a);
    QVERIFY(moved.isValid());
    QVERIFY(!a->isValid());
    QVERIFY(a->acquire(0, 0ns).error() == AcqErrorCode::InvalidState);
}

//=============================================================================================================

void TestAcqStream::testManyConcurrentReaders()
{
    constexpr std::size_t kTotal = 1U << 18;
    constexpr std::size_t kBlock = 64;
    constexpr int kReaders = 8;
    auto writer = AcqWriter::create(makeInfo(4), options(kTotal, kReaders));
    QVERIFY(writer);

    std::vector<AcqReader> readers;
    for (int i = 0; i < kReaders; ++i) {
        AcqReaderOptions o;
        o.wait = i % 3 == 0 ? AcqWait::Block : (i % 3 == 1 ? AcqWait::Spin : AcqWait::SpinThenBlock);
        auto r = writer->stream().reader(i == kReaders - 1 ? AcqReader::ReadMode::LowLatency
                                                           : AcqReader::ReadMode::Lossless,
                                         o);
        QVERIFY(r);
        readers.push_back(std::move(*r));
    }

    std::atomic<int> failures{0};
    std::vector<std::thread> threads;
    threads.reserve(readers.size());
    for (auto& reader : readers) {
        threads.emplace_back([&reader, &failures] {
            std::uint64_t next = 0;
            while (true) {
                auto view = reader.acquire(1, 10s);
                if (!view) {
                    if (view.error() != AcqErrorCode::EndOfStream) {
                        ++failures;
                    }
                    break;
                }
                if (view->firstSample() != next || view->lost() != 0 || !matches(*view)) {
                    ++failures;
                }
                next = view->firstSample() + view->sampleCount();
                if (!reader.release(*view).ok()) {
                    ++failures;
                }
            }
            if (next != kTotal) {
                ++failures;
            }
        });
    }
    std::thread producer([&] {
        for (std::size_t written = 0; written < kTotal; written += kBlock) {
            if (!produce(*writer, kBlock)) {
                ++failures;
            }
        }
        writer->finish();
    });
    producer.join();
    for (auto& t : threads) {
        t.join();
    }
    QCOMPARE(failures.load(), 0);
    for (const auto& reader : readers) {
        QCOMPARE(reader.stats().delivered, std::uint64_t(kTotal));
    }
}

//=============================================================================================================

void TestAcqStream::testSlowReaderLossAccounting()
{
    constexpr std::size_t kTotal = 1U << 16;
    auto writer = AcqWriter::create(makeInfo(2), options(256));
    auto slow = writer->stream().reader(AcqReader::ReadMode::Lossless);
    std::thread producer([&] {
        for (std::size_t written = 0; written < kTotal; written += 16) {
            produce(*writer, 16);
            if (written % 4096 == 0) {
                std::this_thread::sleep_for(1ms);
            }
        }
        writer->finish();
    });
    std::uint64_t lostInViews = 0;
    std::uint64_t next = 0;
    while (true) {
        auto view = slow->acquire(1, 10s, 32);
        if (!view) {
            QVERIFY(view.error() == AcqErrorCode::EndOfStream);
            break;
        }
        QCOMPARE(view->firstSample(), next + view->lost());
        lostInViews += view->lost();
        next = view->firstSample() + view->sampleCount();
        std::this_thread::sleep_for(50us); // stall: the producer laps this reader
        slow->release(*view);
    }
    producer.join();
    const AcqReaderStats stats = slow->stats();
    QCOMPARE(stats.delivered + stats.lost, std::uint64_t(kTotal));
    QVERIFY(stats.lost >= lostInViews);
    QVERIFY(stats.lost > 0);
}

//=============================================================================================================

void TestAcqStream::testNoSilentLossOrCorruption_data()
{
    QTest::addColumn<int>("layout");
    QTest::addColumn<bool>("mirror");
    QTest::addColumn<int>("capacity");
    for (const AcqLayout layout : {AcqLayout::Interleaved, AcqLayout::Planar}) {
        for (const bool mirror : {false, true}) {
            for (const int capacity : {64, 1024}) {
                const std::string name = std::string(layout == AcqLayout::Interleaved ? "interleaved" : "planar") +
                    (mirror ? " mirrored" : "") + " capacity " + std::to_string(capacity);
                QTest::newRow(name.c_str()) << static_cast<int>(layout) << mirror << capacity;
            }
        }
    }
}

/**
 * The contract every reader relies on, under a producer that laps slow readers on a small ring: a sample either
 * reaches the reader intact, in order and exactly once, or is counted as lost; never silently dropped, duplicated
 * or delivered after being overwritten. Readers hold views for random times, so overwrites hit held views.
 */
void TestAcqStream::testNoSilentLossOrCorruption()
{
    QFETCH(int, layout);
    QFETCH(bool, mirror);
    QFETCH(int, capacity);
    constexpr std::size_t kChannels = 4;
    constexpr std::uint64_t kTotal = 1U << 17;
    constexpr int kReaders = 3;
    AcqStreamOptions o = options(static_cast<std::size_t>(capacity), kReaders);
    o.mirror = mirror;
    o.eventCapacity = 16; // spans 592 samples: shorter than the 1024-sample ring, so delivered samples lose events
    auto writer = AcqWriter::create(makeInfo(kChannels, static_cast<AcqLayout>(layout)), o);
    QVERIFY(writer);

    struct Tally
    {
        std::uint64_t delivered = 0;
        std::uint64_t lostInViews = 0;
        std::uint64_t overrunSamples = 0;
        std::uint64_t overruns = 0;
        std::uint64_t events = 0;
        std::uint64_t end = 0;
        std::string failure;
    };
    std::vector<AcqReader> readers;
    for (int i = 0; i < kReaders; ++i) {
        auto r = writer->stream().reader(i == 0 ? AcqReader::ReadMode::LowLatency : AcqReader::ReadMode::Lossless);
        QVERIFY(r);
        readers.push_back(std::move(*r));
    }
    std::vector<Tally> tallies(kReaders);
    std::vector<std::thread> threads;
    threads.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        threads.emplace_back([&, i] {
            AcqReader& reader = readers[static_cast<std::size_t>(i)];
            Tally& t = tallies[static_cast<std::size_t>(i)];
            Lcg rng{static_cast<std::uint64_t>(capacity * 7 + i)};
            auto fail = [&t](std::string what) {
                if (t.failure.empty()) {
                    t.failure = std::move(what);
                }
            };
            while (true) {
                auto view = reader.acquire(1, 10s, 1 + rng.below(static_cast<std::size_t>(capacity)));
                if (!view) {
                    if (view.error() != AcqErrorCode::EndOfStream) {
                        fail("acquire: " + view.error().text());
                    }
                    break;
                }
                const std::uint64_t first = view->firstSample();
                const std::size_t n = view->sampleCount();
                if (n == 0 || first != t.end + view->lost()) {
                    fail("gap not reported before sample " + std::to_string(first));
                }
                if (i == 1 && rng.below(4) == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(rng.below(50)));
                } else if (i == 2) {
                    std::this_thread::sleep_for(std::chrono::microseconds(100 + rng.below(200)));
                }
                const bool intact = matches(*view);
                const bool eventsIntact = eventsValid(*view);
                const std::uint64_t lost = view->lost();
                const std::size_t events = view->events().size();
                const AcqError released = reader.release(*view);
                if (released.ok()) {
                    if (!intact) {
                        fail("overwritten samples released as valid at " + std::to_string(first));
                    }
                    if (!eventsIntact) {
                        fail("wrong events delivered with " + std::to_string(first));
                    }
                    t.delivered += n;
                    t.events += events;
                } else if (released == AcqErrorCode::Overrun) {
                    t.overrunSamples += n;
                    ++t.overruns;
                } else {
                    fail("release: " + released.text());
                }
                t.lostInViews += lost;
                t.end = first + n;
            }
        });
    }

    Lcg rng{static_cast<std::uint64_t>(layout * 131 + (mirror ? 17 : 0) + capacity)};
    for (std::uint64_t written = 0; written < kTotal;) {
        const auto n = std::min<std::uint64_t>(1 + rng.below(static_cast<std::size_t>(capacity) / 2), kTotal - written);
        auto view = writer->beginWrite(static_cast<std::size_t>(n));
        QVERIFY(view);
        fill(*view);
        for (std::uint64_t s = (written + kEventEvery - 1) / kEventEvery * kEventEvery; s < written + n; s += kEventEvery) {
            QVERIFY(writer->pushEvent({s, 1, eventCode(s)}).ok());
        }
        writer->commit(static_cast<std::size_t>(n));
        written += n;
        if (rng.below(8) == 0) {
            std::this_thread::sleep_for(20us);
        }
    }
    writer->finish();
    for (auto& thread : threads) {
        thread.join();
    }

    QCOMPARE(writer->stream().stats().committed, kTotal);
    for (int i = 0; i < kReaders; ++i) {
        const Tally& t = tallies[static_cast<std::size_t>(i)];
        const AcqReaderStats stats = readers[static_cast<std::size_t>(i)].stats();
        QVERIFY2(t.failure.empty(), qPrintable(QString("reader %1: %2").arg(i).arg(t.failure.c_str())));
        QCOMPARE(t.end, kTotal);
        QCOMPARE(stats.position, kTotal);
        QCOMPARE(stats.delivered, t.delivered);
        QCOMPARE(stats.lost, t.lostInViews + t.overrunSamples);
        QCOMPARE(stats.overruns, t.overruns);
        QCOMPARE(stats.delivered + stats.lost, kTotal);                        // every sample accounted for exactly once
        QCOMPARE(t.events + stats.eventsLost, (kTotal - 1) / kEventEvery + 1); // and every event, none twice
    }
    // The slow reader must have met every case the contract covers.
    QVERIFY(tallies[2].lostInViews > 0);
    QVERIFY(capacity < 1024 || readers[2].stats().eventsLost > 0);
}

//=============================================================================================================

void TestAcqStream::testNoAllocationsOrLocksPerBlock()
{
    auto writer = AcqWriter::create(makeInfo(64), options(4096));
    auto lossless = writer->stream().reader(AcqReader::ReadMode::Lossless);
    auto latest = writer->stream().reader(AcqReader::ReadMode::Latest);
    QVERIFY(produce(*writer, 32));
    bool ok = true;

    const std::uint64_t locksBefore = detail::controlLockCount();
    g_allocations = 0;
    g_countAllocations = true;
    for (int i = 0; i < 10'000; ++i) {
        AcqEvent event;
        event.sample = writer->writePosition();
        event.code = i;
        writer->pushEvent(event);
        ok &= produce(*writer, 32);
        auto a = lossless->acquire(32, 0ns);
        ok &= a.hasValue() && lossless->release(*a).ok();
        auto b = latest->acquire(8, 0ns);
        ok &= b.hasValue() && latest->release(*b).ok();
    }
    g_countAllocations = false;
    QVERIFY(ok);
    QCOMPARE(g_allocations.load(), std::size_t(0));
    QCOMPARE(detail::controlLockCount(), locksBefore);
}

//=============================================================================================================

void TestAcqStream::testAdoptExternalBuffer()
{
    constexpr std::size_t kCapacity = 64;
    constexpr std::size_t kBytes = kCapacity * 4 * sizeof(std::int32_t);
    auto* raw = static_cast<std::byte*>(::operator new(kBytes, std::align_val_t(4096)));
    int released = 0;
    {
        AcqStreamOptions o = options(0);
        o.external.data = raw;
        o.external.bytes = kBytes;
        o.external.release = [&released, raw] {
            ++released;
            ::operator delete(raw, std::align_val_t(4096));
        };
        auto writer = AcqWriter::create(makeInfo(4), o);
        QVERIFY(writer);
        QVERIFY(writer->stream().stats().adoptedBuffer);
        QCOMPARE(writer->stream().stats().capacitySamples, kCapacity);
        auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
        QVERIFY(produce(*writer, 3));
        std::int32_t direct = 0;
        std::memcpy(&direct, raw + (2 * 4 + 1) * sizeof(std::int32_t), sizeof(direct));
        QCOMPARE(direct, value(2, 1));
        auto view = reader->acquire(3, 0ns);
        QVERIFY(view);
        QCOMPARE(view->part(0).data, static_cast<const std::byte*>(raw));
        QVERIFY(reader->release(*view).ok());
        QCOMPARE(released, 0);
    }
    QCOMPARE(released, 1);

    alignas(4096) static std::byte scratch[8192];
    AcqStreamOptions misaligned = options(0);
    misaligned.external.data = scratch + 64;
    misaligned.external.bytes = kBytes;
    QVERIFY(AcqWriter::create(makeInfo(4), misaligned).error() == AcqErrorCode::InvalidBuffer);
    AcqStreamOptions notPow2 = options(0);
    notPow2.external.data = scratch;
    notPow2.external.bytes = 48 * 4 * sizeof(std::int32_t);
    QVERIFY(AcqWriter::create(makeInfo(4), notPow2).error() == AcqErrorCode::InvalidBuffer);
}

//=============================================================================================================

void TestAcqStream::testEvents()
{
    AcqStreamOptions o = options(256);
    o.eventCapacity = 4;
    auto writer = AcqWriter::create(makeInfo(2), o);
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    for (const std::uint64_t s : {3ULL, 10ULL, 20ULL}) {
        AcqEvent e;
        e.sample = s;
        e.code = static_cast<std::int32_t>(s);
        QVERIFY(writer->pushEvent(e).ok());
    }
    QVERIFY(produce(*writer, 16));
    auto view = reader->acquire(16, 0ns);
    QVERIFY(view);
    QCOMPARE(view->events().size(), std::size_t(2));
    QCOMPARE(view->events()[0].code, 3);
    QCOMPARE(view->events()[1].code, 10);
    QVERIFY(reader->release(*view).ok());
    QVERIFY(produce(*writer, 16));
    auto second = reader->acquire(16, 0ns);
    QCOMPARE(second->events().size(), std::size_t(1));
    QCOMPARE(second->events()[0].sample, std::uint64_t(20));
    QVERIFY(reader->release(*second).ok());

    for (int i = 0; i < 10; ++i) {
        AcqEvent e;
        e.sample = 32 + static_cast<std::uint64_t>(i);
        e.code = 100 + i;
        writer->pushEvent(e);
    }
    QVERIFY(produce(*writer, 16));
    auto third = reader->acquire(16, 0ns);
    QCOMPARE(third->events().size(), std::size_t(4));
    QCOMPARE(third->events()[0].code, 106);
    QCOMPARE(third->events()[3].code, 109);
    QVERIFY(reader->release(*third).ok());
    QCOMPARE(reader->stats().eventsLost, std::uint64_t(6));
    QCOMPARE(writer->stream().stats().events, std::uint64_t(13));
}

//=============================================================================================================

void TestAcqStream::testSegmentedComplexStream()
{
    constexpr std::size_t kCoils = 32;
    constexpr std::uint32_t kReadout = 512;
    constexpr std::uint32_t kHeader = 340;
    AcqStreamInfo rx;
    rx.name = "rx";
    rx.format = AcqSampleFormat::ComplexInt16;
    rx.layout = AcqLayout::Planar;
    rx.sfreq = 1.0e6;
    for (std::size_t c = 0; c < kCoils; ++c) {
        AcqChannelInfo coil;
        coil.name = "coil" + std::to_string(c);
        coil.kind = AcqChannelKind::MrReceive;
        coil.unit = AcqUnit::Volt;
        coil.scale = 1.0 / 32768.0;
        rx.channels.push_back(coil);
    }
    rx.segmentation.mode = AcqSegmentation::Mode::Triggered;
    rx.segmentation.samplesPerSegment = kReadout;
    rx.segmentation.headerBytes = kHeader;
    rx.segmentation.headerSchema = "ismrmrd.AcquisitionHeader/1";

    auto writer = AcqWriter::create(rx, options(4096));
    QVERIFY(writer);
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    for (std::uint64_t readout = 0; readout < 3; ++readout) {
        AcqSegmentHeader header;
        header.firstSample = readout * kReadout;
        header.sampleCount = kReadout;
        header.counter = readout;
        std::vector<std::byte> payload(kHeader, static_cast<std::byte>(0xA0 + readout));
        QVERIFY(writer->pushSegment(header, payload).ok());
        auto view = writer->beginWrite(kReadout);
        QVERIFY(view);
        for (std::size_t c = 0; c < kCoils; ++c) {
            for (std::size_t s = 0; s < kReadout; ++s) {
                const AcqComplexInt16 iq{static_cast<std::int16_t>(s), static_cast<std::int16_t>(-static_cast<int>(c))};
                std::memcpy(view->part(0).at(c, s), &iq, sizeof(iq));
            }
        }
        writer->commit(kReadout);
    }

    auto view = reader->acquire(3 * kReadout, 0ns);
    QVERIFY(view);
    QCOMPARE(view->format(), AcqSampleFormat::ComplexInt16);
    QCOMPARE(view->segments().size(), std::size_t(3));
    for (std::size_t i = 0; i < 3; ++i) {
        QCOMPARE(view->segments()[i].counter, std::uint64_t(i));
        QCOMPARE(view->segments()[i].firstSample, std::uint64_t(i * kReadout));
        const auto payload = view->segmentPayload(i);
        QCOMPARE(payload.size(), std::size_t(kHeader));
        QCOMPARE(payload[kHeader - 1], static_cast<std::byte>(0xA0 + i));
    }
    AcqComplexInt16 iq{};
    std::memcpy(&iq, view->part(0).at(7, 1000 - kReadout), sizeof(iq));
    QCOMPARE(int(iq.re), int(1000 - kReadout));
    QCOMPARE(int(iq.im), -7);
    QVERIFY(reader->release(*view).ok());
}

//=============================================================================================================

void TestAcqStream::testClockAndTimebasePublished()
{
    auto writer = AcqWriter::create(makeInfo(2), options(64));
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    AcqClockModel clock;
    clock.sfreq = 1000.0;
    clock.anchorSample = 7;
    clock.source = AcqClockSource::Device;
    writer->setClock(clock);
    AcqTimebase tb;
    tb.domain = acqClockDomainId("spectrum:1");
    tb.ticksPerSample = 125;
    tb.lock = AcqLockState::PhaseLocked;
    writer->stream().setTimebase(tb);

    QCOMPARE(writer->stream().clock().anchorSample, std::uint64_t(7));
    QCOMPARE(writer->stream().timebase().ticksPerSample, std::uint64_t(125));
    QVERIFY(produce(*writer, 4));
    auto view = reader->acquire(4, 0ns);
    QVERIFY(view);
    QCOMPARE(view->clock().source, AcqClockSource::Device);
    QCOMPARE(view->timebase().domain, tb.domain);
    QCOMPARE(view->timebase().lock, AcqLockState::PhaseLocked);
    QVERIFY(reader->release(*view).ok());

    // Every later view carries the values current at its acquire, unchanged or updated.
    for (std::uint64_t round = 0; round < 3; ++round) {
        if (round == 2) {
            clock.anchorSample = 99;
            writer->setClock(clock);
            tb.originTick = -5;
            writer->stream().setTimebase(tb);
        }
        QVERIFY(produce(*writer, 4));
        view = reader->acquire(4, 0ns);
        QVERIFY(view);
        QCOMPARE(view->clock().anchorSample, round == 2 ? std::uint64_t(99) : std::uint64_t(7));
        QCOMPARE(view->timebase().originTick, round == 2 ? std::int64_t(-5) : std::int64_t(0));
        QVERIFY(reader->release(*view).ok());
    }
}

//=============================================================================================================

void TestAcqStream::testInfoVersioning()
{
    auto writer = AcqWriter::create(makeInfo(2), options(256));
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    AcqStreamInfo gain = makeInfo(2);
    gain.channels[0].scale = 2.0;
    QVERIFY(writer->updateInfo(gain, 32).ok());
    QVERIFY(writer->updateInfo(makeInfo(3), 64).code() == AcqErrorCode::InvalidParameter);
    QVERIFY(produce(*writer, 64));
    QVERIFY(writer->updateInfo(gain, 10).code() == AcqErrorCode::InvalidParameter);

    QCOMPARE(writer->stream().info()->version, std::uint32_t(1));
    QCOMPARE(writer->stream().infoAt(10)->version, std::uint32_t(0));
    QCOMPARE(writer->stream().infoAt(40)->version, std::uint32_t(1));

    auto before = reader->acquire(1, 0ns);
    QVERIFY(before);
    QCOMPARE(before->sampleCount(), std::size_t(32));
    QCOMPARE(before->infoVersion(), std::uint32_t(0));
    QCOMPARE(before->scales()[0], 1.0);
    QVERIFY(reader->release(*before).ok());
    auto after = reader->acquire(1, 0ns);
    QVERIFY(after);
    QCOMPARE(after->firstSample(), std::uint64_t(32));
    QCOMPARE(after->infoVersion(), std::uint32_t(1));
    QCOMPARE(after->scales()[0], 2.0);
    QVERIFY(reader->release(*after).ok());
}

//=============================================================================================================

void TestAcqStream::testOutputWriterWaitsForDevice()
{
    auto writer = AcqWriter::create(makeInfo(2, AcqLayout::Interleaved, AcqDirection::Output), options(16));
    QVERIFY(writer);
    AcqReaderOptions gating;
    gating.gating = true;
    auto device = writer->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(device);
    QCOMPARE(writer->space(), std::size_t(16));
    QVERIFY(produce(*writer, 16));
    QCOMPARE(writer->space(), std::size_t(0));
    QVERIFY(writer->beginWrite(1, 0ns).error() == AcqErrorCode::Timeout);

    std::thread deviceThread([&] {
        std::this_thread::sleep_for(20ms);
        auto view = device->acquire(4, 1s, 4);
        if (view) {
            device->release(*view);
        }
    });
    auto blocked = writer->beginWrite(4, 5s);
    deviceThread.join();
    QVERIFY(blocked);
    QCOMPARE(blocked->firstSample(), std::uint64_t(16));
    QCOMPARE(blocked->underrun(), std::uint64_t(0));
    fill(*blocked);
    writer->commit(4);
    QVERIFY(writer->stream().stats().writerWaits >= 1);
    QCOMPARE(writer->devicePosition(), std::uint64_t(4));

    auto rest = device->acquire(16, 0ns);
    QVERIFY(rest);
    QCOMPARE(rest->firstSample(), std::uint64_t(4));
    QVERIFY(matches(*rest));
    QVERIFY(device->release(*rest).ok());
}

//=============================================================================================================

void TestAcqStream::testOutputUnderrun()
{
    auto writer = AcqWriter::create(makeInfo(2, AcqLayout::Interleaved, AcqDirection::Output), options(64));
    AcqReaderOptions gating;
    gating.gating = true;
    auto device = writer->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(device->acquire(4, 0ns).error() == AcqErrorCode::Timeout);
    QVERIFY(device->consumeIdle(4).ok());
    QCOMPARE(device->stats().idle, std::uint64_t(4));

    auto view = writer->beginWrite(8);
    QVERIFY(view);
    QCOMPARE(view->firstSample(), std::uint64_t(4));
    QCOMPARE(view->underrun(), std::uint64_t(4));
    fill(*view);
    writer->commit(8);
    QCOMPARE(writer->stream().stats().underrun, std::uint64_t(4));
    QCOMPARE(writer->stream().stats().underruns, std::uint64_t(1));

    auto played = device->acquire(8, 0ns);
    QVERIFY(played);
    QCOMPARE(played->firstSample(), std::uint64_t(4));
    QVERIFY(matches(*played));
    QVERIFY(device->release(*played).ok());
    auto next = writer->beginWrite(1);
    QCOMPARE(next->underrun(), std::uint64_t(0));
}

//=============================================================================================================

void TestAcqStream::testOutputScheduledWrite()
{
    auto writer = AcqWriter::create(makeInfo(2, AcqLayout::Interleaved, AcqDirection::Output), options(128));
    AcqReaderOptions gating;
    gating.gating = true;
    auto device = writer->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(produce(*writer, 8));
    QCOMPARE(writer->earliestWritableSample(), std::uint64_t(8));

    auto scheduled = writer->beginWriteAt(64, 8, 0ns);
    QVERIFY(scheduled);
    QCOMPARE(scheduled->firstSample(), std::uint64_t(64));
    fill(*scheduled);
    writer->commit(8);
    QCOMPARE(writer->writePosition(), std::uint64_t(72));
    QVERIFY(writer->beginWriteAt(10, 4, 0ns).error() == AcqErrorCode::InvalidParameter); // written, not yet played

    auto view = device->acquire(72, 0ns);
    QVERIFY(view);
    std::int32_t gap = -1;
    std::memcpy(&gap, view->part(0).at(1, 30), sizeof(gap));
    QCOMPARE(gap, 0);
    std::int32_t pulse = 0;
    std::memcpy(&pulse, view->part(0).at(1, 64), sizeof(pulse));
    QCOMPARE(pulse, value(64, 1));
    QVERIFY(device->release(*view).ok());

    QVERIFY(writer->beginWriteAt(10, 4, 0ns).error() == AcqErrorCode::Late); // already played
    QVERIFY(device->consumeIdle(50).ok());
    QVERIFY(writer->beginWriteAt(100, 4, 0ns).error() == AcqErrorCode::Late);
    AcqTimebase tb;
    tb.sampleAlignment = 32;
    writer->stream().setTimebase(tb);
    QVERIFY(writer->beginWriteAt(130, 4, 0ns).error() == AcqErrorCode::NotOnGrid);
    QVERIFY(writer->beginWriteAt(128, 4, 0ns));
}

//=============================================================================================================

void TestAcqStream::testScheduledWriteAfterLongIdle()
{
    // Sparse stimulation: the device plays fill far beyond the ring capacity between two pulses.
    auto writer = AcqWriter::create(makeInfo(1, AcqLayout::Interleaved, AcqDirection::Output), options(64));
    AcqReaderOptions gating;
    gating.gating = true;
    auto device = writer->stream().reader(AcqReader::ReadMode::Lossless, gating);
    QVERIFY(device->consumeIdle(1000).ok());
    auto pulse = writer->beginWriteAt(1010, 8, 0ns);
    QVERIFY2(pulse.hasValue(), pulse.error().text().c_str());
    QCOMPARE(pulse->firstSample(), std::uint64_t(1010));
    fill(*pulse);
    writer->commit(8);
    QVERIFY(device->consumeIdle(0).ok());
    auto played = device->acquire(18, 0ns);
    QVERIFY(played);
    QCOMPARE(played->firstSample(), std::uint64_t(1000));
    std::int32_t v = -1;
    std::memcpy(&v, played->part(0).at(0, 9), sizeof(v));
    QCOMPARE(v, 0);
    std::memcpy(&v, played->part(0).at(0, 10), sizeof(v));
    QCOMPARE(v, value(1010, 0));
    QVERIFY(device->release(*played).ok());
    QVERIFY(writer->beginWriteAt(500, 4, 0ns).error() == AcqErrorCode::Late);
}

//=============================================================================================================

void TestAcqStream::testCommitOverPlayedFillIsDropped()
{
    // The writer reserves a pulse in time, but the device plays fill over its start before the commit lands.
    auto writer = AcqWriter::create(makeInfo(1, AcqLayout::Interleaved, AcqDirection::Output), options(64));
    AcqReaderOptions gating;
    gating.gating = true;
    auto device = writer->stream().reader(AcqReader::ReadMode::Lossless, gating);
    auto pulse = writer->beginWriteAt(10, 8, 0ns);
    QVERIFY(pulse);
    fill(*pulse);
    auto nothing = device->acquire(0, 0ns);
    QVERIFY(nothing);
    QCOMPARE(nothing->sampleCount(), std::size_t(0));
    QVERIFY(device->release(*nothing).ok());
    QVERIFY(device->consumeIdle(12).ok());
    writer->commit(8);

    // Its tail would play 2 samples late and truncated; it is dropped whole and counted instead.
    auto tail = device->acquire(0, 0ns);
    QVERIFY(tail);
    QCOMPARE(tail->sampleCount(), std::size_t(0));
    QVERIFY(device->release(*tail).ok());
    QCOMPARE(device->acquire(1, 0ns).error().code(), AcqErrorCode::Timeout);
    QCOMPARE(writer->stream().stats().lateCommits, std::uint64_t(1));
    QVERIFY(device->consumeIdle(6).ok());

    // The next pulse, committed before the device reaches it, plays on its exact sample.
    auto next = writer->beginWriteAt(20, 4, 0ns);
    QVERIFY(next);
    fill(*next);
    writer->commit(4);
    auto played = device->acquire(6, 0ns);
    QVERIFY(played);
    QCOMPARE(played->firstSample(), std::uint64_t(18));
    std::int32_t v = -1;
    std::memcpy(&v, played->part(0).at(0, 1), sizeof(v));
    QCOMPARE(v, 0);
    std::memcpy(&v, played->part(0).at(0, 2), sizeof(v));
    QCOMPARE(v, value(20, 0));
    QVERIFY(device->release(*played).ok());
    QCOMPARE(writer->stream().stats().lateCommits, std::uint64_t(1));
}

//=============================================================================================================

void TestAcqStream::testOutputMonitorAndEvents()
{
    auto writer = AcqWriter::create(makeInfo(2, AcqLayout::Interleaved, AcqDirection::Output), options(64));
    AcqReaderOptions gating;
    gating.gating = true;
    auto device = writer->stream().reader(AcqReader::ReadMode::Lossless, gating);
    auto monitor = writer->stream().reader(AcqReader::ReadMode::Lossless);

    AcqEvent pulse;
    pulse.sample = 40;
    pulse.code = 1;
    QVERIFY(writer->pushEvent(pulse).ok());
    QVERIFY(produce(*writer, 32));
    auto early = device->acquire(32, 0ns);
    QVERIFY(early);
    QCOMPARE(early->events().size(), std::size_t(0));
    QVERIFY(device->release(*early).ok());

    AcqEvent late;
    late.sample = 20;
    QVERIFY(writer->pushEvent(late).code() == AcqErrorCode::Late);
    QCOMPARE(writer->stream().stats().lateEvents, std::uint64_t(1));

    QVERIFY(produce(*writer, 16));
    auto withEvent = device->acquire(16, 0ns);
    QCOMPARE(withEvent->events().size(), std::size_t(1));
    QCOMPARE(withEvent->events()[0].sample, std::uint64_t(40));
    QVERIFY(device->release(*withEvent).ok());

    auto seen = monitor->acquire(48, 0ns);
    QVERIFY(seen);
    QCOMPARE(seen->sampleCount(), std::size_t(48));
    QVERIFY(matches(*seen));
    QVERIFY(monitor->release(*seen).ok());
}

//=============================================================================================================

void TestAcqStream::testWriterHandles()
{
    auto input = AcqWriter::create(makeInfo(2), options(64));
    QVERIFY(input->stream().writer().error() == AcqErrorCode::WrongDirection);
    QCOMPARE(input->devicePosition(), input->writePosition());
    QCOMPARE(input->earliestWritableSample(), input->writePosition());
    QVERIFY(input->beginWriteAt(0, 1).error() == AcqErrorCode::WrongDirection);

    auto output = AcqWriter::create(makeInfo(2, AcqLayout::Interleaved, AcqDirection::Output), options(64));
    QVERIFY(output->stream().writer().error() == AcqErrorCode::WriterInUse);

    AcqWriter moved = std::move(*output);
    QCOMPARE(moved.stream().direction(), AcqDirection::Output);
    AcqWriter empty;
    QVERIFY(!empty.stream().isValid());
}

//=============================================================================================================

void TestAcqStream::testInvalidHandles()
{
    AcqStream stream;
    QVERIFY(!stream.info());
    QVERIFY(!stream.infoAt(0));
    QCOMPARE(stream.direction(), AcqDirection::Input);
    QCOMPARE(stream.stats().capacitySamples, std::size_t(0));
    QCOMPARE(stream.clock().sfreq, 0.0);
    QCOMPARE(stream.timebase().ticksPerSample, std::uint64_t(1));
    stream.setTimebase(AcqTimebase());
    QVERIFY(stream.reader(AcqReader::ReadMode::Lossless).error() == AcqErrorCode::InvalidState);
    QVERIFY(stream.writer().error() == AcqErrorCode::InvalidState);

    AcqWriter writer;
    QCOMPARE(writer.writePosition(), std::uint64_t(0));
    QCOMPARE(writer.devicePosition(), std::uint64_t(0));
    QCOMPARE(writer.space(), std::size_t(0));
    QCOMPARE(writer.earliestWritableSample(), std::uint64_t(0));
    QVERIFY(writer.beginWrite(1).error() == AcqErrorCode::InvalidState);
    QVERIFY(writer.beginWriteAt(0, 1).error() == AcqErrorCode::InvalidState);
    QVERIFY(writer.pushEvent(AcqEvent()).code() == AcqErrorCode::InvalidState);
    QVERIFY(writer.pushSegment(AcqSegmentHeader(), {}).code() == AcqErrorCode::InvalidState);
    QVERIFY(writer.updateInfo(makeInfo(1), 0).code() == AcqErrorCode::InvalidState);
    writer.commit(1);
    writer.setClock(AcqClockModel());
    writer.finish();

    AcqReader reader;
    QVERIFY(!reader.isValid());
    QCOMPARE(reader.mode(), AcqReader::ReadMode::Lossless);
    QCOMPARE(reader.channelCount(), std::size_t(0));
    QCOMPARE(reader.available(), std::size_t(0));
    QCOMPARE(reader.stats().views, std::uint64_t(0));
    QVERIFY(!reader.stream().isValid());
    AcqBlockView view;
    QVERIFY(reader.release(view).code() == AcqErrorCode::InvalidState);
    QVERIFY(reader.consumeIdle(1).code() == AcqErrorCode::InvalidState);

    auto real = AcqWriter::create(makeInfo(2), options(64));
    AcqWriter assigned;
    assigned = std::move(*real);
    QVERIFY(assigned.stream().isValid());
    QVERIFY(produce(assigned, 2));
    assigned.commit(0);
    QCOMPARE(assigned.writePosition(), std::uint64_t(2));

    auto r1 = assigned.stream().reader(AcqReader::ReadMode::Lossless);
    auto r2 = assigned.stream().reader(AcqReader::ReadMode::Latest);
    AcqReader target = std::move(*r1);
    target = std::move(*r2);
    QCOMPARE(target.mode(), AcqReader::ReadMode::Latest);
    QCOMPARE(assigned.stream().stats().activeReaders, std::size_t(1));
}

//=============================================================================================================

void TestAcqStream::testExternalPlanarBuffer()
{
    alignas(4096) static std::byte planar[4 * 512];
    AcqStreamOptions o = options(0);
    o.external.data = planar;
    o.external.bytes = sizeof(planar);
    o.external.channelStride = 512;
    o.capacitySamples = 64;
    auto writer = AcqWriter::create(makeInfo(4, AcqLayout::Planar), o);
    QVERIFY(writer);
    QCOMPARE(writer->stream().stats().capacitySamples, std::size_t(64));
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(produce(*writer, 5));
    std::int32_t direct = 0;
    std::memcpy(&direct, planar + 2 * 512 + 3 * sizeof(std::int32_t), sizeof(direct));
    QCOMPARE(direct, value(3, 2));
    auto view = reader->acquire(5, 0ns);
    QVERIFY(view && matches(*view));
    QVERIFY(reader->release(*view).ok());

    AcqStreamOptions tooLong = o;
    tooLong.capacitySamples = 256;
    QVERIFY(AcqWriter::create(makeInfo(4, AcqLayout::Planar), tooLong).error() == AcqErrorCode::InvalidBuffer);
    AcqStreamOptions mismatch = options(32);
    mismatch.external.data = planar;
    mismatch.external.bytes = sizeof(planar);
    QVERIFY(AcqWriter::create(makeInfo(4), mismatch).error() == AcqErrorCode::InvalidParameter);
    AcqStreamOptions noChannels = options(0);
    noChannels.external.data = planar;
    noChannels.external.bytes = sizeof(planar);
    QVERIFY(AcqWriter::create(makeInfo(0), noChannels).error() == AcqErrorCode::InvalidBuffer);
    AcqStreamOptions badAlignment = options(64);
    badAlignment.rowAlignment = 48;
    QVERIFY(AcqWriter::create(makeInfo(4, AcqLayout::Planar), badAlignment).error() ==
            AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqStream::testDefaultCapacityLimitAndMemoryLock()
{
    AcqStreamInfo wide = makeInfo(0);
    wide.format = AcqSampleFormat::Float64;
    wide.channels.resize(8192);
    AcqStreamOptions o = options(0);
    o.prefault = false;
    auto big = AcqWriter::create(wide, o);
    QVERIFY(big);
    QCOMPARE(big->stream().stats().capacitySamples, std::size_t(4096));

    AcqStreamOptions locked = options(64);
    locked.lockMemory = true;
    auto lockedRing = AcqWriter::create(makeInfo(2), locked);
    QVERIFY(lockedRing);
    QVERIFY(produce(*lockedRing, 8));
}

//=============================================================================================================

void TestAcqStream::testHoldLastUnderrunFill()
{
    AcqStreamInfo info = makeInfo(2, AcqLayout::Planar, AcqDirection::Output);
    info.underrunFill = AcqUnderrunFill::HoldLast;
    auto writer = AcqWriter::create(info, options(16));
    AcqReaderOptions gating;
    gating.gating = true;
    auto device = writer->stream().reader(AcqReader::ReadMode::Lossless, gating);
    auto monitor = writer->stream().reader(AcqReader::ReadMode::Lossless);
    QVERIFY(produce(*writer, 4));
    auto played = device->acquire(4, 0ns);
    QVERIFY(device->release(*played).ok());
    QVERIFY(device->consumeIdle(3).ok());
    auto next = writer->beginWrite(1);
    QVERIFY(next);
    QCOMPARE(next->firstSample(), std::uint64_t(7));
    QCOMPARE(next->underrun(), std::uint64_t(3));
    auto held = monitor->acquire(7, 0ns);
    QVERIFY(held);
    for (std::size_t s = 4; s < 7; ++s) {
        std::int32_t v = 0;
        std::memcpy(&v, held->part(0).at(1, s), sizeof(v));
        QCOMPARE(v, value(3, 1));
    }
    QVERIFY(monitor->release(*held).ok());

    QVERIFY(device->consumeIdle(100).ok());
    auto ahead = writer->beginWrite(2);
    QVERIFY(ahead);
    QCOMPARE(ahead->firstSample(), std::uint64_t(107));
    QCOMPARE(ahead->underrun(), std::uint64_t(100));
}

//=============================================================================================================

void TestAcqStream::testOutputDeviceGoneAndLimits()
{
    auto writer = AcqWriter::create(makeInfo(2, AcqLayout::Interleaved, AcqDirection::Output), options(16));
    QVERIFY(writer->beginWrite(17).error() == AcqErrorCode::InvalidParameter);
    QVERIFY(writer->beginWriteAt(0, 17).error() == AcqErrorCode::InvalidParameter);
    {
        AcqReaderOptions gating;
        gating.gating = true;
        auto device = writer->stream().reader(AcqReader::ReadMode::Lossless, gating);
        QVERIFY(writer->stream().reader(AcqReader::ReadMode::Lossless, gating).error() == AcqErrorCode::InvalidState);
        QVERIFY(produce(*writer, 16));
        QVERIFY(writer->beginWriteAt(16, 4, 1ms).error() == AcqErrorCode::Timeout);
        auto held = device->acquire(1, 0ns, 1);
        QVERIFY(device->consumeIdle(1).code() == AcqErrorCode::InvalidState);
        QVERIFY(device->release(*held).ok());
    }
    QVERIFY(writer->beginWrite(16, 1s).error() == AcqErrorCode::EndOfStream);
}

//=============================================================================================================

void TestAcqStream::testRecordOrdering()
{
    auto input = AcqWriter::create(makeInfo(2), options(64));
    AcqEvent e;
    e.sample = 10;
    QVERIFY(input->pushEvent(e).ok());
    e.sample = 9;
    QVERIFY(input->pushEvent(e).code() == AcqErrorCode::InvalidParameter);

    AcqStreamInfo info = makeInfo(2, AcqLayout::Interleaved, AcqDirection::Output);
    info.segmentation.mode = AcqSegmentation::Mode::Triggered;
    info.segmentation.headerBytes = 4;
    auto output = AcqWriter::create(info, options(64));
    AcqReaderOptions gating;
    gating.gating = true;
    auto device = output->stream().reader(AcqReader::ReadMode::Lossless, gating);
    AcqSegmentHeader h;
    h.firstSample = 20;
    QVERIFY(output->pushSegment(h, {}).ok());
    h.firstSample = 19;
    QVERIFY(output->pushSegment(h, {}).code() == AcqErrorCode::InvalidParameter);
    QVERIFY(device->consumeIdle(30).ok());
    h.firstSample = 25;
    QVERIFY(output->pushSegment(h, {}).code() == AcqErrorCode::Late);
}

//=============================================================================================================

void TestAcqStream::testInfoVersionLimit()
{
    auto writer = AcqWriter::create(makeInfo(1), options(64));
    AcqError last;
    for (int i = 0; i < 1100 && last.ok(); ++i) {
        last = writer->updateInfo(makeInfo(1), 0);
    }
    QVERIFY(last.code() == AcqErrorCode::InvalidState);
    QCOMPARE(writer->stream().info()->version, std::uint32_t(1023));
}

//=============================================================================================================

void TestAcqStream::testViewHelpers()
{
    AcqBlockView view;
    QVERIFY(view.empty());
    QVERIFY(view.events().empty());
    for (const AcqSampleFormat f : {AcqSampleFormat::Int16, AcqSampleFormat::Int24Packed, AcqSampleFormat::Int32,
                                    AcqSampleFormat::Float32, AcqSampleFormat::Float64, AcqSampleFormat::ComplexInt16,
                                    AcqSampleFormat::ComplexFloat32}) {
        AcqStreamInfo info = makeInfo(3);
        info.format = f;
        auto writer = AcqWriter::create(info, options(16));
        QVERIFY(writer);
        auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
        auto w = writer->beginWrite(2);
        QCOMPARE(std::size_t(w->part(0).sampleStride), 3 * bytesPerSample(f));
        writer->commit(2);
        auto r = reader->acquire(2, 0ns);
        QVERIFY(!r->empty());
        QCOMPARE(r->format(), f);
        QVERIFY(reader->release(*r).ok());
    }
}

//=============================================================================================================

void TestAcqStream::testDeviceMemoryNeverTouched()
{
    // GPU memory written by GPUDirect RDMA: every CPU access to this region faults.
    constexpr std::size_t kCapacity = 1024;
    NoAccessRegion gpuIn(kCapacity * 4 * sizeof(std::int32_t));
    QVERIFY(gpuIn.data() != nullptr);
    AcqStreamOptions o = options(0);
    o.lockMemory = true;
    o.external.data = gpuIn.data();
    o.external.bytes = kCapacity * 4 * sizeof(std::int32_t);
    o.external.memory = {AcqMemoryKind::Device, 0};
    auto in = AcqWriter::create(makeInfo(4), o);
    QVERIFY(in);
    QCOMPARE(in->stream().memory(), (AcqMemoryLocation{AcqMemoryKind::Device, 0}));
    QVERIFY(!in->stream().memory().hostAccessible());
    auto reader = in->stream().reader(AcqReader::ReadMode::Lossless);
    auto display = in->stream().reader(AcqReader::ReadMode::Latest);
    QVERIFY(in->grant(512).ok());
    QVERIFY(in->pushEvent({100, 0, 7}).ok());
    in->commit(300);
    auto block = reader->acquire(300, 0ns);
    QVERIFY(block);
    QCOMPARE(block->memory().kind, AcqMemoryKind::Device);
    QCOMPARE(block->part(0).data, static_cast<const std::byte*>(gpuIn.data()));
    QCOMPARE(block->events().size(), std::size_t(1));
    QVERIFY(reader->release(*block).ok());
    auto latest = display->acquire(10, 0ns, 10);
    QVERIFY(latest);
    QCOMPARE(latest->firstSample(), std::uint64_t(290));
    QVERIFY(display->release(*latest).ok());

    // Output to the GPU: scheduled gaps, underrun catch-up and hold-last fill are the device's job.
    AcqStreamInfo outInfo = makeInfo(2, AcqLayout::Interleaved, AcqDirection::Output);
    outInfo.underrunFill = AcqUnderrunFill::HoldLast;
    NoAccessRegion gpuOut(kCapacity * 2 * sizeof(std::int32_t));
    QVERIFY(gpuOut.data() != nullptr);
    AcqStreamOptions oo = options(0);
    oo.external.data = gpuOut.data();
    oo.external.bytes = kCapacity * 2 * sizeof(std::int32_t);
    oo.external.memory = {AcqMemoryKind::Device, 1};
    auto out = AcqWriter::create(outInfo, oo);
    QVERIFY(out);
    AcqReaderOptions gating;
    gating.gating = true;
    auto device = out->stream().reader(AcqReader::ReadMode::Lossless, gating);
    auto pulse = out->beginWriteAt(40, 8, 0ns);
    QVERIFY(pulse);
    QCOMPARE(pulse->memory(), (AcqMemoryLocation{AcqMemoryKind::Device, 1}));
    out->commit(8);
    auto played = device->acquire(48, 0ns);
    QVERIFY(played);
    QVERIFY(device->release(*played).ok());
    QVERIFY(device->consumeIdle(52).ok());
    auto later = out->beginWrite(16, 0ns);
    QVERIFY(later);
    QCOMPARE(later->firstSample(), std::uint64_t(100));
    QCOMPARE(later->underrun(), std::uint64_t(52));
    out->commit(16);
    auto tail = device->acquire(16, 0ns);
    QVERIFY(tail);
    QCOMPARE(tail->firstSample(), std::uint64_t(100));
    QVERIFY(device->release(*tail).ok());

    AcqStreamOptions noOrdinal = o;
    noOrdinal.external.release = {};
    noOrdinal.external.memory = {AcqMemoryKind::Device, -1};
    QVERIFY(AcqWriter::create(makeInfo(4), noOrdinal).error() == AcqErrorCode::InvalidBuffer);

    AcqStreamOptions pinned = options(0);
    pinned.external = hostBuffer(kCapacity * 4 * sizeof(std::int32_t));
    pinned.external.memory = {AcqMemoryKind::HostPinned, 0};
    auto host = AcqWriter::create(makeInfo(4), pinned);
    QVERIFY(host);
    QVERIFY(host->stream().memory().hostAccessible());
    QCOMPARE(AcqWriter::create(makeInfo(4), options(64))->stream().memory(), AcqMemoryLocation{});
}

//=============================================================================================================

void TestAcqStream::testGrantDeviceOwnedWrites()
{
    // The card writes granted samples straight into the ring; the host only grants and commits.
    constexpr std::size_t kCapacity = 64;
    AcqStreamOptions o = options(0);
    o.external = hostBuffer(kCapacity * sizeof(std::int32_t));
    std::byte* ring = o.external.data;
    auto writer = AcqWriter::create(makeInfo(1), o);
    QVERIFY(writer);
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    auto lagging = writer->stream().reader(AcqReader::ReadMode::Lossless);
    auto card = [ring](std::uint64_t from, std::uint64_t to) {
        for (std::uint64_t s = from; s < to; ++s) {
            const std::int32_t v = value(s, 0);
            std::memcpy(ring + (s % kCapacity) * sizeof(v), &v, sizeof(v));
        }
    };

    QCOMPARE(writer->grant(kCapacity + 1).code(), AcqErrorCode::InvalidParameter);
    QVERIFY(writer->grant(40).ok());
    QVERIFY(writer->grant(24).ok());
    QCOMPARE(writer->grant(1).code(), AcqErrorCode::InvalidParameter);
    QVERIFY(writer->grant(0).ok());
    QCOMPARE(writer->writePosition(), std::uint64_t(0));

    card(0, 40);
    writer->commit(40);
    QCOMPARE(writer->writePosition(), std::uint64_t(40));
    auto first = reader->acquire(40, 0ns);
    QVERIFY(first);
    QVERIFY(matches(*first));
    QVERIFY(reader->release(*first).ok());

    // A commit never passes the granted end.
    card(40, 64);
    writer->commit(100);
    QCOMPARE(writer->writePosition(), std::uint64_t(64));
    auto second = reader->acquire(24, 0ns);
    QVERIFY(second);
    QVERIFY(matches(*second));

    // Granting more lets the card overwrite the oldest samples at once: a reader still holding them gets
    // Overrun, and a lagging reader's loss is exact before any new sample is committed.
    QVERIFY(writer->grant(48).ok());
    QCOMPARE(reader->release(*second).code(), AcqErrorCode::Overrun);
    auto lag = lagging->acquire(1, 0ns);
    QVERIFY(lag);
    QCOMPARE(lag->firstSample(), std::uint64_t(48));
    QCOMPARE(lagging->stats().lost, std::uint64_t(48));
    QVERIFY(lagging->release(*lag).ok());

    auto output = AcqWriter::create(makeInfo(1, AcqLayout::Interleaved, AcqDirection::Output), options(64));
    QCOMPARE(output->grant(8).code(), AcqErrorCode::WrongDirection);
    QCOMPARE(AcqWriter().grant(8).code(), AcqErrorCode::InvalidState);
}

//=============================================================================================================

void TestAcqStream::testDmaEngineSimulation()
{
    // Eight blocks in flight, like a card's notify size. As on a Spectrum card, the driver grants only space the
    // recorder has freed, so the card stalls rather than overwrites; every sample is checked.
    constexpr std::size_t kChannels = 8;
    constexpr std::size_t kCapacity = 8192;
    constexpr std::size_t kBlock = 256;
    constexpr std::size_t kInFlight = 8;
    constexpr std::uint64_t kTotal = 1U << 20;
    AcqStreamOptions o = options(0);
    o.external = hostBuffer(kCapacity * kChannels * sizeof(std::int32_t));
    auto writer = AcqWriter::create(makeInfo(kChannels), o);
    QVERIFY(writer);
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    std::atomic<std::uint64_t> wrong{0};
    std::thread recorder([&] {
        while (true) {
            auto view = reader->acquire(kBlock, 1s);
            if (!view) {
                if (view.error() == AcqErrorCode::EndOfStream) {
                    return;
                }
                continue;
            }
            const bool ok = matches(*view);
            if (reader->release(*view).ok() && !ok) {
                wrong.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    DmaEngineSimulator card(o.external.data, kCapacity, kChannels, kBlock);
    g_allocations = 0;
    g_countAllocations = true;
    std::uint64_t granted = 0;
    std::uint64_t committed = 0;
    while (committed < kTotal) {
        const std::uint64_t freed = reader->stats().position + kCapacity;
        while (granted < kTotal && granted - committed < kInFlight * kBlock && granted + kBlock <= freed) {
            if (!writer->grant(kBlock).ok()) {
                break;
            }
            granted += kBlock;
            card.setGrantedEnd(granted);
        }
        const std::uint64_t done = card.written();
        if (done > committed) {
            writer->commit(static_cast<std::size_t>(done - committed));
            committed = done;
        } else {
            std::this_thread::yield();
        }
    }
    g_countAllocations = false;
    writer->finish();
    recorder.join();

    QCOMPARE(writer->writePosition(), kTotal);
    QCOMPARE(g_allocations.load(), std::size_t(0));
    QCOMPARE(wrong.load(), std::uint64_t(0));
    const AcqReaderStats stats = reader->stats();
    QCOMPARE(stats.lost, std::uint64_t(0));
    QCOMPARE(stats.delivered, kTotal);
}

//=============================================================================================================

void TestAcqStream::testRingPlacement()
{
    auto ringStart = [](AcqWriter& writer) -> std::byte* {
        auto view = writer.beginWrite(1);
        return view ? view->part(0).data : nullptr;
    };
    auto addressOf = [&ringStart](AcqWriter& writer) {
        std::byte* start = ringStart(writer);
        return start != nullptr ? reinterpret_cast<std::uintptr_t>(start) : std::uintptr_t(1);
    };

    AcqStreamOptions gpuPages = options(1024);
    gpuPages.alignment = 65536;
    auto aligned = AcqWriter::create(makeInfo(4), gpuPages);
    QVERIFY(aligned);
    QCOMPARE(addressOf(*aligned) % 65536, std::uintptr_t(0));
    for (const std::size_t bad : {std::size_t(0), std::size_t(3), std::size_t(32)}) {
        AcqStreamOptions invalid = options(64);
        invalid.alignment = bad;
        QVERIFY(AcqWriter::create(makeInfo(4), invalid).error() == AcqErrorCode::InvalidParameter);
    }
    AcqStreamOptions misaligned = options(0);
    misaligned.alignment = 65536;
    AcqExternalBuffer buffer = hostBuffer(65536 * 2, 65536);
    misaligned.external.data = buffer.data + 4096;
    misaligned.external.bytes = 4096;
    QVERIFY(AcqWriter::create(makeInfo(4), misaligned).error() == AcqErrorCode::InvalidBuffer);
    buffer.release();

    AcqStreamOptions huge = options(1U << 18);
    huge.hugePages = true;
    auto large = AcqWriter::create(makeInfo(4), huge);
    QVERIFY(large);
    QCOMPARE(addressOf(*large) % (std::uintptr_t(2) << 20), std::uintptr_t(0));
#if defined(__linux__)
    // Transparent huge pages can be disabled ("never"); then madvise fails and the ring says so.
    std::FILE* thp = std::fopen("/sys/kernel/mm/transparent_hugepage/enabled", "r");
    std::array<char, 128> mode{};
    const bool thpOff = thp != nullptr && std::fgets(mode.data(), mode.size(), thp) != nullptr &&
        std::strstr(mode.data(), "[never]") != nullptr;
    if (thp != nullptr) {
        std::fclose(thp);
    }
    QCOMPARE(large->stream().stats().hugePages, !thpOff);
#else
    QVERIFY(!large->stream().stats().hugePages);
#endif

    AcqStreamOptions local = options(1024);
    local.numaNode = 0;
    auto bound = AcqWriter::create(makeInfo(4), local);
    QVERIFY(bound);
#if defined(__linux__) || defined(_WIN32)
    QCOMPARE(bound->stream().stats().numaNode, 0);
#else
    QCOMPARE(bound->stream().stats().numaNode, -1);
#endif
#if defined(__linux__)
    int node = -1;
    void* address = ringStart(*bound);
    QCOMPARE(syscall(SYS_get_mempolicy, &node, nullptr, 0UL, address, 3UL), 0L); // MPOL_F_NODE | MPOL_F_ADDR
    QCOMPARE(node, 0);
#endif
    AcqStreamOptions missing = options(1024);
    missing.numaNode = 4000;
    auto unbound = AcqWriter::create(makeInfo(4), missing);
    QVERIFY(unbound);
    QCOMPARE(unbound->stream().stats().numaNode, -1);
    QVERIFY(produce(*unbound, 8));

    AcqStreamOptions locked = options(64);
    locked.lockMemory = true;
    auto pinned = AcqWriter::create(makeInfo(2), locked);
    QVERIFY(pinned);
#if !defined(_WIN32)
    // One page fits every default RLIMIT_MEMLOCK; the ring reports what the kernel granted.
    void* probe = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    const bool lockable = probe != MAP_FAILED && mlock(probe, 4096) == 0;
    if (probe != MAP_FAILED) {
        munmap(probe, 4096);
    }
    QCOMPARE(pinned->stream().stats().memoryLocked, lockable);
#else
    QVERIFY(pinned->stream().stats().memoryLocked);
#endif
}

//=============================================================================================================

void TestAcqStream::testMirroredRingSinglePart()
{
    // 4096 frames of 16 bytes = 64 KiB, a multiple of every page size and of the Windows mapping granularity.
    constexpr std::size_t kCapacity = 4096;
    auto writer = AcqWriter::create(makeInfo(4), options(kCapacity));
    QVERIFY(writer);
#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
    QVERIFY(writer->stream().stats().mirrored);
#endif
    if (!writer->stream().stats().mirrored) {
        QSKIP("no mirror mapping on this platform");
    }
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    auto start = writer->beginWrite(kCapacity - 100);
    QVERIFY(start);
    std::byte* const ringStart = start->part(0).data;
    fill(*start);
    writer->commit(kCapacity - 100);
    auto head = reader->acquire(kCapacity - 100, 0ns);
    QVERIFY(head);
    QVERIFY(reader->release(*head).ok());

    // A block across the end of the ring is one contiguous part, written and read through the mirror.
    auto across = writer->beginWrite(300);
    QVERIFY(across);
    QCOMPARE(across->partCount(), std::size_t(1));
    QCOMPARE(across->part(0).samples, std::size_t(300));
    fill(*across);
    writer->commit(300);
    auto view = reader->acquire(300, 0ns);
    QVERIFY(view);
    QCOMPARE(view->partCount(), std::size_t(1));
    QVERIFY(matches(*view));
    QVERIFY(reader->release(*view).ok());

    // The mirror is the same memory: sample kCapacity, written past the end, sits at the start of the ring.
    std::int32_t wrapped = 0;
    std::memcpy(&wrapped, ringStart + 2 * sizeof(std::int32_t), sizeof(wrapped));
    QCOMPARE(wrapped, value(kCapacity, 2));

    AcqStreamOptions unmirrored = options(kCapacity);
    unmirrored.mirror = false;
    auto plain = AcqWriter::create(makeInfo(4), unmirrored);
    QVERIFY(!plain->stream().stats().mirrored);
    AcqStreamOptions planar = options(kCapacity);
    auto rows = AcqWriter::create(makeInfo(4, AcqLayout::Planar), planar);
    QVERIFY(!rows->stream().stats().mirrored);
    AcqStreamOptions huge = options(kCapacity);
    huge.hugePages = true;
    QVERIFY(!AcqWriter::create(makeInfo(4), huge)->stream().stats().mirrored);
    QVERIFY(!AcqWriter::create(makeInfo(4), options(64))->stream().stats().mirrored);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestAcqStream)
#include "test_acq_stream.moc"
