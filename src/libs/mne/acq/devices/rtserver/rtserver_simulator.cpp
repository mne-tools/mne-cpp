//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtserver_simulator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November, 2026
 * @brief    An mne_rt_server whose connector replays a simulated Neuromag recording.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserver_transport.h"
#include "../neuromag/neuromag_transport.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <charconv>
#include <cstring>
#include <deque>
#include <format>
#include <mutex>
#include <thread>

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

constexpr std::int32_t kId = 1; // the first client of the server
constexpr std::size_t kDefaultBuffer = 100;

NeuromagTag int_tag(int kind, std::int32_t value)
{
    NeuromagTag tag{kind, fiff::kTypeInt, std::vector<std::byte>(sizeof(value))};
    std::memcpy(tag.data.data(), &value, sizeof(value));
    return tag;
}

class RtServerSimulator final : public RtServerTransport
{
public:
    explicit RtServerSimulator(const RtServerSimulation& simulation)
    : m_simulation(simulation)
    {
    }

    std::string description() const override
    {
        return "simulated mne_rt_server";
    }

    AcqExpected<std::string> command(std::string_view line, std::chrono::milliseconds /*timeout*/) override
    {
        const std::lock_guard lock(m_mutex); // commands come from another thread than the reader
        const std::size_t space = line.find(' ');
        const std::string_view name = line.substr(0, space);
        const std::string_view argument = space == std::string_view::npos ? std::string_view() : line.substr(space + 1);
        std::int64_t value = -1;
        std::from_chars(argument.data(), argument.data() + argument.size(), value);
        if (name == "bufsize") {
            if (value < 1 || m_source) {
                return std::string("Buffer size not set\r\n");
            }
            m_buffer = static_cast<std::size_t>(value);
            return std::format("Buffer size set to {}\r\n", value);
        }
        if (name == "selcon") {
            return std::string("  *  (1) Fiff File Simulator\r\n");
        }
        if (name == "measinfo" && value == kId) {
            if (AcqError e = load(); !e.ok()) {
                return e;
            }
            m_queue.push_back(int_tag(fiff::kBlockStart, fiff::kBlockMeas));
            m_queue.insert(m_queue.end(), m_meas_info.begin(), m_meas_info.end());
            return std::format("Sending measurement info to FiffStreamClient {}\r\n", kId);
        }
        if (name == "start" && value == kId && !m_meas_info.empty()) {
            m_queue.push_back(int_tag(fiff::kBlockStart, fiff::kBlockRawData));
            m_streaming = true;
            return std::format("FiffStreamClient {} starts.\r\n", kId);
        }
        if ((name == "stop" && value == kId) || name == "stop-all") {
            if (m_streaming) {
                m_queue.push_back(int_tag(fiff::kBlockEnd, fiff::kBlockRawData));
                m_streaming = false;
            }
            return std::string("Stopped.\r\n");
        }
        return std::format("{} is no command\r\n", name);
    }

    AcqError send(std::int32_t command, std::string_view /*text*/) override
    {
        const std::lock_guard lock(m_mutex);
        if (command == rt::kGetClientId) {
            m_queue.push_back(int_tag(rt::kClientId, kId));
        }
        return {};
    }

    AcqExpected<const NeuromagTag*> next(std::chrono::milliseconds timeout) override
    {
        std::unique_lock lock(m_mutex);
        if (!m_queue.empty()) {
            m_tag = std::move(m_queue.front());
            m_queue.pop_front();
            return &m_tag;
        }
        if (!m_streaming) {
            lock.unlock();
            std::this_thread::sleep_for(timeout);
            return AcqError(AcqErrorCode::Timeout);
        }
        auto tag = m_source->next(timeout);
        if (!tag) {
            return tag.error();
        }
        const NeuromagTag& t = **tag;
        if (t.kind != fiff::kDataBuffer) {
            return AcqError(AcqErrorCode::Timeout);
        }
        // The connector sends the buffers calibrated, as floats
        const std::size_t count = t.data.size() / sizeof(std::int32_t);
        m_tag = {fiff::kDataBuffer, fiff::kTypeFloat, std::vector<std::byte>(count * sizeof(float))};
        for (std::size_t i = 0; i < count; ++i) {
            std::int32_t raw = 0;
            std::memcpy(&raw, t.data.data() + i * sizeof(raw), sizeof(raw));
            const auto value = static_cast<float>(m_scales[i % m_scales.size()] * raw);
            std::memcpy(m_tag.data.data() + i * sizeof(value), &value, sizeof(value));
        }
        return &m_tag;
    }

private:
    /** Starts the replay and takes its measurement info. */
    AcqError load()
    {
        if (m_source) {
            return {};
        }
        m_source = make_neuromag_simulator({.meg = m_simulation.meg,
                                            .eeg = m_simulation.eeg,
                                            .rate = m_simulation.rate,
                                            .buffer = m_buffer,
                                            .seed = m_simulation.seed,
                                            .realtime = m_simulation.realtime,
                                            .duration = m_simulation.duration,
                                            .gain = 1.0});
        if (AcqError e = m_source->start(); !e.ok()) {
            return e;
        }
        std::vector<NeuromagTag> tags;
        while (true) {
            auto tag = m_source->next(std::chrono::milliseconds(1000));
            if (!tag) {
                return tag.error();
            }
            tags.push_back(**tag);
            if (tags.back().kind == fiff::kBlockStart && tags.back().as_int() == fiff::kBlockRawData) {
                break;
            }
        }
        m_meas_info = block_of(tags, fiff::kBlockMeasInfo);
        for (const NeuromagTag& tag : m_meas_info) {
            if (const auto ch = tag.kind == fiff::kChInfo ? NeuromagChannel::parse(tag.data) : std::nullopt) {
                m_scales.push_back(static_cast<double>(ch->cal) * static_cast<double>(ch->range));
            }
        }
        return {};
    }

    RtServerSimulation m_simulation;
    std::size_t m_buffer = kDefaultBuffer;
    std::unique_ptr<NeuromagTransport> m_source;
    std::vector<NeuromagTag> m_meas_info;
    std::vector<double> m_scales; // calibration times range per channel
    std::deque<NeuromagTag> m_queue;
    NeuromagTag m_tag;
    bool m_streaming = false;
    std::mutex m_mutex;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::unique_ptr<RtServerTransport> mne::detail::make_rtserver_simulator(const RtServerSimulation& simulation)
{
    return std::make_unique<RtServerSimulator>(simulation);
}
