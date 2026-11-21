//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eego_backend.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    eego:// : ANT Neuro eego amplifiers, or their simulator.
 *
 * One input stream "eeg" in the SDK's format: a double per channel, in volts for the inputs. The referential
 * inputs are EEG 001..EEG nnn, the bipolar ones BIP 1..BIP n, the trigger word STI 014 and the sample counter
 * COUNTER. A change of the trigger word away from its value at the start is pushed as an event at that sample.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eego_transport.h"
#include "../acq_devices.h"
#include "../trigger_word.h"
#include "../../acq_backend.h"
#include "../../acq_device_registry.h"
#include "../../acq_thread.h"
#include "../../detail/acq_shared_library.h"
#include "../../detail/acq_uri_params.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
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

constexpr std::chrono::microseconds kPoll(500);
constexpr int kDefaultRate = 512;

struct Config
{
    bool simulate = false;
    std::string serial;
    std::vector<std::string> library; // SDK library candidates
    std::optional<int> rate;
    std::optional<double> referenceRange;
    std::optional<double> bipolarRange;
    EegoSimulation simulation;
    AcqThreadPolicy thread;
};

AcqError invalid(std::string what)
{
    return {AcqErrorCode::InvalidParameter, std::move(what)};
}

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    const auto simulate = paramBool(uri, "simulate", false);
    const auto rate = paramInt(uri, "rate", 0, 1, 1 << 20);
    const auto referenceRange = paramDouble(uri, "reference_range", 0.0, 1e-6, 100.0);
    const auto bipolarRange = paramDouble(uri, "bipolar_range", 0.0, 1e-6, 100.0);
    const auto reference = paramInt(uri, "reference", 64, 1, 256);
    const auto bipolar = paramInt(uri, "bipolar", 24, 0, 64);
    const auto seed = paramInt(uri, "seed", 0, 0, INT64_MAX);
    const auto realtime = paramBool(uri, "realtime", true);
    const auto duration = paramDouble(uri, "duration", 0.0, 0.0, 1e7);
    const auto gain = paramDouble(uri, "gain", 1.0, 0.0, 1e6);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error : {simulate.error(), rate.error(), referenceRange.error(), bipolarRange.error(), reference.error(),
                                  bipolar.error(), seed.error(), realtime.error(), duration.error(), gain.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    Config c;
    c.simulate = *simulate;
    c.serial = uri.target;
    if (const auto sdk = uri.param("sdk")) {
        c.library = {std::string(*sdk)};
    } else {
        c.library = {acqEnvironment("MNE_CPP_EEGO_SDK"), acqSharedLibraryName("eego-SDK")};
    }
    if (uri.param("rate")) {
        c.rate = static_cast<int>(*rate);
    }
    if (uri.param("reference_range")) {
        c.referenceRange = *referenceRange;
    }
    if (uri.param("bipolar_range")) {
        c.bipolarRange = *bipolarRange;
    }
    c.simulation.reference = static_cast<std::size_t>(*reference);
    c.simulation.bipolar = static_cast<std::size_t>(*bipolar);
    c.simulation.seed = static_cast<std::uint64_t>(*seed);
    c.simulation.realtime = *realtime;
    c.simulation.duration = *duration;
    c.simulation.gain = *gain;
    c.thread = std::move(*thread);
    return c;
}

/** @return @p wanted if the amplifier supports it, @p fallback if nothing is wanted, else InvalidParameter. */
template<typename T>
AcqExpected<T> choose(const char* key, const std::optional<T>& wanted, const std::vector<T>& supported, T fallback)
{
    if (supported.empty()) {
        return AcqError(AcqErrorCode::DeviceIo, std::format("the eego amplifier reports no {}", key));
    }
    if (!wanted) {
        return std::find(supported.begin(), supported.end(), fallback) != supported.end() ? fallback : supported.front();
    }
    if (std::find(supported.begin(), supported.end(), *wanted) != supported.end()) {
        return *wanted;
    }
    std::string list;
    for (const T& value : supported) {
        list += std::format("{}{}", list.empty() ? "" : ", ", value);
    }
    return invalid(std::format("{}={}: the amplifier supports {}", key, *wanted, list));
}

AcqChannelInfo channelInfo(EegoChannelType type, std::size_t& reference, std::size_t& bipolar, std::size_t& other)
{
    AcqChannelInfo channel;
    channel.kind = AcqChannelKind::Misc;
    channel.unit = AcqUnit::Volt;
    switch (type) {
        case EegoChannelType::Reference:
            channel.name = std::format("EEG {:03}", ++reference);
            channel.kind = AcqChannelKind::Eeg;
            break;
        case EegoChannelType::Bipolar:
            channel.name = std::format("BIP {}", ++bipolar);
            break;
        case EegoChannelType::Trigger:
            channel.name = "STI 014";
            channel.kind = AcqChannelKind::Stim;
            channel.unit = AcqUnit::None;
            break;
        case EegoChannelType::SampleCounter:
            channel.name = "COUNTER";
            channel.unit = AcqUnit::None;
            break;
        default:
            channel.name = std::format("MISC {:03}", ++other);
            channel.unit = AcqUnit::Unknown;
            break;
    }
    return channel;
}

class EegoBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<EegoTransport> transport;
        if (config->simulate) {
            transport = make_eego_simulator(config->simulation);
        } else {
            auto driver = open_eego_driver(config->library, config->serial);
            if (!driver) {
                return driver.error();
            }
            transport = std::move(*driver);
        }
        auto rate = choose("rate", config->rate, transport->rates(), kDefaultRate);
        if (!rate) {
            return rate.error();
        }
        const auto referenceRanges = transport->reference_ranges();
        auto referenceRange = choose("reference_range", config->referenceRange, referenceRanges, referenceRanges.empty() ? 0.0 : referenceRanges.front());
        if (!referenceRange) {
            return referenceRange.error();
        }
        const auto bipolarRanges = transport->bipolar_ranges();
        auto bipolarRange = choose("bipolar_range", config->bipolarRange, bipolarRanges, bipolarRanges.empty() ? 0.0 : bipolarRanges.front());
        if (!bipolarRange) {
            return bipolarRange.error();
        }
        EegoSetup setup{*rate, *referenceRange, *bipolarRange};

        const std::vector<EegoChannel> device = transport->channels();
        AcqStreamInfo info;
        info.name = "eeg";
        info.sourceId = transport->description();
        info.format = AcqSampleFormat::Float64;
        info.layout = AcqLayout::Interleaved;
        info.sfreq = setup.rate;
        std::size_t reference = 0, bipolar = 0, other = 0;
        std::optional<std::size_t> trigger;
        for (std::size_t c = 0; c < device.size(); ++c) {
            if (device[c].type == EegoChannelType::Trigger && !trigger) {
                trigger = c;
            }
            info.channels.push_back(channelInfo(device[c].type, reference, bipolar, other));
        }
        info.properties = {{"reference_range", std::format("{} V", setup.reference_range)}, {"bipolar_range", std::format("{} V", setup.bipolar_range)}};

        std::unique_ptr<EegoBackend> backend(new EegoBackend(*config, std::move(transport), setup, uri.toString()));
        backend->m_trigger_channel = trigger;
        backend->m_width = device.size();
        backend->m_exg = reference;
        AcqStreamOptions options;
        options.capacitySamples = static_cast<std::size_t>(std::max(setup.rate, 1024));
        auto writer = backend->addInputStream(std::move(info), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = *writer;
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~EegoBackend() override
    {
        stop();
    }

    EegoBackend(const EegoBackend&) = delete;
    EegoBackend& operator=(const EegoBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"eego", "", "ANT Neuro " + m_transport->description(), m_uri};
    }

    AcqError start() override
    {
        if (AcqError e = m_transport->start(m_setup); !e.ok()) {
            return e;
        }
        m_trigger.reset();
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        AcqClockModel clock;
        clock.sfreq = m_setup.rate;
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
        // The referential inputs lead the stream
        std::vector<AcqImpedance> out;
        for (std::size_t c = 0; c < std::min(m_exg, ohms->size()); ++c) {
            out.push_back({0, c, (*ohms)[c]});
        }
        return out;
    }

private:
    EegoBackend(const Config& config, std::unique_ptr<EegoTransport> transport, const EegoSetup& setup, std::string uri)
    : m_config(config)
    , m_transport(std::move(transport))
    , m_setup(setup)
    , m_uri(std::move(uri))
    {
    }

    void run()
    {
        while (!m_stop.load(std::memory_order_acquire)) {
            auto values = m_transport->read();
            if (!values) {
                if (values.error() == AcqErrorCode::EndOfStream) {
                    m_writer->finish();
                } else {
                    reportError(values.error());
                }
                return;
            }
            const std::size_t samples = values->size() / m_width;
            if (samples == 0) {
                std::this_thread::sleep_for(kPoll);
                continue;
            }
            if (!write(values->first(samples * m_width), samples)) {
                return;
            }
        }
    }

    /** Copies @p samples whole samples into the ring and pushes their trigger events. */
    bool write(std::span<const double> values, std::size_t samples)
    {
        AcqExpected<AcqWriteView> view = AcqError(AcqErrorCode::Timeout);
        while (!view && view.error() == AcqErrorCode::Timeout) {
            if (m_stop.load(std::memory_order_acquire)) {
                return false;
            }
            view = m_writer->beginWrite(samples, kPoll); // a gating reader may not have freed the space yet
        }
        if (!view) {
            reportError(view.error());
            return false;
        }
        const std::size_t frame = m_width * sizeof(double);
        std::size_t done = 0;
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqWritePart& part = view->part(p);
            std::memcpy(part.at(0, 0), values.data() + done * m_width, part.samples * frame);
            done += part.samples;
        }
        if (m_trigger_channel) {
            const std::uint64_t first = m_writer->writePosition();
            for (std::size_t s = 0; s < samples; ++s) {
                const auto word = static_cast<std::int32_t>(std::lround(values[s * m_width + *m_trigger_channel]));
                if (m_trigger.changed(word)) {
                    if (AcqError e = m_writer->pushEvent({first + s, 0, word}); !e.ok()) {
                        reportError(e);
                        return false;
                    }
                }
            }
        }
        m_writer->commit(samples);
        return true;
    }

    Config m_config;
    std::unique_ptr<EegoTransport> m_transport;
    EegoSetup m_setup;
    std::string m_uri;
    AcqWriter* m_writer = nullptr;
    std::size_t m_width = 0;                      // channels of a sample
    std::size_t m_exg = 0;                        // referential inputs, leading the stream
    std::optional<std::size_t> m_trigger_channel; // stream index of the trigger word
    TriggerWord m_trigger;                        // owned by the acquisition thread while it runs
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "eego";
    info.description = "ANT Neuro eego amplifiers through the eego SDK, doubles in volts; the target is the serial number. "
                       "simulate=1 digitises a synthetic:// session instead.";
    info.exampleUri = "eego://?simulate=1";
    info.parameters = {
        {"simulate", "bool", "0", "Simulate an amplifier instead of using the eego SDK."},
        {"sdk", "string", "", "Path of the eego SDK library; default MNE_CPP_EEGO_SDK, then eego-SDK from the system loader."},
        {"rate", "int", "512", "Sampling rate in Hz, one the amplifier supports."},
        {"reference_range", "double", "", "Range of the referential inputs in volts; default the amplifier's first."},
        {"bipolar_range", "double", "", "Range of the bipolar inputs in volts; default the amplifier's first."},
        {"cpus", "string", "", "Comma-separated CPUs for the acquisition thread."},
        {"priority", "string", "normal", "Acquisition thread priority: normal, high or realtime."},
        {"reference", "int", "64", "Simulation: referential inputs."},
        {"bipolar", "int", "24", "Simulation: bipolar inputs."},
        {"seed", "int", "0", "Simulation: seed of the synthetic session and the impedances."},
        {"realtime", "bool", "1", "Simulation: deliver samples at the amplifier's pace; 0 as fast as they are read."},
        {"duration", "double", "0", "Simulation: session length in seconds, then end of stream; 0 runs until stopped."},
        {"gain", "double", "1", "Simulation: amplitude factor of the synthetic signal."},
    };
    return info;
}

std::vector<AcqDiscoveredDevice> discover(std::chrono::milliseconds /*timeout*/)
{
    std::vector<AcqDiscoveredDevice> found;
    if (auto serials = list_eego_amplifiers({acqEnvironment("MNE_CPP_EEGO_SDK"), acqSharedLibraryName("eego-SDK")})) {
        for (const std::string& serial : *serials) {
            found.push_back({"eego://" + serial, "ANT Neuro", "eego", "eego " + serial, true});
        }
    }
    return found;
}

/** Registers eego:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(register_eego_scheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::register_eego_scheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &EegoBackend::open, &discover);
    return result;
}
