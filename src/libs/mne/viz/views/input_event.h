//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     input_event.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Toolkit-neutral pointer, wheel and key events for the interactive scene views.
 */

#ifndef VIZ_INPUT_EVENT_H
#define VIZ_INPUT_EVENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
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
 * @brief Mouse buttons held during a pointer event; combine with |.
 *
 * @since 3.0.0
 */
enum class MouseButtons : unsigned
{
    None = 0x0,  /**< No button. */
    Left = 0x1,  /**< Primary button. */
    Right = 0x2, /**< Secondary button. */
    Middle = 0x4 /**< Middle button. */
};

constexpr MouseButtons operator|(MouseButtons a, MouseButtons b)
{
    return static_cast<MouseButtons>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

constexpr bool has_flag(MouseButtons buttons, MouseButtons flag)
{
    return (static_cast<unsigned>(buttons) & static_cast<unsigned>(flag)) == static_cast<unsigned>(flag) &&
        flag != MouseButtons::None;
}

/**
 * @param[in] button   A button.
 *
 * @return The flag of @p button, MouseButtons::None for MouseButton::None.
 *
 * @since 3.0.0
 */
constexpr MouseButtons button_flag(MouseButton button)
{
    switch (button) {
        case MouseButton::Left:
            return MouseButtons::Left;
        case MouseButton::Right:
            return MouseButtons::Right;
        case MouseButton::Middle:
            return MouseButtons::Middle;
        case MouseButton::None:
            break;
    }
    return MouseButtons::None;
}

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

    Type type = Type::Move;                    /**< Kind of event. */
    double x = 0.0;                            /**< Horizontal position, growing to the right. */
    double y = 0.0;                            /**< Vertical position, growing downwards. */
    MouseButton button = MouseButton::None;    /**< Button that caused a press, release or double click; None for moves. */
    Modifier modifiers = Modifier::None;       /**< Modifiers held. */
    std::int64_t timestamp_ms = 0;             /**< Monotonic time in milliseconds, used for drag velocities. */
    MouseButtons buttons = MouseButtons::None; /**< Buttons held after the event, e.g. during a drag. */
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
 * @brief Key of a key event; printable keys are Key::Character with the character in KeyEvent::character.
 *
 * @since 3.0.0
 */
enum class Key
{
    Unknown,   /**< A key without a neutral name. */
    Character, /**< A printable key, see KeyEvent::character. */
    Escape,    /**< Escape. */
    Return,    /**< Return or Enter. */
    Tab,       /**< Tab. */
    Backspace, /**< Backspace. */
    Delete,    /**< Delete. */
    Left,      /**< Arrow left. */
    Right,     /**< Arrow right. */
    Up,        /**< Arrow up. */
    Down,      /**< Arrow down. */
    Home,      /**< Home. */
    End,       /**< End. */
    PageUp,    /**< Page up. */
    PageDown   /**< Page down. */
};

/**
 * @brief A key press.
 *
 * @since 3.0.0
 */
struct KeyEvent
{
    Key key = Key::Unknown;              /**< The key. */
    char32_t character = 0;              /**< For Key::Character the character: typed text in its case, or a key's letter in upper case. */
    Modifier modifiers = Modifier::None; /**< Modifiers held. */
    std::int64_t timestamp_ms = 0;       /**< Monotonic time in milliseconds. */

    /**
     * @param[in] c   A letter or another printable character.
     *
     * @return Whether this is the key of @p c, ignoring the case of letters.
     */
    constexpr bool is_character(char32_t c) const
    {
        const auto upper = [](char32_t x) {
            return (x >= U'a' && x <= U'z') ? x - U'a' + U'A' : x;
        };
        return key == Key::Character && upper(character) == upper(c);
    }
};

/**
 * @brief Pointer shape a view asks its host to show.
 *
 * @since 3.0.0
 */
enum class CursorShape
{
    Default,        /**< The host's default cursor. */
    SizeHorizontal, /**< Horizontal resize arrows, e.g. over a draggable boundary. */
    SizeVertical,   /**< Vertical resize arrows. */
    SizeAll         /**< Arrows in all directions, e.g. over a crossing of boundaries. */
};

} // namespace mne::viz

#endif // VIZ_INPUT_EVENT_H
