//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     input_event.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Toolkit-neutral pointer and wheel events for the interactive scene views.
 */

#ifndef SCENE_INPUT_EVENT_H
#define SCENE_INPUT_EVENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scene_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

/**
 * @brief Mouse button of a pointer event.
 *
 * @since 3.0.0
 */
enum class MouseButton
{
    None,  /**< No button, e.g. a hover move. */
    Left,  /**< Primary button. */
    Right, /**< Secondary button. */
    Middle /**< Middle button or wheel click. */
};

/**
 * @brief Keyboard modifiers held during an input event; combine with |.
 *
 * Ctrl is the modifier a toolkit reports as "control" (on macOS Qt maps the Command key to it).
 *
 * @since 3.0.0
 */
enum class Modifier : unsigned
{
    None = 0x0,  /**< No modifier. */
    Shift = 0x1, /**< Shift key. */
    Ctrl = 0x2,  /**< Control key. */
    Alt = 0x4,   /**< Alt (Option) key. */
    Meta = 0x8   /**< Meta key. */
};

constexpr Modifier operator|(Modifier a, Modifier b)
{
    return static_cast<Modifier>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

constexpr bool has_flag(Modifier modifiers, Modifier flag)
{
    return (static_cast<unsigned>(modifiers) & static_cast<unsigned>(flag)) == static_cast<unsigned>(flag) &&
        flag != Modifier::None;
}

/**
 * @brief A mouse or touch-pad pointer event in logical (device-independent) pixels of the view.
 *
 * @since 3.0.0
 */
struct PointerEvent
{
    /** Kind of pointer event. */
    enum class Type
    {
        Press,      /**< A button went down. */
        Move,       /**< The pointer moved, with or without a held button. */
        Release,    /**< A button went up. */
        DoubleClick /**< A button was clicked twice. */
    };

    Type type = Type::Move;                 /**< Kind of event. */
    double x = 0.0;                         /**< Horizontal position, growing to the right. */
    double y = 0.0;                         /**< Vertical position, growing downwards. */
    MouseButton button = MouseButton::None; /**< Button that caused a press, release or double click; None for moves. */
    Modifier modifiers = Modifier::None;    /**< Modifiers held. */
    std::int64_t timestamp_ms = 0;          /**< Monotonic time in milliseconds, used for drag velocities. */
};

/**
 * @brief A wheel or two-finger scroll event.
 *
 * The angle deltas follow the convention of most toolkits: one notch of a mouse wheel is 120 (eighths of a
 * degree), positive y scrolls up (away from the user) and positive x scrolls left.
 *
 * @since 3.0.0
 */
struct WheelEvent
{
    double x = 0.0;                      /**< Horizontal pointer position. */
    double y = 0.0;                      /**< Vertical pointer position. */
    double angle_delta_x = 0.0;          /**< Horizontal rotation in eighths of a degree. */
    double angle_delta_y = 0.0;          /**< Vertical rotation in eighths of a degree. */
    Modifier modifiers = Modifier::None; /**< Modifiers held. */
    std::int64_t timestamp_ms = 0;       /**< Monotonic time in milliseconds. */
};

/**
 * @brief Pointer shape a view asks its host to show.
 *
 * @since 3.0.0
 */
enum class CursorShape
{
    Default,       /**< The host's default cursor. */
    SizeHorizontal /**< Horizontal resize arrows, e.g. over a draggable boundary. */
};

} // namespace mne::scene

#endif // SCENE_INPUT_EVENT_H
