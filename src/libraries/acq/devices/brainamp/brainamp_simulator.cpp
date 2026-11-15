//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainamp_simulator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    A simulated BrainAmp adapter: a synthetic:// session at 5 kHz, digitised like the amplifier.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainamp_transport.h"

#include "../../acq_device.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>

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

class BrainAmpSimulator final : public BrainAmpTransport
{
public:
    explicit BrainAmpSimulator(const BrainAmpSimulation& simulation)
    : m_simulation(simulation)
    {
    }

    ~BrainAmpSimulator() override
    {
        stop();
    }

    std::string description() const override
    {
        return "simulated adapter, seed " + std::to_string(m_simulation.seed);
    }

    std::array<BrainAmpAmplifier, 4> amplifiers() override
    {
        std::array<BrainAmpAmplifier, 4> slots{};
        for (std::size_t i = 0; i < std::min<std::size_t>(m_simulation.amplifiers, slots.size()); ++i) {
            slots[i] = BrainAmpAmplifier::Standard;
        }
        return slots;
    }

    AcqError setup(const BrainAmpSetup& setup) override
    {
        m_setup = setup;
        m_block.assign(setup.points * (setup.channels.size() + 1), 0);
        return {};
    }

    AcqError start() override
    {
        // The session restarts at sample 0, as the amplifier's buffer does
        std::string uri = "synthetic://?sfreq=5000&stim=1&eeg=" + std::to_string(m_setup.channels.size()) +
            "&block=" + std::to_string(m_setup.points) + "&seed=" + std::to_string(m_simulation.seed) +
            "&realtime=" + (m_simulation.realtime ? "1" : "0");
        if (m_simulation.duration > 0.0) {
            uri += "&duration=" + std::to_string(m_simulation.duration);
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
        m_filled = 0;
        return {};
    }

    void stop() override
    {
        m_reader = AcqReader();
        m_session.close();
    }

    AcqExpected<bool> read(std::span<std::int16_t> block) override
    {
        const std::size_t width = m_setup.channels.size() + 1;
        const double bitsPerVolt = m_simulation.gain / brainAmpVoltsPerBit(m_setup.resolution);
        while (m_filled < m_setup.points) {
            auto view = m_reader.acquire(1, std::chrono::nanoseconds(0), m_setup.points - m_filled);
            if (!view) {
                if (view.error() == AcqErrorCode::Timeout) {
                    return false;
                }
                return view.error();
            }
            for (std::size_t p = 0; p < view->partCount(); ++p) {
                const AcqBlockPart& part = view->part(p);
                for (std::size_t s = 0; s < part.samples; ++s, ++m_filled) {
                    std::int16_t* frame = m_block.data() + m_filled * width;
                    for (std::size_t c = 0; c < width; ++c) {
                        float v = 0.0F;
                        std::memcpy(&v, part.at(c, s), sizeof(v));
                        frame[c] = c + 1 < width ? quantize(v * bitsPerVolt) : static_cast<std::int16_t>(v);
                    }
                }
            }
            if (AcqError e = m_reader.release(*view); !e.ok()) {
                return e;
            }
        }
        std::copy(m_block.begin(), m_block.end(), block.begin());
        m_filled = 0;
        return true;
    }

private:
    static std::int16_t quantize(double bits) noexcept
    {
        constexpr double kLow = std::numeric_limits<std::int16_t>::min();
        constexpr double kHigh = std::numeric_limits<std::int16_t>::max();
        return static_cast<std::int16_t>(std::clamp(std::nearbyint(bits), kLow, kHigh));
    }

    BrainAmpSimulation m_simulation;
    BrainAmpSetup m_setup;
    AcqDevice m_session;
    AcqReader m_reader;
    std::vector<std::int16_t> m_block; // the block being filled
    std::size_t m_filled = 0;          // points of m_block filled
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::unique_ptr<BrainAmpTransport> mne::detail::makeBrainAmpSimulator(const BrainAmpSimulation& simulation)
{
    return std::make_unique<BrainAmpSimulator>(simulation);
}
