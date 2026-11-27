//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sdl_host.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Implementation of SdlHost.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sdl_host.h"

#include <SDL3/SDL.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::sdl;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

Modifier modifiers_of(SDL_Keymod mod)
{
    Modifier result = Modifier::None;
    if ((mod & SDL_KMOD_SHIFT) != 0) {
        result = result | Modifier::Shift;
    }
    if ((mod & SDL_KMOD_CTRL) != 0) {
        result = result | Modifier::Ctrl;
    }
    if ((mod & SDL_KMOD_ALT) != 0) {
        result = result | Modifier::Alt;
    }
    if ((mod & SDL_KMOD_GUI) != 0) {
        result = result | Modifier::Meta;
    }
    return result;
}

MouseButton button_of(std::uint8_t button)
{
    switch (button) {
        case SDL_BUTTON_LEFT:
            return MouseButton::Left;
        case SDL_BUTTON_RIGHT:
            return MouseButton::Right;
        case SDL_BUTTON_MIDDLE:
            return MouseButton::Middle;
        default:
            return MouseButton::None;
    }
}

MouseButtons buttons_of(SDL_MouseButtonFlags flags)
{
    MouseButtons result = MouseButtons::None;
    if ((flags & SDL_BUTTON_LMASK) != 0) {
        result = result | MouseButtons::Left;
    }
    if ((flags & SDL_BUTTON_RMASK) != 0) {
        result = result | MouseButtons::Right;
    }
    if ((flags & SDL_BUTTON_MMASK) != 0) {
        result = result | MouseButtons::Middle;
    }
    return result;
}

KeyEvent key_of(const SDL_KeyboardEvent& event, std::int64_t now)
{
    KeyEvent key;
    key.modifiers = modifiers_of(event.mod);
    key.timestamp_ms = now;
    switch (event.key) {
        case SDLK_ESCAPE:
            key.key = Key::Escape;
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            key.key = Key::Return;
            break;
        case SDLK_TAB:
            key.key = Key::Tab;
            break;
        case SDLK_BACKSPACE:
            key.key = Key::Backspace;
            break;
        case SDLK_DELETE:
            key.key = Key::Delete;
            break;
        case SDLK_LEFT:
            key.key = Key::Left;
            break;
        case SDLK_RIGHT:
            key.key = Key::Right;
            break;
        case SDLK_UP:
            key.key = Key::Up;
            break;
        case SDLK_DOWN:
            key.key = Key::Down;
            break;
        case SDLK_HOME:
            key.key = Key::Home;
            break;
        case SDLK_END:
            key.key = Key::End;
            break;
        case SDLK_PAGEUP:
            key.key = Key::PageUp;
            break;
        case SDLK_PAGEDOWN:
            key.key = Key::PageDown;
            break;
        default:
            // SDL names printable keys by their unshifted character; letters are reported in upper case
            if (event.key >= 0x20 && event.key < 0x7f) {
                key.key = Key::Character;
                key.character = (event.key >= 'a' && event.key <= 'z') ? event.key - 'a' + 'A' : event.key;
            }
            break;
    }
    return key;
}

// Decodes UTF-8 text, e.g. of SDL_EVENT_TEXT_INPUT, into code points
std::vector<char32_t> code_points_of(const char* text)
{
    std::vector<char32_t> result;
    for (const auto* p = reinterpret_cast<const unsigned char*>(text); p && *p;) {
        const unsigned char lead = *p++;
        const int continuation = lead >= 0xF0 ? 3 : lead >= 0xE0 ? 2
            : lead >= 0xC0                                       ? 1
                                                                 : 0;
        char32_t c = continuation == 0 ? lead : lead & (0x3F >> continuation);
        for (int i = 0; i < continuation && (*p & 0xC0) == 0x80; ++i) {
            c = (c << 6) | (*p++ & 0x3F);
        }
        result.push_back(c);
    }
    return result;
}

SDL_SystemCursor system_cursor_of(CursorShape shape)
{
    switch (shape) {
        case CursorShape::SizeHorizontal:
            return SDL_SYSTEM_CURSOR_EW_RESIZE;
        case CursorShape::SizeVertical:
            return SDL_SYSTEM_CURSOR_NS_RESIZE;
        case CursorShape::SizeAll:
            return SDL_SYSTEM_CURSOR_MOVE;
        default:
            return SDL_SYSTEM_CURSOR_DEFAULT;
    }
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SdlHost::SdlHost(Options options)
: m_options(std::move(options))
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        m_error = SDL_GetError();
        return;
    }
    m_window = SDL_CreateWindow(m_options.title.c_str(), m_options.width, m_options.height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    // Any renderer only shows a texture; SDL's software renderer works without a GPU, e.g. on the dummy driver
    m_renderer = m_window ? SDL_CreateRenderer(m_window, nullptr) : nullptr;
    if (m_window && !m_renderer) {
        m_renderer = SDL_CreateRenderer(m_window, SDL_SOFTWARE_RENDERER);
    }
    if (!m_renderer) {
        m_error = SDL_GetError();
    }
}

//=============================================================================================================

SdlHost::~SdlHost()
{
    for (SDL_Cursor* cursor : m_cursors) {
        SDL_DestroyCursor(cursor);
    }
    SDL_DestroyTexture(m_texture);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

//=============================================================================================================

void SdlHost::set_key_handler(std::function<bool(const KeyEvent&)> handler)
{
    m_keyHandler = std::move(handler);
}

//=============================================================================================================

int SdlHost::run(SurfaceClient& client, const std::function<void(std::int64_t)>& before_frame)
{
    if (!is_open()) {
        return 0;
    }

    client.attach(this);
    // Typed text arrives as SDL_EVENT_TEXT_INPUT, in its case and with shifted and composed characters
    SDL_StartTextInput(m_window);
    m_size = SurfaceSize{};
    m_updateRequested = true;

    int frames = 0;
    while (m_options.frame_limit < 0 || frames < m_options.frame_limit) {
        if (!dispatch(client)) {
            break;
        }
        const std::int64_t now = now_ms();
        if (before_frame) {
            before_frame(now);
        }

        SurfaceSize size;
        SDL_GetWindowSize(m_window, &size.width, &size.height);
        SDL_GetWindowSizeInPixels(m_window, &size.pixel_width, &size.pixel_height);
        size.device_pixel_ratio = size.width > 0 ? static_cast<double>(size.pixel_width) / size.width : 1.0;
        if (size.width != m_size.width || size.height != m_size.height || size.pixel_width != m_size.pixel_width) {
            m_size = size;
            client.resize(m_size);
        }

        if (client.tick(now)) {
            m_updateRequested = true;
        }
        // Without a limit an idle window waits for input instead of redrawing unchanged frames
        if (!m_updateRequested && m_options.frame_limit < 0) {
            SDL_WaitEventTimeout(nullptr, 16);
            continue;
        }
        m_updateRequested = false;
        present(client);
        ++frames;
    }

    SDL_StopTextInput(m_window);
    client.attach(nullptr);
    return frames;
}

//=============================================================================================================

void SdlHost::request_update()
{
    m_updateRequested = true;
}

//=============================================================================================================

void SdlHost::set_cursor(CursorShape shape)
{
    const auto index = static_cast<std::size_t>(shape);
    if (index >= m_cursors.size()) {
        return;
    }
    if (!m_cursors[index]) {
        m_cursors[index] = SDL_CreateSystemCursor(system_cursor_of(shape));
    }
    SDL_SetCursor(m_cursors[index]);
}

//=============================================================================================================

std::int64_t SdlHost::now_ms() const
{
    return static_cast<std::int64_t>(SDL_GetTicks());
}

//=============================================================================================================

bool SdlHost::dispatch(SurfaceClient& client)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        const std::int64_t now = now_ms();
        switch (event.type) {
            case SDL_EVENT_QUIT:
                return false;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                PointerEvent pointer;
                pointer.type = event.type == SDL_EVENT_MOUSE_BUTTON_UP ? PointerEvent::Type::Release
                    : event.button.clicks == 2                         ? PointerEvent::Type::DoubleClick
                                                                       : PointerEvent::Type::Press;
                pointer.x = event.button.x;
                pointer.y = event.button.y;
                pointer.button = button_of(event.button.button);
                pointer.buttons = buttons_of(SDL_GetMouseState(nullptr, nullptr));
                pointer.modifiers = modifiers_of(SDL_GetModState());
                pointer.timestamp_ms = now;
                client.handle_pointer(pointer);
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                PointerEvent pointer;
                pointer.x = event.motion.x;
                pointer.y = event.motion.y;
                pointer.buttons = buttons_of(event.motion.state);
                pointer.modifiers = modifiers_of(SDL_GetModState());
                pointer.timestamp_ms = now;
                client.handle_pointer(pointer);
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                WheelEvent wheel;
                wheel.x = event.wheel.mouse_x;
                wheel.y = event.wheel.mouse_y;
                // SDL counts notches, mne_viz eighths of a degree (120 per notch)
                wheel.angle_delta_x = -120.0 * event.wheel.x;
                wheel.angle_delta_y = 120.0 * event.wheel.y;
                wheel.modifiers = modifiers_of(SDL_GetModState());
                wheel.timestamp_ms = now;
                client.handle_wheel(wheel);
                break;
            }
            case SDL_EVENT_KEY_DOWN: {
                const KeyEvent key = key_of(event.key, now);
                // Printable keys without shortcut modifiers come again as text input
                const bool shortcut = has_flag(key.modifiers, Modifier::Ctrl) || has_flag(key.modifiers, Modifier::Alt) ||
                    has_flag(key.modifiers, Modifier::Meta);
                if (key.key == Key::Character && !shortcut && SDL_TextInputActive(m_window)) {
                    break;
                }
                const bool used = (m_keyHandler && m_keyHandler(key)) || client.handle_key(key);
                if (!used && key.key == Key::Escape) {
                    return false;
                }
                break;
            }
            case SDL_EVENT_TEXT_INPUT:
                for (const char32_t c : code_points_of(event.text.text)) {
                    KeyEvent key;
                    key.key = Key::Character;
                    key.character = c;
                    key.modifiers = modifiers_of(SDL_GetModState());
                    key.timestamp_ms = now;
                    if (!(m_keyHandler && m_keyHandler(key))) {
                        client.handle_key(key);
                    }
                }
                break;
            case SDL_EVENT_WINDOW_EXPOSED:
                m_updateRequested = true;
                break;
            default:
                break;
        }
    }
    return true;
}

//=============================================================================================================

void SdlHost::present(SurfaceClient& client)
{
    GpuFrameTarget target;
    target.pixel_width = m_size.pixel_width;
    target.pixel_height = m_size.pixel_height;
    target.device_pixel_ratio = m_size.device_pixel_ratio;
    const Image image = m_software.render(client.frame(target), client.resources());
    if (image.width <= 0 || image.height <= 0) {
        return;
    }

    if (!m_texture || m_textureWidth != image.width || m_textureHeight != image.height) {
        SDL_DestroyTexture(m_texture);
        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, image.width, image.height);
        m_textureWidth = image.width;
        m_textureHeight = image.height;
    }
    SDL_UpdateTexture(m_texture, nullptr, image.rgba.data(), image.width * 4);
    SDL_RenderClear(m_renderer);
    SDL_RenderTexture(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}
