//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     surface.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Surface and SurfaceClient connect a view of mne_viz to the window of a backend.
 *
 * A backend hosts a view in a window: it measures the window, translates its input to the events of
 * views/input_event.h, asks the view for a GpuFrame and executes it with its renderer. The view in turn asks the
 * host for a clock, a redraw and a pointer shape. Surface is the host's side of that contract, SurfaceClient the
 * view's, so any view runs in any backend (Qt, SDL, ...) without knowing which.
 */

#ifndef VIZ_SURFACE_H
#define VIZ_SURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../gpu/gpu_frame.h"
#include "../views/input_event.h"
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

//=============================================================================================================
/**
 * @brief Size of a surface in logical units and in pixels.
 *
 * @since 3.0.0
 */
struct SurfaceSize
{
    int width = 0;                   /**< Width in logical units, the unit of the input events. */
    int height = 0;                  /**< Height in logical units. */
    int pixel_width = 0;             /**< Width in pixels. */
    int pixel_height = 0;            /**< Height in pixels. */
    double device_pixel_ratio = 1.0; /**< Pixels per logical unit. */
};

//=============================================================================================================
/**
 * @brief The host's side: what a view may ask of the window it is shown in.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT Surface
{
public:
    //=========================================================================================================
    /**
     * Destroys the surface.
     *
     * @since 3.0.0
     */
    virtual ~Surface();

    //=========================================================================================================
    /**
     * Asks for a new frame; the host calls SurfaceClient::frame() soon, at most once per display refresh.
     *
     * @since 3.0.0
     */
    virtual void request_update() = 0;

    //=========================================================================================================
    /**
     * Shows another pointer shape over the surface.
     *
     * @param[in] shape     The pointer shape.
     *
     * @since 3.0.0
     */
    virtual void set_cursor(CursorShape shape);

    //=========================================================================================================
    /**
     * @return Milliseconds of a monotonic clock, the time base of the input events and animations.
     *
     * @since 3.0.0
     */
    virtual std::int64_t now_ms() const = 0;
};

//=============================================================================================================
/**
 * @brief The view's side: what a host drives.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT SurfaceClient
{
public:
    //=========================================================================================================
    /**
     * Destroys the client.
     *
     * @since 3.0.0
     */
    virtual ~SurfaceClient();

    //=========================================================================================================
    /**
     * Connects the client to the surface that shows it, or disconnects it with nullptr.
     *
     * @param[in] surface   The host's surface; it outlives the connection.
     *
     * @since 3.0.0
     */
    virtual void attach(Surface* surface);

    //=========================================================================================================
    /**
     * @return The surface the client is shown in, or nullptr.
     *
     * @since 3.0.0
     */
    Surface* surface() const
    {
        return m_surface;
    }

    //=========================================================================================================
    /**
     * The surface changed its size.
     *
     * @param[in] size  The new size.
     *
     * @since 3.0.0
     */
    virtual void resize(const SurfaceSize& size);

    //=========================================================================================================
    /**
     * A pointer pressed, moved, released or double-clicked over the surface.
     *
     * @param[in] event     The pointer event.
     *
     * @return Whether the client used the event.
     *
     * @since 3.0.0
     */
    virtual bool handle_pointer(const PointerEvent& event);

    //=========================================================================================================
    /**
     * The wheel turned or the touchpad scrolled over the surface.
     *
     * @param[in] event     The wheel event.
     *
     * @return Whether the client used the event.
     *
     * @since 3.0.0
     */
    virtual bool handle_wheel(const WheelEvent& event);

    //=========================================================================================================
    /**
     * A key was pressed while the surface had the focus.
     *
     * @param[in] event     The key event.
     *
     * @return Whether the client used the event.
     *
     * @since 3.0.0
     */
    virtual bool handle_key(const KeyEvent& event);

    //=========================================================================================================
    /**
     * Advances animations before a frame.
     *
     * @param[in] now_ms    The surface's clock.
     *
     * @return Whether an animation still runs, so the host should draw another frame.
     *
     * @since 3.0.0
     */
    virtual bool tick(std::int64_t now_ms);

    //=========================================================================================================
    /**
     * Describes the next frame.
     *
     * @param[in] target    Pixel size and clip-space correction of the host's render target.
     *
     * @return The frame, referring to resources().
     *
     * @since 3.0.0
     */
    virtual GpuFrame frame(const GpuFrameTarget& target) = 0;

    //=========================================================================================================
    /**
     * @return Buffers, textures and pipelines the frames refer to.
     *
     * @since 3.0.0
     */
    virtual const GpuResources& resources() const = 0;

private:
    Surface* m_surface = nullptr;
};

} // namespace mne::viz

#endif // VIZ_SURFACE_H
