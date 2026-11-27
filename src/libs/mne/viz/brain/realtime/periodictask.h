//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     periodictask.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    PeriodicTask: runs a callback at a fixed cadence on a dedicated std::jthread.
 *
 * Replaces the QTimer-on-GUI-thread + QThread-worker pair used by the real-time
 * streaming controllers. The callback runs on the task's own thread; consumers
 * that live on the GUI thread receive its results through queued signals.
 */

#ifndef VIZ_PERIODICTASK_H
#define VIZ_PERIODICTASK_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <stop_token>
#include <thread>

namespace mne::viz
{

//=============================================================================================================
/**
 * Calls a tick function every @c interval while running. The first tick fires one interval after
 * start(). Ticks never overlap and do not queue up: a tick that overruns pushes the next one back.
 * stop() does not interrupt a tick that is already running.
 */
class VIZSHARED_EXPORT PeriodicTask
{
public:
    explicit PeriodicTask(std::function<void()> tick);

    ~PeriodicTask();

    PeriodicTask(const PeriodicTask&) = delete;
    PeriodicTask& operator=(const PeriodicTask&) = delete;

    void start(std::chrono::milliseconds interval);

    void stop();

    void setInterval(std::chrono::milliseconds interval);

    bool isRunning() const;

private:
    void run(const std::stop_token& stopToken);

    std::function<void()> m_tick;
    mutable std::mutex m_mutex;
    std::condition_variable_any m_wake;
    bool m_running = false;
    std::chrono::milliseconds m_interval{17};
    std::uint64_t m_generation = 0; /**< Bumped on every start/stop/interval change to restart the wait. */
    std::jthread m_thread;          /**< Declared last: joined before the members above are destroyed. */
};

} // namespace mne::viz

#endif // VIZ_PERIODICTASK_H
