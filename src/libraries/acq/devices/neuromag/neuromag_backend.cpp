//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     neuromag_backend.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    neuromag:// : the data acquisition of Neuromag (MEGIN) Vectorview and TRIUX systems, or its simulator.
 *
 * One input stream "raw" in the acquisition's format: an int32 per channel and sample, with cal * range as
 * scale, channels as the measurement info describes them and the whole info as the fiff.MeasInfo/1 extension,
 * so the FIFF bridge restores projectors, digitizer points and transforms. A change of the trigger channel
 * (STI 014, else STI101, else the first stimulus channel) away from its value at the start is an event.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag_transport.h"
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
#include <atomic>
#include <chrono>
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

constexpr std::chrono::milliseconds kWait(100);
constexpr std::chrono::microseconds kPoll(500);

struct Config
{
    bool simulate = false;
    NeuromagDacq dacq;
    NeuromagSimulation simulation;
    std::chrono::milliseconds timeout{10000}; // for the measurement info
    AcqThreadPolicy thread;
};

AcqError invalid(std::string what)
{
    return {AcqErrorCode::InvalidParameter, std::move(what)};
}

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    if (!uri.target.empty()) {
        return invalid("target '" + uri.target + "': neuromag:// reads the acquisition of this machine and takes no target");
    }
    const auto simulate = paramBool(uri, "simulate", false);
    const auto port = paramInt(uri, "port", 11122, 1, 65535);
    const auto client = paramInt(uri, "client", 13014, 1, 1 << 20);
    const auto buffer = paramInt(uri, "buffer", 0, 0, 100000);
    const auto autostart = paramBool(uri, "autostart", true);
    const auto timeout = paramDouble(uri, "timeout", 10.0, 0.0, 3600.0);
    const auto meg = paramInt(uri, "meg", 306, 0, 1024);
    const auto eeg = paramInt(uri, "eeg", 0, 0, 512);
    const auto rate = paramDouble(uri, "rate", 1000.0, 1.0, 1e5);
    const auto seed = paramInt(uri, "seed", 0, 0, INT64_MAX);
    const auto realtime = paramBool(uri, "realtime", true);
    const auto duration = paramDouble(uri, "duration", 0.0, 0.0, 1e7);
    const auto gain = paramDouble(uri, "gain", 1.0, 0.0, 1e6);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error : {simulate.error(), port.error(), client.error(), buffer.error(), autostart.error(), timeout.error(),
                                  meg.error(), eeg.error(), rate.error(), seed.error(), realtime.error(), duration.error(), gain.error(),
                                  thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    Config c;
    c.simulate = *simulate;
    c.dacq.root = std::string(uri.param("root").value_or(c.dacq.root));
    c.dacq.host = std::string(uri.param("host").value_or(c.dacq.host));
    c.dacq.port = static_cast<int>(*port);
    c.dacq.client = static_cast<int>(*client);
    c.dacq.buffer = static_cast<int>(*buffer);
    c.dacq.autostart = *autostart;
    c.simulation.meg = static_cast<std::size_t>(*meg);
    c.simulation.eeg = static_cast<std::size_t>(*eeg);
    c.simulation.rate = *rate;
    c.simulation.buffer = *buffer > 0 ? static_cast<std::size_t>(*buffer) : 100;
    c.simulation.seed = static_cast<std::uint64_t>(*seed);
    c.simulation.realtime = *realtime;
    c.simulation.duration = *duration;
    c.simulation.gain = *gain;
    c.timeout = std::chrono::milliseconds(static_cast<std::int64_t>(*timeout * 1000.0));
    c.thread = std::move(*thread);
    return c;
}

bool is_block(const NeuromagTag& tag, int edge, int block)
{
    return tag.kind == edge && tag.as_int() == block;
}

/** @return The tags of the next measurement info, from its block start to its block end. */
AcqExpected<std::vector<NeuromagTag>> read_meas_info(NeuromagTransport& transport, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::vector<NeuromagTag> tags;
    bool inside = false;
    while (true) {
        const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        if (left.count() <= 0) {
            return AcqError(AcqErrorCode::Timeout, "the Neuromag acquisition sent no measurement info");
        }
        auto tag = transport.next(std::min(left, kWait));
        if (!tag) {
            if (tag.error() == AcqErrorCode::Timeout) {
                continue;
            }
            return tag.error();
        }
        const NeuromagTag& t = **tag;
        if (t.kind == fiff::kErrorMessage) {
            return AcqError(AcqErrorCode::DeviceIo,
                            "the Neuromag acquisition reports: " + std::string(reinterpret_cast<const char*>(t.data.data()), t.data.size()));
        }
        inside = inside || is_block(t, fiff::kBlockStart, fiff::kBlockMeasInfo);
        if (inside) {
            tags.push_back(t);
            if (is_block(t, fiff::kBlockEnd, fiff::kBlockMeasInfo)) {
                return tags;
            }
        }
    }
}

class NeuromagBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<NeuromagTransport> transport;
        if (config->simulate) {
            transport = make_neuromag_simulator(config->simulation);
        } else {
            auto client = make_neuromag_client(config->dacq);
            if (!client) {
                return client.error();
            }
            transport = std::move(*client);
        }

        // The measurement info comes at the start of a measurement
        if (AcqError e = transport->start(); !e.ok()) {
            return e;
        }
        auto meas_info = read_meas_info(*transport, config->timeout);
        transport->stop();
        if (!meas_info) {
            return meas_info.error();
        }
        auto info = stream_info(*meas_info, transport->description(), AcqSampleFormat::Int32);
        if (!info) {
            return info.error();
        }
        const std::optional<std::size_t> trigger = trigger_channel(*info);
        const std::size_t width = info->channelCount();
        const double rate = info->sfreq;

        std::unique_ptr<NeuromagBackend> backend(new NeuromagBackend(*config, std::move(transport), uri.toString()));
        backend->m_width = width;
        backend->m_rate = rate;
        backend->m_trigger_channel = trigger;
        AcqStreamOptions options;
        options.capacitySamples = static_cast<std::size_t>(std::max(4.0 * rate, 4096.0));
        auto writer = backend->addInputStream(std::move(*info), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = *writer;
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~NeuromagBackend() override
    {
        stop();
    }

    NeuromagBackend(const NeuromagBackend&) = delete;
    NeuromagBackend& operator=(const NeuromagBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"neuromag", "", "Neuromag " + m_transport->description(), m_uri};
    }

    AcqError start() override
    {
        if (AcqError e = m_transport->start(); !e.ok()) {
            return e;
        }
        // A new measurement repeats its info; its channels must be those of the stream
        auto meas_info = read_meas_info(*m_transport, m_config.timeout);
        if (meas_info) {
            if (auto info = stream_info(*meas_info, m_transport->description(), AcqSampleFormat::Int32); !info || info->channelCount() != m_width) {
                meas_info = AcqError(AcqErrorCode::InvalidState, "the Neuromag measurement has other channels than when it was opened");
            }
        }
        if (!meas_info) {
            m_transport->stop();
            return meas_info.error();
        }
        m_trigger.reset();
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
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

private:
    NeuromagBackend(const Config& config, std::unique_ptr<NeuromagTransport> transport, std::string uri)
    : m_config(config)
    , m_transport(std::move(transport))
    , m_uri(std::move(uri))
    {
    }

    void run()
    {
        while (!m_stop.load(std::memory_order_acquire)) {
            auto tag = m_transport->next(kWait);
            if (!tag) {
                if (tag.error() == AcqErrorCode::Timeout) {
                    continue;
                }
                if (tag.error() == AcqErrorCode::EndOfStream) {
                    m_writer->finish();
                } else {
                    reportError(tag.error());
                }
                return;
            }
            const NeuromagTag& t = **tag;
            if (t.kind == fiff::kCloseFile) {
                m_writer->finish(); // the measurement was stopped at the console
                return;
            }
            if (t.kind == fiff::kErrorMessage) {
                reportError({AcqErrorCode::DeviceIo,
                             "the Neuromag acquisition reports: " + std::string(reinterpret_cast<const char*>(t.data.data()), t.data.size())});
                return;
            }
            if (t.kind == fiff::kDataBuffer && !write(t.data)) {
                return;
            }
        }
    }

    /** Copies a data buffer into the ring and pushes its trigger events. */
    bool write(std::span<const std::byte> data)
    {
        const std::size_t frame = m_width * sizeof(std::int32_t);
        if (data.empty() || data.size() % frame != 0) {
            reportError({AcqErrorCode::DeviceIo, std::format("a Neuromag data buffer of {} bytes holds no whole samples of {} channels", data.size(), m_width)});
            return false;
        }
        const std::size_t samples = data.size() / frame;
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
        std::size_t done = 0;
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqWritePart& part = view->part(p);
            std::memcpy(part.at(0, 0), data.data() + done * frame, part.samples * frame);
            done += part.samples;
        }
        if (m_trigger_channel) {
            const std::uint64_t first = m_writer->writePosition();
            for (std::size_t s = 0; s < samples; ++s) {
                std::int32_t word = 0;
                std::memcpy(&word, data.data() + s * frame + *m_trigger_channel * sizeof(word), sizeof(word));
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
    std::unique_ptr<NeuromagTransport> m_transport;
    std::string m_uri;
    AcqWriter* m_writer = nullptr;
    std::size_t m_width = 0;
    double m_rate = 0.0;
    std::optional<std::size_t> m_trigger_channel;
    TriggerWord m_trigger; // owned by the acquisition thread while it runs
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "neuromag";
    info.description = "The data acquisition of a Neuromag (MEGIN) Vectorview or TRIUX system, read as a client of its data "
                       "server on the acquisition computer; int32 samples with the measurement info. simulate=1 digitises a "
                       "synthetic:// session instead.";
    info.exampleUri = "neuromag://?simulate=1";
    info.parameters = {
        {"simulate", "bool", "0", "Simulate the acquisition instead of reading it."},
        {"root", "string", "/neuro/dacq", "Directory with the acquisition's sockets/, shmem/ and raw/."},
        {"host", "string", "127.0.0.1", "Host of the acquisition's collector."},
        {"port", "int", "11122", "Port of the collector."},
        {"client", "int", "13014", "Client id at the data server; above 10000 for shared memory."},
        {"buffer", "int", "0", "Samples per data buffer to set on the collector; 0 keeps its setting (simulation: 100)."},
        {"autostart", "bool", "1", "Start and stop measurements through the collector."},
        {"timeout", "double", "10", "Seconds to wait for the measurement info."},
        {"cpus", "string", "", "Comma-separated CPUs for the acquisition thread."},
        {"priority", "string", "normal", "Acquisition thread priority: normal, high or realtime."},
        {"meg", "int", "306", "Simulation: magnetometer channels."},
        {"eeg", "int", "0", "Simulation: EEG channels."},
        {"rate", "double", "1000", "Simulation: sampling rate in Hz."},
        {"seed", "int", "0", "Simulation: seed of the synthetic session."},
        {"realtime", "bool", "1", "Simulation: deliver buffers at the acquisition's pace; 0 as fast as they are read."},
        {"duration", "double", "0", "Simulation: session length in seconds, then end of stream; 0 runs until stopped."},
        {"gain", "double", "1", "Simulation: amplitude factor of the synthetic signal."},
    };
    return info;
}

/** Registers neuromag:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(register_neuromag_scheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

#if !defined(__unix__) && !defined(__APPLE__) || defined(__EMSCRIPTEN__)
AcqExpected<std::unique_ptr<NeuromagTransport>> mne::detail::make_neuromag_client(const NeuromagDacq& /*dacq*/)
{
    return AcqError(AcqErrorCode::PlatformUnsupported, "the Neuromag acquisition runs on Linux and macOS; use simulate=1 for the simulator");
}
#endif

//=============================================================================================================

AcqError mne::register_neuromag_scheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &NeuromagBackend::open);
    return result;
}
