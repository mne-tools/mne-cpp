//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     natus_simulator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November, 2026
 * @brief    A simulated Natus streaming machine and the UDP receiver of the real one.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "natus_transport.h"
#include "../transport_error.h"
#include "../../acq_device.h"
#include <mne/com/udp_socket.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cstring>
#include <format>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;
using mne::UdpSocket;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

class NatusSimulator final : public NatusTransport
{
public:
    explicit NatusSimulator(const NatusSimulation& simulation)
    : m_simulation(simulation)
    {
    }

    std::string description() const override
    {
        return std::format("simulated Natus streaming machine, seed {}", m_simulation.seed);
    }

    AcqExpected<std::size_t> receive(std::span<std::byte> buffer, std::chrono::milliseconds timeout) override
    {
        if (!m_reader.isValid()) {
            if (AcqError e = start(); !e.ok()) {
                return e;
            }
        }
        const std::size_t samples = m_simulation.block;
        const std::size_t channels = m_simulation.channels;
        auto view = m_reader.acquire(samples, timeout, samples);
        if (!view) {
            return view.error();
        }
        if (view->sampleCount() < samples) {
            static_cast<void>(m_reader.release(*view)); // whole datagrams only; the end of the session cuts the rest
            return AcqError(AcqErrorCode::EndOfStream);
        }
        const std::size_t bytes = (kNatusHeaderFloats + samples * channels) * sizeof(float);
        if (buffer.size() < bytes) {
            static_cast<void>(m_reader.release(*view));
            return AcqError(AcqErrorCode::Internal, "datagram buffer too small");
        }
        const std::array<float, kNatusHeaderFloats> header = {static_cast<float>(m_sent), static_cast<float>(samples), static_cast<float>(channels)};
        std::memcpy(buffer.data(), header.data(), sizeof(header));
        auto* out = buffer.data() + sizeof(header);
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t s = 0; s < part.samples; ++s) {
                for (std::size_t c = 0; c < channels; ++c) {
                    float volts = 0.0F;
                    std::memcpy(&volts, part.at(c, s), sizeof(volts));
                    const auto value = static_cast<float>(m_simulation.gain * volts / kNatusScale);
                    std::memcpy(out, &value, sizeof(value));
                    out += sizeof(value);
                }
            }
        }
        ++m_sent;
        if (AcqError e = m_reader.release(*view); !e.ok()) {
            return e;
        }
        return bytes;
    }

private:
    AcqError start()
    {
        std::string uri = std::format("synthetic://?sfreq={}&stim=0&eeg={}&block={}&seed={}&realtime={}", m_simulation.rate, m_simulation.channels,
                                      m_simulation.block, m_simulation.seed, m_simulation.realtime ? 1 : 0);
        if (m_simulation.duration > 0.0) {
            uri += std::format("&duration={}", m_simulation.duration);
        }
        auto session = AcqDevice::open(uri);
        if (!session) {
            return session.error();
        }
        AcqReaderOptions gating;
        gating.gating = true; // the session waits for this reader instead of overwriting unread samples
        auto reader = session->stream().reader(AcqReader::ReadMode::Lossless, gating);
        if (!reader) {
            return reader.error();
        }
        if (AcqError e = session->start(); !e.ok()) {
            return e;
        }
        m_session = std::move(*session);
        m_reader = std::move(*reader);
        return {};
    }

    NatusSimulation m_simulation;
    AcqDevice m_session;
    AcqReader m_reader;
    std::uint64_t m_sent = 0;
};

class NatusReceiver final : public NatusTransport
{
public:
    NatusReceiver(UdpSocket socket, std::string name)
    : m_socket(std::move(socket))
    , m_name(std::move(name))
    {
    }

    std::string description() const override
    {
        return m_name;
    }

    AcqExpected<std::size_t> receive(std::span<std::byte> buffer, std::chrono::milliseconds timeout) override
    {
        auto size = m_socket.receive(buffer, timeout);
        if (!size) {
            return transport_error(size.error(), AcqErrorCode::DeviceIo, "cannot receive on " + m_name);
        }
        return *size;
    }

private:
    UdpSocket m_socket;
    std::string m_name;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::unique_ptr<NatusTransport> mne::detail::make_natus_simulator(const NatusSimulation& simulation)
{
    return std::make_unique<NatusSimulator>(simulation);
}

//=============================================================================================================

AcqExpected<std::unique_ptr<NatusTransport>> mne::detail::open_natus_receiver(const std::string& address, std::uint16_t port)
{
    auto socket = UdpSocket::bind(address, port);
    if (!socket) {
        return transport_error(socket.error(), AcqErrorCode::DeviceIo, std::format("cannot receive on {}:{}; is another program using it?", address, port));
    }
    std::string name = std::format("UDP {}:{}", address, socket->port());
    return std::unique_ptr<NatusTransport>(std::make_unique<NatusReceiver>(std::move(*socket), std::move(name)));
}
