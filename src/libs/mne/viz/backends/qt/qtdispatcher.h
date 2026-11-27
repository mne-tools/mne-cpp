//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     qtdispatcher.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    viz::Dispatcher that posts tasks to the event loop of a QObject's thread.
 */

#ifndef VIZ_QT_QTDISPATCHER_H
#define VIZ_QT_QTDISPATCHER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/host/dispatcher.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMetaObject>
#include <QObject>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
/**
 * @param[in] context   Object whose thread runs the tasks; tasks still queued when it is destroyed are dropped.
 * @return A dispatcher that queues each task to the event loop of @p context's thread.
 * @since 3.0.0
 */
inline viz::Dispatcher queuedDispatcher(QObject* context)
{
    return [context](std::function<void()> task) {
        QMetaObject::invokeMethod(context, std::move(task), Qt::QueuedConnection);
    };
}

} // namespace mne::viz::qt

#endif // VIZ_QT_QTDISPATCHER_H
