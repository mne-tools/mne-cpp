//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     eego_simulator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    A simulated eego amplifier: a synthetic:// session in the SDK's sample format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eego_transport.h"
#include "../../acq_device.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

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

class EegoSimulator final : public EegoTransport
{
public:
    explicit EegoSimulator(const EegoSimulation& simulation)
    : m_simulation(simulation)
    {
        int index = 0;
        for (std::size_t i = 0; i < simulation.reference; ++i) {
            m_channels.push_back({index++, EegoChannelType::Reference});
        }
        for (std::size_t i = 0; i < simulation.bipolar; ++i) {
            m_channels.push_back({index++, EegoChannelType::Bipolar});
        }
        m_channels.push_back({index++, EegoChannelType::Trigger});
        m_channels.push_back({index, EegoChannelType::SampleCounter});
    }

    ~EegoSimulator() override
    {
        stop();
    }

    std::string description() const override
    {
        return std::format("simulated EE{}, seed {}", m_simulation.reference + m_simulation.bipolar, m_simulation.seed);
    }

    std::vector<EegoChannel> channels() const override
    {
        return m_channels;
    }

    std::vector<int> rates() const override
    {
        return {500, 512, 1000, 1024, 2000, 2048, 4000, 4096, 8000, 8192, 16000, 16384};
    }

    std::vector<double> reference_ranges() const override
    {
        return {1.0, 0.75, 0.15};
    }

    std::vector<double> bipolar_ranges() const override
    {
        return {4.0, 1.5, 0.7, 0.35};
    }

    AcqError start(const EegoSetup& setup) override
    {
        // The session restarts at sample 0, as the amplifier's counter does
        const auto block = std::max(1, setup.rate / 100);
        std::string uri = std::format("synthetic://?sfreq={}&stim=1&eeg={}&block={}&seed={}&realtime={}", setup.rate,
                                      m_simulation.reference + m_simulation.bipolar, block, m_simulation.seed, m_simulation.realtime ? 1 : 0);
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
        m_samples.resize(static_cast<std::size_t>(4 * block) * m_channels.size());
        m_counter = 0;
        return {};
    }

    void stop() override
    {
        m_reader = AcqReader();
        m_session.close();
    }

    AcqExpected<std::span<const double>> read() override
    {
        const std::size_t width = m_channels.size();
        auto view = m_reader.acquire(1, std::chrono::nanoseconds(0), m_samples.size() / width);
        if (!view) {
            if (view.error() == AcqErrorCode::Timeout) {
                return std::span<const double>();
            }
            return view.error();
        }
        // The session's channels are the inputs, then the trigger code
        const std::size_t inputs = width - 2;
        std::size_t samples = 0;
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t s = 0; s < part.samples; ++s, ++samples) {
                double* frame = m_samples.data() + samples * width;
                for (std::size_t c = 0; c <= inputs; ++c) {
                    float v = 0.0F;
                    std::memcpy(&v, part.at(c, s), sizeof(v));
                    frame[c] = c < inputs ? m_simulation.gain * v : v;
                }
                frame[inputs + 1] = static_cast<double>(m_counter++);
            }
        }
        if (AcqError e = m_reader.release(*view); !e.ok()) {
            return e;
        }
        return std::span<const double>(m_samples.data(), samples * width);
    }

    AcqExpected<std::vector<double>> impedances(std::chrono::milliseconds /*timeout*/) override
    {
        // Electrodes of a fresh gel cap: a few kilo-ohms, the same for a seed
        std::mt19937_64 random(m_simulation.seed);
        std::uniform_real_distribution<double> ohms(1e3, 20e3);
        std::vector<double> values(m_simulation.reference);
        for (double& value : values) {
            value = ohms(random);
        }
        return values;
    }

private:
    EegoSimulation m_simulation;
    std::vector<EegoChannel> m_channels;
    AcqDevice m_session;
    AcqReader m_reader;
    std::vector<double> m_samples; // one read of all stream channels
    std::uint64_t m_counter = 0;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::unique_ptr<EegoTransport> mne::detail::make_eego_simulator(const EegoSimulation& simulation)
{
    return std::make_unique<EegoSimulator>(simulation);
}
