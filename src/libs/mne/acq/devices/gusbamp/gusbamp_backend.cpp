//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     gusbamp_backend.cpp
 * @author   Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     November, 2015
 * @brief    gusbamp:// : g.tec g.USBamp amplifiers, up to four synchronised, or their simulator.
 *
 * One input stream "eeg" in the API's format: a float per channel in microvolts, so with 1e-6 as scale. The
 * inputs of the master come first, then those of each slave, as EEG 001..EEG nnn; the master's trigger line,
 * if enabled, is STI 014. A change of the trigger line away from its value at the start is pushed as an event
 * at that sample, with the value as code.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbamp_transport.h"
#include "../acq_devices.h"
#include "../trigger_word.h"
#include "../../acq_backend.h"
#include "../../acq_device_registry.h"
#include "../../acq_thread.h"
#include "../../detail/acq_uri_params.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
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

constexpr std::chrono::milliseconds kWait(100);
constexpr std::chrono::microseconds kPoll(500);

/** Supported rates in Hz and the scans per transfer the API documentation recommends for them. */
constexpr std::array<std::pair<std::uint16_t, std::uint16_t>, 12> kRates = {
    {{32, 1}, {64, 2}, {128, 8}, {256, 16}, {512, 32}, {600, 64}, {1200, 128}, {2400, 128}, {4800, 256}, {9600, 512}, {19200, 512}, {38400, 512}}};

struct Config
{
    bool simulate = false;
    std::vector<std::string> serials; // master first; empty = the first connected unit
    GusbampSetup setup;
    GusbampSimulation simulation;
    AcqThreadPolicy thread;
};

AcqError invalid(std::string what)
{
    return {AcqErrorCode::InvalidParameter, std::move(what)};
}

/** @return The tokens of @p text between commas; one empty token for an empty text. */
std::vector<std::string_view> split(std::string_view text)
{
    std::vector<std::string_view> tokens;
    std::size_t begin = 0;
    while (true) {
        const std::size_t end = text.find(',', begin);
        tokens.push_back(text.substr(begin, end - begin));
        if (end == std::string_view::npos) {
            return tokens;
        }
        begin = end + 1;
    }
}

/** @return Inputs from a list such as "1-8,11,13", ascending and without repeats. */
AcqExpected<std::vector<std::uint8_t>> parseInputs(std::string_view text)
{
    std::array<bool, kGusbampInputs + 1> chosen{};
    for (const std::string_view token : split(text)) {
        const std::size_t dash = token.find('-');
        const std::string_view low = token.substr(0, dash);
        const std::string_view high = dash == std::string_view::npos ? low : token.substr(dash + 1);
        unsigned first = 0;
        unsigned last = 0;
        const auto a = std::from_chars(low.data(), low.data() + low.size(), first);
        const auto b = std::from_chars(high.data(), high.data() + high.size(), last);
        if (a.ec != std::errc() || a.ptr != low.data() + low.size() || b.ec != std::errc() || b.ptr != high.data() + high.size() ||
            first < 1 || last > kGusbampInputs || first > last) {
            return invalid(std::format("inputs={}: a list of inputs 1..16 such as 1-8,11", text));
        }
        for (unsigned i = first; i <= last; ++i) {
            chosen[i] = true;
        }
    }
    std::vector<std::uint8_t> inputs;
    for (unsigned i = 1; i <= kGusbampInputs; ++i) {
        if (chosen[i]) {
            inputs.push_back(static_cast<std::uint8_t>(i));
        }
    }
    return inputs;
}

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    Config c;
    if (!uri.target.empty()) {
        for (const std::string_view serial : split(uri.target)) {
            if (serial.empty() || std::find(c.serials.begin(), c.serials.end(), serial) != c.serials.end()) {
                return invalid("target '" + uri.target + "': distinct serial numbers, master first, separated by commas");
            }
            c.serials.emplace_back(serial);
        }
        if (c.serials.size() > kGusbampMaxUnits) {
            return invalid(std::format("target '{}': at most {} units run synchronised", uri.target, kGusbampMaxUnits));
        }
    }
    auto inputs = parseInputs(uri.param("inputs").value_or("1-16"));
    const auto simulate = paramBool(uri, "simulate", false);
    const auto rate = paramInt(uri, "rate", 1200, 1, 38400);
    const auto trigger = paramBool(uri, "trigger", false);
    const auto seed = paramInt(uri, "seed", 0, 0, INT64_MAX);
    const auto realtime = paramBool(uri, "realtime", true);
    const auto duration = paramDouble(uri, "duration", 0.0, 0.0, 1e7);
    const auto gain = paramDouble(uri, "gain", 1.0, 0.0, 1e6);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error : {inputs.error(), simulate.error(), rate.error(), trigger.error(), seed.error(), realtime.error(),
                                  duration.error(), gain.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    const auto supported = std::find_if(kRates.begin(), kRates.end(), [&](const auto& r) { return r.first == *rate; });
    if (supported == kRates.end()) {
        return invalid(std::format("rate={}: one of 32, 64, 128, 256, 512, 600, 1200, 2400, 4800, 9600, 19200 or 38400", *rate));
    }
    c.simulate = *simulate;
    c.setup.inputs = std::move(*inputs);
    c.setup.rate = supported->first;
    c.setup.scans = supported->second;
    c.setup.trigger = *trigger;
    c.simulation.seed = static_cast<std::uint64_t>(*seed);
    c.simulation.realtime = *realtime;
    c.simulation.duration = *duration;
    c.simulation.gain = *gain;
    c.thread = std::move(*thread);
    return c;
}

class GusbampBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<GusbampTransport> transport;
        if (config->simulate) {
            transport = make_gusbamp_simulator(config->serials.empty() ? std::vector<std::string>{"UB-0000.00.01"} : config->serials,
                                               config->simulation);
        } else {
            auto driver = open_gusbamp_driver(config->serials);
            if (!driver) {
                return driver.error();
            }
            transport = std::move(*driver);
        }
        const std::size_t units = transport->serials().size();

        AcqStreamInfo info;
        info.name = "eeg";
        info.sourceId = transport->description();
        info.format = AcqSampleFormat::Float32;
        info.layout = AcqLayout::Interleaved;
        info.sfreq = config->setup.rate;
        for (std::size_t c = 0; c < units * config->setup.inputs.size(); ++c) {
            AcqChannelInfo& channel = info.channels.emplace_back();
            channel.name = std::format("EEG {:03}", c + 1);
            channel.kind = AcqChannelKind::Eeg;
            channel.unit = AcqUnit::Volt;
            channel.scale = 1e-6;
        }
        if (config->setup.trigger) {
            AcqChannelInfo& channel = info.channels.emplace_back();
            channel.name = "STI 014";
            channel.kind = AcqChannelKind::Stim;
            channel.unit = AcqUnit::None;
        }

        std::unique_ptr<GusbampBackend> backend(new GusbampBackend(std::move(*config), std::move(transport), uri.toString()));
        AcqStreamOptions options;
        options.capacitySamples = std::max<std::size_t>(16 * std::size_t{backend->m_config.setup.scans}, backend->m_config.setup.rate);
        auto writer = backend->addInputStream(std::move(info), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = *writer;
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~GusbampBackend() override
    {
        stop();
    }

    GusbampBackend(const GusbampBackend&) = delete;
    GusbampBackend& operator=(const GusbampBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"gusbamp", "", "g.tec " + m_transport->description(), m_uri};
    }

    AcqError start() override
    {
        if (AcqError e = m_transport->start(m_config.setup); !e.ok()) {
            return e;
        }
        m_trigger.reset();
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        AcqClockModel clock;
        clock.sfreq = m_config.setup.rate;
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

    AcqExpected<std::vector<AcqImpedance>> measureImpedances(std::chrono::milliseconds /*timeout*/) override
    {
        auto ohms = m_transport->impedances(m_config.setup.inputs);
        if (!ohms) {
            return ohms.error();
        }
        std::vector<AcqImpedance> out;
        out.reserve(ohms->size());
        for (std::size_t c = 0; c < ohms->size(); ++c) {
            out.push_back({0, c, (*ohms)[c]});
        }
        return out;
    }

private:
    GusbampBackend(Config config, std::unique_ptr<GusbampTransport> transport, std::string uri)
    : m_config(std::move(config))
    , m_transport(std::move(transport))
    , m_uri(std::move(uri))
    {
    }

    void run()
    {
        const GusbampSetup& setup = m_config.setup;
        const std::size_t inputs = setup.inputs.size();
        while (!m_stop.load(std::memory_order_acquire)) {
            auto transfers = m_transport->read(kWait);
            if (!transfers) {
                if (transfers.error() == AcqErrorCode::Timeout) {
                    continue;
                }
                if (transfers.error() == AcqErrorCode::EndOfStream) {
                    m_writer->finish();
                } else {
                    reportError(transfers.error());
                }
                return;
            }
            AcqExpected<AcqWriteView> view = AcqError(AcqErrorCode::Timeout);
            while (!view && view.error() == AcqErrorCode::Timeout) {
                if (m_stop.load(std::memory_order_acquire)) {
                    return;
                }
                view = m_writer->beginWrite(setup.scans, kPoll); // a gating reader may not have freed the space yet
            }
            if (!view) {
                reportError(view.error());
                return;
            }
            // Scan after scan: the inputs of every unit, master first, then the master's trigger line
            const std::uint64_t first = m_writer->writePosition();
            std::size_t scan = 0;
            for (std::size_t p = 0; p < view->partCount(); ++p) {
                const AcqWritePart& part = view->part(p);
                for (std::size_t i = 0; i < part.samples; ++i, ++scan) {
                    auto* frame = static_cast<std::byte*>(part.at(0, i));
                    for (std::size_t u = 0; u < transfers->size(); ++u) {
                        const std::byte* values = (*transfers)[u].data() + kGusbampHeaderBytes + scan * setup.width(u) * sizeof(float);
                        std::memcpy(frame + u * inputs * sizeof(float), values, inputs * sizeof(float));
                        if (setup.width(u) > inputs) {
                            std::memcpy(frame + transfers->size() * inputs * sizeof(float), values + inputs * sizeof(float), sizeof(float));
                            float level = 0.0F;
                            std::memcpy(&level, values + inputs * sizeof(float), sizeof(level));
                            if (const auto code = static_cast<std::int32_t>(std::lround(level)); m_trigger.changed(code)) {
                                if (AcqError e = m_writer->pushEvent({first + scan, 0, code}); !e.ok()) {
                                    reportError(e);
                                    return;
                                }
                            }
                        }
                    }
                }
            }
            m_writer->commit(setup.scans);
        }
    }

    Config m_config;
    std::unique_ptr<GusbampTransport> m_transport;
    std::string m_uri;
    AcqWriter* m_writer = nullptr;
    TriggerWord m_trigger; // owned by the acquisition thread while it runs
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "gusbamp";
    info.description = "g.tec g.USBamp amplifiers through the g.USBamp API, up to four synchronised with the first serial number "
                       "as master; floats in microvolts. simulate=1 digitises a synthetic:// session instead.";
    info.exampleUri = "gusbamp://?simulate=1";
    info.parameters = {
        {"simulate", "bool", "0", "Simulate the units instead of using the g.USBamp API."},
        {"inputs", "string", "1-16", "Inputs of every unit, e.g. 1-8,11."},
        {"rate", "int", "1200", "Sampling rate in Hz: 32, 64, 128, 256, 512, 600, 1200, 2400, 4800, 9600, 19200 or 38400."},
        {"trigger", "bool", "0", "Deliver the master's trigger line as STI 014."},
        {"cpus", "string", "", "Comma-separated CPUs for the acquisition thread."},
        {"priority", "string", "normal", "Acquisition thread priority: normal, high or realtime."},
        {"seed", "int", "0", "Simulation: seed of the synthetic session and the impedances."},
        {"realtime", "bool", "1", "Simulation: deliver transfers at the amplifier's pace; 0 as fast as they are read."},
        {"duration", "double", "0", "Simulation: session length in seconds, then end of stream; 0 runs until stopped."},
        {"gain", "double", "1", "Simulation: amplitude factor of the synthetic signal."},
    };
    return info;
}

std::vector<AcqDiscoveredDevice> discover(std::chrono::milliseconds /*timeout*/)
{
    std::vector<AcqDiscoveredDevice> found;
    if (auto units = list_gusbamp_units()) {
        for (const std::string& serial : *units) {
            found.push_back({"gusbamp://" + serial, "g.tec", "gusbamp", "g.USBamp " + serial, true});
        }
    }
    return found;
}

/** Registers gusbamp:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(register_gusbamp_scheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

#if !defined(_WIN32)
AcqExpected<std::unique_ptr<GusbampTransport>> mne::detail::open_gusbamp_driver(std::span<const std::string> /*serials*/)
{
    return AcqError(AcqErrorCode::BackendUnavailable, "the g.USBamp API is only available on Windows; use simulate=1 for the simulator");
}

AcqExpected<std::vector<std::string>> mne::detail::list_gusbamp_units()
{
    return AcqError(AcqErrorCode::BackendUnavailable, "the g.USBamp API is only available on Windows");
}
#endif

//=============================================================================================================

AcqError mne::register_gusbamp_scheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &GusbampBackend::open, &discover);
    return result;
}
