//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     rtserver_backend.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>
 * @since    0.1.0
 * @date     December, 2012
 * @brief    rtserver:// : the active connector of an mne_rt_server, e.g. its FIFF file simulator.
 *
 * One input stream "raw" of Float32 physical values with the channels and measurement info the server sends,
 * from the buffers it sends after start(). Changes of STI 014 (or STI101) become events. When the server stops
 * sending, the stream ends.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserver_transport.h"

#include "../acq_devices.h"
#include "../trigger_word.h"
#include "../../acq_backend.h"
#include "../../acq_device_registry.h"
#include "../../acq_thread.h"
#include "../../detail/acq_uri_params.h"

#include <atomic>
#include <charconv>
#include <cmath>
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
constexpr std::uint16_t kDefaultPort = 4217;
constexpr std::size_t kRingBytes = 64U << 20U;

struct Config
{
    bool simulate = false;
    std::string host = "localhost";
    std::uint16_t port = kDefaultPort;
    std::int64_t connector = 0;
    std::int64_t buffer = 0;
    std::string alias = "mne_acq";
    std::string simfile;
    std::chrono::milliseconds timeout{10000};
    RtServerSimulation simulation;
    AcqThreadPolicy thread;
};

AcqError invalid(std::string what)
{
    return {AcqErrorCode::InvalidParameter, std::move(what)};
}

AcqExpected<Config> parseConfig(const AcqUri& uri)
{
    Config c;
    const auto simulate = paramBool(uri, "simulate", false);
    const auto connector = paramInt(uri, "connector", 0, 0, INT32_MAX);
    const auto buffer = paramInt(uri, "buffer", 0, 0, 1000000);
    const auto timeout = paramDouble(uri, "timeout", 10.0, 0.1, 3600.0);
    const auto meg = paramInt(uri, "meg", 306, 0, 4096);
    const auto eeg = paramInt(uri, "eeg", 0, 0, 4096);
    const auto rate = paramDouble(uri, "rate", 1000.0, 1.0, 1e5);
    const auto seed = paramInt(uri, "seed", 0, 0, INT64_MAX);
    const auto realtime = paramBool(uri, "realtime", true);
    const auto duration = paramDouble(uri, "duration", 0.0, 0.0, 1e7);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error : {simulate.error(), connector.error(), buffer.error(), timeout.error(), meg.error(), eeg.error(), rate.error(),
                                  seed.error(), realtime.error(), duration.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    c.simulate = *simulate;
    if (!uri.target.empty()) {
        if (c.simulate) {
            return invalid("target '" + uri.target + "': the simulator takes no server");
        }
        const std::size_t colon = uri.target.rfind(':');
        c.host = uri.target.substr(0, colon);
        if (colon != std::string::npos) {
            const std::string_view port = std::string_view(uri.target).substr(colon + 1);
            unsigned value = 0;
            const auto [end, ec] = std::from_chars(port.data(), port.data() + port.size(), value);
            if (ec != std::errc() || end != port.data() + port.size() || value == 0 || value > 65534) {
                return invalid("target '" + uri.target + "': host:port with a command port 1..65534");
            }
            c.port = static_cast<std::uint16_t>(value);
        }
        if (c.host.empty()) {
            return invalid("target '" + uri.target + "': host:port");
        }
    }
    if (const auto alias = uri.param("alias")) {
        c.alias = *alias;
    }
    if (const auto simfile = uri.param("simfile")) {
        if (c.simulate) {
            return invalid("simfile: the simulator replays a synthetic recording");
        }
        c.simfile = *simfile;
    }
    if (*meg + *eeg == 0) {
        return invalid("meg=0&eeg=0: the simulated recording needs channels");
    }
    c.connector = *connector;
    c.buffer = *buffer;
    c.timeout = std::chrono::milliseconds(static_cast<std::int64_t>(*timeout * 1000.0));
    c.simulation = {static_cast<std::size_t>(*meg), static_cast<std::size_t>(*eeg), *rate, static_cast<std::uint64_t>(*seed), *realtime, *duration};
    c.thread = std::move(*thread);
    return c;
}

bool is_block(const NeuromagTag& tag, int edge, int block)
{
    return tag.kind == edge && tag.as_int() == block;
}

/** @return The tags of the data port up to the first one @p last accepts, which is included. */
template<typename Last>
AcqExpected<std::vector<NeuromagTag>> read_until(RtServerTransport& transport, std::chrono::milliseconds timeout, std::string_view what, Last last)
{
    std::vector<NeuromagTag> tags;
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true) {
        auto tag = transport.next(kWait);
        if (tag) {
            tags.push_back(**tag);
            if (last(tags.back())) {
                return tags;
            }
        } else if (tag.error() != AcqErrorCode::Timeout) {
            return tag.error();
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            return AcqError(AcqErrorCode::Timeout, std::format("{} sends no {}", transport.description(), what));
        }
    }
}

class RtServerBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<RtServerTransport> transport;
        if (config->simulate) {
            transport = make_rtserver_simulator(config->simulation);
        } else {
            auto client = connect_rtserver(config->host, config->port, std::min(config->timeout, std::chrono::milliseconds(5000)));
            if (!client) {
                return client.error();
            }
            transport = std::move(*client);
        }
        if (config->connector > 0) {
            auto reply = transport->command(std::format("selcon {}", config->connector), config->timeout);
            if (!reply) {
                return reply.error();
            }
            if (reply->find("doesn't match") != std::string::npos) {
                return invalid(std::format("connector={}: {} has no such connector", config->connector, transport->description()));
            }
        }
        if (!config->simfile.empty()) {
            auto reply = transport->command("simfile " + config->simfile, config->timeout);
            if (!reply) {
                return reply.error();
            }
            if (reply->find("not set") != std::string::npos || reply->find("succe") == std::string::npos) {
                return invalid(std::format("simfile={}: the connector does not replay it", config->simfile));
            }
        }
        if (config->buffer > 0) {
            auto reply = transport->command(std::format("bufsize {}", config->buffer), config->timeout);
            if (!reply) {
                return reply.error();
            }
            if (reply->find("not set") != std::string::npos) {
                return invalid(std::format("buffer={}: the connector keeps its buffer size", config->buffer));
            }
        }

        // The commands name this client by the id the data port tells it
        if (AcqError e = transport->send(rt::kGetClientId, {}); !e.ok()) {
            return e;
        }
        auto id_tags = read_until(*transport, config->timeout, "client id", [](const NeuromagTag& t) { return t.kind == rt::kClientId; });
        if (!id_tags) {
            return id_tags.error();
        }
        const std::int32_t id = id_tags->back().as_int().value_or(-1);
        if (AcqError e = transport->send(rt::kSetAlias, config->alias); !e.ok()) {
            return e;
        }
        if (auto reply = transport->command(std::format("measinfo {}", id), config->timeout); !reply) {
            return reply.error();
        }
        auto tags = read_until(*transport, config->timeout, "measurement info",
                               [](const NeuromagTag& t) { return is_block(t, fiff::kBlockEnd, fiff::kBlockMeasInfo); });
        if (!tags) {
            return tags.error();
        }
        // The connector sends calibrated values
        auto info = stream_info(block_of(*tags, fiff::kBlockMeasInfo), transport->description(), AcqSampleFormat::Float32);
        if (!info) {
            return info.error();
        }

        std::unique_ptr<RtServerBackend> backend(new RtServerBackend(std::move(*config), std::move(transport), uri.toString()));
        backend->m_id = id;
        backend->m_width = info->channelCount();
        backend->m_rate = info->sfreq;
        backend->m_trigger_channel = trigger_channel(*info);
        // Four seconds, though no more than kRingBytes when a simulator replays a recording many times faster
        AcqStreamOptions options;
        const std::size_t wholeFrames = kRingBytes / (backend->m_width * sizeof(float));
        const auto frames = static_cast<double>(wholeFrames);
        options.capacitySamples = static_cast<std::size_t>(std::max(std::min(4.0 * info->sfreq, frames), 4096.0));
        auto writer = backend->addInputStream(std::move(*info), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = *writer;
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~RtServerBackend() override
    {
        stop();
    }

    RtServerBackend(const RtServerBackend&) = delete;
    RtServerBackend& operator=(const RtServerBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"rtserver", "", "mne_rt_server " + m_transport->description(), m_uri};
    }

    AcqError start() override
    {
        if (auto reply = m_transport->command(std::format("start {}", m_id), m_config.timeout); !reply) {
            return reply.error();
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
            static_cast<void>(m_transport->command(std::format("stop {}", m_id), m_config.timeout));
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
            static_cast<void>(m_transport->command(std::format("stop {}", m_id), m_config.timeout));
        }
    }

private:
    RtServerBackend(Config config, std::unique_ptr<RtServerTransport> transport, std::string uri)
    : m_config(std::move(config))
    , m_transport(std::move(transport))
    , m_uri(std::move(uri))
    {
    }

    void run()
    {
        // Buffers of an earlier start may still be on their way; this start's begin with its raw-data block
        bool started = false;
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
            if (is_block(t, fiff::kBlockStart, fiff::kBlockRawData)) {
                started = true;
            } else if (started && is_block(t, fiff::kBlockEnd, fiff::kBlockRawData)) {
                m_writer->finish(); // stopped at the server
                return;
            } else if (started && t.kind == fiff::kDataBuffer && !write(t.data)) {
                return;
            }
        }
    }

    /** Copies a data buffer into the ring and pushes its trigger events. */
    bool write(std::span<const std::byte> data)
    {
        const std::size_t frame = m_width * sizeof(float);
        if (data.empty() || data.size() % frame != 0) {
            reportError({AcqErrorCode::DeviceIo, std::format("a data buffer of {} bytes holds no whole samples of {} channels", data.size(), m_width)});
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
                float value = 0.0F;
                std::memcpy(&value, data.data() + s * frame + *m_trigger_channel * sizeof(value), sizeof(value));
                const auto code = static_cast<std::int32_t>(std::lround(value));
                if (m_trigger.changed(code)) {
                    if (AcqError e = m_writer->pushEvent({first + s, 0, code}); !e.ok()) {
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
    std::unique_ptr<RtServerTransport> m_transport;
    std::string m_uri;
    AcqWriter* m_writer = nullptr;
    std::int32_t m_id = -1;
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
    info.scheme = "rtserver";
    info.description = "The active connector of an mne_rt_server (the target, host:port of its command port, default localhost:4217; "
                       "the data port is the next one), e.g. its FIFF file simulator: the channels and measurement info it sends, "
                       "calibrated. simulate=1 replays a simulated Neuromag recording instead.";
    info.exampleUri = "rtserver://?simulate=1";
    info.parameters = {
        {"simulate", "bool", "0", "Simulate the server instead of connecting to one."},
        {"connector", "int", "0", "Connector to activate, by its id in the server's \"conlist\"; 0 keeps the active one."},
        {"buffer", "int", "0", "Samples per buffer to set at the connector; 0 keeps its setting."},
        {"simfile", "string", "", "FIFF file the server's FIFF file simulator replays; empty keeps its file."},
        {"alias", "string", "mne_acq", "Name of this client in the server's \"clist\"."},
        {"timeout", "double", "10", "Seconds to wait for the connection and for each answer."},
        {"cpus", "string", "", "Comma-separated CPUs for the acquisition thread."},
        {"priority", "string", "normal", "Acquisition thread priority: normal, high or realtime."},
        {"meg", "int", "306", "Simulation: magnetometer channels."},
        {"eeg", "int", "0", "Simulation: EEG channels."},
        {"rate", "double", "1000", "Simulation: sampling rate in Hz."},
        {"seed", "int", "0", "Simulation: seed of the synthetic session."},
        {"realtime", "bool", "1", "Simulation: send buffers at the recording's pace; 0 as fast as they are read."},
        {"duration", "double", "0", "Simulation: session length in seconds, then end of stream; 0 runs until stopped."},
    };
    return info;
}

/** Registers rtserver:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(register_rtserver_scheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::register_rtserver_scheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &RtServerBackend::open);
    return result;
}
