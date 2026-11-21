//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     babymeg_simulator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November, 2026
 * @brief    A BabyMEG server that sends a synthetic:// session's MEG and trigger lines.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_transport.h"
#include "../../acq_device.h"
#include "../../detail/acq_fiff_codes.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <bit>
#include <cmath>
#include <cstring>
#include <format>
#include <numbers>

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

constexpr int kCoilInner = 7002;     // FIFFV_COIL_BABY_MAG
constexpr int kCoilOuter = 7003;     // FIFFV_COIL_BABY_REF_MAG
constexpr float kMegCal = 1e-13F;    // tesla per unit at gain 1
constexpr float kMegGain = 0.5F;     // the SQUID electronics' gain
constexpr float kTriggerHigh = 5.0F; // an active trigger line, in volts
constexpr std::size_t kTriggerLines = 8;

/** @return The description of the simulated system: inner and outer magnetometers on spheres, trigger lines. */
BabyMegInfo simulated_system(const BabyMegSimulation& simulation)
{
    BabyMegInfo info;
    info.rate = simulation.rate;
    info.samples = simulation.samples;
    const std::size_t sensors = simulation.meg + simulation.ref;
    for (std::size_t i = 0; i < sensors; ++i) {
        const bool inner = i < simulation.meg;
        const std::size_t n = inner ? simulation.meg : simulation.ref;
        const std::size_t k = inner ? i : i - simulation.meg;
        // A Fibonacci lattice on the upper half sphere, normals pointing outward
        const double z = 1.0 - (static_cast<double>(k) + 0.5) / static_cast<double>(n);
        const double r = std::sqrt(1.0 - z * z);
        const double phi = static_cast<double>(k) * std::numbers::pi * (3.0 - std::sqrt(5.0));
        const std::array<double, 3> normal = {r * std::cos(phi), r * std::sin(phi), z};
        const double radius = inner ? 0.075 : 0.095;
        BabyMegChannel& channel = info.channels.emplace_back();
        channel.name = std::format("MEG{:03}", i + 1);
        channel.coil_type = inner ? kCoilInner : kCoilOuter;
        channel.cal = kMegCal;
        channel.gain = kMegGain;
        for (std::size_t c = 0; c < 3; ++c) {
            channel.loc[c] = static_cast<float>(radius * normal[c]);
            channel.loc[9 + c] = static_cast<float>(normal[c]);
        }
        channel.loc[3] = static_cast<float>(-std::sin(phi)); // ex, tangential
        channel.loc[4] = static_cast<float>(std::cos(phi));
        channel.loc[6] = static_cast<float>(normal[1] * channel.loc[5] - normal[2] * channel.loc[4]); // ey = ez x ex
        channel.loc[7] = static_cast<float>(normal[2] * channel.loc[3] - normal[0] * channel.loc[5]);
        channel.loc[8] = static_cast<float>(normal[0] * channel.loc[4] - normal[1] * channel.loc[3]);
    }
    for (std::size_t t = 0; t < kTriggerLines; ++t) {
        BabyMegChannel& channel = info.channels.emplace_back();
        channel.name = std::format("TRG{:03}", t + 1);
        channel.coil_type = fiff_kind::kStim;
    }
    BabyMegChannel& digital = info.channels.emplace_back();
    digital.name = "DTRG01";
    digital.coil_type = fiff_kind::kStim;
    return info;
}

void put_big(std::byte* out, float value)
{
    const auto bits = std::bit_cast<std::uint32_t>(value);
    for (std::size_t i = 0; i < 4; ++i) {
        out[i] = static_cast<std::byte>((bits >> (8 * (3 - i))) & 0xFFU);
    }
}

class BabyMegSimulator final : public BabyMegTransport
{
public:
    explicit BabyMegSimulator(const BabyMegSimulation& simulation)
    : m_simulation(simulation)
    , m_info(simulated_system(simulation))
    {
    }

    std::string description() const override
    {
        return std::format("simulated BabyMEG server, seed {}", m_simulation.seed);
    }

    AcqExpected<std::string> info(std::chrono::milliseconds /*timeout*/) override
    {
        return babymeg_info_text(m_info);
    }

    AcqError start() override
    {
        std::string uri = std::format("synthetic://?sfreq={}&meg={}&eeg=0&stim=1&block={}&seed={}&realtime={}", m_simulation.rate,
                                      m_simulation.meg + m_simulation.ref, m_simulation.samples, m_simulation.seed, m_simulation.realtime ? 1 : 0);
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

    AcqExpected<std::span<const std::byte>> next(std::chrono::milliseconds timeout) override
    {
        const std::size_t samples = m_simulation.samples;
        const std::size_t sensors = m_simulation.meg + m_simulation.ref;
        const std::size_t width = m_info.channels.size();
        auto view = m_reader.acquire(samples, timeout, samples);
        if (!view) {
            return view.error();
        }
        if (view->sampleCount() < samples) {
            static_cast<void>(m_reader.release(*view)); // whole packages only; the end of the session cuts the rest
            return AcqError(AcqErrorCode::EndOfStream);
        }
        m_body.assign(1 + samples * width * sizeof(float), std::byte{0});
        m_body[0] = std::byte{'4'};
        std::byte* out = m_body.data() + 1;
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t s = 0; s < part.samples; ++s) {
                for (std::size_t c = 0; c < sensors; ++c) {
                    float tesla = 0.0F;
                    std::memcpy(&tesla, part.at(c, s), sizeof(tesla));
                    put_big(out, tesla * kMegGain / kMegCal);
                    out += sizeof(float);
                }
                float code = 0.0F;
                std::memcpy(&code, part.at(sensors, s), sizeof(code));
                const auto word = static_cast<std::uint32_t>(std::lround(code));
                for (std::size_t t = 0; t < kTriggerLines; ++t) {
                    put_big(out, ((word >> t) & 1U) != 0 ? kTriggerHigh : 0.0F);
                    out += sizeof(float);
                }
                out += sizeof(float); // DTRG01 is the client's to fill
            }
        }
        if (AcqError e = m_reader.release(*view); !e.ok()) {
            return e;
        }
        return std::span<const std::byte>(m_body);
    }

    void stop() override
    {
        m_reader = AcqReader();
        m_session.close();
    }

private:
    BabyMegSimulation m_simulation;
    BabyMegInfo m_info;
    AcqDevice m_session;
    AcqReader m_reader;
    std::vector<std::byte> m_body;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::unique_ptr<BabyMegTransport> mne::detail::make_babymeg_simulator(const BabyMegSimulation& simulation)
{
    return std::make_unique<BabyMegSimulator>(simulation);
}
