//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     dispatcher.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Dispatcher hands work from background threads to the thread that owns a view.
 *
 * Models of mne_viz load and stream on background threads but change their state and report to their owner on the
 * owner's thread. The host provides that hand-over: the Qt backend posts the task to a QObject's event loop, an
 * SDL host to its main loop, a test runs it at once.
 */

#ifndef VIZ_DISPATCHER_H
#define VIZ_DISPATCHER_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * Runs a task later on the owner's thread. It may be called from any thread; tasks run in the order they were
 * handed over.
 *
 * @since 3.0.0
 */
using Dispatcher = std::function<void(std::function<void()> task)>;

//=============================================================================================================
/**
 * @return A dispatcher that runs every task at once on the calling thread, for owners without an event loop.
 * @since 3.0.0
 */
inline Dispatcher immediate_dispatcher()
{
    return [](std::function<void()> task) {
        std::move(task)();
    };
}

} // namespace mne::viz

#endif // VIZ_DISPATCHER_H
