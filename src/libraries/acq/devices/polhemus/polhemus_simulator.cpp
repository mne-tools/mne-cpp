//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     polhemus_simulator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May, 2026
 * @brief    A simulated Fastrak: the stylus sweeps a 10 cm sphere, the tracker circles inside it.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "polhemus_transport.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>
#include <numbers>
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

using Clock = std::chrono::steady_clock;

constexpr double kRadius = 0.10;     // metres, a head
constexpr int kRecordsPerLap = 60;   // updates of one station per sweep
constexpr std::uint64_t kBurst = 64; // records generated per read at most

class PolhemusSimulator final : public PolhemusTransport
{
public:
    explicit PolhemusSimulator(const PolhemusSimulation& simulation)
    : m_simulation(simulation)
    {
    }

    std::string description() const override
    {
        return "simulated Fastrak";
    }

    AcqError write(std::string_view command) override
    {
        while (!command.empty()) {
            const std::size_t end = std::min(command.find('\r'), command.size());
            const std::string_view one = command.substr(0, end);
            if (one == "C") {
                m_streaming = true;
                m_epoch = Clock::now() - std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(static_cast<double>(m_record) / m_simulation.rate));
            } else if (one == "P") {
                m_streaming = false;
            }
            command.remove_prefix(std::min(end + 1, command.size()));
        }
        return {};
    }

    AcqExpected<std::size_t> read(std::span<char> buffer, std::chrono::milliseconds timeout) override
    {
        const auto deadline = Clock::now() + timeout;
        while (true) {
            if (!m_pending.empty()) {
                const std::size_t n = std::min(buffer.size(), m_pending.size());
                std::memcpy(buffer.data(), m_pending.data(), n);
                m_pending.erase(0, n);
                return n;
            }
            if (m_simulation.duration > 0.0 && static_cast<double>(m_record) >= m_simulation.duration * m_simulation.rate) {
                return AcqError(AcqErrorCode::EndOfStream);
            }
            if (m_streaming) {
                const std::uint64_t due = m_simulation.realtime
                    ? static_cast<std::uint64_t>(std::chrono::duration<double>(Clock::now() - m_epoch).count() * m_simulation.rate)
                    : m_record + kBurst;
                if (due > m_record) {
                    std::uint64_t end = std::min(due, m_record + kBurst);
                    if (m_simulation.duration > 0.0) {
                        end = std::min(end, static_cast<std::uint64_t>(std::ceil(m_simulation.duration * m_simulation.rate)));
                    }
                    generate(end);
                    continue;
                }
                const auto next = m_epoch + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(static_cast<double>(m_record + 1) / m_simulation.rate));
                if (next < deadline) {
                    std::this_thread::sleep_until(next);
                    continue;
                }
            }
            std::this_thread::sleep_until(deadline);
            return AcqError(AcqErrorCode::Timeout);
        }
    }

private:
    /** Appends the records up to @p end, or up to a button press, which stops the continuous output. */
    void generate(std::uint64_t end)
    {
        const std::size_t stations = m_simulation.stations.size();
        const double per_station = m_simulation.rate / static_cast<double>(stations);
        const double scale = m_simulation.units == FastrakUnits::Inches ? 1.0 / 0.0254 : 100.0;
        while (m_record < end) {
            const int station = m_simulation.stations[m_record % stations];
            const std::uint64_t update = m_record / stations;
            ++m_record;
            const double phase = 2.0 * std::numbers::pi * static_cast<double>((update + m_simulation.seed) % kRecordsPerLap) / kRecordsPerLap;
            const double elevation = 0.3 * std::sin(3.0 * phase);
            double x = 0.05 * station;
            double y = 0.0;
            double z = 0.0;
            double azimuth = 0.0;
            if (station == 1) {
                x = kRadius * std::cos(phase + 0.3);
                y = kRadius * std::sin(phase + 0.3);
                z = kRadius * elevation + 0.02;
                azimuth = phase * 180.0 / std::numbers::pi;
                if (azimuth > 180.0) {
                    azimuth -= 360.0;
                }
            } else if (station == 2) {
                x = kRadius * std::cos(phase);
                y = kRadius * std::sin(phase);
                z = kRadius * elevation;
            }
            m_pending += std::format("0{:1d} {:9.4f}{:9.4f}{:9.4f}{:9.3f}{:9.3f}{:9.3f}\r\n", station, x * scale, y * scale, z * scale, azimuth, 0.0, 0.0);

            if (m_simulation.button > 0.0 && m_record % stations == 0) {
                const auto press = static_cast<std::uint64_t>(m_simulation.button * per_station);
                if (press > 0 && (update + 1) % press == 0) {
                    m_streaming = false; // the stylus button pauses the output until the next "C"
                    return;
                }
            }
        }
    }

    PolhemusSimulation m_simulation;
    bool m_streaming = false;
    std::uint64_t m_record = 0;
    Clock::time_point m_epoch = Clock::now();
    std::string m_pending;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::unique_ptr<PolhemusTransport> mne::detail::make_polhemus_simulator(const PolhemusSimulation& simulation)
{
    return std::make_unique<PolhemusSimulator>(simulation);
}
