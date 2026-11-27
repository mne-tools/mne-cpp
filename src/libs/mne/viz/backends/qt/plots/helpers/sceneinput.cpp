//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sceneinput.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the Qt to mne_scene input event translation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sceneinput.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMouseEvent>
#include <QWheelEvent>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <chrono>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;
using namespace mne::viz::qt;

namespace
{

mne::viz::Modifier toSceneModifiers(Qt::KeyboardModifiers modifiers)
{
    viz::Modifier result = viz::Modifier::None;
    if (modifiers & Qt::ShiftModifier)
        result = result | viz::Modifier::Shift;
    if (modifiers & Qt::ControlModifier)
        result = result | viz::Modifier::Ctrl;
    if (modifiers & Qt::AltModifier)
        result = result | viz::Modifier::Alt;
    if (modifiers & Qt::MetaModifier)
        result = result | viz::Modifier::Meta;
    return result;
}

// Qt's mouse buttons and their scene counterparts
constexpr std::array<std::pair<Qt::MouseButton, viz::MouseButton>, 3> kButtons = {{
    {Qt::LeftButton, viz::MouseButton::Left},
    {Qt::RightButton, viz::MouseButton::Right},
    {Qt::MiddleButton, viz::MouseButton::Middle},
}};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::int64_t mne::viz::qt::sceneTimestampMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

//=============================================================================================================

mne::viz::PointerEvent mne::viz::qt::toScenePointerEvent(const QMouseEvent& event, viz::PointerEvent::Type type)
{
    viz::PointerEvent pointer;
    pointer.type = type;
    pointer.x = event.position().x();
    pointer.y = event.position().y();
    for (const auto& [qtButton, button] : kButtons) {
        if (event.button() == qtButton) {
            pointer.button = button;
        }
        if (event.buttons() & qtButton) {
            pointer.buttons = pointer.buttons | viz::button_flag(button);
        }
    }
    pointer.modifiers = toSceneModifiers(event.modifiers());
    pointer.timestamp_ms = sceneTimestampMs();
    return pointer;
}

//=============================================================================================================

mne::viz::WheelEvent mne::viz::qt::toSceneWheelEvent(const QWheelEvent& event)
{
    viz::WheelEvent wheel;
    wheel.x = event.position().x();
    wheel.y = event.position().y();
    wheel.angle_delta_x = event.angleDelta().x();
    wheel.angle_delta_y = event.angleDelta().y();
    wheel.modifiers = toSceneModifiers(event.modifiers());
    wheel.timestamp_ms = sceneTimestampMs();
    return wheel;
}
