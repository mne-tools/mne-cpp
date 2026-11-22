//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     natus_backend.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2018
 * @brief    natus:// : a Natus Quantum amplifier through the datagrams of its streaming machine, or a simulator.
 *
 * One input stream "eeg" in the datagrams' format: floats with 1e-7 V per unit, EEG 001..EEG nnn. The datagrams
 * name neither the rate nor the channels, so both are parameters; a datagram with a different channel count is
 * an error.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "natus_transport.h"
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
#include <cmath>
#include <cstring>
#include <format>
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
constexpr std::size_t kMaxDatagram = 65536;

struct Config
{
    bool simulate = false;
    std::string address = "0.0.0.0";
    std::uint16_t port = 50000;
    std::size_t channels = 46;
    double rate = 2048.0;
    NatusSimulation simulation;
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
    const auto port = paramInt(uri, "port", 50000, 1, 65535);
    const auto channels = paramInt(uri, "channels", 46, 1, 1024);
    const auto rate = paramDouble(uri, "rate", 2048.0, 1.0, 1e5);
    const auto block = paramInt(uri, "block", 16, 1, 4096);
    const auto seed = paramInt(uri, "seed", 0, 0, INT64_MAX);
    const auto realtime = paramBool(uri, "realtime", true);
    const auto duration = paramDouble(uri, "duration", 0.0, 0.0, 1e7);
    const auto gain = paramDouble(uri, "gain", 1.0, 0.0, 1e6);
    auto thread = paramThreadPolicy(uri);
    for (const AcqError& error : {simulate.error(), port.error(), channels.error(), rate.error(), block.error(), seed.error(), realtime.error(),
                                  duration.error(), gain.error(), thread.error()}) {
        if (!error.ok()) {
            return error;
        }
    }
    c.simulate = *simulate;
    if (!uri.target.empty()) {
        if (c.simulate) {
            return invalid("target '" + uri.target + "': the simulator takes no address");
        }
        c.address = uri.target;
    }
    c.port = static_cast<std::uint16_t>(*port);
    c.channels = static_cast<std::size_t>(*channels);
    c.rate = *rate;
    if ((kNatusHeaderFloats + static_cast<std::size_t>(*block) * c.channels) * sizeof(float) > kMaxDatagram) {
        return invalid(std::format("block={}: {} channels of {} samples do not fit a datagram", *block, c.channels, *block));
    }
    c.simulation = {c.channels, c.rate, static_cast<std::size_t>(*block), static_cast<std::uint64_t>(*seed), *realtime, *duration, *gain};
    c.thread = std::move(*thread);
    return c;
}

class NatusBackend final : public AcqBackend
{
public:
    static AcqExpected<std::unique_ptr<AcqBackend>> open(const AcqUri& uri)
    {
        auto config = parseConfig(uri);
        if (!config) {
            return config.error();
        }
        std::unique_ptr<NatusTransport> transport;
        if (config->simulate) {
            transport = make_natus_simulator(config->simulation);
        } else {
            auto receiver = open_natus_receiver(config->address, config->port);
            if (!receiver) {
                return receiver.error();
            }
            transport = std::move(*receiver);
        }

        AcqStreamInfo info;
        info.name = "eeg";
        info.sourceId = transport->description();
        info.format = AcqSampleFormat::Float32;
        info.layout = AcqLayout::Interleaved;
        info.sfreq = config->rate;
        for (std::size_t c = 0; c < config->channels; ++c) {
            AcqChannelInfo& channel = info.channels.emplace_back();
            channel.name = std::format("EEG {:03}", c + 1);
            channel.kind = AcqChannelKind::Eeg;
            channel.unit = AcqUnit::Volt;
            channel.scale = kNatusScale;
        }

        std::unique_ptr<NatusBackend> backend(new NatusBackend(std::move(*config), std::move(transport), uri.toString()));
        AcqStreamOptions options;
        options.capacitySamples = static_cast<std::size_t>(std::max(4.0 * backend->m_config.rate, 4096.0));
        auto writer = backend->addInputStream(std::move(info), options);
        if (!writer) {
            return writer.error();
        }
        backend->m_writer = *writer;
        return std::unique_ptr<AcqBackend>(std::move(backend));
    }

    ~NatusBackend() override
    {
        stop();
    }

    NatusBackend(const NatusBackend&) = delete;
    NatusBackend& operator=(const NatusBackend&) = delete;

    AcqDeviceDescription description() const override
    {
        return {"natus", "", "Natus " + m_transport->description(), m_uri};
    }

    AcqError start() override
    {
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        AcqClockModel clock;
        clock.sfreq = m_config.rate;
        clock.anchorSample = m_writer->writePosition();
        clock.anchorHostNs = now;
        clock.utcOffsetNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - now;
        clock.source = AcqClockSource::Nominal;
        m_writer->setClock(clock);
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
    NatusBackend(Config config, std::unique_ptr<NatusTransport> transport, std::string uri)
    : m_config(std::move(config))
    , m_transport(std::move(transport))
    , m_uri(std::move(uri))
    , m_datagram(kMaxDatagram)
    {
    }

    /** @return The samples of a datagram, or the error that ends the stream. */
    AcqExpected<std::size_t> samples_of(std::size_t size) const
    {
        constexpr std::size_t kHeaderBytes = kNatusHeaderFloats * sizeof(float);
        std::array<float, kNatusHeaderFloats> header{};
        if (size >= kHeaderBytes) {
            std::memcpy(header.data(), m_datagram.data(), kHeaderBytes);
        }
        const float samples = header[1];
        const float channels = header[2];
        if (size < kHeaderBytes || !(samples >= 0.0F) || samples != std::trunc(samples) || samples > static_cast<float>(kMaxDatagram)) {
            return AcqError(AcqErrorCode::DeviceIo, "a datagram is not from the Natus streaming machine");
        }
        if (channels != static_cast<float>(m_config.channels)) {
            return AcqError(AcqErrorCode::DeviceIo,
                            std::format("the Natus streaming machine sends {} channels, channels={} is set", channels, m_config.channels));
        }
        const auto count = static_cast<std::size_t>(samples);
        if (size < kHeaderBytes + count * m_config.channels * sizeof(float)) {
            return AcqError(AcqErrorCode::DeviceIo, "a datagram of the Natus streaming machine is truncated");
        }
        return count;
    }

    void run()
    {
        const std::size_t frame = m_config.channels * sizeof(float);
        while (!m_stop.load(std::memory_order_acquire)) {
            auto size = m_transport->receive(m_datagram, kWait);
            if (!size) {
                if (size.error() == AcqErrorCode::Timeout) {
                    continue;
                }
                if (size.error() == AcqErrorCode::EndOfStream) {
                    m_writer->finish();
                } else {
                    reportError(size.error());
                }
                return;
            }
            const auto samples = samples_of(*size);
            if (!samples) {
                reportError(samples.error());
                return;
            }
            AcqExpected<AcqWriteView> view = AcqError(AcqErrorCode::Timeout);
            while (!view && view.error() == AcqErrorCode::Timeout) {
                if (m_stop.load(std::memory_order_acquire)) {
                    return;
                }
                view = m_writer->beginWrite(*samples, kPoll); // a gating reader may not have freed the space yet
            }
            if (!view) {
                reportError(view.error());
                return;
            }
            const std::byte* source = m_datagram.data() + kNatusHeaderFloats * sizeof(float);
            for (std::size_t p = 0; p < view->partCount(); ++p) {
                const AcqWritePart& part = view->part(p);
                std::memcpy(part.at(0, 0), source, part.samples * frame);
                source += part.samples * frame;
            }
            m_writer->commit(*samples);
        }
    }

    Config m_config;
    std::unique_ptr<NatusTransport> m_transport;
    std::string m_uri;
    AcqWriter* m_writer = nullptr;
    std::vector<std::byte> m_datagram; // owned by the acquisition thread while it runs
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

AcqSchemeInfo schemeInfo()
{
    AcqSchemeInfo info;
    info.scheme = "natus";
    info.description = "A Natus Quantum amplifier through the UDP datagrams of its streaming machine (startDataTransmit, Natus SDK), "
                       "received on the target address (default 0.0.0.0) and port; floats with 1e-7 V per unit. simulate=1 "
                       "sends a synthetic:// session instead.";
    info.exampleUri = "natus://?simulate=1";
    info.parameters = {
        {"simulate", "bool", "0", "Simulate the streaming machine instead of receiving its datagrams."},
        {"port", "int", "50000", "UDP port the streaming machine sends to."},
        {"channels", "int", "46", "Channels the streaming machine sends."},
        {"rate", "double", "2048", "Sampling rate in Hz set on the amplifier."},
        {"cpus", "string", "", "Comma-separated CPUs for the acquisition thread."},
        {"priority", "string", "normal", "Acquisition thread priority: normal, high or realtime."},
        {"block", "int", "16", "Simulation: samples per datagram."},
        {"seed", "int", "0", "Simulation: seed of the synthetic session."},
        {"realtime", "bool", "1", "Simulation: send at the amplifier's pace; 0 as fast as they are read."},
        {"duration", "double", "0", "Simulation: session length in seconds, then end of stream; 0 runs until stopped."},
        {"gain", "double", "1", "Simulation: amplitude factor of the synthetic signal."},
    };
    return info;
}

/** Registers natus:// when the library is loaded. */
struct Registrar
{
    Registrar() noexcept
    {
        static_cast<void>(register_natus_scheme());
    }
};

const Registrar kRegistrar;

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::register_natus_scheme()
{
    static const AcqError result = AcqDeviceRegistry::instance().registerScheme(schemeInfo(), &NatusBackend::open);
    return result;
}
