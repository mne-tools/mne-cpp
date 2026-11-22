//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_fiff_replay.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    The fiff:// backend: native-format FIFF replay with pacing, looping and trigger events.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_fiff_replay.h"

#include "acq_fiff.h"
#include "../acq_backend.h"
#include "../acq_clock_model.h"
#include "../acq_device_registry.h"
#include "../acq_thread.h"
#include "../detail/acq_uri_params.h"
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_ch_name.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_file.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_stream.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

//=============================================================================================================
// DEFINE LOCAL METHODS
//=============================================================================================================

namespace
{

using Clock = std::chrono::steady_clock;

constexpr std::streamoff kTagHeaderBytes = 16; // kind, type, size, next

struct Config
{
    std::string path;
    bool loop = false;
    bool realtime = true;
    double speed = 1.0;
    double start = 0.0;
    std::size_t block = 0;
    bool events = true;
    std::string stim;
    AcqThreadPolicy thread;
};

AcqError invalid(std::string what)
{
    return {AcqErrorCode::InvalidParameter, std::move(what)};
}

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    Config c;
    c.path = uri.target;
#ifdef _WIN32
    if (c.path.size() >= 3 && c.path[0] == '/' && c.path[2] == ':') {
        c.path.erase(0, 1); // fiff:///C:/data/raw.fif
    }
#endif
    if (c.path.empty()) {
        return invalid("fiff:// needs a file, e.g. fiff:///data/sample_audvis_raw.fif");
    }
    const auto loop = paramBool(uri, "loop", false);
    const auto realtime = paramBool(uri, "realtime", true);
    const auto speed = paramDouble(uri, "speed", 1.0, 0.01, 1e4);
    const auto start = paramDouble(uri, "start", 0.0, 0.0, 1e9);
    const auto block = paramInt(uri, "block", 0, 1, 1 << 20);
    const auto events = paramBool(uri, "events", true);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error :
         {loop.error(), realtime.error(), speed.error(), start.error(), block.error(), events.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    c.loop = *loop;
    c.realtime = *realtime;
    c.speed = *speed;
    c.start = *start;
    c.block = static_cast<std::size_t>(*block);
    c.events = *events;
    c.stim = std::string(uri.param("stim").value_or(""));
    c.thread = std::move(*thread);
    return c;
}

/** @return The stream format of the file's data buffers; all must share one type. */
AcqExpected<AcqSampleFormat> nativeFormat(const FiffRawData& raw)
{
    fiff_int_t type = -1;
    for (const FiffRawDir& dir : raw.rawdir) {
        if (dir.ent.kind == -1) {
            continue; // skip
        }
        if (type != -1 && dir.ent.type != type) {
            return AcqError(AcqErrorCode::UnsupportedFormat, "data buffers of different types in one file");
        }
        type = dir.ent.type;
    }
    switch (type) {
        case FIFFT_DAU_PACK16:
        case FIFFT_SHORT:
            return AcqSampleFormat::Int16;
        case FIFFT_INT:
            return AcqSampleFormat::Int32;
        case -1: // only skips
        case FIFFT_FLOAT:
            return AcqSampleFormat::Float32;
        case FIFFT_DOUBLE:
            return AcqSampleFormat::Float64;
        default:
            return AcqError(AcqErrorCode::UnsupportedFormat, "data buffers of FIFF type " + std::to_string(type));
    }
}

/** Converts @p elements big-endian FIFF elements of @p size bytes to host order in place. */
void toHostOrder(std::byte* data, std::size_t elements, std::size_t size) noexcept
{
    if constexpr (std::endian::native == std::endian::little) {
        for (std::size_t e = 0; e < elements; ++e, data += size) {
            std::reverse(data, data + size);
        }
    }
}

/** @return The index of the trigger channel, -1 if there is none, or InvalidParameter for an unknown @p name. */
AcqExpected<int> stimChannel(const FiffInfo& info, const std::string& name)
{
    if (!name.empty()) {
        const int index = channel_index(info.ch_names, name);
        if (index < 0) {
            return invalid("stim=" + name + ": the file has no such channel");
        }
        return index;
    }
    for (const char* candidate : {"STI101", "STI014"}) {
        const int index = channel_index(info.ch_names, candidate);
        if (index >= 0) {
            return index;
        }
    }
    for (int k = 0; k < info.nchan; ++k) {
        if (info.chs[static_cast<std::size_t>(k)].kind == FIFFV_STIM_CH) {
            return k;
        }
    }
    return -1;
}

//=============================================================================================================

class FiffReplayBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<FiffReplayBackend> backend(new FiffReplayBackend(*config, uri.toString()));
        if (const AcqError error = backend->load(); !error.ok()) {
            return error;
        }
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~FiffReplayBackend() override
    {
        stop();
    }

    FiffReplayBackend(const FiffReplayBackend&) = delete;
    FiffReplayBackend& operator=(const FiffReplayBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"fiff", "", m_config.path, m_uri};
    }

    AcqError start() override
    {
        m_stop.store(false, std::memory_order_relaxed);
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
        AcqClockModel clock;
        clock.sfreq = m_sfreq;
        clock.anchorSample = m_next;
        clock.anchorHostNs = now;
        clock.utcOffsetNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count() -
            now;
        clock.source = AcqClockSource::Nominal;
        m_writer->setClock(clock);
        auto thread = start_thread(m_config.thread, [this] { run(); });
        if (!thread) {
            return thread.error();
        }
        m_thread = std::move(*thread);
        return {};
    }

    void stop() override
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex); // pairs with the pacing wait, so no wake-up is lost
            m_stop.store(true, std::memory_order_release);
        }
        m_wake.notify_all();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

private:
    FiffReplayBackend(const Config& config, std::string uri)
    : m_config(config)
    , m_uri(std::move(uri))
    {
    }

    /** Reads the measurement info and the buffer directory, scans the trigger channel, creates the stream. */
    AcqError load()
    {
        if (!std::filesystem::exists(mne::path_from_utf8(m_config.path))) {
            return {AcqErrorCode::DeviceNotFound, "fiff://" + m_config.path + ": no such file"};
        }
        auto raw = FiffRawData::read(m_config.path);
        if (!raw) {
            return {AcqErrorCode::UnsupportedFormat, m_config.path + " is not a raw FIFF file: " + raw.error().what()};
        }
        m_raw = std::move(*raw);
        auto format = nativeFormat(m_raw);
        if (!format) {
            return format.error();
        }
        m_sampleBytes = bytesPerSample(*format);
        m_floats = *format == AcqSampleFormat::Float32;
        m_channels = static_cast<std::size_t>(m_raw.info.nchan);
        m_frameBytes = m_channels * m_sampleBytes;
        m_sfreq = static_cast<double>(m_raw.info.sfreq);
        m_length = static_cast<std::uint64_t>(static_cast<std::int64_t>(m_raw.last_samp) - m_raw.first_samp + 1);
        std::size_t largest = 0;
        for (const FiffRawDir& dir : m_raw.rawdir) {
            m_starts.push_back(static_cast<std::uint64_t>(dir.first - m_raw.first_samp));
            largest = std::max(largest, static_cast<std::size_t>(dir.nsamp));
        }
        m_bytes.resize(largest * m_frameBytes);
        m_position = static_cast<std::uint64_t>(std::llround(m_config.start * m_sfreq));
        if (m_length == 0 || m_position >= m_length) {
            return invalid("start=" + std::to_string(m_config.start) + " lies beyond the " +
                           std::to_string(static_cast<double>(m_length) / m_sfreq) + " s of " + m_config.path);
        }
        if (m_config.block == 0) {
            m_config.block = static_cast<std::size_t>(std::max(1.0, std::round(m_sfreq / 100.0)));
        }
        if (m_config.events) {
            auto stim = stimChannel(m_raw.info, m_config.stim);
            if (!stim) {
                return stim.error();
            }
            if (*stim >= 0) {
                if (const AcqError error = scanEvents(static_cast<std::size_t>(*stim)); !error.ok()) {
                    return error;
                }
            }
        }
        m_event = firstEventAt(m_position);

        AcqStreamInfo info = fromFiffInfo(m_raw.info, *format);
        info.sourceId = m_config.path;
        AcqStreamOptions options;
        options.capacitySamples = 4 * m_config.block > 65536 ? 4 * m_config.block : 0;
        auto writer = addInputStream(std::move(info), options);
        if (!writer) {
            return writer.error();
        }
        m_writer = *writer;
        return {};
    }

    /** Loads buffer @p index into m_bytes (host byte order), unless it is a skip. */
    AcqError loadBuffer(std::size_t index)
    {
        if (index == m_loaded) {
            return {};
        }
        const FiffRawDir& dir = m_raw.rawdir[index];
        if (dir.ent.kind != -1) {
            const auto bytes = static_cast<std::size_t>(dir.nsamp) * m_frameBytes;
            if (!m_file.is_open()) {
                m_file.open(mne::path_from_utf8(m_config.path), std::ios::binary);
                if (!m_file.is_open()) {
                    return {AcqErrorCode::DeviceIo, "cannot open " + m_config.path};
                }
            }
            m_file.clear();
            if (!m_file.seekg(static_cast<std::streamoff>(dir.ent.pos) + kTagHeaderBytes) ||
                !m_file.read(reinterpret_cast<char*>(m_bytes.data()), static_cast<std::streamsize>(bytes))) {
                return {AcqErrorCode::DeviceIo, "cannot read data buffer " + std::to_string(index) + " of " + m_config.path};
            }
            toHostOrder(m_bytes.data(), bytes / m_sampleBytes, m_sampleBytes);
        }
        m_loaded = index;
        return {};
    }

    /** @return The buffer that holds file sample @p sample. */
    std::size_t bufferOf(std::uint64_t sample) const
    {
        return static_cast<std::size_t>(std::upper_bound(m_starts.begin(), m_starts.end(), sample) - m_starts.begin()) - 1;
    }

    /** Copies @p count frames from file sample @p sample on, within one pass of the file, to @p dst. */
    AcqError copy(std::uint64_t sample, std::size_t count, std::byte* dst)
    {
        while (count > 0) {
            const std::size_t index = bufferOf(sample);
            if (const AcqError error = loadBuffer(index); !error.ok()) {
                return error;
            }
            const FiffRawDir& dir = m_raw.rawdir[index];
            const std::uint64_t offset = sample - m_starts[index];
            const std::size_t take = static_cast<std::size_t>(
                std::min<std::uint64_t>(count, static_cast<std::uint64_t>(dir.nsamp) - offset));
            if (dir.ent.kind == -1) {
                std::memset(dst, 0, take * m_frameBytes);
            } else {
                std::memcpy(dst, m_bytes.data() + offset * m_frameBytes, take * m_frameBytes);
            }
            dst += take * m_frameBytes;
            sample += take;
            count -= take;
        }
        return {};
    }

    /** Collects the trigger events of the whole file. */
    AcqError scanEvents(std::size_t channel)
    {
        const FiffChInfo& ch = m_raw.info.chs[channel];
        const double scale = static_cast<double>(ch.range) * static_cast<double>(ch.cal);
        std::int64_t previous = 0;
        std::uint64_t sample = 0;
        for (std::size_t index = 0; index < m_starts.size(); ++index) {
            if (const AcqError error = loadBuffer(index); !error.ok()) {
                return error;
            }
            const FiffRawDir& dir = m_raw.rawdir[index];
            for (std::size_t i = 0; i < static_cast<std::size_t>(dir.nsamp); ++i, ++sample) {
                std::int64_t value = 0;
                if (dir.ent.kind != -1) {
                    value = std::llround(stored(m_bytes.data() + i * m_frameBytes + channel * m_sampleBytes) * scale);
                }
                if (sample > 0 && value != previous) {
                    if (!m_events.empty() && m_events.back().duration == 0) {
                        m_events.back().duration = static_cast<std::uint32_t>(sample - m_events.back().sample);
                    }
                    if (value != 0) {
                        m_events.push_back({sample, 0, static_cast<std::int32_t>(value)});
                    }
                }
                previous = value;
            }
        }
        if (!m_events.empty() && m_events.back().duration == 0) {
            m_events.back().duration = static_cast<std::uint32_t>(sample - m_events.back().sample);
        }
        return {};
    }

    /** @return The stored value at @p at in the file's format, as double. */
    double stored(const std::byte* at) const noexcept
    {
        switch (m_sampleBytes) {
            case 2: {
                std::int16_t v = 0;
                std::memcpy(&v, at, sizeof(v));
                return v;
            }
            case 4: {
                if (m_floats) {
                    float v = 0.0F;
                    std::memcpy(&v, at, sizeof(v));
                    return v;
                }
                std::int32_t v = 0;
                std::memcpy(&v, at, sizeof(v));
                return v;
            }
            default: {
                double v = 0.0;
                std::memcpy(&v, at, sizeof(v));
                return v;
            }
        }
    }

    /** @return The index of the first event at or after file sample @p sample. */
    std::size_t firstEventAt(std::uint64_t sample) const
    {
        return static_cast<std::size_t>(
            std::lower_bound(m_events.begin(), m_events.end(), sample,
                             [](const AcqEvent& event, std::uint64_t s) { return event.sample < s; }) -
            m_events.begin());
    }

    /** Waits for ring space; false on stop() or an error. */
    bool waitForSpace(std::size_t n)
    {
        constexpr std::chrono::milliseconds kPoll(20);
        while (!m_stop.load(std::memory_order_acquire)) {
            const AcqError e = m_writer->waitForSpace(n, kPoll);
            if (e.ok()) {
                return true;
            }
            if (e.code() != AcqErrorCode::Timeout) {
                reportError(e);
                return false;
            }
        }
        return false;
    }

    /** Writes the next @p n samples (wrapping at the end when looping) and their events; false on an error. */
    bool write(std::size_t n)
    {
        auto view = m_writer->beginWrite(n);
        if (!view) {
            reportError(view.error());
            return false;
        }
        std::uint64_t stream = m_next;
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqWritePart& part = view->part(p);
            std::size_t done = 0;
            while (done < part.samples) {
                if (m_position == m_length) {
                    m_position = 0;
                    m_event = 0;
                }
                const std::size_t take =
                    static_cast<std::size_t>(std::min<std::uint64_t>(part.samples - done, m_length - m_position));
                if (const AcqError error = copy(m_position, take, static_cast<std::byte*>(part.at(0, done)));
                    !error.ok()) {
                    reportError(error);
                    return false;
                }
                for (; m_event < m_events.size() && m_events[m_event].sample < m_position + take; ++m_event) {
                    const AcqEvent& event = m_events[m_event];
                    const AcqError pushed = m_writer->pushEvent({stream + (event.sample - m_position), event.duration, event.code});
                    if (!pushed.ok()) {
                        reportError(pushed);
                        return false;
                    }
                }
                m_position += take;
                stream += take;
                done += take;
            }
        }
        return true;
    }

    void run()
    {
        const Clock::time_point start = Clock::now();
        const std::uint64_t startSample = m_next;
        while (true) {
            std::size_t n = m_config.block;
            if (!m_config.loop) {
                if (m_position == m_length) {
                    m_writer->finish();
                    return;
                }
                n = static_cast<std::size_t>(std::min<std::uint64_t>(n, m_length - m_position));
            }
            if (!waitForSpace(n)) {
                return;
            }
            if (m_config.realtime) {
                const std::chrono::duration<double> due(static_cast<double>(m_next + n - startSample) /
                                                        (m_sfreq * m_config.speed));
                const auto deadline = start + std::chrono::duration_cast<Clock::duration>(due);
                std::unique_lock<std::mutex> lock(m_mutex);
                m_wake.wait_until(lock, deadline, [this] { return m_stop.load(std::memory_order_acquire); });
            }
            if (m_stop.load(std::memory_order_acquire) || !write(n)) {
                return;
            }
            m_writer->commit(n);
            m_next += n;
        }
    }

    Config m_config;
    std::string m_uri;
    std::ifstream m_file;
    FiffRawData m_raw;
    std::vector<std::uint64_t> m_starts; // first file sample of every buffer, relative to first_samp
    std::vector<std::byte> m_bytes;      // the loaded buffer in host byte order
    std::size_t m_loaded = static_cast<std::size_t>(-1);
    std::vector<AcqEvent> m_events; // trigger events in file samples
    std::size_t m_event = 0;        // next event to push
    std::size_t m_channels = 0;
    std::size_t m_sampleBytes = 0;
    std::size_t m_frameBytes = 0;
    bool m_floats = false;
    double m_sfreq = 0.0;
    std::uint64_t m_length = 0;   // samples in the file
    std::uint64_t m_position = 0; // next file sample; owned by the producer thread while it runs
    std::uint64_t m_next = 0;     // next stream sample
    AcqWriter* m_writer = nullptr;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "fiff";
    info.description = "Replays a raw FIFF file in its native sample format, paced or as fast as possible, with "
                       "the trigger channel's changes as events.";
    info.exampleUri = "fiff:///data/sample_audvis_raw.fif";
    info.parameters = {
        {"loop", "bool", "0", "Restart at the first file sample at the end; 0 ends the stream there."},
        {"realtime", "bool", "1",
         "Pace blocks by the clock. 0 replays as fast as possible, which a gating reader turns into a lossless "
         "read of the whole file."},
        {"speed", "double", "1", "Pacing factor of realtime, e.g. 10 replays ten times faster."},
        {"start", "double", "0", "Seconds into the file at which stream sample 0 lies."},
        {"block", "int", "auto", "Samples per block; auto = 10 ms."},
        {"events", "bool", "1", "Emit trigger channel changes as events."},
        {"stim", "string", "", "Trigger channel; empty picks STI101, STI 101, STI 014 or the first stim channel."},
        {"cpus", "string", "", "Comma-separated CPUs for the producer thread."},
        {"priority", "string", "normal", "Producer thread priority: normal, high or realtime."},
    };
    return info;
}

/** Registers fiff:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(registerFiffScheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::registerFiffScheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &FiffReplayBackend::open);
    return result;
}
