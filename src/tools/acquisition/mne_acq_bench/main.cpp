//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    mne_acq_bench: performance suite of the acquisition library.
 *
 * Measures the per-call cost of the stream protocol, commit-to-wake latency, sustained ring throughput over a
 * channel sweep and the converters, and writes a table and optionally a JSON report. The measured code is
 * standard C++ against the mne_acq API only, so the tool also builds against an older mne_acq for A/B runs.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq.h>
#if defined(MNE_ACQ_BENCH_RECORDING) // the standalone A/B build has no FIFF bridge
#include <acq/fiff/acq_recorder.h>

#include <QTemporaryDir>
#endif

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <complex>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#ifndef MNE_CPP_VERSION // set by the MNE-CPP build; absent in the standalone A/B build (scripts/ci/acq_bench)
#define MNE_CPP_VERSION "standalone"
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

namespace
{
using Clock = std::chrono::steady_clock;

struct Metric
{
    std::string name;
    double value = 0.0;
    const char* unit = "";
    bool higherIsBetter = false;
    double target = 0.0; /**< Performance target, 0 if none; a minimum if higherIsBetter, else a maximum. */
};

struct Settings
{
    bool quick = false;
    int repeats = 3;
    double pacedSeconds = 0.0; /**< Duration of each paced run; 0 = 2 s with --quick, else 10 s. */
};

double secondsSince(Clock::time_point start)
{
    return std::chrono::duration<double>(Clock::now() - start).count();
}

std::int64_t nowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
}

AcqStreamInfo streamInfo(std::size_t channels, AcqSampleFormat format, AcqLayout layout = AcqLayout::Interleaved,
                         AcqDirection direction = AcqDirection::Input)
{
    AcqStreamInfo info;
    info.name = "bench";
    info.format = format;
    info.layout = layout;
    info.direction = direction;
    info.sfreq = 1000.0;
    for (std::size_t c = 0; c < channels; ++c) {
        AcqChannelInfo ch;
        ch.name = "ch" + std::to_string(c);
        ch.scale = 1e-6;
        ch.offset = 0.5;
        info.channels.push_back(ch);
    }
    return info;
}

AcqStreamOptions ringOptions(std::size_t capacity)
{
    AcqStreamOptions options;
    options.capacitySamples = capacity;
    options.lockMemory = false;
    return options;
}

/**
 * Shortest time of @p call over @p trials trials of @p calls calls each, in seconds per call. Interrupts and core
 * migrations only ever add time, so the minimum is the stable estimate of the cost itself.
 */
template<typename Call>
double minSecondsPerCall(int trials, int calls, Call call)
{
    double best = 1e300;
    for (int t = 0; t < trials; ++t) {
        const auto start = Clock::now();
        for (int i = 0; i < calls; ++i) {
            call();
        }
        best = std::min(best, secondsSince(start) / calls);
    }
    return best;
}

/** Best of the repeats: the least disturbed run, which is what a regression check compares. */
template<typename Measure>
double best(const Settings& settings, bool higherIsBetter, Measure measure)
{
    double result = measure();
    for (int r = 1; r < settings.repeats && result >= 0.0; ++r) {
        const double v = measure();
        result = v < 0.0 ? v : (higherIsBetter ? std::max(result, v) : std::min(result, v)); // a loss is never hidden
    }
    return result;
}

/** ns per beginWrite + commit + @p readers x (acquire + release), one thread, one-sample blocks. */
double opCost(int readers, int ops)
{
    auto writer = AcqWriter::create(streamInfo(1, AcqSampleFormat::Float32), ringOptions(4096));
    std::vector<AcqReader> all;
    all.reserve(static_cast<std::size_t>(readers));
    for (int i = 0; i < readers; ++i) {
        all.push_back(std::move(writer->stream().reader(AcqReader::ReadMode::Lossless).value()));
    }
    return 1e9 * minSecondsPerCall(50, ops / 50, [&] {
               static_cast<void>(writer->beginWrite(1));
               writer->commit(1);
               for (AcqReader& reader : all) {
                   auto view = reader.acquire(1, 0ns);
                   static_cast<void>(reader.release(*view));
               }
           });
}

/** CPUs of a producer and its consumer thread; -1 = wherever the scheduler puts them. */
using CpuPair = std::array<int, 2>;
constexpr CpuPair kUnpinned{-1, -1};

/** Pins the calling thread to @p cpu, unless it is -1. */
void pinTo(int cpu)
{
    if (cpu >= 0) {
        const auto c = static_cast<unsigned>(cpu);
        static_cast<void>(acqPinThisThread(std::span<const unsigned>(&c, 1)));
    }
}

/** @return Two CPUs of NUMA node 0, the ring's node, or nothing where threads cannot be pinned (macOS). */
std::optional<CpuPair> pinnablePair()
{
    const auto cpus = acqNumaNodeCpus(0);
    if (!cpus || cpus->size() < 2) {
        return std::nullopt;
    }
    // The last two: CPU 0 takes most interrupts.
    const CpuPair pair{static_cast<int>((*cpus)[cpus->size() - 1]), static_cast<int>((*cpus)[cpus->size() - 2])};
    bool pinned = false;
    std::thread([&] {
        const auto c = static_cast<unsigned>(pair[0]);
        pinned = acqPinThisThread(std::span<const unsigned>(&c, 1)).ok();
    }).join();
    return pinned ? std::optional<CpuPair>(pair) : std::nullopt;
}

/** Runs @p body on a fresh thread, so pinning it leaves the calling thread alone. */
template<typename Body>
auto onOwnThread(Body body)
{
    decltype(body()) result{};
    std::thread([&] { result = body(); }).join();
    return result;
}

/**
 * Commit-to-consume latency: the producer commits one sample carrying its commit time every 50 us; the consumer
 * subtracts it from the time it returned from acquire(). Input: a reader in @p mode; output: the device's gating
 * reader. The calling thread produces. @return p50, p99, p99.9 in ns.
 */
std::array<double, 3> wakeLatency(AcqReader::ReadMode mode, int commits, AcqDirection direction = AcqDirection::Input,
                                  CpuPair cpus = kUnpinned)
{
    pinTo(cpus[0]);
    auto writer = AcqWriter::create(streamInfo(2, AcqSampleFormat::Int32, AcqLayout::Interleaved, direction),
                                    ringOptions(65536));
    AcqReaderOptions options;
    options.gating = direction == AcqDirection::Output;
    auto reader = writer->stream().reader(mode, options);
    std::vector<std::int64_t> latencies;
    latencies.reserve(static_cast<std::size_t>(commits));
    std::thread consumer([&] {
        pinTo(cpus[1]);
        for (int i = 0; i < commits; ++i) {
            auto view = reader->acquire(1, 5s, 1);
            const std::int64_t woke = nowNs();
            if (!view) {
                return;
            }
            std::int32_t words[2] = {};
            std::memcpy(&words[0], view->part(0).at(0, 0), sizeof(std::int32_t));
            std::memcpy(&words[1], view->part(0).at(1, 0), sizeof(std::int32_t));
            const std::int64_t stamp =
                (static_cast<std::int64_t>(words[0]) << 32) | static_cast<std::int64_t>(static_cast<std::uint32_t>(words[1]));
            latencies.push_back(woke - stamp);
            static_cast<void>(reader->release(*view));
        }
    });
    for (int i = 0; i < commits; ++i) {
        const auto due = Clock::now() + 50us;
        while (Clock::now() < due) {
        }
        auto view = writer->beginWrite(1, 5s);
        if (!view) {
            break;
        }
        const std::int64_t stamp = nowNs();
        const auto hi = static_cast<std::int32_t>(stamp >> 32);
        const auto lo = static_cast<std::int32_t>(static_cast<std::uint32_t>(stamp & 0xFFFFFFFF));
        std::memcpy(view->part(0).at(0, 0), &hi, sizeof(hi));
        std::memcpy(view->part(0).at(1, 0), &lo, sizeof(lo));
        writer->commit(1);
    }
    writer->finish();
    consumer.join();
    std::sort(latencies.begin(), latencies.end());
    const auto at = [&](double q) {
        return latencies.empty() ? 0.0 : static_cast<double>(latencies[static_cast<std::size_t>(q * static_cast<double>(latencies.size() - 1))]);
    };
    return {at(0.5), at(0.99), at(0.999)};
}

/** How a stream run fills the ring. */
enum class Feed : std::uint8_t
{
    Copy,  /**< The producer writes through beginWrite()/commit(), like a USB or network backend. */
    Dma,   /**< A card thread writes granted space of an adopted buffer; the driver commits what it reports. */
    Output /**< The application writes an output stream; the device's gating reader consumes it. */
};

struct StreamCase
{
    std::size_t channels = 1;
    AcqSampleFormat format = AcqSampleFormat::Int16;
    Feed feed = Feed::Copy;
    bool display = false;     /**< Also attach a Latest reader polling the newest second, as a display does. */
    double rate = 0.0;        /**< Samples per second and channel; 0 = as fast as the consumer takes them. */
    std::uint64_t bytes = 0;  /**< Data to move. */
    AcqStreamOptions options; /**< Ring knobs; capacitySamples 0 = max(65536, 16 blocks). */
    CpuPair cpus = kUnpinned; /**< Producer (the calling thread) and consumer CPUs. */
};

/** One page of an adopted DMA buffer; a vector of them is page-aligned and contiguous. */
struct alignas(4096) Page
{
    std::array<std::byte, 4096> bytes;
};

struct StreamResult
{
    double gigabytesPerSecond = -1.0; /**< -1 if a sample was lost, corrupted or not delivered. */
    double cpuSecondsPerGigabyte = 0.0;
};

/**
 * Moves @p c.bytes through one ring into a consumer that checks every byte. Unpaced runs gate the producer on the
 * consumer, so nothing may be lost and the number is the end-to-end rate; paced runs do not, as a live device
 * cannot wait, so the rate shows whether the consumer keeps up.
 */
StreamResult streamThroughput(const StreamCase& c)
{
    const std::size_t frame = c.channels * bytesPerSample(c.format);
    const std::size_t block =
        c.rate > 0.0 ? std::bit_floor(std::max<std::size_t>(1, static_cast<std::size_t>(c.rate / 1000.0)))
                     : std::bit_floor(std::max<std::size_t>(1, 65536 / frame));
    AcqStreamOptions options = c.options;
    if (options.capacitySamples == 0) {
        options.capacitySamples = std::max<std::size_t>(65536, 16 * block);
    }
    const std::size_t capacity = std::bit_ceil(options.capacitySamples);
    std::vector<Page> dmaPages;
    std::byte* dmaBuffer = nullptr;
    if (c.feed == Feed::Dma) {
        dmaPages.resize((capacity * frame + sizeof(Page) - 1) / sizeof(Page));
        dmaBuffer = dmaPages.front().bytes.data();
        options.external.data = dmaBuffer;
        options.external.bytes = capacity * frame;
    }
    const AcqDirection direction = c.feed == Feed::Output ? AcqDirection::Output : AcqDirection::Input;
    auto writer = AcqWriter::create(streamInfo(c.channels, c.format, AcqLayout::Interleaved, direction), options);
    if (!writer) {
        return {};
    }
    AcqReaderOptions consumerOptions;
    consumerOptions.gating = c.rate <= 0.0 || c.feed == Feed::Output;
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless, consumerOptions);
    auto display = writer->stream().reader(AcqReader::ReadMode::Latest);
    const std::uint64_t total = std::max<std::uint64_t>(c.bytes / frame / block, 1) * block;
    // Every byte of sample s is fillByte(s), so the consumer can check what it reads without a reference copy.
    const auto fillByte = [block](std::uint64_t s) {
        return static_cast<std::uint8_t>((s / block) % 251 + 1);
    };
    std::uint64_t byteSum = 0;
    std::uint64_t wrong = 0;
    std::uint64_t received = 0;
    pinTo(c.cpus[0]);
    std::thread consumer([&] {
        pinTo(c.cpus[1]);
        while (true) {
            auto view = reader->acquire(1, 5s);
            if (!view) {
                return;
            }
            std::uint64_t s = view->firstSample();
            for (std::size_t p = 0; p < view->partCount(); ++p) {
                const AcqBlockPart& part = view->part(p);
                const auto* bytes = reinterpret_cast<const std::uint8_t*>(part.data);
                std::uint64_t sum = 0;
                for (std::size_t k = 0; k < part.samples * frame; ++k) {
                    sum += bytes[k];
                }
                for (std::size_t i = 0; i < part.samples; ++i, ++s) {
                    wrong += bytes[i * frame] != fillByte(s) || bytes[i * frame + frame - 1] != fillByte(s) ? 1U : 0U;
                }
                byteSum += sum;
            }
            received += view->sampleCount();
            static_cast<void>(reader->release(*view));
        }
    });
    std::atomic<bool> producing{true};
    std::thread viewer;
    if (c.display) {
        viewer = std::thread([&] {
            const auto window = static_cast<std::size_t>(c.rate > 0.0 ? c.rate : 1000.0);
            while (producing.load(std::memory_order_acquire)) {
                if (auto view = display->acquire(1, 20ms, window)) {
                    static_cast<void>(display->release(*view));
                }
                std::this_thread::sleep_for(16ms);
            }
        });
    }

    std::uint64_t expectedSum = 0;
    const auto start = Clock::now();
    const std::clock_t cpuStart = std::clock();
    if (c.feed == Feed::Dma) {
        constexpr std::uint64_t kInFlight = 8; // blocks the card may have granted at once, like its notify queue
        std::atomic<std::uint64_t> granted{0};
        std::atomic<std::uint64_t> written{0};
        std::thread card([&] {
            std::uint64_t w = 0;
            while (w < total) {
                const std::uint64_t g = granted.load(std::memory_order_acquire);
                if (g == w) {
                    std::this_thread::yield();
                    continue;
                }
                for (; w < g; w += block) {
                    std::memset(dmaBuffer + (w & (capacity - 1)) * frame, fillByte(w), block * frame);
                }
                written.store(w, std::memory_order_release);
            }
        });
        std::uint64_t g = 0;
        std::uint64_t committed = 0;
        while (committed < total) {
            while (g < total && g - committed < kInFlight * block && writer->grant(block).ok()) {
                g += block;
                granted.store(g, std::memory_order_release);
            }
            const std::uint64_t done = written.load(std::memory_order_acquire);
            if (done > committed) {
                writer->commit(static_cast<std::size_t>(done - committed));
                committed = done;
            } else {
                std::this_thread::yield();
            }
        }
        card.join();
        for (std::uint64_t s = 0; s < total; s += block) {
            expectedSum += static_cast<std::uint64_t>(fillByte(s)) * block * frame;
        }
    } else {
        for (std::uint64_t written = 0; written < total; written += block) {
            if (c.rate > 0.0) {
                std::this_thread::sleep_until(start + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(static_cast<double>(written) / c.rate)));
            }
            auto view = writer->beginWrite(block, 10s);
            if (!view) {
                break;
            }
            for (std::size_t p = 0; p < view->partCount(); ++p) {
                std::memset(view->part(p).data, fillByte(written), view->part(p).samples * frame);
            }
            writer->commit(block);
            expectedSum += static_cast<std::uint64_t>(fillByte(written)) * block * frame;
        }
    }
    writer->finish();
    consumer.join();
    const double s = secondsSince(start);
    const double cpu = static_cast<double>(std::clock() - cpuStart) / CLOCKS_PER_SEC;
    producing.store(false, std::memory_order_release);
    if (viewer.joinable()) {
        viewer.join();
    }
    const AcqStreamStats producer = writer->stream().stats();
    if (received != total || reader->stats().lost != 0 || wrong != 0 || byteSum != expectedSum ||
        producer.underrun != 0) {
        return {};
    }
    const double gigabytes = static_cast<double>(total * frame) / 1e9;
    return {gigabytes / s, cpu / gigabytes};
}

#if defined(MNE_ACQ_BENCH_RECORDING)
struct RecordResult
{
    double ratePerChannel = -1.0; /**< Producer rate in kS/s per channel; -1 if the recorder lost a sample. */
    double producerGigabytesPerSecond = 0.0;
    double diskMegabytesPerSecond = 0.0; /**< Bytes the recorder wrote per second until everything was on disk. */
};

/**
 * Records a stream to a temporary FIFF file while a producer fills it. Paced runs (@p rate > 0) must record every
 * sample at the full rate; unpaced runs push @p bytes as fast as the ring allows, so they show that the recorder's
 * reader never slows the producer and how fast the recorder writes.
 */
RecordResult recordRun(std::size_t channels, AcqSampleFormat format, double rate, std::uint64_t bytes)
{
    QTemporaryDir dir;
    AcqStreamInfo info = streamInfo(channels, format);
    for (AcqChannelInfo& ch : info.channels) {
        ch.offset = 0.0; // native samples, no physical conversion
    }
    info.sfreq = rate > 0.0 ? rate : 1000.0;
    const std::size_t frame = channels * bytesPerSample(format);
    const std::size_t block = rate > 0.0 ? std::bit_floor(std::max<std::size_t>(1, static_cast<std::size_t>(rate / 1000.0)))
                                         : std::bit_floor(std::max<std::size_t>(1, 65536 / frame));
    auto writer = AcqWriter::create(info, ringOptions(std::max<std::size_t>(65536, 2 * static_cast<std::size_t>(rate))));
    if (!writer || !dir.isValid()) {
        return {};
    }
    AcqRecorderOptions options;
    options.bufferSamples = rate > 0.0 ? static_cast<std::size_t>(rate / 4.0) : 16 * block;
    auto recorder = AcqRecorder::start(writer->stream(), dir.filePath("bench_raw.fif").toStdString(), options);
    if (!recorder) {
        return {};
    }
    const std::uint64_t total = std::max<std::uint64_t>(bytes / frame / block, 1) * block;
    const auto start = Clock::now();
    for (std::uint64_t written = 0; written < total; written += block) {
        if (rate > 0.0) {
            std::this_thread::sleep_until(start + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(static_cast<double>(written) / rate)));
        }
        auto view = writer->beginWrite(block);
        if (!view) {
            return {};
        }
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            std::memset(view->part(p).data, static_cast<int>(written % 251), view->part(p).samples * frame);
        }
        writer->commit(block);
    }
    const double produced = secondsSince(start);
    writer->finish();
    if (!recorder->stop().ok()) {
        return {};
    }
    const double recorded = secondsSince(start);
    const AcqRecorderStats stats = recorder->stats();
    RecordResult result;
    result.producerGigabytesPerSecond = static_cast<double>(total * frame) / 1e9 / produced;
    result.diskMegabytesPerSecond = static_cast<double>(stats.bytes) / 1e6 / recorded;
    if (stats.samples == total && (rate <= 0.0 || stats.lost == 0)) {
        result.ratePerChannel = static_cast<double>(total) / produced / 1e3;
    }
    return result;
}
#endif

/** decode() of a 64 x 4096 interleaved block into a column-major T matrix. @return G output elements/s. */
template<typename T>
double decodeRate(AcqSampleFormat format, int trials)
{
    constexpr std::size_t kChannels = 64;
    constexpr std::size_t kBlock = 4096;
    auto writer = AcqWriter::create(streamInfo(kChannels, format), ringOptions(8 * kBlock));
    auto reader = writer->stream().reader(AcqReader::ReadMode::Lossless);
    auto in = writer->beginWrite(kBlock);
    for (std::size_t p = 0; p < in->partCount(); ++p) {
        std::memset(in->part(p).data, 1, in->part(p).samples * kChannels * bytesPerSample(format));
    }
    writer->commit(kBlock);
    auto view = reader->acquire(kBlock, 0ns);
    std::vector<T> out(kChannels * kBlock);
    const auto target = AcqMatrixRef<T>::columnMajor(out.data(), kChannels, kBlock);
    const double rate = static_cast<double>(kChannels * kBlock) / 1e9 /
        minSecondsPerCall(trials, 4, [&] { static_cast<void>(decode(*view, target)); });
    static_cast<void>(reader->release(*view));
    return rate;
}

/** encode() of a column-major float 64 x 4096 block into an output stream. @return G input elements/s. */
double encodeRate(AcqSampleFormat format, int trials)
{
    constexpr std::size_t kChannels = 64;
    constexpr std::size_t kBlock = 4096;
    auto writer = AcqWriter::create(streamInfo(kChannels, format, AcqLayout::Interleaved, AcqDirection::Output),
                                    ringOptions(8 * kBlock));
    std::vector<float> in(kChannels * kBlock, 0.25F);
    const auto source = AcqMatrixRef<const float>::columnMajor(in.data(), kChannels, kBlock);
    auto view = writer->beginWrite(kBlock);
    return static_cast<double>(kChannels * kBlock) / 1e9 /
        minSecondsPerCall(trials, 4, [&] { static_cast<void>(encode(source, *view)); });
}

std::vector<Metric> run(const Settings& s)
{
    std::vector<Metric> metrics;
    const int ops = s.quick ? 200000 : 2000000;
    for (const int readers : {1, 2, 4, 8}) {
        metrics.push_back({"op_cost_" + std::to_string(readers) + "_readers",
                           best(s, false, [&] { return opCost(readers, ops); }), "ns", false});
    }
    const int commits = s.quick ? 2000 : 20000;
    const struct
    {
        AcqReader::ReadMode mode;
        const char* name;
    } modes[] = {{AcqReader::ReadMode::LowLatency, "spin"}, {AcqReader::ReadMode::Lossless, "block"}};
    for (const auto& m : modes) {
        std::array<double, 3> bestLatency{1e18, 1e18, 1e18};
        for (int r = 0; r < s.repeats; ++r) {
            const auto l = wakeLatency(m.mode, commits);
            for (std::size_t i = 0; i < 3; ++i) {
                bestLatency[i] = std::min(bestLatency[i], l[i]);
            }
        }
        const std::string prefix = std::string("wake_") + m.name + "_";
        metrics.push_back({prefix + "p50", bestLatency[0], "ns", false});
        metrics.push_back({prefix + "p99", bestLatency[1], "ns", false, m.mode == AcqReader::ReadMode::LowLatency ? 50000.0 : 0.0});
        metrics.push_back({prefix + "p999", bestLatency[2], "ns", false});
    }
    std::array<double, 3> output{1e18, 1e18, 1e18};
    for (int r = 0; r < s.repeats; ++r) {
        const auto l = wakeLatency(AcqReader::ReadMode::LowLatency, commits, AcqDirection::Output);
        for (std::size_t i = 0; i < 3; ++i) {
            output[i] = std::min(output[i], l[i]);
        }
    }
    metrics.push_back({"wake_output_p50", output[0], "ns", false});
    metrics.push_back({"wake_output_p99", output[1], "ns", false});
    metrics.push_back({"wake_output_p999", output[2], "ns", false});
    // Pinned producer and consumer on the ring's NUMA node: no migration, so no cold caches after a move.
    const std::optional<CpuPair> pair = pinnablePair();
    if (pair) {
        std::array<double, 3> pinned{1e18, 1e18, 1e18};
        for (int r = 0; r < s.repeats; ++r) {
            const auto l = onOwnThread([&] {
                return wakeLatency(AcqReader::ReadMode::LowLatency, commits, AcqDirection::Input, *pair);
            });
            for (std::size_t i = 0; i < 3; ++i) {
                pinned[i] = std::min(pinned[i], l[i]);
            }
        }
        metrics.push_back({"wake_spin_pinned_p50", pinned[0], "ns", false});
        metrics.push_back({"wake_spin_pinned_p99", pinned[1], "ns", false, 50000.0});
        metrics.push_back({"wake_spin_pinned_p999", pinned[2], "ns", false});
    }
    const std::uint64_t bytes = s.quick ? (std::uint64_t(256) << 20) : (std::uint64_t(4) << 30);
    // Best of several shorter streams: a stream is two threads, and one slow placement must not set the value.
    Settings streams = s;
    streams.repeats = 8 * s.repeats;
    const auto gbps = [&](const StreamCase& c) {
        return best(streams, true, [&] { return streamThroughput(c).gigabytesPerSecond; });
    };
    for (const std::size_t channels : {1U, 64U, 1024U, 4096U}) {
        metrics.push_back({"ring_int16_" + std::to_string(channels) + "_ch",
                           gbps({channels, AcqSampleFormat::Int16, Feed::Copy, false, 0.0, bytes / 8, {}}), "GB/s", true});
    }

    // Ring knobs on the 1,024-channel stream.
    AcqStreamOptions knob;
    knob.lockMemory = true;
    metrics.push_back({"ring_int16_1024_ch_locked", gbps({1024, AcqSampleFormat::Int16, Feed::Copy, false, 0.0, bytes / 8, knob}),
                       "GB/s", true});
    knob = {};
    knob.mirror = false;
    metrics.push_back({"ring_int16_1024_ch_unmirrored",
                       gbps({1024, AcqSampleFormat::Int16, Feed::Copy, false, 0.0, bytes / 8, knob}), "GB/s", true});
    knob.hugePages = true;
    metrics.push_back({"ring_int16_1024_ch_hugepages",
                       gbps({1024, AcqSampleFormat::Int16, Feed::Copy, false, 0.0, bytes / 8, knob}), "GB/s", true});

    // A digitizer card writing by DMA (4 channels, any rate up to its limit) and an output stream to a device.
    metrics.push_back({"dma_int16_4_ch", gbps({4, AcqSampleFormat::Int16, Feed::Dma, false, 0.0, bytes / 8, {}}), "GB/s", true});
    metrics.push_back({"output_float32_64_ch", gbps({64, AcqSampleFormat::Float32, Feed::Output, false, 0.0, bytes / 8, {}}),
                       "GB/s", true});

    // 1,000 float32 channels: sustained with a recorder plus a display, and paced at 20 kHz without loss.
    AcqStreamOptions wide;
    wide.capacitySamples = 16384;
    const auto sustainedRun = [&](CpuPair cpus) {
        const StreamCase sustained{1000, AcqSampleFormat::Float32, Feed::Copy, true, 0.0, bytes / 8, wide, cpus};
        StreamResult bestRun{};
        for (int r = 0; r < streams.repeats; ++r) {
            const StreamResult run = onOwnThread([&] { return streamThroughput(sustained); });
            if (r == 0 || run.gigabytesPerSecond < 0.0 || run.gigabytesPerSecond > bestRun.gigabytesPerSecond) {
                bestRun = run;
            }
            if (bestRun.gigabytesPerSecond < 0.0) {
                break; // a loss is never hidden by a later good run
            }
        }
        return bestRun;
    };
    const StreamResult bestSustained = sustainedRun(kUnpinned);
    metrics.push_back({"sustained_float32_1000_ch_2_readers", bestSustained.gigabytesPerSecond, "GB/s", true, 2.0});
    metrics.push_back({"cpu_sustained_float32_1000_ch", bestSustained.cpuSecondsPerGigabyte, "s/GB", false});
    if (pair) {
        metrics.push_back({"sustained_float32_1000_ch_pinned", sustainedRun(*pair).gigabytesPerSecond, "GB/s", true, 2.0});
    }

    // Paced rate sweep, all without loss: a 64-channel EEG at 1 kHz, 1,000 channels at 20 kHz, and a 4-channel
    // digitizer at 62.5 MHz (250 MS/s aggregate).
    const double pacedSeconds = s.pacedSeconds > 0.0 ? s.pacedSeconds : (s.quick ? 2.0 : 10.0);
    const auto paced = [&](const char* name, std::size_t channels, AcqSampleFormat format, double rate, bool display,
                           const AcqStreamOptions& options) {
        const double bytesPerSecond = rate * static_cast<double>(channels * bytesPerSample(format));
        const StreamCase c{channels, format, Feed::Copy, display, rate,
                           static_cast<std::uint64_t>(pacedSeconds * bytesPerSecond), options};
        const double measured = streamThroughput(c).gigabytesPerSecond;
        metrics.push_back({name, measured < 0.0 ? -1.0 : measured * 1e9 / bytesPerSecond * rate / 1e3, "kS/s per ch", true,
                           rate / 1e3 * 0.99});
    };
    paced("paced_float32_64_ch_1_khz", 64, AcqSampleFormat::Float32, 1000.0, true, {});
    paced("paced_float32_1000_ch_20_khz", 1000, AcqSampleFormat::Float32, 20000.0, true, wide);
    paced("paced_int16_4_ch_62500_khz", 4, AcqSampleFormat::Int16, 62.5e6, false, {});

#if defined(MNE_ACQ_BENCH_RECORDING)
    // Recording to FIFF: 256 channels at 20 kHz must reach the disk without a lost sample; flat out, the
    // recorder's reader must not slow the producer below the ring rate (compare ring_int16_64_ch).
    const RecordResult paced256 = recordRun(256, AcqSampleFormat::Float32, 20000.0,
                                            static_cast<std::uint64_t>(pacedSeconds * 20000.0 * 256 * 4));
    metrics.push_back({"record_float32_256_ch_20_khz", paced256.ratePerChannel, "kS/s per ch", true, 20.0 * 0.99});
    const RecordResult flatOut = recordRun(64, AcqSampleFormat::Int16, 0.0, bytes / 8);
    metrics.push_back({"record_producer_int16_64_ch", flatOut.producerGigabytesPerSecond, "GB/s", true});
    metrics.push_back({"record_disk_int16_64_ch", flatOut.diskMegabytesPerSecond, "MB/s", true});
#endif

    const int trials = s.quick ? 50 : 200;
    for (const AcqSampleFormat f : {AcqSampleFormat::Int16, AcqSampleFormat::Int24Packed, AcqSampleFormat::Int32,
                                    AcqSampleFormat::Float32}) {
        metrics.push_back({"decode_" + std::string(sampleFormatName(f)) + "_to_float",
                           best(s, true, [&] { return decodeRate<float>(f, trials); }), "Gelem/s", true});
    }
    metrics.push_back({"decode_ComplexInt16_to_cfloat",
                       best(s, true, [&] { return decodeRate<std::complex<float>>(AcqSampleFormat::ComplexInt16, trials); }),
                       "Gelem/s", true});
    for (const AcqSampleFormat f : {AcqSampleFormat::Int16, AcqSampleFormat::Float32}) {
        metrics.push_back({"encode_float_to_" + std::string(sampleFormatName(f)),
                           best(s, true, [&] { return encodeRate(f, trials); }), "Gelem/s", true});
    }
    return metrics;
}

bool writeJson(const QString& path, const std::vector<Metric>& metrics, const Settings& s)
{
    QJsonArray list;
    for (const Metric& m : metrics) {
        list.append(QJsonObject{{"name", QString::fromStdString(m.name)},
                                {"value", m.value},
                                {"unit", QString::fromLatin1(m.unit)},
                                {"better", m.higherIsBetter ? "higher" : "lower"},
                                {"target", m.target}});
    }
    const QJsonObject report{{"tool", "mne_acq_bench"},
                             {"version", MNE_CPP_VERSION},
                             {"build", acq_build_hash()},
                             {"quick", s.quick},
                             {"repeats", s.repeats},
                             {"paced_seconds", s.pacedSeconds},
                             {"threads", static_cast<int>(std::thread::hardware_concurrency())},
                             {"metrics", list}};
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
        file.write(QJsonDocument(report).toJson()) > 0;
}
} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_acq_bench");
    QCoreApplication::setApplicationVersion(MNE_CPP_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Performance suite of the MNE-CPP acquisition library: per-call cost of the stream protocol, commit-to-wake "
        "latency (spinning, blocking and output readers), verified throughput over a channel sweep, with ring knobs, "
        "a DMA card, an output stream and 1,000 channels with a recorder and a display, paced runs from 1 kHz to "
        "62.5 MHz that must not lose a sample, recording to FIFF (paced without loss, and flat out), and the "
        "decode/encode converters. Where threads can be pinned, the "
        "spinning wake-up and the 1,000-channel stream are repeated with producer and consumer pinned to two CPUs of "
        "the ring's NUMA node. Every value is the best of --repeats runs.");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption quickOpt("quick", "Short runs (about 10 s), for CI regression checks.");
    const QCommandLineOption repeatsOpt("repeats", "Runs per measurement (default 3).", "n", "3");
    const QCommandLineOption jsonOpt("json", "Also write the results as JSON.", "file");
    const QCommandLineOption pacedOpt("paced-seconds",
                                      "Duration of each paced run, e.g. 600 for a soak test "
                                      "(default 10, or 2 with --quick).",
                                      "s");
    parser.addOptions({quickOpt, repeatsOpt, jsonOpt, pacedOpt});
    parser.process(app);

    Settings settings;
    settings.quick = parser.isSet(quickOpt);
    bool ok = false;
    settings.repeats = parser.value(repeatsOpt).toInt(&ok);
    if (!ok || settings.repeats < 1) {
        qCritical("--repeats needs a positive integer, got '%s'", qPrintable(parser.value(repeatsOpt)));
        return 1;
    }
    if (parser.isSet(pacedOpt)) {
        settings.pacedSeconds = parser.value(pacedOpt).toDouble(&ok);
        if (!ok || settings.pacedSeconds <= 0.0) {
            qCritical("--paced-seconds needs a positive number, got '%s'", qPrintable(parser.value(pacedOpt)));
            return 1;
        }
    }

    QTextStream out(stdout);
    out << "mne_acq_bench " << MNE_CPP_VERSION << " (mne_acq " << acq_build_hash() << "), "
        << std::thread::hardware_concurrency() << " hardware threads\n";
    out.flush();
    const std::vector<Metric> metrics = run(settings);
    bool lost = false;
    for (const Metric& m : metrics) {
        out << "  " << QString::fromStdString(m.name).leftJustified(38) << QString::number(m.value, 'g', 4).rightJustified(12)
            << ' ' << QString::fromLatin1(m.unit).leftJustified(12);
        if (m.target > 0.0) {
            const bool met = m.value >= 0.0 && (m.higherIsBetter ? m.value >= m.target : m.value <= m.target);
            out << (m.higherIsBetter ? " target >= " : " target <= ") << m.target << (met ? "  met" : "  MISSED");
        }
        out << '\n';
        lost |= m.value < 0.0;
    }
    out.flush();
    if (parser.isSet(jsonOpt) && !writeJson(parser.value(jsonOpt), metrics, settings)) {
        qCritical("Cannot write %s", qPrintable(parser.value(jsonOpt)));
        return 1;
    }
    if (lost) {
        qCritical("A stream run lost, corrupted or did not deliver samples");
        return 1;
    }
    return 0;
}
