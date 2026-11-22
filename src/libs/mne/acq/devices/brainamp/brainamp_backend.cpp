//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     brainamp_backend.cpp
 * @author   Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    brainamp:// : Brain Products BrainAmp amplifiers, or their simulator.
 *
 * One input stream "eeg" in the amplifier's wire format: int16 channels EEG 001..EEG nnn (volts, scale = the
 * resolution) and STI 014 (the marker word), at 5 kHz. The driver's blocks go into the ring unconverted. A
 * change of the marker word to a non-zero value is pushed as an event at that sample, with the word as code.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainamp_transport.h"
#include "../acq_devices.h"
#include "../../acq_backend.h"
#include "../../acq_device_registry.h"
#include "../../acq_thread.h"
#include "../../detail/acq_uri_params.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <format>
#include <thread>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

using Clock = std::chrono::steady_clock;

constexpr std::chrono::microseconds kPoll(500); // a block of 50 points takes 10 ms

struct Config
{
    bool simulate = false;
    BrainAmpSetup setup;
    BrainAmpSimulation simulation;
    AcqThreadPolicy thread;
};

AcqError invalid(std::string what)
{
    return {AcqErrorCode::InvalidParameter, std::move(what)};
}

AcqExpected<BrainAmpResolution> parseResolution(const AcqUri& uri)
{
    const auto value = uri.param("resolution");
    if (!value || *value == "100nV") {
        return BrainAmpResolution::Nv100;
    }
    if (*value == "500nV") {
        return BrainAmpResolution::Nv500;
    }
    if (*value == "10uV") {
        return BrainAmpResolution::Uv10;
    }
    if (*value == "152.6uV") {
        return BrainAmpResolution::Uv152;
    }
    return invalid("resolution=" + std::string(*value) + " is not one of 100nV, 500nV, 10uV, 152.6uV");
}

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    if (!uri.target.empty()) {
        return invalid("target '" + uri.target + "': brainamp:// takes no target, only parameters");
    }
    const auto simulate = paramBool(uri, "simulate", false);
    const auto channels = paramInt(uri, "channels", 32, 1, 4 * kBrainAmpChannelsPerAmplifier);
    const auto block = paramInt(uri, "block", 50, 1, 5000);
    const auto resolution = parseResolution(uri);
    const auto lowpass = paramInt(uri, "lowpass", 1000, 250, 1000);
    const auto dc = paramBool(uri, "dc", false);
    const auto seed = paramInt(uri, "seed", 0, 0, INT64_MAX);
    const auto amplifiers = paramInt(uri, "amplifiers", 0, 0, 4);
    const auto realtime = paramBool(uri, "realtime", true);
    const auto duration = paramDouble(uri, "duration", 0.0, 0.0, 1e7);
    const auto gain = paramDouble(uri, "gain", 1.0, 0.0, 1e6);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error : {simulate.error(), channels.error(), block.error(), resolution.error(), lowpass.error(),
                                  dc.error(), seed.error(), amplifiers.error(), realtime.error(), duration.error(),
                                  gain.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    if (*lowpass != 250 && *lowpass != 1000) {
        return invalid("lowpass=" + std::to_string(*lowpass) + " is not one of the hardware filters 250, 1000");
    }
    Config c;
    c.simulate = *simulate;
    for (std::int64_t i = 0; i < *channels; ++i) {
        c.setup.channels.push_back(static_cast<std::uint8_t>(i));
    }
    c.setup.points = static_cast<std::uint32_t>(*block);
    c.setup.resolution = *resolution;
    c.setup.lowPass250Hz = *lowpass == 250;
    c.setup.dcCoupling = *dc;
    const std::size_t needed = (c.setup.channels.size() + kBrainAmpChannelsPerAmplifier - 1) / kBrainAmpChannelsPerAmplifier;
    c.simulation.amplifiers = *amplifiers > 0 ? static_cast<std::size_t>(*amplifiers) : needed;
    c.simulation.seed = static_cast<std::uint64_t>(*seed);
    c.simulation.realtime = *realtime;
    c.simulation.duration = *duration;
    c.simulation.gain = *gain;
    c.thread = std::move(*thread);
    return c;
}

AcqStreamInfo streamInfo(const Config& config, const std::string& source)
{
    AcqStreamInfo info;
    info.name = "eeg";
    info.sourceId = source;
    info.format = AcqSampleFormat::Int16;
    info.layout = AcqLayout::Interleaved;
    info.sfreq = kBrainAmpRate;
    const double step = brainAmpVoltsPerBit(config.setup.resolution);
    for (std::size_t c = 0; c < config.setup.channels.size(); ++c) {
        AcqChannelInfo channel;
        channel.name = std::format("EEG {:03}", c + 1);
        channel.kind = AcqChannelKind::Eeg;
        channel.unit = AcqUnit::Volt;
        channel.scale = step;
        channel.rangeMin = -32768.0 * step;
        channel.rangeMax = 32767.0 * step;
        info.channels.push_back(std::move(channel));
    }
    AcqChannelInfo marker;
    marker.name = "STI 014";
    marker.kind = AcqChannelKind::Stim;
    marker.unit = AcqUnit::None;
    info.channels.push_back(std::move(marker));
    info.properties = {{"lowpass", config.setup.lowPass250Hz ? "250 Hz" : "1000 Hz"},
                       {"coupling", config.setup.dcCoupling ? "DC" : "AC"}};
    return info;
}

class BrainAmpBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<BrainAmpTransport> transport;
        if (config->simulate) {
            transport = makeBrainAmpSimulator(config->simulation);
        } else {
            auto driver = openBrainAmpDriver();
            if (!driver) {
                return driver.error();
            }
            transport = std::move(*driver);
        }

        const auto slots = transport->amplifiers();
        const auto connected = static_cast<std::size_t>(
            std::find(slots.begin(), slots.end(), BrainAmpAmplifier::None) - slots.begin());
        const std::size_t needed = (config->setup.channels.size() + kBrainAmpChannelsPerAmplifier - 1) / kBrainAmpChannelsPerAmplifier;
        if (connected < needed) {
            return AcqError(AcqErrorCode::DeviceNotFound,
                            std::format("{} channels need {} amplifiers, {} connected", config->setup.channels.size(), needed, connected));
        }
        if (AcqError e = transport->setup(config->setup); !e.ok()) {
            return e;
        }

        std::unique_ptr<BrainAmpBackend> backend(new BrainAmpBackend(*config, std::move(transport), uri.toString()));
        AcqStreamOptions options;
        options.capacitySamples = std::max<std::size_t>(16 * config->setup.points, static_cast<std::size_t>(kBrainAmpRate));
        auto writer = backend->addInputStream(streamInfo(*config, backend->m_transport->description()), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = *writer;
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~BrainAmpBackend() override
    {
        stop();
    }

    BrainAmpBackend(const BrainAmpBackend&) = delete;
    BrainAmpBackend& operator=(const BrainAmpBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"brainamp", "", "BrainAmp, " + m_transport->description(), m_uri};
    }

    AcqError start() override
    {
        if (AcqError e = m_transport->start(); !e.ok()) {
            return e;
        }
        m_marker = 0;
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
        AcqClockModel clock;
        clock.sfreq = kBrainAmpRate;
        clock.anchorSample = m_writer->writePosition();
        clock.anchorHostNs = now;
        clock.utcOffsetNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - now;
        clock.source = AcqClockSource::Nominal;
        m_writer->setClock(clock);

        m_stop.store(false, std::memory_order_relaxed);
        auto thread = start_thread(m_config.thread, [this] { run(); });
        if (!thread) {
            m_transport->stop();
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
            m_transport->stop();
        }
    }

private:
    BrainAmpBackend(const Config& config, std::unique_ptr<BrainAmpTransport> transport, std::string uri)
    : m_config(config)
    , m_transport(std::move(transport))
    , m_uri(std::move(uri))
    , m_scratch(config.setup.points * (config.setup.channels.size() + 1))
    {
    }

    void run()
    {
        const std::size_t points = m_config.setup.points;
        const std::size_t width = m_config.setup.channels.size() + 1;
        while (!m_stop.load(std::memory_order_acquire)) {
            auto view = m_writer->beginWrite(points, kPoll);
            if (!view && view.error() == AcqErrorCode::Timeout) {
                continue; // a gating reader has not freed the space yet
            }
            if (!view) {
                reportError(view.error());
                return;
            }
            // Read straight into the ring unless the block wraps around its end
            const bool direct = view->partCount() == 1;
            auto* target = direct ? reinterpret_cast<std::int16_t*>(view->part(0).at(0, 0)) : m_scratch.data();
            auto got = m_transport->read(std::span<std::int16_t>(target, points * width));
            if (!got) {
                if (got.error() == AcqErrorCode::EndOfStream) {
                    m_writer->finish();
                } else {
                    reportError(got.error());
                }
                return;
            }
            if (!*got) {
                std::this_thread::sleep_for(kPoll);
                continue;
            }
            if (!direct) {
                const std::byte* from = reinterpret_cast<const std::byte*>(m_scratch.data());
                for (std::size_t p = 0; p < view->partCount(); ++p) {
                    const AcqWritePart& part = view->part(p);
                    const std::size_t bytes = part.samples * width * sizeof(std::int16_t);
                    std::memcpy(part.at(0, 0), from, bytes);
                    from += bytes;
                }
            }
            if (!pushMarkers(target, points, width)) {
                return;
            }
            m_writer->commit(points);
        }
    }

    /** Pushes an event for every change of the marker word to a non-zero value. */
    bool pushMarkers(const std::int16_t* block, std::size_t points, std::size_t width)
    {
        const std::uint64_t first = m_writer->writePosition();
        for (std::size_t s = 0; s < points; ++s) {
            const std::int16_t marker = block[s * width + width - 1];
            if (marker != m_marker && marker != 0) {
                if (AcqError e = m_writer->pushEvent({first + s, 0, marker}); !e.ok()) {
                    reportError(e);
                    return false;
                }
            }
            m_marker = marker;
        }
        return true;
    }

    Config m_config;
    std::unique_ptr<BrainAmpTransport> m_transport;
    std::string m_uri;
    std::vector<std::int16_t> m_scratch; // one block, for blocks that wrap around the ring's end
    AcqWriter* m_writer = nullptr;
    std::int16_t m_marker = 0; // marker word of the previous point; owned by the acquisition thread
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "brainamp";
    info.description = "Brain Products BrainAmp amplifiers (up to four, 32 channels each) at 5 kHz in their 16-bit "
                       "wire format with the marker word; simulate=1 digitises a synthetic:// session instead.";
    info.exampleUri = "brainamp://?simulate=1";
    info.parameters = {
        {"simulate", "bool", "0", "Simulate the adapter and amplifiers instead of using the BrainAmp driver."},
        {"channels", "int", "32", "EEG channels, 1 to 128 (32 per amplifier)."},
        {"block", "int", "50", "Points per driver read (10 ms)."},
        {"resolution", "enum:100nV|500nV|10uV|152.6uV", "100nV", "Step of one bit; the range is 32768 steps either way."},
        {"lowpass", "enum:1000|250", "1000", "Hardware low-pass filter in Hz."},
        {"dc", "bool", "0", "DC instead of AC coupling."},
        {"cpus", "string", "", "Comma-separated CPUs for the acquisition thread."},
        {"priority", "string", "normal", "Acquisition thread priority: normal, high or realtime."},
        {"seed", "int", "0", "Simulation: seed of the synthetic session."},
        {"amplifiers", "int", "auto", "Simulation: connected amplifiers, 1 to 4; auto = as many as the channels need."},
        {"realtime", "bool", "1", "Simulation: deliver blocks at the amplifier's pace; 0 as fast as they are read."},
        {"duration", "double", "0", "Simulation: session length in seconds, then end of stream; 0 runs until stopped."},
        {"gain", "double", "1", "Simulation: amplitude factor of the synthetic signal."},
    };
    return info;
}

std::vector<AcqDiscoveredDevice> discover(std::chrono::milliseconds /*timeout*/)
{
    std::vector<AcqDiscoveredDevice> found;
    if (auto driver = openBrainAmpDriver()) {
        found.push_back({"brainamp://", "BrainAmp", "brainamp", (*driver)->description(), true});
    }
    return found;
}

/** Registers brainamp:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(registerBrainAmpScheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

#if !defined(MNE_ACQ_BRAINAMP_HARDWARE)
AcqExpected<std::unique_ptr<BrainAmpTransport>> mne::detail::openBrainAmpDriver()
{
    return AcqError(AcqErrorCode::BackendUnavailable,
                    "this build has no BrainAmp driver interface (it needs Windows and the BrainAmp SDK header, "
                    "CMake variable MNE_BRAINAMP_SDK_DIR); use simulate=1 for the simulator");
}
#endif

//=============================================================================================================

AcqError mne::registerBrainAmpScheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &BrainAmpBackend::open, &discover);
    return result;
}
