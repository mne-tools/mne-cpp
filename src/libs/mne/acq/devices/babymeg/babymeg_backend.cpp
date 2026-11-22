//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     babymeg_backend.cpp
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2013
 * @brief    babymeg:// : the acquisition server of a BabyMEG system.
 *
 * One input stream "raw" of Float32 values scaled by cal / gain, with the channels, sensor positions and coil types
 * the server describes; its gains are read again at every open. The trigger lines TRG001 to TRG008 above the
 * threshold make up a digital trigger word, which goes into DTRG01 and whose changes are events.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_transport.h"
#include "../acq_devices.h"
#include "../neuromag/neuromag_fiff.h"
#include "../trigger_word.h"
#include "../../acq_backend.h"
#include "../../acq_device_registry.h"
#include "../../acq_thread.h"
#include "../../detail/acq_fiff_codes.h"
#include "../../detail/acq_uri_params.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <atomic>
#include <bit>
#include <cstring>
#include <format>
#include <optional>
#include <thread>
#include <vector>

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
constexpr std::size_t kTriggerLines = 8;

// FIFF codes of the server's channel description
constexpr int kCoilNone = 0;
constexpr int kCoilEeg = 1;
constexpr int kCoilBabyMag = 7002;
constexpr int kCoilBabyRefMag = 7003;
constexpr int kCoilBabyRefMag2 = 7004;
constexpr int kQuat1 = 701;
constexpr int kHpiGoodness = 707;

struct Config
{
    bool simulate = false;
    std::string host = "localhost";
    std::uint16_t port = 6340;
    std::uint16_t command_port = 6341;
    double threshold = 1.0;
    std::chrono::milliseconds timeout{10000};
    BabyMegSimulation simulation;
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
    const auto port = paramInt(uri, "port", 6340, 1, 65535);
    const auto command_port = paramInt(uri, "command_port", 6341, 1, 65535);
    const auto threshold = paramDouble(uri, "threshold", 1.0, -1e6, 1e6);
    const auto timeout = paramDouble(uri, "timeout", 10.0, 0.1, 3600.0);
    const auto meg = paramInt(uri, "meg", 270, 0, 4096);
    const auto ref = paramInt(uri, "ref", 114, 0, 4096);
    const auto rate = paramDouble(uri, "rate", 1000.0, 1.0, 1e5);
    const auto samples = paramInt(uri, "samples", 100, 1, 100000);
    const auto seed = paramInt(uri, "seed", 0, 0, INT64_MAX);
    const auto realtime = paramBool(uri, "realtime", true);
    const auto duration = paramDouble(uri, "duration", 0.0, 0.0, 1e7);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error : {simulate.error(), port.error(), command_port.error(), threshold.error(), timeout.error(), meg.error(),
                                  ref.error(), rate.error(), samples.error(), seed.error(), realtime.error(), duration.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    c.simulate = *simulate;
    if (!uri.target.empty()) {
        if (c.simulate) {
            return invalid("target '" + uri.target + "': the simulator takes no server");
        }
        c.host = uri.target;
    }
    if (*meg + *ref == 0) {
        return invalid("meg=0&ref=0: the simulated system needs magnetometers");
    }
    c.port = static_cast<std::uint16_t>(*port);
    c.command_port = static_cast<std::uint16_t>(*command_port);
    c.threshold = *threshold;
    c.timeout = std::chrono::milliseconds(static_cast<std::int64_t>(*timeout * 1000.0));
    c.simulation = {static_cast<std::size_t>(*meg),
                    static_cast<std::size_t>(*ref),
                    *rate,
                    static_cast<std::size_t>(*samples),
                    static_cast<std::uint64_t>(*seed),
                    *realtime,
                    *duration};
    c.thread = std::move(*thread);
    return c;
}

NeuromagTag int_tag(int kind, std::int32_t value)
{
    NeuromagTag tag{kind, fiff::kTypeInt, std::vector<std::byte>(sizeof(value))};
    std::memcpy(tag.data.data(), &value, sizeof(value));
    return tag;
}

/** @return The FIFF channel record of a channel of the server's description. */
NeuromagChannel channel_record(const BabyMegChannel& channel, std::size_t index)
{
    NeuromagChannel ch;
    ch.scan_no = static_cast<std::int32_t>(index + 1);
    ch.log_no = static_cast<std::int32_t>(index + 1);
    ch.cal = channel.cal;
    ch.range = 1.0F / channel.gain;
    ch.loc = channel.loc;
    ch.name = channel.name;
    ch.unit = fiff_unit::kVolt;
    ch.coil_type = kCoilNone;
    switch (channel.coil_type) {
        case kCoilBabyMag:
            ch.kind = fiff_kind::kMeg;
            ch.unit = fiff_unit::kTesla;
            ch.coil_type = kCoilBabyMag;
            break;
        case kCoilBabyRefMag:
        case kCoilBabyRefMag2:
            ch.kind = fiff_kind::kRefMeg;
            ch.unit = fiff_unit::kTesla;
            ch.coil_type = channel.coil_type;
            break;
        case fiff_kind::kStim:
            ch.kind = fiff_kind::kStim;
            break;
        case fiff_kind::kEeg:
            ch.kind = fiff_kind::kEeg;
            ch.coil_type = kCoilEeg;
            break;
        default:
            if (channel.coil_type >= kQuat1 && channel.coil_type <= kHpiGoodness) {
                ch.kind = channel.coil_type; // head position: quaternions and goodness of fit
            } else {
                ch.kind = fiff_kind::kMeg;
                ch.unit = fiff_unit::kTesla;
            }
    }
    return ch;
}

/** @return The stream of the server's description. */
AcqExpected<AcqStreamInfo> stream_of(const BabyMegInfo& description, const std::string& source)
{
    std::vector<NeuromagTag> meas_info;
    meas_info.push_back(int_tag(fiff::kBlockStart, fiff::kBlockMeasInfo));
    meas_info.push_back(int_tag(fiff::kNchan, static_cast<std::int32_t>(description.channels.size())));
    NeuromagTag sfreq{fiff::kSfreq, fiff::kTypeFloat, std::vector<std::byte>(sizeof(float))};
    const auto rate = static_cast<float>(description.rate);
    std::memcpy(sfreq.data.data(), &rate, sizeof(rate));
    meas_info.push_back(std::move(sfreq));
    for (std::size_t i = 0; i < description.channels.size(); ++i) {
        meas_info.push_back({fiff::kChInfo, fiff::kTypeChInfo, channel_record(description.channels[i], i).serialize()});
    }
    meas_info.push_back(int_tag(fiff::kBlockEnd, fiff::kBlockMeasInfo));
    auto info = stream_info(meas_info, source, AcqSampleFormat::Float32);
    if (!info) {
        return info.error();
    }
    for (AcqChannelInfo& channel : info->channels) {
        channel.scale = static_cast<double>(channel.fiff->cal) * static_cast<double>(channel.fiff->range);
    }
    return info;
}

class BabyMegBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<BabyMegTransport> transport;
        if (config->simulate) {
            transport = make_babymeg_simulator(config->simulation);
        } else {
            auto client = connect_babymeg(config->host, config->port, config->command_port, std::min(config->timeout, std::chrono::milliseconds(5000)));
            if (!client) {
                return client.error();
            }
            transport = std::move(*client);
        }
        auto text = transport->info(config->timeout);
        if (!text) {
            return text.error();
        }
        auto description = parse_babymeg_info(*text);
        if (!description) {
            return description.error();
        }
        auto info = stream_of(*description, transport->description());
        if (!info) {
            return info.error();
        }

        std::unique_ptr<BabyMegBackend> backend(new BabyMegBackend(std::move(*config), std::move(transport), uri.toString()));
        backend->m_width = info->channelCount();
        backend->m_rate = info->sfreq;
        for (std::size_t t = 0; t < kTriggerLines; ++t) {
            const std::string name = std::format("TRG{:03}", t + 1);
            const auto it = std::find_if(info->channels.begin(), info->channels.end(), [&](const AcqChannelInfo& c) { return c.name == name; });
            if (it != info->channels.end()) {
                backend->m_lines.emplace_back(static_cast<std::size_t>(it - info->channels.begin()), it->scale, std::uint32_t{1} << t);
            }
        }
        const auto digital = std::find_if(info->channels.begin(), info->channels.end(), [](const AcqChannelInfo& c) { return c.name == "DTRG01"; });
        if (digital != info->channels.end()) {
            backend->m_digital = static_cast<std::size_t>(digital - info->channels.begin());
            backend->m_digital_scale = digital->scale;
        }
        AcqStreamOptions options;
        options.capacitySamples = static_cast<std::size_t>(std::max(4.0 * info->sfreq, 4096.0));
        auto writer = backend->addInputStream(std::move(*info), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = *writer;
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~BabyMegBackend() override
    {
        stop();
    }

    BabyMegBackend(const BabyMegBackend&) = delete;
    BabyMegBackend& operator=(const BabyMegBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"babymeg", "", m_transport->description(), m_uri};
    }

    AcqError start() override
    {
        if (AcqError e = m_transport->start(); !e.ok()) {
            return e;
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
    /** A trigger line: its channel, scale and bit in the trigger word. */
    struct Line
    {
        std::size_t channel;
        double scale;
        std::uint32_t bit;
    };

    BabyMegBackend(Config config, std::unique_ptr<BabyMegTransport> transport, std::string uri)
    : m_config(std::move(config))
    , m_transport(std::move(transport))
    , m_uri(std::move(uri))
    {
    }

    void run()
    {
        while (!m_stop.load(std::memory_order_acquire)) {
            auto package = m_transport->next(kWait);
            if (!package) {
                if (package.error() == AcqErrorCode::Timeout) {
                    continue;
                }
                if (package.error() == AcqErrorCode::EndOfStream) {
                    m_writer->finish();
                } else {
                    reportError(package.error());
                }
                return;
            }
            if (!write(*package)) {
                return;
            }
        }
    }

    /** Copies a data package into the ring, with the trigger word, and pushes its events. */
    bool write(std::span<const std::byte> package)
    {
        const std::size_t frame = m_width * sizeof(float);
        if (package.empty() || package[0] != std::byte{'4'}) {
            reportError({AcqErrorCode::UnsupportedFormat, "the BabyMEG server sends values of other than 4 bytes"});
            return false;
        }
        const std::span<const std::byte> values = package.subspan(1);
        if (values.empty() || values.size() % frame != 0) {
            reportError({AcqErrorCode::DeviceIo, std::format("a BabyMEG package of {} bytes holds no whole samples of {} channels", values.size(), m_width)});
            return false;
        }
        const std::size_t samples = values.size() / frame;
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
        const std::uint64_t first = m_writer->writePosition();
        std::vector<float>& sample = m_sample;
        sample.resize(m_width);
        std::size_t s = 0;
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqWritePart& part = view->part(p);
            for (std::size_t i = 0; i < part.samples; ++i, ++s) {
                const std::byte* in = values.data() + s * frame;
                for (std::size_t c = 0; c < m_width; ++c) {
                    std::uint32_t bits = 0;
                    for (std::size_t b = 0; b < 4; ++b) {
                        bits = (bits << 8) | std::to_integer<std::uint32_t>(in[c * 4 + b]);
                    }
                    sample[c] = std::bit_cast<float>(bits);
                }
                std::uint32_t word = 0;
                for (const Line& line : m_lines) {
                    if (static_cast<double>(sample[line.channel]) * line.scale >= m_config.threshold) {
                        word |= line.bit;
                    }
                }
                if (m_digital) {
                    sample[*m_digital] = static_cast<float>(static_cast<double>(word) / m_digital_scale);
                }
                std::memcpy(part.at(0, i), sample.data(), frame);
                if (!m_lines.empty() && m_trigger.changed(static_cast<std::int32_t>(word))) {
                    if (AcqError e = m_writer->pushEvent({first + s, 0, static_cast<std::int32_t>(word)}); !e.ok()) {
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
    std::unique_ptr<BabyMegTransport> m_transport;
    std::string m_uri;
    AcqWriter* m_writer = nullptr;
    std::size_t m_width = 0;
    double m_rate = 0.0;
    std::vector<Line> m_lines;
    std::optional<std::size_t> m_digital;
    double m_digital_scale = 1.0;
    // Owned by the acquisition thread while it runs
    std::vector<float> m_sample;
    TriggerWord m_trigger;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "babymeg";
    info.description = "The acquisition server of a BabyMEG system (the target, default localhost): channels, sensor positions and "
                       "gains from its command port, the data from its data port; the trigger lines TRG001 to TRG008 make up the "
                       "trigger word in DTRG01 and the events. simulate=1 serves a synthetic session instead.";
    info.exampleUri = "babymeg://?simulate=1";
    info.parameters = {
        {"simulate", "bool", "0", "Simulate the server instead of connecting to it."},
        {"port", "int", "6340", "Data port of the server."},
        {"command_port", "int", "6341", "Command port of the server, which describes the channels."},
        {"threshold", "double", "1", "Volts above which a trigger line counts as active."},
        {"timeout", "double", "10", "Seconds to wait for the connection and the channel description."},
        {"cpus", "string", "", "Comma-separated CPUs for the acquisition thread."},
        {"priority", "string", "normal", "Acquisition thread priority: normal, high or realtime."},
        {"meg", "int", "270", "Simulation: inner layer magnetometers."},
        {"ref", "int", "114", "Simulation: outer layer and reference magnetometers."},
        {"rate", "double", "1000", "Simulation: sampling rate in Hz."},
        {"samples", "int", "100", "Simulation: samples per data package."},
        {"seed", "int", "0", "Simulation: seed of the synthetic session."},
        {"realtime", "bool", "1", "Simulation: send at the system's pace; 0 as fast as they are read."},
        {"duration", "double", "0", "Simulation: session length in seconds, then end of stream; 0 runs until stopped."},
    };
    return info;
}

/** Registers babymeg:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(register_babymeg_scheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::register_babymeg_scheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &BabyMegBackend::open);
    return result;
}
