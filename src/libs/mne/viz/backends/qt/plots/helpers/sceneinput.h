//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sceneinput.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Translation of Qt mouse and wheel events to the toolkit-neutral input events of mne_scene.
 *
 * The interactive scene views (viz::TraceView, viz::BrainViewInteraction) take viz::PointerEvent and
 * viz::WheelEvent; their Qt hosts translate QMouseEvent and QWheelEvent with these functions.
 */

#ifndef DISP_SCENEINPUT_H
#define DISP_SCENEINPUT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"
#include <mne/viz/views/input_event.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>

class QMouseEvent;
class QWheelEvent;

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

/**
 * @return Monotonic time in milliseconds, the clock of the timestamps of the translated events.
 *
 * @since 3.0.0
 */
VIZQTSHARED_EXPORT std::int64_t sceneTimestampMs();

/**
 * @param[in] event   Mouse event in logical coordinates of the view.
 * @param[in] type    Kind of pointer event.
 *
 * @return The position, buttons and modifiers of @p event, stamped with sceneTimestampMs().
 *
 * @since 3.0.0
 */
VIZQTSHARED_EXPORT viz::PointerEvent toScenePointerEvent(const QMouseEvent& event, viz::PointerEvent::Type type);

/**
 * @param[in] event   Wheel event in logical coordinates of the view.
 *
 * @return The position, angle deltas and modifiers of @p event, stamped with sceneTimestampMs().
 *
 * @since 3.0.0
 */
VIZQTSHARED_EXPORT viz::WheelEvent toSceneWheelEvent(const QWheelEvent& event);

} // namespace mne::viz::qt

#endif // DISP_SCENEINPUT_H
