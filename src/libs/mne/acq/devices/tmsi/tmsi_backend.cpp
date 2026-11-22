//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     tmsi_backend.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     September, 2013
 * @brief    tmsi:// : TMSi Refa and Porti amplifiers, or their simulator.
 *
 * One input stream "eeg" in the SDK's format: an int32 per channel, with each channel's gain, offset and
 * exponent as its scale and offset (volts for the analog inputs). The ExG inputs are EEG 001..EEG nnn, the
 * digital inputs STI 014. A change of the digital word away from its value at the start is pushed as an event
 * at that sample, with the word as code.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsi_transport.h"
#include "../acq_devices.h"
#include "../trigger_word.h"
#include "../../acq_backend.h"
#include "../../acq_device_registry.h"
#include "../../acq_thread.h"
#include "../../detail/acq_uri_params.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <format>
#include <optional>
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

constexpr std::chrono::microseconds kPoll(500);

struct Config
{
    bool simulate = false;
    TmsiSetup setup;
    std::size_t channels = 0; // leading device channels to deliver; 0 = all
    std::size_t block = 64;   // samples per SDK read at most
    TmsiSimulation simulation;
    AcqThreadPolicy thread;
};

AcqError invalid(std::string what)
{
    return {AcqErrorCode::InvalidParameter, std::move(what)};
}

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    if (!uri.target.empty()) {
        return invalid("target '" + uri.target + "': tmsi:// takes no target, only parameters");
    }
    const auto simulate = paramBool(uri, "simulate", false);
    const auto rate = paramDouble(uri, "rate", 2048.0, 1.0, 1e5);
    const auto channels = paramInt(uri, "channels", 0, 0, 1024);
    const auto block = paramInt(uri, "block", 64, 1, 65536);
    const auto average = paramBool(uri, "average", false);
    const auto seed = paramInt(uri, "seed", 0, 0, INT64_MAX);
    const auto exg = paramInt(uri, "exg", 64, 1, 256);
    const auto realtime = paramBool(uri, "realtime", true);
    const auto duration = paramDouble(uri, "duration", 0.0, 0.0, 1e7);
    const auto gain = paramDouble(uri, "gain", 1.0, 0.0, 1e6);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error : {simulate.error(), rate.error(), channels.error(), block.error(), average.error(), seed.error(),
                                  exg.error(), realtime.error(), duration.error(), gain.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    Config c;
    c.simulate = *simulate;
    c.setup.rate = *rate;
    c.setup.commonAverage = *average;
    c.channels = static_cast<std::size_t>(*channels);
    c.block = static_cast<std::size_t>(*block);
    c.simulation.exg = static_cast<std::size_t>(*exg);
    c.simulation.seed = static_cast<std::uint64_t>(*seed);
    c.simulation.realtime = *realtime;
    c.simulation.duration = *duration;
    c.simulation.gain = *gain;
    c.thread = std::move(*thread);
    return c;
}

AcqChannelInfo channelInfo(const TmsiChannel& device, std::size_t& exg, std::size_t& bip, std::size_t& aux, std::size_t& dig)
{
    AcqChannelInfo channel;
    channel.kind = AcqChannelKind::Misc;
    switch (device.type) {
        case TmsiChannelType::Exg:
            channel.name = std::format("EEG {:03}", ++exg);
            channel.kind = AcqChannelKind::Eeg;
            break;
        case TmsiChannelType::Bip:
            channel.name = std::format("BIP {}", ++bip);
            break;
        case TmsiChannelType::Aux:
            channel.name = std::format("AUX {}", ++aux);
            break;
        case TmsiChannelType::Dig:
            channel.name = std::format("STI {:03}", 14 + dig++);
            channel.kind = AcqChannelKind::Stim;
            break;
        case TmsiChannelType::Saw:
            channel.name = "SAW";
            break;
        default:
            channel.name = device.name;
            break;
    }
    channel.unit = device.unit == TmsiUnit::Volt ? AcqUnit::Volt : device.unit == TmsiUnit::Bit ? AcqUnit::None
                                                                                                : AcqUnit::Unknown;
    const double power = std::pow(10.0, device.exponent);
    channel.scale = device.gain * power;
    channel.offset = device.offset * power;
    return channel;
}

class TmsiBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<TmsiTransport> transport;
        if (config->simulate) {
            transport = makeTmsiSimulator(config->simulation);
        } else {
            auto driver = openTmsiDriver();
            if (!driver) {
                return driver.error();
            }
            transport = std::move(*driver);
        }

        const std::vector<TmsiChannel> device = transport->channels();
        if (config->channels > device.size()) {
            return invalid(std::format("channels={}: the amplifier has {}", config->channels, device.size()));
        }
        if (config->channels == 0) {
            config->channels = device.size();
        }
        const auto rate = transport->setup(config->setup);
        if (!rate) {
            return rate.error();
        }

        AcqStreamInfo info;
        info.name = "eeg";
        info.sourceId = transport->description();
        info.format = AcqSampleFormat::Int32;
        info.layout = AcqLayout::Interleaved;
        info.sfreq = *rate;
        std::size_t exg = 0, bip = 0, aux = 0, dig = 0;
        std::optional<std::size_t> digital;
        for (std::size_t c = 0; c < config->channels; ++c) {
            if (device[c].type == TmsiChannelType::Dig && !digital) {
                digital = c;
            }
            info.channels.push_back(channelInfo(device[c], exg, bip, aux, dig));
        }
        info.properties = {{"reference", config->setup.commonAverage ? "common average" : "amplifier"}};

        std::unique_ptr<TmsiBackend> backend(new TmsiBackend(*config, std::move(transport), device, uri.toString()));
        backend->m_digital = digital;
        backend->m_rate = *rate;
        AcqStreamOptions options;
        options.capacitySamples = std::max<std::size_t>(16 * config->block, static_cast<std::size_t>(*rate));
        auto writer = backend->addInputStream(std::move(info), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = *writer;
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~TmsiBackend() override
    {
        stop();
    }

    TmsiBackend(const TmsiBackend&) = delete;
    TmsiBackend& operator=(const TmsiBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"tmsi", "", "TMSi, " + m_transport->description(), m_uri};
    }

    AcqError start() override
    {
        if (AcqError e = m_transport->start(); !e.ok()) {
            return e;
        }
        m_trigger.reset();
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
        AcqClockModel clock;
        clock.sfreq = m_rate;
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

    AcqExpected<std::vector<AcqImpedance>> measureImpedances(std::chrono::milliseconds timeout) override
    {
        auto ohms = m_transport->impedances(timeout);
        if (!ohms) {
            return ohms.error();
        }
        std::vector<AcqImpedance> out;
        std::size_t exg = 0;
        for (std::size_t c = 0; c < m_config.channels; ++c) {
            if (m_device[c].type == TmsiChannelType::Exg && exg < ohms->size()) {
                out.push_back({0, c, (*ohms)[exg++]});
            }
        }
        return out;
    }

private:
    TmsiBackend(const Config& config, std::unique_ptr<TmsiTransport> transport, std::vector<TmsiChannel> device, std::string uri)
    : m_config(config)
    , m_transport(std::move(transport))
    , m_device(std::move(device))
    , m_uri(std::move(uri))
    , m_scratch(config.block * m_device.size())
    {
    }

    void run()
    {
        const std::size_t width = m_device.size();
        const std::size_t delivered = m_config.channels;
        while (!m_stop.load(std::memory_order_acquire)) {
            auto got = m_transport->read(m_scratch);
            if (!got) {
                if (got.error() == AcqErrorCode::EndOfStream) {
                    m_writer->finish();
                } else {
                    reportError(got.error());
                }
                return;
            }
            const std::size_t samples = *got;
            if (samples == 0) {
                std::this_thread::sleep_for(kPoll);
                continue;
            }
            AcqExpected<AcqWriteView> view = AcqError(AcqErrorCode::Timeout);
            while (!view && view.error() == AcqErrorCode::Timeout) {
                if (m_stop.load(std::memory_order_acquire)) {
                    return;
                }
                view = m_writer->beginWrite(samples, kPoll); // a gating reader may not have freed the space yet
            }
            if (!view) {
                reportError(view.error());
                return;
            }
            std::size_t s = 0;
            for (std::size_t p = 0; p < view->partCount(); ++p) {
                const AcqWritePart& part = view->part(p);
                for (std::size_t i = 0; i < part.samples; ++i, ++s) {
                    std::memcpy(part.at(0, i), m_scratch.data() + s * width, delivered * sizeof(std::int32_t));
                }
            }
            if (!pushTriggers(samples, width)) {
                return;
            }
            m_writer->commit(samples);
        }
    }

    /** Pushes an event for every change of the digital word to a value other than its idle one. */
    bool pushTriggers(std::size_t samples, std::size_t width)
    {
        if (!m_digital) {
            return true;
        }
        const std::uint64_t first = m_writer->writePosition();
        for (std::size_t s = 0; s < samples; ++s) {
            const std::int32_t word = m_scratch[s * width + *m_digital];
            if (m_trigger.changed(word)) {
                if (AcqError e = m_writer->pushEvent({first + s, 0, word}); !e.ok()) {
                    reportError(e);
                    return false;
                }
            }
        }
        return true;
    }

    Config m_config;
    std::unique_ptr<TmsiTransport> m_transport;
    std::vector<TmsiChannel> m_device;
    std::string m_uri;
    std::vector<std::int32_t> m_scratch; // one read of all device channels
    AcqWriter* m_writer = nullptr;
    double m_rate = 0.0;
    std::optional<std::size_t> m_digital; // stream index of the digital inputs
    TriggerWord m_trigger;                // owned by the acquisition thread while it runs
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "tmsi";
    info.description = "TMSi Refa and Porti amplifiers through the TMSi SDK, every channel as a 32-bit integer with "
                       "its gain, offset and exponent; simulate=1 digitises a synthetic:// session instead.";
    info.exampleUri = "tmsi://?simulate=1";
    info.parameters = {
        {"simulate", "bool", "0", "Simulate a Refa instead of using the TMSi SDK."},
        {"rate", "double", "2048", "Sampling rate in Hz; the amplifier runs at the supported rate nearest to it."},
        {"channels", "int", "0", "Leading amplifier channels to deliver; 0 = all."},
        {"block", "int", "64", "Samples per SDK read at most."},
        {"average", "bool", "0", "Reference the ExG inputs to their common average in the amplifier."},
        {"cpus", "string", "", "Comma-separated CPUs for the acquisition thread."},
        {"priority", "string", "normal", "Acquisition thread priority: normal, high or realtime."},
        {"seed", "int", "0", "Simulation: seed of the synthetic session and the impedances."},
        {"exg", "int", "64", "Simulation: ExG inputs; four bipolar and four auxiliary inputs, the digital inputs and the test ramp follow."},
        {"realtime", "bool", "1", "Simulation: deliver samples at the amplifier's pace; 0 as fast as they are read."},
        {"duration", "double", "0", "Simulation: session length in seconds, then end of stream; 0 runs until stopped."},
        {"gain", "double", "1", "Simulation: amplitude factor of the synthetic signal."},
    };
    return info;
}

std::vector<AcqDiscoveredDevice> discover(std::chrono::milliseconds /*timeout*/)
{
    std::vector<AcqDiscoveredDevice> found;
    if (auto driver = openTmsiDriver()) {
        found.push_back({"tmsi://", "TMSi", "tmsi", (*driver)->description(), true});
    }
    return found;
}

/** Registers tmsi:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(registerTmsiScheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

#if !defined(_WIN32)
AcqExpected<std::unique_ptr<TmsiTransport>> mne::detail::openTmsiDriver()
{
    return AcqError(AcqErrorCode::BackendUnavailable, "the TMSi SDK is only available on Windows; use simulate=1 for the simulator");
}
#endif

//=============================================================================================================

AcqError mne::registerTmsiScheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &TmsiBackend::open, &discover);
    return result;
}
