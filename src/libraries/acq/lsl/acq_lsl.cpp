//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_lsl.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    The lsl:// backend on the in-tree mne_lsl: native sample formats, channels from the stream description.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_lsl.h"

#include "../acq_backend.h"
#include "../acq_device.h"
#include "../acq_clock_model.h"
#include "../acq_device_registry.h"
#include "../acq_thread.h"
#include "../detail/acq_uri_params.h"
#include <lsl/lsl_stream_discovery.h>
#include <lsl/lsl_stream_inlet.h>
#include <lsl/lsl_stream_info.h>
#include <lsl/lsl_stream_outlet.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>
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

std::string lower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

AcqChannelKind kindOf(const std::string& type)
{
    const std::string t = lower(type);
    if (t == "eeg") {
        return AcqChannelKind::Eeg;
    }
    if (t == "meg") {
        return AcqChannelKind::Meg;
    }
    if (t == "emg") {
        return AcqChannelKind::Emg;
    }
    if (t == "ecg" || t == "ekg") {
        return AcqChannelKind::Ecg;
    }
    if (t == "eog") {
        return AcqChannelKind::Eog;
    }
    if (t == "stim" || t == "trigger" || t == "markers") {
        return AcqChannelKind::Stim;
    }
    if (t == "acc" || t == "accelerometer") {
        return AcqChannelKind::Accelerometer;
    }
    if (t == "gsr" || t == "eda") {
        return AcqChannelKind::Gsr;
    }
    if (t == "resp" || t == "respiration") {
        return AcqChannelKind::Respiration;
    }
    if (t == "nirs" || t == "fnirs") {
        return AcqChannelKind::Fnirs;
    }
    return AcqChannelKind::Unknown;
}

/** Sets unit and power of ten from an XDF unit string (microvolts, uV, mV, V, fT, T, ...). */
void unitOf(const std::string& unit, AcqChannelInfo& channel)
{
    const std::string u = lower(unit);
    struct Entry
    {
        const char* name;
        AcqUnit unit;
        int exponent;
    };
    static constexpr Entry kUnits[] = {
        {"microvolts", AcqUnit::Volt, -6},
        {"microvolt", AcqUnit::Volt, -6},
        {"uv", AcqUnit::Volt, -6},
        {"\xc2\xb5v", AcqUnit::Volt, -6},
        {"millivolts", AcqUnit::Volt, -3},
        {"mv", AcqUnit::Volt, -3},
        {"volts", AcqUnit::Volt, 0},
        {"volt", AcqUnit::Volt, 0},
        {"v", AcqUnit::Volt, 0},
        {"femtotesla", AcqUnit::Tesla, -15},
        {"ft", AcqUnit::Tesla, -15},
        {"tesla", AcqUnit::Tesla, 0},
        {"t", AcqUnit::Tesla, 0},
    };
    for (const Entry& entry : kUnits) {
        if (u == entry.name) {
            channel.unit = entry.unit;
            channel.unitExponent = entry.exponent;
            return;
        }
    }
    if (channel.kind == AcqChannelKind::Stim) {
        channel.unit = AcqUnit::None;
    }
}

struct Config
{
    std::string name;
    std::string type;
    std::string sourceId;
    double timeout = 5.0;
    AcqThreadPolicy thread;
    bool simulate = false;
    std::size_t channels = 8;
    double sfreq = 250.0;
    std::int64_t seed = 0;
};

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    Config c;
    c.name = uri.target;
    c.type = std::string(uri.param("type").value_or(""));
    c.sourceId = std::string(uri.param("source_id").value_or(""));
    const auto timeout = paramDouble(uri, "timeout", 5.0, 0.0, 3600.0);
    auto thread = paramThreadPolicy(uri);
    const auto simulate = paramBool(uri, "simulate", false);
    const auto channels = paramInt(uri, "channels", 8, 1, 1024);
    const auto sfreq = paramDouble(uri, "sfreq", 250.0, 1.0, 100000.0);
    const auto seed = paramInt(uri, "seed", 0, 0, std::numeric_limits<std::int32_t>::max());
    for (const AcqError& error : {timeout.error(), thread.error(), simulate.error(), channels.error(), sfreq.error(), seed.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    c.simulate = *simulate;
    c.channels = static_cast<std::size_t>(*channels);
    c.sfreq = *sfreq;
    c.seed = *seed;
    if (c.simulate) {
        // Resolved by a source id of its own, so a real stream of the same name is never read instead
        static std::atomic<unsigned> count{0};
        c.name = c.name.empty() ? "MNE-CPP simulator" : c.name;
        c.type = "EEG";
        c.sourceId = std::format("mne-acq-simulator-{}-{}", Clock::now().time_since_epoch().count(), count++);
    }
    if (c.name.empty() && c.type.empty() && c.sourceId.empty()) {
        return AcqError(AcqErrorCode::InvalidParameter, "lsl:// needs a stream name, type= or source_id=, e.g. lsl://BioSemi");
    }
    c.timeout = *timeout;
    c.thread = std::move(*thread);
    return c;
}

/** The property to resolve by: the name, else source_id, else type. */
std::pair<std::string, std::string> selector(const Config& c)
{
    if (c.simulate) {
        return {"source_id", c.sourceId};
    }
    if (!c.name.empty()) {
        return {"name", c.name};
    }
    if (!c.sourceId.empty()) {
        return {"source_id", c.sourceId};
    }
    return {"type", c.type};
}

/** The connected inlet: pulls samples as doubles, which hold every value exactly, and stores them in @p format. */
class LslLink
{
public:
    LslLink(const mne::stream_info& info, AcqSampleFormat format, std::size_t channels)
    : m_inlet(info)
    , m_format(format)
    , m_channels(channels)
    {
    }

    /** @return Success, or DeviceIo naming why the outlet refused the stream. */
    AcqError open()
    {
        try {
            m_inlet.open_stream();
        } catch (const std::exception& e) {
            return {AcqErrorCode::DeviceIo, e.what()};
        }
        return {};
    }

    ~LslLink()
    {
        m_inlet.close_stream();
    }

    LslLink(const LslLink&) = delete;
    LslLink& operator=(const LslLink&) = delete;

    /** Pulls up to @p frames frames into @p out; returns the count and the timestamp of the last. */
    std::size_t pull(std::byte* out, std::size_t frames, double& lastTimestamp)
    {
        if (m_pending.empty()) {
            if (m_inlet.samples_available()) {
                m_pending = m_inlet.pull_chunk_double(m_stamps);
                m_next = 0;
            }
            if (m_pending.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                return 0;
            }
        }
        const std::size_t size = bytesPerSample(m_format);
        std::size_t n = 0;
        for (; n < frames && m_next < m_pending.size(); ++n, ++m_next) {
            const std::vector<double>& sample = m_pending[m_next];
            for (std::size_t c = 0; c < m_channels; ++c) {
                store(out + (n * m_channels + c) * size, c < sample.size() ? sample[c] : 0.0);
            }
        }
        lastTimestamp = m_stamps[m_next - 1];
        if (m_next == m_pending.size()) {
            m_pending.clear();
        }
        return n;
    }

    /** @return Seconds to add to stream timestamps for the local LSL clock; the last estimate if none comes. */
    double timeCorrection()
    {
        const double correction = m_inlet.time_correction(2.0);
        if (correction == correction) {
            m_correction = correction;
        }
        return m_correction;
    }

private:
    template<typename T>
    static void storeAs(std::byte* at, double value)
    {
        const auto v = static_cast<T>(value);
        std::memcpy(at, &v, sizeof(T));
    }

    void store(std::byte* at, double value) const
    {
        switch (m_format) {
            case AcqSampleFormat::Float64:
                storeAs<double>(at, value);
                break;
            case AcqSampleFormat::Int32:
                storeAs<std::int32_t>(at, value);
                break;
            case AcqSampleFormat::Int16:
                storeAs<std::int16_t>(at, value);
                break;
            default:
                storeAs<float>(at, value);
                break;
        }
    }

    mne::stream_inlet m_inlet;
    AcqSampleFormat m_format;
    std::size_t m_channels;
    std::vector<std::vector<double>> m_pending;
    std::vector<double> m_stamps;
    std::size_t m_next = 0;
    double m_correction = 0.0;
};

//=============================================================================================================

/** A synthetic:// EEG session published as an in-tree LSL outlet: the stream that simulate=1 reads. */
class LslSimulator
{
public:
    static AcqExpected<std::unique_ptr<LslSimulator>> start(const Config& c)
    {
        const auto block = static_cast<std::size_t>(std::max(1.0, std::round(c.sfreq / 100.0)));
        auto session = AcqDevice::open(
            std::format("synthetic://?sfreq={}&eeg={}&stim=0&block={}&seed={}&realtime=1", c.sfreq, c.channels, block, c.seed));
        if (!session) {
            return session.error();
        }
        AcqReaderOptions gating;
        gating.gating = true;
        auto reader = session->stream().reader(AcqReader::ReadMode::Lossless, gating);
        if (!reader) {
            return reader.error();
        }
        std::unique_ptr<LslSimulator> simulator(new LslSimulator(
            c, block, std::move(*session), std::move(*reader)));
        if (AcqError e = simulator->m_session.start(); !e.ok()) {
            return e;
        }
        simulator->m_thread = std::thread([s = simulator.get()] { s->run(); });
        return simulator;
    }

    ~LslSimulator()
    {
        m_stop.store(true, std::memory_order_release);
        if (m_thread.joinable()) {
            m_thread.join();
        }
        m_session.close();
    }

    LslSimulator(const LslSimulator&) = delete;
    LslSimulator& operator=(const LslSimulator&) = delete;

private:
    /** @return The outlet's info: the session's channel names, in volts, as the stream description. */
    static mne::stream_info outletInfo(const Config& c, const AcqDevice& session)
    {
        mne::stream_info info(c.name, c.type, static_cast<int>(c.channels), c.sfreq, mne::ChannelFormat::Float32, c.sourceId);
        std::vector<mne::channel_description> channels;
        for (const AcqChannelInfo& channel : session.stream().info()->channels) {
            channels.push_back({channel.name, "EEG", "volts"});
        }
        info.set_channels(std::move(channels));
        return info;
    }

    LslSimulator(const Config& c, std::size_t block, AcqDevice session, AcqReader reader)
    : m_outlet(outletInfo(c, session))
    , m_session(std::move(session))
    , m_reader(std::move(reader))
    , m_chunk(block, std::vector<float>(c.channels))
    {
    }

    void run()
    {
        while (!m_stop.load(std::memory_order_acquire)) {
            auto view = m_reader.acquire(m_chunk.size(), std::chrono::milliseconds(200), m_chunk.size());
            if (!view) {
                if (view.error().code() == AcqErrorCode::Timeout) {
                    continue;
                }
                return;
            }
            std::size_t s = 0;
            for (std::size_t p = 0; p < view->partCount(); ++p) {
                const AcqBlockPart& part = view->part(p);
                for (std::size_t i = 0; i < part.samples; ++i, ++s) {
                    for (std::size_t c = 0; c < m_chunk[s].size(); ++c) {
                        std::memcpy(&m_chunk[s][c], part.at(c, i), sizeof(float));
                    }
                }
            }
            const bool whole = s == m_chunk.size();
            if (!m_reader.release(*view).ok() || !whole) {
                return;
            }
            m_outlet.push_chunk(m_chunk);
        }
    }

    mne::stream_outlet m_outlet;
    AcqDevice m_session;
    AcqReader m_reader;
    std::vector<std::vector<float>> m_chunk;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

//=============================================================================================================

class LslBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<LslBackend> backend(new LslBackend(*config, uri.toString()));
        if (config->simulate) {
            auto simulator = LslSimulator::start(*config);
            if (!simulator) {
                return simulator.error();
            }
            backend->m_simulator = std::move(*simulator);
        }
        const AcqError error = backend->connect();
        if (!error.ok()) {
            return error;
        }
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~LslBackend() override
    {
        stop();
    }

    LslBackend(const LslBackend&) = delete;
    LslBackend& operator=(const LslBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"lsl", "in-tree", m_device, m_uri};
    }

    AcqError start() override
    {
        m_stop.store(false, std::memory_order_relaxed);
        auto thread = start_thread(m_config.thread, [this] { run(); });
        if (!thread) {
            return thread.error();
        }
        m_thread = std::move(*thread);
        return {};
    }

    void stop() override
    {
        m_stop.store(true, std::memory_order_release);
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

private:
    LslBackend(const Config& config, std::string uri)
    : m_config(config)
    , m_uri(std::move(uri))
    {
    }

    AcqError connect()
    {
        const auto [property, value] = selector(m_config);
        const std::vector<mne::stream_info> found = mne::resolve_stream(property, value, m_config.timeout, 1);
        if (found.empty()) {
            return {AcqErrorCode::DeviceNotFound, "no LSL stream with " + property + "=" + value + " within " + std::to_string(m_config.timeout) + " s"};
        }
        AcqStreamInfo stream;
        if (AcqError described = describe(found.front(), stream); !described.ok()) {
            return described;
        }
        auto link = std::make_unique<LslLink>(found.front(), stream.format, stream.channels.size());
        if (AcqError opened = link->open(); !opened.ok()) {
            return opened;
        }
        m_link = std::move(link);
        return addStream(std::move(stream));
    }

    /** Fills @p stream from the stream's full info: native format, channel names, kinds and units. */
    AcqError describe(const mne::stream_info& resolved, AcqStreamInfo& stream)
    {
        switch (resolved.channel_format()) {
            case mne::ChannelFormat::Float32:
                stream.format = AcqSampleFormat::Float32;
                break;
            case mne::ChannelFormat::Double64:
            case mne::ChannelFormat::Int64:
                stream.format = AcqSampleFormat::Float64;
                break;
            case mne::ChannelFormat::Int32:
                stream.format = AcqSampleFormat::Int32;
                break;
            case mne::ChannelFormat::Int16:
            case mne::ChannelFormat::Int8:
                stream.format = AcqSampleFormat::Int16;
                break;
            case mne::ChannelFormat::String:
                return {AcqErrorCode::UnsupportedFormat, "string (marker) LSL streams are not supported yet"};
            default:
                return {AcqErrorCode::UnsupportedFormat, "LSL channel format " + std::to_string(static_cast<int>(resolved.channel_format()))};
        }
        if (resolved.nominal_srate() <= 0.0) {
            return {AcqErrorCode::UnsupportedFormat, "irregular-rate LSL streams are not supported yet"};
        }
        mne::stream_info info;
        try {
            mne::stream_inlet inlet(resolved);
            info = inlet.info(std::max(m_config.timeout, 1.0));
        } catch (const std::exception& e) {
            return {AcqErrorCode::DeviceIo, std::string("cannot read the LSL stream description: ") + e.what()};
        }
        stream.name = info.name();
        stream.sourceId = info.source_id();
        stream.sfreq = info.nominal_srate();
        stream.clockDomain = "lsl:" + info.source_id();
        m_device = info.name() + " (" + info.type() + ") on " + info.hostname();
        const std::vector<mne::channel_description>& described = info.channels();
        for (std::size_t c = 0; c < static_cast<std::size_t>(std::max(0, info.channel_count())); ++c) {
            const mne::channel_description none;
            const mne::channel_description& d = c < described.size() ? described[c] : none;
            AcqChannelInfo ch;
            ch.name = d.label.empty() ? info.type() + " " + std::to_string(c + 1) : d.label;
            ch.kind = kindOf(d.type.empty() ? info.type() : d.type);
            unitOf(d.unit, ch);
            stream.channels.push_back(std::move(ch));
        }
        return {};
    }

    AcqError addStream(AcqStreamInfo stream)
    {
        m_frameBytes = stream.channels.size() * bytesPerSample(stream.format);
        m_sfreq = stream.sfreq;
        m_block = static_cast<std::size_t>(std::max(1.0, std::round(m_sfreq / 100.0)));
        m_scratch.resize(m_block * stream.channels.size() * 8);
        auto writer = addInputStream(std::move(stream));
        if (!writer) {
            return writer.error();
        }
        m_writer = *writer;
        return {};
    }

    /** Clock model for the sample just committed (@p sample, LSL time @p timestamp), or arrival time without one. */
    /**
     * Reads the steady clock and the local LSL clock as one instant: the steady reading between two LSL readings,
     * against their midpoint, of the narrowest of a few tries. A thread preempted between two separate readings
     * would otherwise shift the mapping by the time it waited.
     */
    std::pair<std::int64_t, double> sameInstant() const
    {
        std::pair<std::int64_t, double> best{0, std::numeric_limits<double>::quiet_NaN()};
        double width = std::numeric_limits<double>::infinity();
        for (int attempt = 0; attempt < 3; ++attempt) {
            const double before = mne::local_clock();
            const auto steady = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
            const double after = mne::local_clock();
            if (!(after - before < width)) {
                continue;
            }
            width = after - before;
            best = {steady, 0.5 * (before + after)};
        }
        if (width == std::numeric_limits<double>::infinity()) { // no LSL clock
            best.first = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
        }
        return best;
    }

    void publishClock(std::uint64_t sample, double timestamp, bool refresh)
    {
        const auto [steady, local] = sameInstant();
        AcqClockModel clock;
        clock.sfreq = m_sfreq;
        clock.anchorSample = sample;
        clock.utcOffsetNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count() -
            steady;
        if (timestamp == timestamp && local == local) {
            if (refresh) {
                m_correction = m_link->timeCorrection();
            }
            // LSL time to steady_clock: both are monotonic, but need not share an epoch.
            clock.anchorHostNs = steady + static_cast<std::int64_t>((timestamp + m_correction - local) * 1e9);
            clock.source = AcqClockSource::Lsl;
        } else {
            clock.anchorHostNs = steady;
            clock.source = AcqClockSource::Host;
        }
        m_writer->setClock(clock);
    }

    void run()
    {
        std::uint64_t next = 0;
        Clock::time_point lastClock;
        while (!m_stop.load(std::memory_order_acquire)) {
            double timestamp = std::numeric_limits<double>::quiet_NaN();
            const std::size_t n = m_link->pull(m_scratch.data(), m_block, timestamp);
            if (n == 0) {
                continue;
            }
            // A gating reader may hold the ring; the samples already pulled wait, later ones stay in LSL's buffer.
            AcqError space = m_writer->waitForSpace(n, std::chrono::milliseconds(200));
            while (space.code() == AcqErrorCode::Timeout && !m_stop.load(std::memory_order_acquire)) {
                space = m_writer->waitForSpace(n, std::chrono::milliseconds(200));
            }
            if (!space.ok()) {
                return;
            }
            auto view = m_writer->beginWrite(n);
            if (!view) {
                reportError(view.error());
                return;
            }
            std::size_t done = 0;
            for (std::size_t p = 0; p < view->partCount(); ++p) {
                const AcqWritePart& part = view->part(p);
                std::memcpy(part.at(0, 0), m_scratch.data() + done * m_frameBytes, part.samples * m_frameBytes);
                done += part.samples;
            }
            m_writer->commit(n);
            next += n;
            const Clock::time_point now = Clock::now();
            const bool refresh = lastClock == Clock::time_point{} || now - lastClock >= std::chrono::seconds(1);
            if (refresh) {
                publishClock(next - 1, timestamp, true);
                lastClock = now;
            }
        }
    }

    Config m_config;
    std::string m_uri;
    std::string m_device;
    std::unique_ptr<LslSimulator> m_simulator; // outlives the link that reads it
    std::unique_ptr<LslLink> m_link;
    AcqWriter* m_writer = nullptr;
    std::vector<std::byte> m_scratch;
    std::size_t m_frameBytes = 0;
    std::size_t m_block = 1;
    double m_sfreq = 0.0;
    double m_correction = 0.0;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "lsl";
    info.description = "Reads a Lab Streaming Layer stream with the in-tree LSL implementation. "
                       "simulate=1 publishes a synthetic:// EEG session as an LSL stream and reads that instead.";
    info.exampleUri = "lsl://BioSemi";
    info.parameters = {
        {"type", "string", "", "Resolve by stream type (e.g. EEG) instead of the name."},
        {"source_id", "string", "", "Resolve by source id instead of the name."},
        {"timeout", "double", "5", "Seconds to wait for the stream to appear."},
        {"cpus", "string", "", "Comma-separated CPUs for the reader thread."},
        {"priority", "string", "normal", "Reader thread priority: normal, high or realtime."},
        {"simulate", "bool", "0", "Publish a synthetic EEG stream and read it instead of an existing one."},
        {"channels", "int", "8", "Simulation: EEG channels."},
        {"sfreq", "double", "250", "Simulation: sampling rate in Hz."},
        {"seed", "int", "0", "Simulation: seed of the synthetic session."},
    };
    return info;
}

std::vector<AcqDiscoveredDevice> discover(std::chrono::milliseconds timeout)
{
    std::vector<AcqDiscoveredDevice> out;
    const double seconds = static_cast<double>(timeout.count()) / 1000.0;
    for (const mne::stream_info& info : mne::resolve_streams(seconds)) {
        out.push_back({"lsl://" + info.name(), info.name(), "lsl",
                       std::to_string(info.channel_count()) + " ch, " +
                           std::to_string(static_cast<long long>(info.nominal_srate())) + " Hz, " + info.type(),
                       true});
    }
    return out;
}

/** Registers lsl:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(registerLslScheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::registerLslScheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &LslBackend::open, &discover);
    return result;
}
