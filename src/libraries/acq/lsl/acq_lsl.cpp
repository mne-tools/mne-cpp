//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_lsl.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    The lsl:// backend: liblsl loaded at runtime, the in-tree mne_lsl as fallback.
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
#include "../detail/acq_shared_library.h"
#include "../detail/acq_uri_params.h"

#include <lsl/lsl_stream_discovery.h>
#include <lsl/lsl_stream_inlet.h>
#include <lsl/lsl_stream_info.h>
#include <lsl/lsl_stream_outlet.h>

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
#include <type_traits>
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

constexpr int kFloat32 = 1; // lsl_channel_format_t
constexpr int kDouble64 = 2;
constexpr int kString = 3;
constexpr int kInt32 = 4;
constexpr int kInt16 = 5;
constexpr int kInt8 = 6;
constexpr int kInt64 = 7;
constexpr int kLostError = -2; // lsl_error_code_t

/** The part of the liblsl C API this backend uses, declared here so no liblsl header is needed. */
struct Liblsl
{
    using Info = void*;
    using Inlet = void*;
    using Xml = void*;

    AcqSharedLibrary library;
    int (*libraryVersion)() = nullptr;
    double (*localClock)() = nullptr;
    int (*resolveByProp)(Info*, unsigned, const char*, const char*, int, double) = nullptr;
    int (*resolveAll)(Info*, unsigned, double) = nullptr;
    void (*destroyInfo)(Info) = nullptr;
    Inlet (*createInlet)(Info, int, int, int) = nullptr;
    void (*destroyInlet)(Inlet) = nullptr;
    Info (*fullInfo)(Inlet, double, int*) = nullptr;
    void (*openStream)(Inlet, double, int*) = nullptr;
    double (*timeCorrection)(Inlet, double, int*) = nullptr;
    unsigned long (*pullF)(Inlet, float*, double*, unsigned long, unsigned long, double, int*) = nullptr;
    unsigned long (*pullD)(Inlet, double*, double*, unsigned long, unsigned long, double, int*) = nullptr;
    unsigned long (*pullI)(Inlet, std::int32_t*, double*, unsigned long, unsigned long, double, int*) = nullptr;
    unsigned long (*pullS)(Inlet, std::int16_t*, double*, unsigned long, unsigned long, double, int*) = nullptr;
    const char* (*name)(Info) = nullptr;
    const char* (*type)(Info) = nullptr;
    const char* (*sourceId)(Info) = nullptr;
    const char* (*hostname)(Info) = nullptr;
    int (*channelCount)(Info) = nullptr;
    double (*nominalSrate)(Info) = nullptr;
    int (*channelFormat)(Info) = nullptr;
    Xml (*desc)(Info) = nullptr;
    Xml (*child)(Xml, const char*) = nullptr;
    Xml (*nextSiblingNamed)(Xml, const char*) = nullptr;
    const char* (*childValue)(Xml, const char*) = nullptr;
    int (*empty)(Xml) = nullptr;

    /** @return The process-wide liblsl, loaded on first use; null with @p why set if it is missing. */
    static const Liblsl* instance(std::string* why = nullptr)
    {
        static const AcqExpected<std::unique_ptr<Liblsl>> loaded = load();
        if (!loaded) {
            if (why != nullptr) {
                *why = loaded.error().text();
            }
            return nullptr;
        }
        return loaded->get();
    }

private:
    static AcqExpected<std::unique_ptr<Liblsl>> load()
    {
        std::vector<std::string> candidates;
        candidates.push_back(acqEnvironment("MNE_CPP_LIBLSL"));
        const std::string pylsl = acqPythonPackageDir("pylsl", "lib");
        if (!pylsl.empty()) {
            candidates.push_back(pylsl + "/" + acqSharedLibraryName("lsl"));
        }
        candidates.push_back(acqSharedLibraryName("lsl"));
#if defined(__APPLE__)
        candidates.emplace_back("/opt/homebrew/lib/liblsl.dylib");
        candidates.emplace_back("/usr/local/lib/liblsl.dylib");
#endif
        auto library = AcqSharedLibrary::load(candidates);
        if (!library) {
            return AcqError(AcqErrorCode::BackendUnavailable, "liblsl " + std::string(library.error().context()));
        }
        auto lsl = std::make_unique<Liblsl>();
        lsl->library = std::move(*library);
        bool complete = true;
        const auto bind = [&](auto& function, const char* symbol) {
            function = reinterpret_cast<std::remove_reference_t<decltype(function)>>(lsl->library.symbol(symbol));
            complete = complete && function != nullptr;
        };
        bind(lsl->libraryVersion, "lsl_library_version");
        bind(lsl->localClock, "lsl_local_clock");
        bind(lsl->resolveByProp, "lsl_resolve_byprop");
        bind(lsl->resolveAll, "lsl_resolve_all");
        bind(lsl->destroyInfo, "lsl_destroy_streaminfo");
        bind(lsl->createInlet, "lsl_create_inlet");
        bind(lsl->destroyInlet, "lsl_destroy_inlet");
        bind(lsl->fullInfo, "lsl_get_fullinfo");
        bind(lsl->openStream, "lsl_open_stream");
        bind(lsl->timeCorrection, "lsl_time_correction");
        bind(lsl->pullF, "lsl_pull_chunk_f");
        bind(lsl->pullD, "lsl_pull_chunk_d");
        bind(lsl->pullI, "lsl_pull_chunk_i");
        bind(lsl->pullS, "lsl_pull_chunk_s");
        bind(lsl->name, "lsl_get_name");
        bind(lsl->type, "lsl_get_type");
        bind(lsl->sourceId, "lsl_get_source_id");
        bind(lsl->hostname, "lsl_get_hostname");
        bind(lsl->channelCount, "lsl_get_channel_count");
        bind(lsl->nominalSrate, "lsl_get_nominal_srate");
        bind(lsl->channelFormat, "lsl_get_channel_format");
        bind(lsl->desc, "lsl_get_desc");
        bind(lsl->child, "lsl_child");
        bind(lsl->nextSiblingNamed, "lsl_next_sibling_n");
        bind(lsl->childValue, "lsl_child_value_n");
        bind(lsl->empty, "lsl_empty");
        if (!complete) {
            return AcqError(AcqErrorCode::UnsupportedVersion, lsl->library.path() + " lacks parts of the liblsl C API");
        }
        // pull_chunk_* and channel_format behave as used here from liblsl 1.13 on.
        if (lsl->libraryVersion() < 113) {
            return AcqError(AcqErrorCode::UnsupportedVersion,
                            lsl->library.path() + " is liblsl " + std::to_string(lsl->libraryVersion()) +
                                "; 1.13 or newer is needed");
        }
        return lsl;
    }
};

std::string versionText(int version)
{
    return std::to_string(version / 100) + "." + std::to_string(version % 100);
}

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
    std::string library = "auto";
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
    c.library = std::string(uri.param("library").value_or("auto"));
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
    if (c.library != "auto" && c.library != "liblsl" && c.library != "intree") {
        return AcqError(AcqErrorCode::InvalidParameter, "library=" + c.library + ": use auto, liblsl or intree");
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

/** One connected stream, of either implementation. */
class Link
{
public:
    virtual ~Link() = default;
    /** Pulls up to @p frames frames into @p out; returns the count and the timestamp of the last, or an error. */
    virtual AcqExpected<std::size_t> pull(std::byte* out, std::size_t frames, double& lastTimestamp) = 0;
    /** @return Seconds to add to stream timestamps for the local LSL clock, refreshed by the caller. */
    virtual double timeCorrection() = 0;
    /** @return Local LSL clock in seconds, or NaN without one. */
    virtual double localClock() const = 0;
};

class LiblslLink final : public Link
{
public:
    LiblslLink(const Liblsl& lsl, Liblsl::Inlet inlet, int format, std::size_t channels)
    : m_lsl(lsl)
    , m_inlet(inlet)
    , m_format(format)
    , m_channels(channels)
    {
    }

    ~LiblslLink() override
    {
        m_lsl.destroyInlet(m_inlet);
    }

    LiblslLink(const LiblslLink&) = delete;
    LiblslLink& operator=(const LiblslLink&) = delete;

    AcqExpected<std::size_t> pull(std::byte* out, std::size_t frames, double& lastTimestamp) override
    {
        m_stamps.resize(frames);
        const unsigned long elements = static_cast<unsigned long>(frames * m_channels);
        int ec = 0;
        unsigned long got = 0;
        switch (m_format) {
            case kFloat32:
                got = m_lsl.pullF(m_inlet, reinterpret_cast<float*>(out), m_stamps.data(), elements, static_cast<unsigned long>(frames), 0.0, &ec);
                break;
            case kDouble64:
            case kInt64:
                got = m_lsl.pullD(m_inlet, reinterpret_cast<double*>(out), m_stamps.data(), elements, static_cast<unsigned long>(frames), 0.0, &ec);
                break;
            case kInt32:
                got = m_lsl.pullI(m_inlet, reinterpret_cast<std::int32_t*>(out), m_stamps.data(), elements, static_cast<unsigned long>(frames), 0.0, &ec);
                break;
            default:
                got = m_lsl.pullS(m_inlet, reinterpret_cast<std::int16_t*>(out), m_stamps.data(), elements, static_cast<unsigned long>(frames), 0.0, &ec);
                break;
        }
        if (ec == kLostError) {
            return AcqError(AcqErrorCode::DeviceIo, "the LSL stream was lost");
        }
        const std::size_t n = got / m_channels;
        if (n > 0) {
            lastTimestamp = m_stamps[n - 1];
            return n;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // nothing buffered yet
        return std::size_t(0);
    }

    double timeCorrection() override
    {
        int ec = 0;
        const double correction = m_lsl.timeCorrection(m_inlet, 2.0, &ec);
        if (ec == 0) {
            m_correction = correction;
        }
        return m_correction;
    }

    double localClock() const override
    {
        return m_lsl.localClock();
    }

private:
    const Liblsl& m_lsl;
    Liblsl::Inlet m_inlet;
    int m_format;
    std::size_t m_channels;
    std::vector<double> m_stamps;
    double m_correction = 0.0;
};

class InTreeLink final : public Link
{
public:
    InTreeLink(const mne::stream_info& info, std::size_t channels)
    : m_inlet(info)
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

    ~InTreeLink() override
    {
        m_inlet.close_stream();
    }

    InTreeLink(const InTreeLink&) = delete;
    InTreeLink& operator=(const InTreeLink&) = delete;

    AcqExpected<std::size_t> pull(std::byte* out, std::size_t frames, double& lastTimestamp) override
    {
        if (m_pending.empty()) {
            if (m_inlet.samples_available()) {
                m_pending = m_inlet.pull_chunk_float(m_stamps);
                m_next = 0;
            }
            if (m_pending.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                return std::size_t(0);
            }
        }
        std::size_t n = 0;
        for (; n < frames && m_next < m_pending.size(); ++n, ++m_next) {
            const std::vector<float>& sample = m_pending[m_next];
            for (std::size_t c = 0; c < m_channels; ++c) {
                const float v = c < sample.size() ? sample[c] : 0.0F;
                std::memcpy(out + (n * m_channels + c) * sizeof(float), &v, sizeof(float));
            }
        }
        lastTimestamp = m_stamps[m_next - 1];
        if (m_next == m_pending.size()) {
            m_pending.clear();
        }
        return n;
    }

    double timeCorrection() override
    {
        const double correction = m_inlet.time_correction(2.0);
        if (correction == correction) {
            m_correction = correction;
        }
        return m_correction;
    }

    double localClock() const override
    {
        return mne::local_clock();
    }

private:
    mne::stream_inlet m_inlet;
    std::size_t m_channels;
    std::vector<std::vector<float>> m_pending;
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
    LslSimulator(const Config& c, std::size_t block, AcqDevice session, AcqReader reader)
    : m_outlet(mne::stream_info(c.name, c.type, static_cast<int>(c.channels), c.sfreq, mne::ChannelFormat::Float32, c.sourceId))
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
        std::string why;
        const Liblsl* lsl = config->library == "intree" ? nullptr : Liblsl::instance(&why);
        if (lsl == nullptr && config->library == "liblsl") {
            return AcqError(AcqErrorCode::BackendUnavailable, why);
        }
        const AcqError error = lsl != nullptr ? backend->connect(*lsl) : backend->connectInTree();
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
        return {"lsl", m_runtime, m_device, m_uri};
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

    AcqError connect(const Liblsl& lsl)
    {
        const auto [property, value] = selector(m_config);
        Liblsl::Info found = nullptr;
        if (lsl.resolveByProp(&found, 1, property.c_str(), value.c_str(), 1, m_config.timeout) < 1 || found == nullptr) {
            return {AcqErrorCode::DeviceNotFound, "no LSL stream with " + property + "=" + value + " within " + std::to_string(m_config.timeout) + " s"};
        }
        Liblsl::Inlet inlet = lsl.createInlet(found, 360, 0, 1);
        lsl.destroyInfo(found);
        if (inlet == nullptr) {
            return {AcqErrorCode::DeviceIo, "cannot create an LSL inlet"};
        }
        int ec = 0;
        Liblsl::Info info = lsl.fullInfo(inlet, std::max(m_config.timeout, 1.0), &ec);
        if (info == nullptr || ec != 0) {
            lsl.destroyInlet(inlet);
            return {AcqErrorCode::DeviceIo, "cannot read the LSL stream description"};
        }
        AcqStreamInfo stream;
        AcqError described = describe(lsl, info, stream);
        const int format = lsl.channelFormat(info);
        lsl.destroyInfo(info);
        if (!described.ok()) {
            lsl.destroyInlet(inlet);
            return described;
        }
        lsl.openStream(inlet, std::max(m_config.timeout, 1.0), &ec);
        if (ec != 0) {
            lsl.destroyInlet(inlet);
            return {AcqErrorCode::DeviceIo, "cannot open the LSL stream (error " + std::to_string(ec) + ")"};
        }
        m_link = std::make_unique<LiblslLink>(lsl, inlet, format, stream.channels.size());
        m_runtime = "liblsl " + versionText(lsl.libraryVersion());
        return addStream(std::move(stream));
    }

    AcqError describe(const Liblsl& lsl, Liblsl::Info info, AcqStreamInfo& stream)
    {
        const int format = lsl.channelFormat(info);
        switch (format) {
            case kFloat32:
                stream.format = AcqSampleFormat::Float32;
                break;
            case kDouble64:
            case kInt64:
                stream.format = AcqSampleFormat::Float64;
                break;
            case kInt32:
                stream.format = AcqSampleFormat::Int32;
                break;
            case kInt16:
            case kInt8:
                stream.format = AcqSampleFormat::Int16;
                break;
            case kString:
                return {AcqErrorCode::UnsupportedFormat, "string (marker) LSL streams are not supported yet"};
            default:
                return {AcqErrorCode::UnsupportedFormat, "LSL channel format " + std::to_string(format)};
        }
        stream.sfreq = lsl.nominalSrate(info);
        if (stream.sfreq <= 0.0) {
            return {AcqErrorCode::UnsupportedFormat, "irregular-rate LSL streams are not supported yet"};
        }
        stream.name = lsl.name(info);
        stream.sourceId = lsl.sourceId(info);
        m_device = stream.name + " (" + lsl.type(info) + ") on " + lsl.hostname(info);
        stream.clockDomain = std::string("lsl:") + lsl.sourceId(info);
        const std::string streamType = lsl.type(info);
        const auto count = static_cast<std::size_t>(std::max(0, lsl.channelCount(info)));
        Liblsl::Xml channel = lsl.child(lsl.child(lsl.desc(info), "channels"), "channel");
        for (std::size_t c = 0; c < count; ++c) {
            AcqChannelInfo ch;
            const bool described = lsl.empty(channel) == 0;
            const std::string label = described ? lsl.childValue(channel, "label") : "";
            const std::string type = described ? lsl.childValue(channel, "type") : "";
            ch.name = label.empty() ? streamType + " " + std::to_string(c + 1) : label;
            ch.kind = kindOf(type.empty() ? streamType : type);
            unitOf(described ? lsl.childValue(channel, "unit") : "", ch);
            stream.channels.push_back(std::move(ch));
            if (described) {
                channel = lsl.nextSiblingNamed(channel, "channel");
            }
        }
        return {};
    }

    AcqError connectInTree()
    {
        const auto [property, value] = selector(m_config);
        const std::vector<mne::stream_info> found = mne::resolve_stream(property, value, m_config.timeout, 1);
        if (found.empty()) {
            return {AcqErrorCode::DeviceNotFound, "no LSL stream with " + property + "=" + value + " within " + std::to_string(m_config.timeout) + " s (in-tree LSL)"};
        }
        const mne::stream_info& info = found.front();
        if (info.channel_format() == mne::ChannelFormat::String) {
            return {AcqErrorCode::UnsupportedFormat, "string (marker) LSL streams are not supported yet"};
        }
        if (info.nominal_srate() <= 0.0) {
            return {AcqErrorCode::UnsupportedFormat, "irregular-rate LSL streams are not supported yet"};
        }
        AcqStreamInfo stream;
        stream.name = info.name();
        stream.sourceId = info.source_id();
        stream.format = AcqSampleFormat::Float32;
        stream.sfreq = info.nominal_srate();
        stream.clockDomain = "lsl:" + info.source_id();
        for (int c = 0; c < info.channel_count(); ++c) {
            AcqChannelInfo ch;
            ch.name = info.type() + " " + std::to_string(c + 1);
            ch.kind = kindOf(info.type());
            stream.channels.push_back(std::move(ch));
        }
        m_device = info.name() + " (" + info.type() + ") on " + info.hostname();
        auto link = std::make_unique<InTreeLink>(info, stream.channels.size());
        if (AcqError opened = link->open(); !opened.ok()) {
            return opened;
        }
        m_link = std::move(link);
        m_runtime = "in-tree";
        return addStream(std::move(stream));
    }

    AcqError addStream(AcqStreamInfo stream)
    {
        if (m_simulator) {
            for (AcqChannelInfo& channel : stream.channels) {
                channel.unit = AcqUnit::Volt; // the outlet carries no channel description
            }
        }
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
    void publishClock(std::uint64_t sample, double timestamp, bool refresh)
    {
        const auto steady = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
        AcqClockModel clock;
        clock.sfreq = m_sfreq;
        clock.anchorSample = sample;
        clock.utcOffsetNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count() -
            steady;
        const double local = m_link->localClock();
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
            auto got = m_link->pull(m_scratch.data(), m_block, timestamp);
            if (!got) {
                reportError(got.error());
                return;
            }
            const std::size_t n = *got;
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
    std::string m_runtime;
    std::string m_device;
    std::unique_ptr<LslSimulator> m_simulator; // outlives the link that reads it
    std::unique_ptr<Link> m_link;
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
    info.description = "Reads a Lab Streaming Layer stream through liblsl (loaded at runtime) or the in-tree LSL. "
                       "simulate=1 publishes a synthetic:// EEG session as an LSL stream and reads that instead.";
    info.exampleUri = "lsl://BioSemi";
    info.parameters = {
        {"type", "string", "", "Resolve by stream type (e.g. EEG) instead of the name."},
        {"source_id", "string", "", "Resolve by source id instead of the name."},
        {"timeout", "double", "5", "Seconds to wait for the stream to appear."},
        {"library", "enum:auto|liblsl|intree", "auto", "LSL implementation; auto prefers liblsl."},
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
    if (const Liblsl* lsl = Liblsl::instance()) {
        Liblsl::Info found[64] = {};
        const int n = lsl->resolveAll(found, 64, seconds);
        for (int i = 0; i < n; ++i) {
            const std::string name = lsl->name(found[i]);
            out.push_back({"lsl://" + name, name, "lsl",
                           std::to_string(lsl->channelCount(found[i])) + " ch, " +
                               std::to_string(static_cast<long long>(lsl->nominalSrate(found[i]))) + " Hz, " + lsl->type(found[i]),
                           true});
            lsl->destroyInfo(found[i]);
        }
        return out;
    }
    for (const mne::stream_info& info : mne::resolve_streams(seconds)) {
        out.push_back({"lsl://" + info.name() + "?library=intree", info.name(), "lsl",
                       std::to_string(info.channel_count()) + " ch, " + info.type() + " (in-tree LSL)", true});
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

//=============================================================================================================

std::string mne::lslRuntime()
{
    std::string why;
    if (const Liblsl* lsl = Liblsl::instance(&why)) {
        return "liblsl " + versionText(lsl->libraryVersion()) + " (" + lsl->library.path() + ")";
    }
    return "in-tree (" + why + ")";
}
