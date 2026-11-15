//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     tmsi_simulator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    A simulated TMSi Refa: a synthetic:// session digitised in the SDK's 32-bit channel format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsi_transport.h"

#include "../../acq_device.h"

#include <algorithm>
#include <cmath>
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

constexpr double kBaseRate = 2048.0;
constexpr std::size_t kBip = 4;
constexpr std::size_t kAux = 4;

/** The supported rate (the base rate divided by 1, 2, 4, 8 or 16) nearest @p requested. */
double supportedRate(double requested)
{
    double best = kBaseRate;
    for (int divider = 1; divider <= 16; divider *= 2) {
        const double rate = kBaseRate / divider;
        if (std::abs(std::log2(rate / requested)) < std::abs(std::log2(best / requested))) {
            best = rate;
        }
    }
    return best;
}

std::vector<TmsiChannel> refaChannels(std::size_t exg)
{
    std::vector<TmsiChannel> channels;
    channels.reserve(exg + kBip + kAux + 2);
    // 71.5 nV per bit on the inputs, as a Refa's 22-bit converters
    for (std::size_t i = 0; i < exg; ++i) {
        channels.push_back({std::format("ExG{}", i + 1), TmsiChannelType::Exg, TmsiUnit::Volt, 0.0715256, 0.0, -6});
    }
    for (std::size_t i = 0; i < kBip; ++i) {
        channels.push_back({std::format("Bip{}", i + 1), TmsiChannelType::Bip, TmsiUnit::Volt, 0.0715256, 0.0, -6});
    }
    for (std::size_t i = 0; i < kAux; ++i) {
        channels.push_back({std::format("Aux{}", i + 1), TmsiChannelType::Aux, TmsiUnit::Volt, 1.0, 100.0, -6});
    }
    channels.push_back({"Digi", TmsiChannelType::Dig, TmsiUnit::Bit, 1.0, 0.0, 0});
    channels.push_back({"Saw", TmsiChannelType::Saw, TmsiUnit::Unknown, 1.0, 0.0, 0});
    return channels;
}

class TmsiSimulator final : public TmsiTransport
{
public:
    explicit TmsiSimulator(const TmsiSimulation& simulation)
    : m_simulation(simulation)
    , m_channels(refaChannels(simulation.exg))
    {
    }

    ~TmsiSimulator() override
    {
        stop();
    }

    std::string description() const override
    {
        return "simulated Refa, seed " + std::to_string(m_simulation.seed);
    }

    std::vector<TmsiChannel> channels() const override
    {
        return m_channels;
    }

    AcqExpected<double> setup(const TmsiSetup& setup) override
    {
        m_rate = supportedRate(setup.rate);
        return m_rate;
    }

    AcqError start() override
    {
        // The session restarts at sample 0, as the amplifier's buffer does
        const auto block = std::max<std::size_t>(1, static_cast<std::size_t>(m_rate / 128.0));
        std::string uri = std::format("synthetic://?sfreq={}&stim=1&eeg={}&block={}&seed={}&realtime={}", m_rate,
                                      m_simulation.exg + kBip + kAux, block, m_simulation.seed, m_simulation.realtime ? 1 : 0);
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
        m_saw = 0;
        return {};
    }

    void stop() override
    {
        m_reader = AcqReader();
        m_session.close();
    }

    AcqExpected<std::size_t> read(std::span<std::int32_t> frames) override
    {
        const std::size_t width = m_channels.size();
        auto view = m_reader.acquire(1, std::chrono::nanoseconds(0), frames.size() / width);
        if (!view) {
            if (view.error() == AcqErrorCode::Timeout) {
                return std::size_t(0);
            }
            return view.error();
        }
        // The session's channels are the analog inputs, then the trigger code
        std::size_t samples = 0;
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t s = 0; s < part.samples; ++s, ++samples) {
                std::int32_t* frame = frames.data() + samples * width;
                for (std::size_t c = 0; c + 2 < width; ++c) {
                    float v = 0.0F;
                    std::memcpy(&v, part.at(c, s), sizeof(v));
                    frame[c] = encode(m_simulation.gain * v, m_channels[c]);
                }
                float code = 0.0F;
                std::memcpy(&code, part.at(width - 2, s), sizeof(code));
                frame[width - 2] = static_cast<std::int32_t>(code);
                frame[width - 1] = static_cast<std::int32_t>(m_saw++ & 0xFFFF);
            }
        }
        if (AcqError e = m_reader.release(*view); !e.ok()) {
            return e;
        }
        return samples;
    }

    AcqExpected<std::vector<double>> impedances(std::chrono::milliseconds /*timeout*/) override
    {
        // Electrodes of a fresh gel cap: a few kilo-ohms, the same for a seed
        std::mt19937_64 random(m_simulation.seed);
        std::uniform_real_distribution<double> ohms(1e3, 20e3);
        std::vector<double> values(m_simulation.exg);
        for (double& value : values) {
            value = ohms(random);
        }
        return values;
    }

private:
    static std::int32_t encode(double volts, const TmsiChannel& channel) noexcept
    {
        const double raw = (volts * std::pow(10.0, -channel.exponent) - channel.offset) / channel.gain;
        constexpr double kLow = std::numeric_limits<std::int32_t>::min() + 1.0; // the minimum flags an overflow
        constexpr double kHigh = std::numeric_limits<std::int32_t>::max();
        return static_cast<std::int32_t>(std::clamp(std::nearbyint(raw), kLow, kHigh));
    }

    TmsiSimulation m_simulation;
    std::vector<TmsiChannel> m_channels;
    double m_rate = kBaseRate;
    AcqDevice m_session;
    AcqReader m_reader;
    std::uint32_t m_saw = 0;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::unique_ptr<TmsiTransport> mne::detail::makeTmsiSimulator(const TmsiSimulation& simulation)
{
    return std::make_unique<TmsiSimulator>(simulation);
}
