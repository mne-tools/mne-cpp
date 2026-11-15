//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     gusbamp_simulator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Simulated g.USBamps: a synthetic:// session in the transfers of the g.USBamp API.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbamp_transport.h"

#include "../../acq_device.h"

#include <algorithm>
#include <cstring>
#include <format>
#include <random>

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

constexpr std::byte kHeaderFill{0xA5}; // header content the backend must skip

class GusbampSimulator final : public GusbampTransport
{
public:
    GusbampSimulator(std::vector<std::string> serials, const GusbampSimulation& simulation)
    : m_serials(std::move(serials))
    , m_simulation(simulation)
    {
    }

    ~GusbampSimulator() override
    {
        stop();
    }

    std::vector<std::string> serials() const override
    {
        return m_serials;
    }

    std::string description() const override
    {
        std::string text = "simulated g.USBamp";
        for (std::size_t u = 0; u < m_serials.size(); ++u) {
            text += (u == 0 ? " " : ", ") + m_serials[u] + (u == 0 && m_serials.size() > 1 ? " (master)" : "");
        }
        return text + ", seed " + std::to_string(m_simulation.seed);
    }

    AcqError start(const GusbampSetup& setup) override
    {
        const std::size_t analog = m_serials.size() * setup.inputs.size();
        std::string uri = std::format("synthetic://?sfreq={}&stim=1&eeg={}&block={}&seed={}&realtime={}", setup.rate, analog,
                                      setup.scans, m_simulation.seed, m_simulation.realtime ? 1 : 0);
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
        m_setup = setup;
        m_session = std::move(*session);
        m_reader = std::move(*reader);
        m_buffers.resize(m_serials.size());
        m_transfers.resize(m_serials.size());
        for (std::size_t u = 0; u < m_serials.size(); ++u) {
            m_buffers[u].assign(setup.transfer_bytes(u), kHeaderFill);
            m_transfers[u] = m_buffers[u];
        }
        return {};
    }

    void stop() override
    {
        m_reader = AcqReader();
        m_session.close();
    }

    AcqExpected<std::span<const std::span<const std::byte>>> read(std::chrono::milliseconds timeout) override
    {
        const std::size_t scans = m_setup.scans;
        auto view = m_reader.acquire(scans, timeout, scans);
        if (!view) {
            return view.error();
        }
        if (view->sampleCount() < scans) {
            // The units deliver whole transfers only; the end of the session cuts the rest
            static_cast<void>(m_reader.release(*view));
            return AcqError(AcqErrorCode::EndOfStream);
        }
        // The session's channels are the inputs of every unit, then the trigger code
        const std::size_t inputs = m_setup.inputs.size();
        const std::size_t code = m_serials.size() * inputs;
        std::size_t scan = 0;
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t s = 0; s < part.samples; ++s, ++scan) {
                for (std::size_t u = 0; u < m_serials.size(); ++u) {
                    std::byte* values = m_buffers[u].data() + kGusbampHeaderBytes + scan * m_setup.width(u) * sizeof(float);
                    for (std::size_t i = 0; i < inputs; ++i) {
                        float volts = 0.0F;
                        std::memcpy(&volts, part.at(u * inputs + i, s), sizeof(volts));
                        const auto microvolts = static_cast<float>(m_simulation.gain * volts * 1e6);
                        std::memcpy(values + i * sizeof(float), &microvolts, sizeof(float));
                    }
                    if (m_setup.width(u) > inputs) {
                        std::memcpy(values + inputs * sizeof(float), part.at(code, s), sizeof(float));
                    }
                }
            }
        }
        if (AcqError e = m_reader.release(*view); !e.ok()) {
            return e;
        }
        return std::span<const std::span<const std::byte>>(m_transfers);
    }

    AcqExpected<std::vector<double>> impedances(std::span<const std::uint8_t> inputs) override
    {
        // Electrodes of a fresh gel cap: a few kilo-ohms, the same per unit and input for a seed
        std::mt19937_64 random(m_simulation.seed);
        std::uniform_real_distribution<double> ohms(1e3, 20e3);
        std::vector<double> all(m_serials.size() * kGusbampInputs);
        for (double& value : all) {
            value = ohms(random);
        }
        std::vector<double> values;
        values.reserve(m_serials.size() * inputs.size());
        for (std::size_t u = 0; u < m_serials.size(); ++u) {
            for (const std::uint8_t input : inputs) {
                values.push_back(all[u * kGusbampInputs + input - 1]);
            }
        }
        return values;
    }

private:
    std::vector<std::string> m_serials;
    GusbampSimulation m_simulation;
    GusbampSetup m_setup;
    AcqDevice m_session;
    AcqReader m_reader;
    std::vector<std::vector<std::byte>> m_buffers; // one transfer per unit
    std::vector<std::span<const std::byte>> m_transfers;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::unique_ptr<GusbampTransport> mne::detail::make_gusbamp_simulator(std::vector<std::string> serials, const GusbampSimulation& simulation)
{
    return std::make_unique<GusbampSimulator>(std::move(serials), simulation);
}
