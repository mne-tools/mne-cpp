//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     periodictask.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    PeriodicTask definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "periodictask.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <utility>

using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PeriodicTask::PeriodicTask(std::function<void()> tick)
: m_tick(std::move(tick))
, m_thread([this](const std::stop_token& stopToken) { run(stopToken); })
{
}

//=============================================================================================================

PeriodicTask::~PeriodicTask()
{
    m_thread.request_stop();
    m_thread.join();
}

//=============================================================================================================

void PeriodicTask::start(std::chrono::milliseconds interval)
{
    {
        std::lock_guard lock(m_mutex);
        m_interval = std::max(interval, std::chrono::milliseconds(1));
        m_running = true;
        ++m_generation;
    }
    m_wake.notify_all();
}

//=============================================================================================================

void PeriodicTask::stop()
{
    {
        std::lock_guard lock(m_mutex);
        m_running = false;
        ++m_generation;
    }
    m_wake.notify_all();
}

//=============================================================================================================

void PeriodicTask::setInterval(std::chrono::milliseconds interval)
{
    {
        std::lock_guard lock(m_mutex);
        m_interval = std::max(interval, std::chrono::milliseconds(1));
        ++m_generation;
    }
    m_wake.notify_all();
}

//=============================================================================================================

bool PeriodicTask::isRunning() const
{
    std::lock_guard lock(m_mutex);
    return m_running;
}

//=============================================================================================================

void PeriodicTask::run(const std::stop_token& stopToken)
{
    using Clock = std::chrono::steady_clock;

    std::unique_lock lock(m_mutex);
    while (!stopToken.stop_requested()) {
        if (!m_running) {
            m_wake.wait(lock, stopToken, [this] { return m_running; });
            continue;
        }

        const std::uint64_t generation = m_generation;
        Clock::time_point deadline = Clock::now() + m_interval;
        while (!stopToken.stop_requested() && generation == m_generation) {
            if (m_wake.wait_until(lock, stopToken, deadline, [&] { return generation != m_generation; })) {
                break;
            }
            if (stopToken.stop_requested()) {
                break;
            }
            lock.unlock();
            m_tick();
            lock.lock();

            // Keep the cadence; after an overrun restart it rather than firing a burst of catch-up ticks.
            deadline += m_interval;
            const Clock::time_point now = Clock::now();
            if (deadline < now) {
                deadline = now + m_interval;
            }
        }
    }
}
