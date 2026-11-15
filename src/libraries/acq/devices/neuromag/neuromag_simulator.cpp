//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     neuromag_simulator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    A simulated Neuromag acquisition server: a synthetic:// session as measurement info and data buffers.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag_transport.h"

#include "../../acq_device.h"
#include "../../detail/acq_fiff_codes.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>
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

constexpr int kCoilEeg = 1;
constexpr int kCoilMagnetometer = 3024; // FIFFV_COIL_VV_MAG_T3
constexpr float kMegCal = 1e-15F;       // T per bit
constexpr float kEegCal = 1e-8F;        // V per bit

template<typename T>
std::vector<std::byte> bytes_of(T value)
{
    std::vector<std::byte> data(sizeof(T));
    std::memcpy(data.data(), &value, sizeof(T));
    return data;
}

NeuromagTag tag(int kind, int type, std::vector<std::byte> data)
{
    return {kind, type, std::move(data)};
}

class NeuromagSimulator final : public NeuromagTransport
{
public:
    explicit NeuromagSimulator(const NeuromagSimulation& simulation)
    : m_simulation(simulation)
    {
        for (std::size_t i = 0; i < simulation.meg; ++i) {
            m_channels.push_back(channel(std::format("MEG {:04}", i + 1), fiff_kind::kMeg, kCoilMagnetometer, fiff_unit::kTesla, kMegCal));
        }
        for (std::size_t i = 0; i < simulation.eeg; ++i) {
            m_channels.push_back(channel(std::format("EEG {:03}", i + 1), fiff_kind::kEeg, kCoilEeg, fiff_unit::kVolt, kEegCal));
        }
        m_channels.push_back(channel("STI 014", fiff_kind::kStim, 0, fiff_unit::kNone, 1.0F));
    }

    ~NeuromagSimulator() override
    {
        stop();
    }

    std::string description() const override
    {
        return std::format("simulated Vectorview, seed {}", m_simulation.seed);
    }

    AcqError start() override
    {
        // The session restarts at sample 0, as a new measurement does
        std::string uri = std::format("synthetic://?sfreq={}&stim=1&meg={}&eeg={}&block={}&seed={}&realtime={}", m_simulation.rate,
                                      m_simulation.meg, m_simulation.eeg, m_simulation.buffer, m_simulation.seed, m_simulation.realtime ? 1 : 0);
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
        m_pending = measurement_start();
        m_next = 0;
        return {};
    }

    void stop() override
    {
        m_reader = AcqReader();
        m_session.close();
        m_pending.clear();
    }

    AcqExpected<const NeuromagTag*> next(std::chrono::milliseconds timeout) override
    {
        if (m_next < m_pending.size()) {
            return &m_pending[m_next++];
        }
        const std::size_t samples = m_simulation.buffer;
        auto view = m_reader.acquire(samples, timeout, samples);
        if (!view) {
            return view.error();
        }
        if (view->sampleCount() < samples) {
            // The server sends whole buffers only; the end of the session cuts the rest
            static_cast<void>(m_reader.release(*view));
            return AcqError(AcqErrorCode::EndOfStream);
        }
        // The session's channels are EEG, MEG, then the trigger code; the server's MEG, EEG, then STI 014
        const std::size_t meg = m_simulation.meg;
        const std::size_t eeg = m_simulation.eeg;
        const std::size_t width = m_channels.size();
        m_buffer.kind = fiff::kDataBuffer;
        m_buffer.type = fiff::kTypeInt;
        m_buffer.data.resize(samples * width * sizeof(std::int32_t));
        auto* values = reinterpret_cast<std::int32_t*>(m_buffer.data.data());
        std::size_t s = 0;
        for (std::size_t p = 0; p < view->partCount(); ++p) {
            const AcqBlockPart& part = view->part(p);
            for (std::size_t i = 0; i < part.samples; ++i, ++s) {
                std::int32_t* frame = values + s * width;
                for (std::size_t c = 0; c < width; ++c) {
                    const std::size_t source = c < meg ? eeg + c : c < meg + eeg ? c - meg
                                                                                 : meg + eeg;
                    float v = 0.0F;
                    std::memcpy(&v, part.at(source, i), sizeof(v));
                    const double physical = c + 1 < width ? m_simulation.gain * v : v;
                    frame[c] = encode(physical / static_cast<double>(m_channels[c].cal));
                }
            }
        }
        if (AcqError e = m_reader.release(*view); !e.ok()) {
            return e;
        }
        return &m_buffer;
    }

private:
    static NeuromagChannel channel(std::string name, int kind, int coil, int unit, float cal)
    {
        NeuromagChannel ch;
        ch.kind = kind;
        ch.coil_type = coil;
        ch.unit = unit;
        ch.cal = cal;
        ch.loc[11] = 1.0F; // ez
        ch.name = std::move(name);
        return ch;
    }

    static std::int32_t encode(double raw) noexcept
    {
        constexpr double kLow = std::numeric_limits<std::int32_t>::min();
        constexpr double kHigh = std::numeric_limits<std::int32_t>::max();
        return static_cast<std::int32_t>(std::clamp(std::nearbyint(raw), kLow, kHigh));
    }

    /** The tags a measurement starts with: the measurement info, then the raw data block. */
    std::vector<NeuromagTag> measurement_start()
    {
        std::vector<NeuromagTag> tags;
        tags.push_back(tag(fiff::kBlockStart, fiff::kTypeInt, bytes_of<std::int32_t>(fiff::kBlockMeasInfo)));
        tags.push_back(tag(fiff::kNchan, fiff::kTypeInt, bytes_of<std::int32_t>(static_cast<std::int32_t>(m_channels.size()))));
        tags.push_back(tag(fiff::kSfreq, fiff::kTypeFloat, bytes_of<float>(static_cast<float>(m_simulation.rate))));
        tags.push_back(tag(fiff::kLowpass, fiff::kTypeFloat, bytes_of<float>(static_cast<float>(m_simulation.rate / 3.0))));
        tags.push_back(tag(fiff::kHighpass, fiff::kTypeFloat, bytes_of<float>(0.1F)));
        for (std::size_t c = 0; c < m_channels.size(); ++c) {
            NeuromagChannel ch = m_channels[c];
            ch.scan_no = static_cast<std::int32_t>(c + 1);
            ch.log_no = static_cast<std::int32_t>(c + 1);
            tags.push_back(tag(fiff::kChInfo, fiff::kTypeChInfo, ch.serialize()));
        }
        tags.push_back(tag(fiff::kBlockEnd, fiff::kTypeInt, bytes_of<std::int32_t>(fiff::kBlockMeasInfo)));
        tags.push_back(tag(fiff::kBlockStart, fiff::kTypeInt, bytes_of<std::int32_t>(fiff::kBlockRawData)));
        return tags;
    }

    NeuromagSimulation m_simulation;
    std::vector<NeuromagChannel> m_channels;
    AcqDevice m_session;
    AcqReader m_reader;
    std::vector<NeuromagTag> m_pending; // tags before the data
    std::size_t m_next = 0;
    NeuromagTag m_buffer; // the last data buffer
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::unique_ptr<NeuromagTransport> mne::detail::make_neuromag_simulator(const NeuromagSimulation& simulation)
{
    return std::make_unique<NeuromagSimulator>(simulation);
}
