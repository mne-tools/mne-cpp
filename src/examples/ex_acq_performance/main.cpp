//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Closed-loop acquisition and stimulation with mne_acq, and its sustained throughput per device class.
 *
 * Part 1 emulates one card with a 1024-channel, 30 kHz, int16 recording input and a stimulator output on the
 * same clock (an intracortical array with a stimulator, as in closed-loop neuromodulation). One acquisition
 * thread writes 1 ms blocks into the input ring and plays the output ring, like the card's DMA engine.
 * Three independent readers consume the same input ring. A recorder (Lossless) checks every sample of every
 * channel against the generator and every spike marker. A display (Latest) draws the newest 250 ms of channel 0
 * and the RMS of four channels. A closed-loop controller (LowLatency) detects spikes on channel 0 and schedules
 * a biphasic stimulus exactly 3 ms after each spike; the output side verifies that every pulse starts on that
 * exact sample. Commit-to-wake latency of the controller and heap allocations during acquisition are measured.
 *
 * Part 2 offers the data rate of real device classes (64-channel EEG up to a 2 GB/s digitizer card) from an
 * emulated DMA engine that writes into the ring itself: the ring is the card's buffer, the driver grants
 * the card space ahead and commits what it wrote, so the host never copies a sample. A recorder checksums
 * every byte, next to a display and a low-latency reader.
 *
 * Every number printed is measured; the program exits non-zero if any sample is wrong, a stimulus lands on a
 * wrong sample, or the data path allocates.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq.h>
#if defined(_WIN32)
#define NOMINMAX
#include <io.h>
#include <malloc.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string_view>
#include <thread>
#include <vector>

//=============================================================================================================
// HEAP ALLOCATION COUNTER
//=============================================================================================================

namespace
{
std::atomic<bool> g_countAllocations{false};
std::atomic<std::uint64_t> g_allocations{0};

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
    void* p = alignment <= alignof(std::max_align_t) ? std::malloc(size)
                                                     : std::aligned_alloc(alignment, (size + alignment - 1) / alignment * alignment);
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

void* operator new(std::size_t n)
{
    return countedAllocOrThrow(n, alignof(std::max_align_t));
}
void* operator new[](std::size_t n)
{
    return countedAllocOrThrow(n, alignof(std::max_align_t));
}
void* operator new(std::size_t n, std::align_val_t a)
{
    return countedAllocOrThrow(n, static_cast<std::size_t>(a));
}
void* operator new[](std::size_t n, std::align_val_t a)
{
    return countedAllocOrThrow(n, static_cast<std::size_t>(a));
}
void* operator new(std::size_t n, const std::nothrow_t&) noexcept
{
    return countedAlloc(n, alignof(std::max_align_t));
}
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept
{
    return countedAlloc(n, alignof(std::max_align_t));
}
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept
{
    return countedAlloc(n, static_cast<std::size_t>(a));
}
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept
{
    return countedAlloc(n, static_cast<std::size_t>(a));
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
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

namespace
{

//=============================================================================================================
// HELPERS
//=============================================================================================================

std::int64_t nowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
}

/**
 * Starts a thread that takes its place per @p policy and then waits for @p go to run @p body; returns once the
 * thread is placed, so threads exist before allocations are counted. Placement is best effort: without pinning
 * (macOS) or the right to raise the priority (Linux without CAP_SYS_NICE) the thread still runs.
 */
template<typename Body>
std::thread startGated(std::atomic<bool>& go, Body body, const AcqThreadPolicy& policy = {})
{
    std::atomic<bool> placed{false};
    std::thread thread([&go, &placed, body, policy] {
        if (!policy.cpus.empty()) {
            static_cast<void>(acqPinThisThread(policy.cpus));
        }
        static_cast<void>(acqSetThisThreadPriority(policy.priority));
        placed.store(true, std::memory_order_release); // the thread's last access to placed
        while (!go.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        body();
    });
    while (!placed.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    return thread;
}

bool isTerminal()
{
#if defined(_WIN32)
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

int terminalColumns()
{
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO console{};
    return GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console) != 0
        ? console.srWindow.Right - console.srWindow.Left + 1
        : 0;
#else
    winsize ws{};
    return ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 ? ws.ws_col : 0;
#endif
}

constexpr std::uint64_t mix(std::uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27U)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31U);
}

/** Fixed-size latency histogram (1 us bins up to 2 ms); written by one thread, read by another. */
class LatencyHistogram
{
public:
    void add(std::int64_t ns) noexcept
    {
        const auto us = static_cast<std::size_t>(std::clamp<std::int64_t>(ns / 1000, 0, kBins - 1));
        m_bins[us].fetch_add(1, std::memory_order_relaxed);
        m_count.fetch_add(1, std::memory_order_relaxed);
        if (ns > m_maxNs.load(std::memory_order_relaxed)) {
            m_maxNs.store(ns, std::memory_order_relaxed);
        }
    }

    [[nodiscard]] std::int64_t maxUs() const noexcept
    {
        return (m_maxNs.load(std::memory_order_relaxed) + 999) / 1000;
    }

    [[nodiscard]] std::uint64_t count() const noexcept
    {
        return m_count.load(std::memory_order_relaxed);
    }

    /** @return Upper bound in microseconds of the @p q quantile; kBins means "above the range". */
    [[nodiscard]] std::int64_t quantileUs(double q) const noexcept
    {
        const auto total = count();
        if (total == 0) {
            return 0;
        }
        const auto target = static_cast<std::uint64_t>(std::ceil(q * static_cast<double>(total)));
        std::uint64_t seen = 0;
        for (std::int64_t i = 0; i < kBins; ++i) {
            seen += m_bins[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
            if (seen >= target) {
                return i + 1;
            }
        }
        return kBins;
    }

private:
    static constexpr std::int64_t kBins = 2000;
    std::array<std::atomic<std::uint64_t>, kBins> m_bins{};
    std::atomic<std::uint64_t> m_count{0};
    std::atomic<std::int64_t> m_maxNs{0};
};

void formatLatency(char* out, std::size_t size, std::int64_t us)
{
    if (us >= 2000) {
        std::snprintf(out, size, "%.1f ms", static_cast<double>(us) / 1000.0);
    } else {
        std::snprintf(out, size, "%lld us", static_cast<long long>(us));
    }
}

//=============================================================================================================
// PART 1: CLOSED-LOOP RECORDING AND STIMULATION
//=============================================================================================================

constexpr std::size_t kChannels = 1024;
constexpr double kSfreq = 30000.0;
constexpr std::size_t kBlock = 30;           // 1 ms
constexpr std::uint64_t kSpikePeriod = 3000; // a spike on channel 0 every 100 ms
constexpr std::uint64_t kSpikeOffset = 1000;
constexpr std::uint64_t kSpikeWidth = 15;
constexpr std::int16_t kSpikeAmplitude = 9000;
constexpr std::int16_t kSpikeThreshold = 6000;
constexpr std::uint64_t kStimDelay = 90;                                // 3 ms after the spike, on the same clock
constexpr auto kStimDelayTicks = static_cast<std::int64_t>(kStimDelay); // the same delay in card-clock ticks (one per sample)
constexpr std::size_t kStimWidth = 30;                                  // 0.5 ms cathodic + 0.5 ms anodic
constexpr std::int16_t kStimAmplitude = 20000;
constexpr std::size_t kCommitSlots = 4096;
constexpr auto kBiphasic = [] {
    std::array<double, kStimWidth> pulse{};
    for (std::size_t j = 0; j < kStimWidth; ++j) {
        pulse[j] = j < kStimWidth / 2 ? -kStimAmplitude : kStimAmplitude;
    }
    return pulse;
}();

/** Neural-like test signal: per-channel oscillation, noise and, on channel 0, a periodic spike. */
class SignalGenerator
{
public:
    SignalGenerator()
    {
        for (std::size_t i = 0; i < m_sine.size(); ++i) {
            m_sine[i] = static_cast<std::int16_t>(std::lround(1500.0 * std::sin(2.0 * 3.14159265358979323846 * static_cast<double>(i) / 4096.0)));
        }
        for (std::size_t c = 0; c < kChannels; ++c) {
            const double hz = 4.0 + static_cast<double>(c % 16);
            m_increment[c] = static_cast<std::uint64_t>(hz * 4096.0 * 4294967296.0 / kSfreq);
        }
    }

    [[nodiscard]] std::int16_t value(std::uint64_t s, std::size_t c) const noexcept
    {
        int v = m_sine[((s * m_increment[c]) >> 32U) & 4095U] + static_cast<int>(mix(s * kChannels + c) & 511U) - 256;
        if (c == 0 && s >= kSpikeOffset && (s - kSpikeOffset) % kSpikePeriod < kSpikeWidth) {
            v += kSpikeAmplitude;
        }
        return static_cast<std::int16_t>(v);
    }

    [[nodiscard]] static bool isSpikeStart(std::uint64_t s) noexcept
    {
        return s >= kSpikeOffset && (s - kSpikeOffset) % kSpikePeriod == 0;
    }

private:
    std::array<std::int16_t, 4096> m_sine{};
    std::array<std::uint64_t, kChannels> m_increment{};
};

struct ClosedLoopState
{
    std::atomic<bool> running{true};
    std::array<std::atomic<std::int64_t>, kCommitSlots> commitNs{};
    LatencyHistogram wakeLatency;

    std::atomic<std::uint64_t> recordedBytes{0};
    std::atomic<std::uint64_t> recordedSamples{0};
    std::atomic<std::uint64_t> sampleMismatches{0};
    std::atomic<std::uint64_t> markersVerified{0};
    std::atomic<std::uint64_t> markerMismatches{0};

    std::atomic<std::uint64_t> spikesInjected{0};
    std::atomic<std::uint64_t> spikesDetected{0};
    std::atomic<std::uint64_t> stimuliScheduled{0};
    std::atomic<std::uint64_t> stimuliLate{0};
    std::atomic<std::uint64_t> stimuliDropped{0}; // committed, but the card had already passed their start
    std::atomic<std::uint64_t> stimuliExact{0};
    std::atomic<std::uint64_t> stimuliMisplaced{0};
    std::atomic<std::uint64_t> pulseSampleErrors{0}; // played samples off the biphasic shape
};

/** The card: writes the input ring in 1 ms blocks at the real-time rate and plays the output ring. */
void runCard(AcqWriter& input, AcqReader& dac, const SignalGenerator& gen, ClosedLoopState& st, double seconds)
{
    const auto blocks = static_cast<std::uint64_t>(seconds * kSfreq / static_cast<double>(kBlock));
    const auto period = std::chrono::nanoseconds(static_cast<std::int64_t>(1e9 * kBlock / kSfreq));
    const auto start = Clock::now();
    bool previousNonZero = false;
    std::uint64_t pulseStart = 0;
    for (std::uint64_t k = 0; k < blocks; ++k) {
        std::this_thread::sleep_until(start + period * static_cast<std::int64_t>(k + 1));

        //! [acq_writer_commit]
        auto view = input.beginWrite(kBlock);
        std::uint64_t s = view->firstSample();
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqWritePart& part = view->part(p);
            for (std::size_t i = 0; i < part.samples; ++i, ++s) {
                auto* frame = reinterpret_cast<std::int16_t*>(part.at(0, i));
                for (std::size_t c = 0; c < kChannels; ++c) {
                    frame[c] = gen.value(s, c);
                }
                if (SignalGenerator::isSpikeStart(s)) {
                    AcqEvent marker;
                    marker.sample = s;
                    marker.duration = kSpikeWidth;
                    marker.code = 1;
                    input.pushEvent(marker);
                    st.spikesInjected.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }
        st.commitNs[k % kCommitSlots].store(nowNs(), std::memory_order_relaxed);
        input.commit(kBlock);
        //! [acq_writer_commit]

        // Same card, same clock: the DAC plays the samples of this millisecond.
        std::size_t got = 0;
        if (auto out = dac.acquire(0, 0ns, kBlock)) {
            got = out->sampleCount();
            std::uint64_t t = out->firstSample();
            for (std::size_t p = 0; p < out->partCount(); ++p) {
                for (std::size_t i = 0; i < out->part(p).samples; ++i, ++t) {
                    std::int16_t v = 0;
                    std::memcpy(&v, out->part(p).at(0, i), sizeof(v));
                    const bool nonZero = v != 0;
                    if (nonZero && !previousNonZero) {
                        const bool exact = t >= kStimDelay && SignalGenerator::isSpikeStart(t - kStimDelay);
                        (exact ? st.stimuliExact : st.stimuliMisplaced).fetch_add(1, std::memory_order_relaxed);
                        pulseStart = t;
                    }
                    if (nonZero) {
                        const std::int16_t expected =
                            t - pulseStart < kStimWidth / 2 ? -kStimAmplitude : kStimAmplitude;
                        st.pulseSampleErrors.fetch_add(v != expected ? 1U : 0U, std::memory_order_relaxed);
                    }
                    previousNonZero = nonZero;
                }
            }
            dac.release(*out);
        }
        if (got < kBlock) {
            dac.consumeIdle(kBlock - got);
            previousNonZero = false;
        }
    }
    input.finish();
    st.running.store(false, std::memory_order_release);
}

/** Lossless recorder: every sample of every channel and every spike marker is checked. */
void runRecorder(AcqReader& reader, const SignalGenerator& gen, ClosedLoopState& st)
{
    while (true) {
        //! [acq_reader_acquire]
        auto view = reader.acquire(kBlock, 100ms);
        if (!view) {
            if (view.error() == AcqErrorCode::EndOfStream) {
                break;
            }
            continue;
        }
        //! [acq_reader_acquire]
        //! [acq_reader_block]
        std::uint64_t s = view->firstSample();
        std::uint64_t mismatches = 0;
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t i = 0; i < part.samples; ++i, ++s) {
                const auto* frame = reinterpret_cast<const std::int16_t*>(part.at(0, i));
                for (std::size_t c = 0; c < kChannels; ++c) {
                    mismatches += frame[c] != gen.value(s, c) ? 1U : 0U;
                }
            }
        }
        for (std::size_t e = 0; e < view->events().size(); ++e) {
            const AcqEvent& marker = view->events()[e];
            const bool ok = SignalGenerator::isSpikeStart(marker.sample) && marker.duration == kSpikeWidth;
            (ok ? st.markersVerified : st.markerMismatches).fetch_add(1, std::memory_order_relaxed);
        }
        //! [acq_reader_block]
        const std::size_t n = view->sampleCount();
        if (reader.release(*view).ok()) {
            st.sampleMismatches.fetch_add(mismatches, std::memory_order_relaxed);
            st.recordedSamples.fetch_add(n, std::memory_order_relaxed);
            st.recordedBytes.fetch_add(n * kChannels * sizeof(std::int16_t), std::memory_order_relaxed);
        }
    }
}

/** Closed-loop controller: spike on channel 0 -> biphasic stimulus exactly kStimDelay samples later. */
void runController(AcqReader& reader, AcqWriter& stimulator, ClosedLoopState& st)
{
    const AcqTimebase inClock = reader.stream().timebase();
    const AcqTimebase outClock = stimulator.stream().timebase();
    bool above = false;
    while (true) {
        auto view = reader.acquire(1, 100ms);
        if (!view) {
            if (view.error() == AcqErrorCode::EndOfStream) {
                break;
            }
            continue;
        }
        const std::uint64_t lastBlock = (view->firstSample() + view->sampleCount() - 1) / kBlock;
        st.wakeLatency.add(nowNs() - st.commitNs[lastBlock % kCommitSlots].load(std::memory_order_relaxed));

        std::uint64_t s = view->firstSample();
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t i = 0; i < part.samples; ++i, ++s) {
                std::int16_t v = 0;
                std::memcpy(&v, part.at(0, i), sizeof(v));
                const bool now = v > kSpikeThreshold;
                if (now && !above) {
                    st.spikesDetected.fetch_add(1, std::memory_order_relaxed);
                    //! [acq_writer_schedule]
                    // The output sample whose instant is kStimDelay clock ticks after input sample s.
                    bool scheduled = false;
                    if (auto at = mapSampleExact(inClock, outClock, s, kStimDelayTicks)) {
                        if (auto pulse = stimulator.beginWriteAt(*at, kStimWidth, 0ns)) {
                            // encode() handles a pulse that crosses the end of the ring (two parts).
                            AcqConvertOptions raw;
                            raw.scaling = AcqScaling::Raw;
                            (void)encode(AcqMatrixRef<const double>::rowMajor(kBiphasic.data(), 1, kStimWidth), *pulse,
                                         raw);
                            stimulator.commit(kStimWidth);
                            scheduled = true;
                        }
                    }
                    (scheduled ? st.stimuliScheduled : st.stimuliLate).fetch_add(1, std::memory_order_relaxed);
                    //! [acq_writer_schedule]
                }
                above = now;
            }
        }
        reader.release(*view);
    }
}

/** Display: newest 250 ms of channel 0 as a sparkline (spikes as full blocks), RMS of four channels. */
struct DisplayFrame
{
    std::array<char, 256> trace{};
    std::array<double, 4> rms{};
};

void updateDisplay(AcqReader& display, DisplayFrame& frame)
{
    constexpr std::size_t kWindow = 7500;
    constexpr std::size_t kColumns = 60;
    constexpr std::size_t kSamplesPerColumn = kWindow / kColumns;
    static_assert(kSamplesPerColumn * kColumns == kWindow);
    auto view = display.acquire(kWindow, 50ms, kWindow);
    if (!view) {
        return;
    }
    std::array<double, kColumns> mean{};
    std::array<bool, kColumns> spike{};
    std::array<double, 4> sum{};
    std::size_t i = 0;
    for (std::size_t p = 0; p < view->partCount(); ++p) {
        const AcqBlockPart& part = view->part(p);
        for (std::size_t j = 0; j < part.samples; ++j, ++i) {
            const auto* f = reinterpret_cast<const std::int16_t*>(part.at(0, j));
            const std::size_t col = i * kColumns / kWindow;
            mean[col] += f[0];
            spike[col] = spike[col] || f[0] > kSpikeThreshold;
            for (std::size_t c = 0; c < 4; ++c) {
                sum[c] += static_cast<double>(f[c * 256]) * static_cast<double>(f[c * 256]);
            }
        }
    }
    display.release(*view);

    static constexpr std::array<std::string_view, 8> kLevels{"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    std::size_t pos = 0;
    for (std::size_t col = 0; col < kColumns; ++col) {
        const double m = mean[col] / static_cast<double>(kSamplesPerColumn);
        const int level = spike[col] ? 7 : std::clamp(static_cast<int>((m + 1600.0) * 7.0 / 3200.0), 0, 6);
        const std::string_view glyph = kLevels[static_cast<std::size_t>(level)];
        std::memcpy(frame.trace.data() + pos, glyph.data(), glyph.size());
        pos += glyph.size();
    }
    frame.trace[pos] = '\0';
    for (std::size_t c = 0; c < 4; ++c) {
        frame.rms[c] = std::sqrt(sum[c] / static_cast<double>(i));
    }
}

constexpr int kDashboardLines = 11;

void printDashboard(const ClosedLoopState& st, const DisplayFrame& frame, const AcqReader& recorder,
                    double elapsed, bool redraw)
{
    if (redraw) {
        std::printf("\x1b[%dA\x1b[J", kDashboardLines);
    }
    std::array<char, 32> p50{}, p99{}, p999{}, max{};
    formatLatency(p50.data(), p50.size(), st.wakeLatency.quantileUs(0.50));
    formatLatency(p99.data(), p99.size(), st.wakeLatency.quantileUs(0.99));
    formatLatency(p999.data(), p999.size(), st.wakeLatency.quantileUs(0.999));
    formatLatency(max.data(), max.size(), st.wakeLatency.maxUs());
    const auto u = [](const std::atomic<std::uint64_t>& v) {
        return static_cast<unsigned long long>(v.load());
    };
    std::printf("  t = %4.1f s    in: 1024 ch x 30 kHz int16 = 61.4 MB/s    out: 1 stim ch, same clock\n", elapsed);
    std::printf("  Recorder    Lossless    %7.1f MB, %llu x 1024 samples checked: %llu wrong, %llu lost\n",
                static_cast<double>(st.recordedBytes.load()) / 1e6, u(st.recordedSamples), u(st.sampleMismatches),
                static_cast<unsigned long long>(recorder.stats().lost));
    std::printf("                          spike markers: %llu at the right sample, %llu wrong\n", u(st.markersVerified),
                u(st.markerMismatches));
    std::printf("  Display     Latest      %s  ch 0\n", frame.trace.data());
    std::printf("                          RMS ch 0/256/512/768: %.0f %.0f %.0f %.0f counts (last 250 ms)\n",
                frame.rms[0], frame.rms[1], frame.rms[2], frame.rms[3]);
    std::printf("  Controller  LowLatency  wake after commit: p50 %s  p99 %s  p99.9 %s  max %s\n", p50.data(),
                p99.data(), p999.data(), max.data());
    std::printf("                          spikes %llu injected, %llu detected -> %llu stimuli scheduled, %llu refused late\n",
                u(st.spikesInjected), u(st.spikesDetected), u(st.stimuliScheduled), u(st.stimuliLate));
    std::printf("  Stimulator  output      %llu pulses on the exact sample (spike + 3.000 ms), %llu misplaced,\n"
                "                          %llu samples off the biphasic shape, %llu dropped by the card as late\n",
                u(st.stimuliExact), u(st.stimuliMisplaced), u(st.pulseSampleErrors), u(st.stimuliDropped));
    std::printf("  Data path   heap allocations after start: %llu\n",
                static_cast<unsigned long long>(g_allocations.load()));
    std::printf("\n");
    std::fflush(stdout);
}

bool runClosedLoop(double seconds)
{
    std::printf("\nPart 1 - closed-loop recording and stimulation on one card (one clock): a spike on channel 0\n"
                "         triggers a biphasic pulse 3 ms later; three readers share the input ring\n\n");

    //! [acq_stream_info]
    AcqStreamInfo inInfo;
    inInfo.name = "neural";
    inInfo.format = AcqSampleFormat::Int16;
    inInfo.sfreq = kSfreq;
    inInfo.channels.resize(kChannels);
    for (std::size_t c = 0; c < kChannels; ++c) {
        inInfo.channels[c].name = "e" + std::to_string(c);
        inInfo.channels[c].kind = AcqChannelKind::Seeg;
        inInfo.channels[c].unit = AcqUnit::Volt;
        inInfo.channels[c].scale = 0.195e-6;
    }
    //! [acq_stream_info]
    AcqStreamInfo outInfo;
    outInfo.name = "stim";
    outInfo.direction = AcqDirection::Output;
    outInfo.format = AcqSampleFormat::Int16;
    outInfo.sfreq = kSfreq;
    outInfo.channels.resize(1);
    outInfo.channels[0].name = "stim0";
    outInfo.channels[0].kind = AcqChannelKind::StimCurrent;
    outInfo.channels[0].unit = AcqUnit::Ampere;
    outInfo.channels[0].scale = 1e-6 / 400.0;
    outInfo.underrunFill = AcqUnderrunFill::Zero;

    //! [acq_stream_setup]
    auto input = AcqWriter::create(inInfo);
    auto stimulator = AcqWriter::create(outInfo);
    if (!input || !stimulator) {
        std::fprintf(stderr, "%s\n", (!input ? input.error() : stimulator.error()).text().c_str());
        return false;
    }
    AcqReaderOptions gating;
    gating.gating = true;
    auto dac = stimulator->stream().reader(AcqReader::ReadMode::Lossless, gating);
    auto recorder = input->stream().reader(AcqReader::ReadMode::Lossless);
    auto display = input->stream().reader(AcqReader::ReadMode::Latest);
    auto controller = input->stream().reader(AcqReader::ReadMode::LowLatency);
    //! [acq_stream_setup]

    //! [acq_timebase_setup]
    // One card, one clock: both streams start on the same hardware trigger, so their samples are
    // phase-locked and the controller can schedule a delay sample-exact from input to output.
    AcqTimebase cardClock;
    cardClock.domain = acqClockDomainId("demo-card:0");
    cardClock.ticksPerSecond = static_cast<std::uint64_t>(kSfreq);
    cardClock.lock = AcqLockState::PhaseLocked;
    input->stream().setTimebase(cardClock);
    stimulator->stream().setTimebase(cardClock);
    //! [acq_timebase_setup]

    const SignalGenerator gen;
    ClosedLoopState st;
    DisplayFrame frame;
    const bool live = isTerminal() && terminalColumns() >= 100;

    std::atomic<bool> go{false};
    std::thread card = startGated(go, [&] { runCard(*input, *dac, gen, st, seconds); });
    std::thread rec = startGated(go, [&] { runRecorder(*recorder, gen, st); });
    //! [acq_thread_policy]
    // The closed-loop controller spins on the newest samples: pin it to a core of the ring's NUMA node, so it never
    // wakes up on a cold cache after the scheduler moved it, and raise it above other work.
    AcqThreadPolicy controllerPolicy;
    controllerPolicy.priority = AcqThreadPriority::High;
    if (const auto cpus = acqNumaNodeCpus(std::max(0, input->stream().stats().numaNode)); cpus && cpus->size() > 1) {
        controllerPolicy.cpus = {cpus->back()};
    }
    std::thread loop = startGated(go, [&] { runController(*controller, *stimulator, st); }, controllerPolicy);
    //! [acq_thread_policy]

    if (!live) {
        std::printf("  running for %.1f s ...\n\n", seconds);
        std::fflush(stdout);
    }
    g_allocations = 0;
    g_countAllocations = true;
    go = true;
    const auto start = Clock::now();
    auto nextPrint = start;
    bool printed = false;
    while (st.running.load(std::memory_order_acquire)) {
        updateDisplay(*display, frame);
        st.stimuliDropped = stimulator->stream().stats().lateCommits;
        if (live && Clock::now() >= nextPrint) {
            printDashboard(st, frame, *recorder, std::chrono::duration<double>(Clock::now() - start).count(), printed);
            printed = true;
            nextPrint += 200ms;
        }
        std::this_thread::sleep_for(20ms);
    }
    card.join();
    rec.join();
    loop.join();
    g_countAllocations = false;
    st.stimuliDropped = stimulator->stream().stats().lateCommits;
    printDashboard(st, frame, *recorder, std::chrono::duration<double>(Clock::now() - start).count(), printed);

    //! [acq_stream_stats]
    const AcqStreamStats produced = input->stream().stats();
    const AcqReaderStats recorded = recorder->stats();
    const bool lossless = recorded.lost == 0 && recorded.eventsLost == 0 && recorded.delivered == produced.committed;
    //! [acq_stream_stats]
    const bool ok = lossless && st.sampleMismatches == 0 && st.markerMismatches == 0 &&
        st.markersVerified == st.spikesInjected && st.stimuliMisplaced == 0 && st.pulseSampleErrors == 0 &&
        st.stimuliExact + st.stimuliDropped == st.stimuliScheduled && g_allocations == 0 &&
        st.recordedSamples == static_cast<std::uint64_t>(seconds * kSfreq / kBlock) * kBlock;
    std::printf("  %s\n", ok ? "PASS: every sample bit-exact and none lost, every stimulus on its exact sample or\n"
                               "        refused or dropped as late, never truncated or shifted; no heap allocation on the data path"
                             : "FAIL: see the counters above");
    return ok;
}

//=============================================================================================================
// PART 2: SUSTAINED THROUGHPUT PER DEVICE CLASS
//=============================================================================================================

struct DeviceClass
{
    const char* name;
    std::size_t channels;
    double sfreq;
    AcqSampleFormat format;
};

struct ThroughputResult
{
    double offeredMBs = 0.0;
    double deliveredMBs = 0.0;
    std::uint64_t lost = 0;
    bool checksumOk = false;
    std::int64_t p50 = 0;
    std::int64_t p99 = 0;
    std::int64_t p999 = 0;
    std::uint64_t allocations = 0;
};

std::uint64_t sumWords(const std::byte* data, std::size_t bytes) noexcept
{
    std::uint64_t sum = 0;
    for (std::size_t i = 0; i + 8 <= bytes; i += 8) {
        std::uint64_t w = 0;
        std::memcpy(&w, data + i, 8);
        sum += w;
    }
    return sum;
}

ThroughputResult measure(const DeviceClass& dev, double seconds)
{
    AcqStreamInfo info;
    info.name = dev.name;
    info.format = dev.format;
    info.sfreq = dev.sfreq;
    info.channels.resize(dev.channels);
    const std::size_t block = std::max<std::size_t>(1, static_cast<std::size_t>(dev.sfreq / 1000.0));
    const std::size_t blockBytes = block * info.frameBytes();

    // Four pre-generated DMA buffers the emulated card copies from, with their checksums.
    constexpr std::size_t kPatterns = 4;
    std::vector<std::byte> patterns(kPatterns * blockBytes);
    std::array<std::uint64_t, kPatterns> patternSum{};
    for (std::size_t i = 0; i < patterns.size(); i += 8) {
        const std::uint64_t w = mix(i);
        std::memcpy(patterns.data() + i, &w, std::min<std::size_t>(8, patterns.size() - i));
    }
    for (std::size_t p = 0; p < kPatterns; ++p) {
        patternSum[p] = sumWords(patterns.data() + p * blockBytes, blockBytes);
    }

    // The card's DMA buffer is the ring (2 s of data, at most 256 MiB, power of two as a ring needs).
    std::size_t capacity = std::bit_ceil(std::max<std::size_t>(65536, static_cast<std::size_t>(2.0 * dev.sfreq)));
    while (capacity * info.frameBytes() > (std::size_t(256) << 20)) {
        capacity /= 2;
    }
    const std::size_t ringBytes = capacity * info.frameBytes();
    AcqStreamOptions dma;
    dma.external.data = static_cast<std::byte*>(::operator new(ringBytes, std::align_val_t(4096)));
    dma.external.bytes = ringBytes;
    dma.external.memory = {AcqMemoryKind::HostPinned, 0};
    dma.external.release = [data = dma.external.data] {
        ::operator delete(data, std::align_val_t(4096));
    };
    std::byte* const cardBuffer = dma.external.data;
    auto writer = AcqWriter::create(info, dma);
    auto recorder = writer->stream().reader(AcqReader::ReadMode::Lossless);
    auto display = writer->stream().reader(AcqReader::ReadMode::Latest);
    auto lowLatency = writer->stream().reader(AcqReader::ReadMode::LowLatency);
    std::vector<std::atomic<std::int64_t>> commitNs(kCommitSlots);
    LatencyHistogram latency;
    std::atomic<std::uint64_t> recorderSum{0};
    std::atomic<std::uint64_t> recordedBytes{0};
    std::uint64_t producerSum = 0;
    const auto blocks = static_cast<std::uint64_t>(seconds * dev.sfreq / static_cast<double>(block));

    std::atomic<bool> go{false};
    std::thread rec = startGated(go, [&] {
        std::uint64_t sum = 0;
        std::uint64_t bytes = 0;
        while (true) {
            auto view = recorder->acquire(block, 100ms);
            if (!view) {
                if (view.error() == AcqErrorCode::EndOfStream) {
                    break;
                }
                continue;
            }
            for (std::size_t p = 0; p < view->partCount(); ++p) {
                const std::size_t n = view->part(p).samples * info.frameBytes();
                sum += sumWords(view->part(p).data, n);
                bytes += n;
            }
            recorder->release(*view);
        }
        recorderSum = sum;
        recordedBytes = bytes;
    });
    std::thread fast = startGated(go, [&] {
        while (true) {
            auto view = lowLatency->acquire(1, 100ms);
            if (!view) {
                if (view.error() == AcqErrorCode::EndOfStream) {
                    break;
                }
                continue;
            }
            const std::uint64_t last = (view->firstSample() + view->sampleCount() - 1) / block;
            latency.add(nowNs() - commitNs[last % kCommitSlots].load(std::memory_order_relaxed));
            lowLatency->release(*view);
        }
    });
    std::thread show = startGated(go, [&] {
        volatile std::uint64_t sink = 0;
        while (true) {
            auto view = display->acquire(block, 100ms, block);
            if (!view) {
                if (view.error() == AcqErrorCode::EndOfStream) {
                    break;
                }
                continue;
            }
            sink = sink + sumWords(view->part(0).data, std::min<std::size_t>(64, view->part(0).samples * info.frameBytes()));
            display->release(*view);
            std::this_thread::sleep_for(20ms);
        }
    });

    const auto period = std::chrono::nanoseconds(static_cast<std::int64_t>(1e9 * static_cast<double>(block) / dev.sfreq));
    g_allocations = 0;
    g_countAllocations = true;
    go = true;
    const auto start = Clock::now();
    constexpr std::uint64_t kInFlight = 8; // blocks granted to the card ahead of the data, like its notify queue
    std::uint64_t granted = 0;
    for (std::uint64_t k = 0; k < blocks; ++k) {
        //! [acq_dma_grant]
        // Driver: keep the card's queue full, so it never waits for the host.
        while (granted < std::min(k + kInFlight, blocks)) {
            writer->grant(block);
            ++granted;
        }
        // Card: its DMA engine writes block k straight into the ring (here a copy stands in for the bus).
        std::this_thread::sleep_until(start + period * static_cast<std::int64_t>(k + 1));
        const std::byte* src = patterns.data() + (k % kPatterns) * blockBytes;
        const std::size_t offset = ((k * block) % capacity) * info.frameBytes();
        const std::size_t head = std::min(blockBytes, ringBytes - offset);
        std::memcpy(cardBuffer + offset, src, head);
        std::memcpy(cardBuffer, src + head, blockBytes - head);
        // Driver: the card reports the block written; publish it to every reader.
        commitNs[k % kCommitSlots].store(nowNs(), std::memory_order_relaxed);
        writer->commit(block);
        //! [acq_dma_grant]
        producerSum += patternSum[k % kPatterns];
    }
    const double elapsed = std::chrono::duration<double>(Clock::now() - start).count();
    writer->finish();
    rec.join();
    fast.join();
    show.join();
    g_countAllocations = false;

    ThroughputResult r;
    r.offeredMBs = static_cast<double>(blocks * blockBytes) / elapsed / 1e6;
    r.deliveredMBs = static_cast<double>(recordedBytes.load()) / elapsed / 1e6;
    r.lost = recorder->stats().lost;
    r.checksumOk = r.lost == 0 && recorderSum.load() == producerSum;
    r.p50 = latency.quantileUs(0.50);
    r.p99 = latency.quantileUs(0.99);
    r.p999 = latency.quantileUs(0.999);
    r.allocations = g_allocations.load();
    return r;
}

bool runThroughput(double seconds)
{
    static constexpr std::array<DeviceClass, 6> kClasses{{
        {"64-ch EEG, 1 kHz float32", 64, 1000.0, AcqSampleFormat::Float32},
        {"256-ch HD-EEG/OPM, 5 kHz float32", 256, 5000.0, AcqSampleFormat::Float32},
        {"1000-ch array, 20 kHz float32", 1000, 20000.0, AcqSampleFormat::Float32},
        {"4 x 384-ch probes, 30 kHz int16", 1536, 30000.0, AcqSampleFormat::Int16},
        {"32-coil MR receiver, 10 MS/s cint16", 32, 10.0e6, AcqSampleFormat::ComplexInt16},
        {"4-ch digitizer card, 250 MS/s int16", 4, 250.0e6, AcqSampleFormat::Int16},
    }};
    std::printf("\nPart 2 - sustained throughput, the card writes 1 ms blocks into the ring by DMA (no host copy);\n"
                "         per stream: a Lossless recorder that checksums every byte, a Latest display and a\n"
                "         LowLatency reader (%.1f s per row)\n\n",
                seconds);
    std::printf("  %-38s %10s %10s %6s %9s %18s %7s\n", "device class", "offered", "recorded", "lost", "checksum",
                "wake p50/p99/p99.9", "allocs");
    bool ok = true;
    for (const DeviceClass& dev : kClasses) {
        const ThroughputResult r = measure(dev, seconds);
        std::array<char, 40> lat{};
        std::snprintf(lat.data(), lat.size(), "%lld/%lld/%lld us", static_cast<long long>(r.p50),
                      static_cast<long long>(r.p99), static_cast<long long>(r.p999));
        auto rate = [](double mbs, char* out, std::size_t n) {
            if (mbs >= 1000.0) {
                std::snprintf(out, n, "%.2f GB/s", mbs / 1000.0);
            } else {
                std::snprintf(out, n, "%.1f MB/s", mbs);
            }
        };
        std::array<char, 16> offered{}, recorded{};
        rate(r.offeredMBs, offered.data(), offered.size());
        rate(r.deliveredMBs, recorded.data(), recorded.size());
        std::printf("  %-38s %10s %10s %6llu %9s %18s %7llu\n", dev.name, offered.data(), recorded.data(),
                    static_cast<unsigned long long>(r.lost), r.lost != 0 ? "n/a" : (r.checksumOk ? "ok" : "WRONG"),
                    lat.data(), static_cast<unsigned long long>(r.allocations));
        std::fflush(stdout);
        ok = ok && r.allocations == 0 && (r.lost != 0 || r.checksumOk);
    }
    std::printf("\n  Lost samples are counted exactly per reader; the producer never waits for a slow reader.\n");
    return ok;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

namespace
{
/** Parses a duration of at least 0.1 s; @return false for anything else. */
bool parseSeconds(const char* text, double& seconds)
{
    char* end = nullptr;
    const double value = std::strtod(text, &end);
    if (end == text || *end != '\0' || !(value >= 0.1) || !std::isfinite(value)) {
        return false;
    }
    seconds = value;
    return true;
}
} // namespace

int main(int argc, char* argv[])
{
    double closedLoopSeconds = 10.0;
    double rowSeconds = 2.0;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--seconds" && i + 1 < argc && parseSeconds(argv[i + 1], closedLoopSeconds)) {
            ++i;
        } else if (arg == "--row-seconds" && i + 1 < argc && parseSeconds(argv[i + 1], rowSeconds)) {
            ++i;
        } else {
            std::printf("usage: %s [--seconds <closed-loop duration, default 10>] [--row-seconds <per throughput row, "
                        "default 2>]\n",
                        argv[0]);
            return arg == "--help" || arg == "-h" ? 0 : 2;
        }
    }

    std::printf("MNE-CPP acquisition (mne_acq %s) - standard C++20 only, no Qt, no Eigen on the data path\n",
                mne::acq_build_hash());
    const bool closedLoopOk = runClosedLoop(closedLoopSeconds);
    const bool throughputOk = runThroughput(rowSeconds);
    return closedLoopOk && throughputOk ? 0 : 1;
}
