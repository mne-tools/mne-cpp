//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ftbuffer_simulator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November, 2026
 * @brief    A FieldTrip buffer that neuromag2ft fills from a simulated Neuromag acquisition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffer_transport.h"
#include "../neuromag/neuromag_fiff.h"
#include "../neuromag/neuromag_transport.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstring>
#include <deque>

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

using Clock = std::chrono::steady_clock;

constexpr std::chrono::milliseconds kSlice(20);
constexpr double kRingSeconds = 10.0; // samples older than this are gone, as in a real buffer

template<typename T>
void put(std::vector<std::byte>& out, T value)
{
    const auto* bytes = reinterpret_cast<const std::byte*>(&value);
    out.insert(out.end(), bytes, bytes + sizeof(T));
}

template<typename T>
T get(std::span<const std::byte> data, std::size_t offset)
{
    T value{};
    std::memcpy(&value, data.data() + offset, sizeof(T));
    return value;
}

class FtBufferSimulator final : public FtBufferTransport
{
public:
    explicit FtBufferSimulator(const FtBufferSimulation& simulation)
    : m_source(make_neuromag_simulator({.meg = simulation.meg,
                                        .eeg = simulation.eeg,
                                        .rate = simulation.rate,
                                        .buffer = simulation.block,
                                        .seed = simulation.seed,
                                        .realtime = simulation.realtime,
                                        .duration = simulation.duration,
                                        .gain = 1.0}))
    , m_ring(static_cast<std::size_t>(kRingSeconds * simulation.rate))
    {
    }

    std::string description() const override
    {
        return "simulated neuromag2ft";
    }

    AcqExpected<FtMessage> request(std::uint16_t command, std::span<const std::byte> payload, std::chrono::milliseconds timeout) override
    {
        if (!m_started) {
            if (AcqError e = m_source->start(); !e.ok()) {
                return e;
            }
            m_started = true;
        }
        switch (command) {
            case ft::kGetHdr:
                return header(timeout);
            case ft::kWaitDat:
                return wait(payload, timeout);
            case ft::kGetDat:
                return data(payload);
            default:
                return FtMessage{ft::kGetErr, {}}; // the relay puts no events
        }
    }

private:
    /** Takes the acquisition's next tag; @return false when none came within @p timeout. */
    AcqExpected<bool> pump(std::chrono::milliseconds timeout)
    {
        if (m_ended) {
            return AcqError(AcqErrorCode::EndOfStream);
        }
        auto tag = m_source->next(timeout);
        if (!tag) {
            if (tag.error() == AcqErrorCode::Timeout) {
                return false;
            }
            m_ended = tag.error() == AcqErrorCode::EndOfStream;
            return tag.error();
        }
        const NeuromagTag& t = **tag;
        if (t.kind != fiff::kDataBuffer) {
            if (m_header.empty()) {
                m_tags.push_back(t);
                if (t.kind == fiff::kBlockStart && t.as_int() == fiff::kBlockRawData) {
                    make_header();
                }
            }
            return true;
        }
        // The relay applies the calibration and puts floats, sample after sample
        const std::size_t width = m_scales.size();
        const std::size_t samples = t.data.size() / (width * sizeof(std::int32_t));
        for (std::size_t s = 0; s < samples; ++s) {
            for (std::size_t c = 0; c < width; ++c) {
                const auto raw = get<std::int32_t>(t.data, (s * width + c) * sizeof(std::int32_t));
                m_samples.push_back(static_cast<float>(m_scales[c] * raw));
            }
        }
        m_total += samples;
        while (m_samples.size() > m_ring * width) {
            m_samples.erase(m_samples.begin(), m_samples.begin() + static_cast<std::ptrdiff_t>(width));
        }
        return true;
    }

    void make_header()
    {
        const std::vector<NeuromagTag> meas_info = block_of(m_tags, fiff::kBlockMeasInfo);
        std::vector<std::byte> names;
        float sfreq = 0.0F;
        for (const NeuromagTag& tag : meas_info) {
            if (tag.kind == fiff::kSfreq) {
                sfreq = tag.as_float().value_or(0.0F);
            } else if (const auto ch = tag.kind == fiff::kChInfo ? NeuromagChannel::parse(tag.data) : std::nullopt) {
                m_scales.push_back(static_cast<double>(ch->cal) * static_cast<double>(ch->range));
                const auto* chars = reinterpret_cast<const std::byte*>(ch->name.c_str());
                names.insert(names.end(), chars, chars + ch->name.size() + 1);
            }
        }
        const std::vector<std::byte> fiff = tag_file(m_tags, false);
        std::vector<std::byte> chunks;
        using Chunk = std::pair<std::uint32_t, const std::vector<std::byte>*>;
        for (const auto& [type, data] : {Chunk{ft::kChunkChannelNames, &names}, Chunk{ft::kChunkNeuromagHeader, &fiff}}) {
            put(chunks, type);
            put(chunks, static_cast<std::uint32_t>(data->size()));
            chunks.insert(chunks.end(), data->begin(), data->end());
        }
        put(m_header, static_cast<std::uint32_t>(m_scales.size()));
        m_header_counts = m_header.size();
        put(m_header, std::uint32_t{0}); // nsamples, set on every answer
        put(m_header, std::uint32_t{0}); // nevents
        put(m_header, sfreq);
        put(m_header, ft::kTypeFloat32);
        put(m_header, static_cast<std::uint32_t>(chunks.size()));
        m_header.insert(m_header.end(), chunks.begin(), chunks.end());
    }

    AcqExpected<FtMessage> header(std::chrono::milliseconds timeout)
    {
        const auto deadline = Clock::now() + timeout;
        while (m_header.empty() && Clock::now() < deadline) {
            if (auto pumped = pump(kSlice); !pumped) {
                return pumped.error();
            }
        }
        if (m_header.empty()) {
            return FtMessage{ft::kGetErr, {}};
        }
        FtMessage answer{ft::kGetOk, m_header};
        const auto total = static_cast<std::uint32_t>(m_total);
        std::memcpy(answer.payload.data() + m_header_counts, &total, sizeof(total));
        return answer;
    }

    AcqExpected<FtMessage> wait(std::span<const std::byte> payload, std::chrono::milliseconds timeout)
    {
        if (payload.size() < 12) {
            return FtMessage{ft::kWaitErr, {}};
        }
        const auto threshold = get<std::uint32_t>(payload, 0);
        const auto deadline = Clock::now() + std::min(timeout, std::chrono::milliseconds(get<std::uint32_t>(payload, 8)));
        while ((m_header.empty() || m_total <= threshold) && Clock::now() < deadline) {
            if (auto pumped = pump(kSlice); !pumped) {
                if (pumped.error() != AcqErrorCode::EndOfStream || m_total <= threshold) {
                    return pumped.error();
                }
                break;
            }
        }
        FtMessage answer{ft::kWaitOk, {}};
        put(answer.payload, static_cast<std::uint32_t>(m_total));
        put(answer.payload, std::uint32_t{0});
        return answer;
    }

    AcqExpected<FtMessage> data(std::span<const std::byte> payload) const
    {
        const std::size_t width = m_scales.size();
        const std::size_t held = width == 0 ? 0 : m_samples.size() / width;
        if (payload.size() < 8 || width == 0) {
            return FtMessage{ft::kGetErr, {}};
        }
        const std::size_t begin = get<std::uint32_t>(payload, 0);
        const std::size_t end = get<std::uint32_t>(payload, 4);
        const std::size_t first = m_total - held;
        if (begin < first || end < begin || end >= m_total) {
            return FtMessage{ft::kGetErr, {}};
        }
        const std::size_t count = end - begin + 1;
        FtMessage answer{ft::kGetOk, {}};
        put(answer.payload, static_cast<std::uint32_t>(width));
        put(answer.payload, static_cast<std::uint32_t>(count));
        put(answer.payload, ft::kTypeFloat32);
        put(answer.payload, static_cast<std::uint32_t>(count * width * sizeof(float)));
        const auto from = m_samples.begin() + static_cast<std::ptrdiff_t>((begin - first) * width);
        for (auto it = from; it != from + static_cast<std::ptrdiff_t>(count * width); ++it) {
            put(answer.payload, *it);
        }
        return answer;
    }

    std::unique_ptr<NeuromagTransport> m_source;
    std::size_t m_ring;
    bool m_started = false;
    bool m_ended = false;
    std::vector<NeuromagTag> m_tags; // the measurement's tags before its data
    std::vector<double> m_scales;    // calibration times range per channel
    std::vector<std::byte> m_header; // headerdef and chunks
    std::size_t m_header_counts = 0; // offset of nsamples in m_header
    std::deque<float> m_samples;     // the ring, sample after sample
    std::size_t m_total = 0;         // samples put since the header
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::unique_ptr<FtBufferTransport> mne::detail::make_ftbuffer_simulator(const FtBufferSimulation& simulation)
{
    return std::make_unique<FtBufferSimulator>(simulation);
}
