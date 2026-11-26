//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sdl_host.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SdlHost shows any mne_viz view in an SDL3 window.
 *
 * SdlHost opens an SDL3 window, translates SDL mouse, wheel and key events to the input events of mne_viz and
 * hands them to a SurfaceClient. Each frame the client describes is executed by the software renderer of mne_viz
 * and shown as an SDL texture, so neither Qt nor a GPU is needed; SDL's dummy video driver runs it headless.
 */

#ifndef VIZ_SDL_SDL_HOST_H
#define VIZ_SDL_SDL_HOST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "viz_sdl_global.h"
#include <mne/viz/gpu/software_renderer.h>
#include <mne/viz/host/surface.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cstdint>
#include <functional>
#include <string>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Cursor;

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::sdl
//=============================================================================================================

namespace mne::viz::sdl
{

//=============================================================================================================
/**
 * @brief An SDL3 window that hosts a SurfaceClient.
 *
 * @snippet ex_scene_sdl/main.cpp sdl_host_usage
 *
 * @since 3.0.0
 */
class VIZSDLSHARED_EXPORT SdlHost : public Surface
{
public:
    //=========================================================================================================
    /**
     * @brief Window and loop settings.
     */
    struct Options
    {
        std::string title = "MNE-CPP"; /**< Window title. */
        int width = 1000;              /**< Initial width in logical units. */
        int height = 640;              /**< Initial height in logical units. */
        int frame_limit = -1;          /**< Frames to draw before run() returns; -1 runs until the window closes. */
    };

    //=========================================================================================================
    /**
     * Initialises SDL's video subsystem and opens the window. Check is_open() afterwards.
     *
     * @param[in] options   Window and loop settings.
     *
     * @since 3.0.0
     */
    explicit SdlHost(Options options);

    //=========================================================================================================
    /**
     * Closes the window and shuts SDL down.
     *
     * @since 3.0.0
     */
    ~SdlHost() override;

    SdlHost(const SdlHost&) = delete;
    SdlHost& operator=(const SdlHost&) = delete;

    //=========================================================================================================
    /**
     * @return Whether the window could be opened.
     *
     * @since 3.0.0
     */
    bool is_open() const
    {
        return m_renderer != nullptr;
    }

    //=========================================================================================================
    /**
     * @return SDL's message when the window could not be opened.
     *
     * @since 3.0.0
     */
    const std::string& error() const
    {
        return m_error;
    }

    //=========================================================================================================
    /**
     * Sets a handler that sees key presses before the client, e.g. for the application's shortcuts.
     *
     * @param[in] handler   Returns whether it used the key; Escape closes the window when nobody uses it.
     *
     * @since 3.0.0
     */
    void set_key_handler(std::function<bool(const KeyEvent&)> handler);

    //=========================================================================================================
    /**
     * Shows @p client and runs the event loop until the window closes, Escape is pressed or the frame limit
     * is reached.
     *
     * @param[in] client        The view to show; attached for the duration of the call.
     * @param[in] before_frame  Called with the clock before each frame, e.g. to stream new data in.
     *
     * @return The number of frames drawn.
     *
     * @since 3.0.0
     */
    int run(SurfaceClient& client, const std::function<void(std::int64_t)>& before_frame = {});

    void request_update() override;
    void set_cursor(CursorShape shape) override;
    std::int64_t now_ms() const override;

private:
    bool dispatch(SurfaceClient& client);
    void present(SurfaceClient& client);

    Options m_options;
    std::string m_error;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;
    std::array<SDL_Cursor*, 4> m_cursors{}; // one per CursorShape, created on first use
    int m_textureWidth = 0;
    int m_textureHeight = 0;
    SurfaceSize m_size;
    SoftwareRenderer m_software;
    std::function<bool(const KeyEvent&)> m_keyHandler;
    bool m_updateRequested = true;
};

} // namespace mne::viz::sdl

#endif // VIZ_SDL_SDL_HOST_H
