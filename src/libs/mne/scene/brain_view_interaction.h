//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brain_view_interaction.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    BrainViewInteraction: pointer, wheel and key handling of the 3-D brain view (orbit, pan, zoom, panes).
 */

#ifndef SCENE_BRAIN_VIEW_INTERACTION_H
#define SCENE_BRAIN_VIEW_INTERACTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "camera.h"
#include "geometry.h"
#include "input_event.h"
#include "pane_layout.h"
#include "scene_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

/**
 * @brief Input state machine of the 3-D brain view.
 *
 * Drags follow the buttons held according to the pointer events. In the single view a left-button drag
 * orbits the camera; in the multi view it orbits the perspective pane
 * under the pointer and pans the other panes, and dragging a splitter resizes the panes. The wheel zooms the
 * view (or the pane) under the pointer, R resets the rotation of the single view and S asks for a snapshot. A
 * left click without a drag reports the clicked point, a hover move the hovered point.
 *
 * The camera states are owned by the host (attach_cameras()); the host keeps the pane setup up to date with
 * set_view_size() and set_panes() before it forwards events. Effects go out through Effects; positions are
 * rounded to whole logical pixels, as pane hit-testing is done in pixels.
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT BrainViewInteraction
{
public:
    /** Reactions of the host; empty callbacks are skipped. */
    struct Effects
    {
        std::function<void()> redraw;                          /**< A camera changed. */
        std::function<void()> layout_changed;                  /**< A splitter moved. */
        std::function<void()> settings_changed;                /**< Persistable view state changed. */
        std::function<void(CursorShape)> cursor_changed;       /**< Show another pointer shape. */
        std::function<void(int)> viewport_selected;            /**< A press selected a pane for editing. */
        std::function<bool(int, Point)> hits_viewport_label;   /**< Whether a point is on the name label of a pane. */
        std::function<void(int, Point)> preset_menu_requested; /**< A press on a pane name. */
        std::function<void(Point)> hovered;                    /**< The pointer moved without a button. */
        std::function<void(Point)> clicked;                    /**< A left click without a drag ended. */
        std::function<bool(Point)> double_clicked;             /**< A left double click; returns whether it was used. */
        std::function<void()> snapshot_requested;              /**< S was pressed. */
    };

    //=========================================================================================================
    /**
     * Sets the reactions of the host.
     *
     * @param[in] effects   Callbacks.
     */
    void set_effects(Effects effects);

    //=========================================================================================================
    /**
     * Attaches the camera states the input changes; they are not owned and must outlive their use.
     *
     * @param[in] single   Camera of the single view: rotation and zoom.
     * @param[in] panes    Camera of each viewport of the multi view, by viewport index.
     */
    void attach_cameras(PaneCamera* single, std::vector<PaneCamera*> panes);

    //=========================================================================================================
    /**
     * @param[in] size   Size of the view in logical pixels.
     */
    void set_view_size(PixelSize size)
    {
        m_size = size;
    }

    /** @return The size of the view. */
    PixelSize view_size() const
    {
        return m_size;
    }

    //=========================================================================================================
    /**
     * Sets the pane setup.
     *
     * @param[in] multi_view   Whether the view is split into panes.
     * @param[in] viewports    Viewport index of each pane in slot order (multi view).
     */
    void set_panes(bool multi_view, std::vector<int> viewports);

    /** @return Whether the view is split into panes. */
    bool multi_view() const
    {
        return m_multi_view;
    }

    /** @param[in] size   Extent of the scene, which sets the pan speed. */
    void set_scene_size(float size)
    {
        m_scene_size = size;
    }

    /** @return The pane layout. */
    PaneLayout& layout()
    {
        return m_layout;
    }

    /** @return The pane layout. */
    const PaneLayout& layout() const
    {
        return m_layout;
    }

    //=========================================================================================================
    /**
     * @param[in] pos   Pointer position.
     *
     * @return The viewport under the pointer: 0 in the single view, -1 between or outside the panes.
     */
    int viewport_at(Point pos) const;

    /** @return Whether a splitter is being dragged. */
    bool dragging_splitter() const
    {
        return m_dragging_splitter;
    }

    /** Ends a splitter drag, e.g. when the view leaves the multi view. */
    void cancel_splitter_drag();

    //=========================================================================================================
    /**
     * Handles a pointer event.
     *
     * @param[in] event   Event.
     *
     * @return Whether the event was used; a double click is used when Effects::double_clicked says so.
     */
    bool handle_pointer(const PointerEvent& event);

    //=========================================================================================================
    /**
     * Zooms the view, or the pane under the pointer, by one step per wheel notch.
     *
     * @param[in] event   Event.
     */
    void handle_wheel(const WheelEvent& event);

    //=========================================================================================================
    /**
     * Handles R (reset the single-view rotation) and S (snapshot).
     *
     * @param[in] event   Event.
     *
     * @return Whether the key was used.
     */
    bool handle_key(const KeyEvent& event);

    //=========================================================================================================
    /**
     * Orbits the single view, or every perspective pane of the multi view, as a pointer drag would; used for
     * the automated rotation.
     *
     * @param[in] dx   Horizontal drag in pixels.
     * @param[in] dy   Vertical drag in pixels.
     */
    void orbit_perspective_views(double dx, double dy);

private:
    PaneCamera* pane_camera(int viewport) const;
    void update_cursor(Point pos);
    void press(Point pos, MouseButton button);
    void move(Point pos, bool left_held);
    void release(Point pos, MouseButton button);

    Effects m_effects;
    PaneLayout m_layout;
    PaneCamera* m_single = nullptr;
    std::vector<PaneCamera*> m_panes;
    std::vector<int> m_viewports{0};
    PixelSize m_size;
    float m_scene_size = 0.3f;
    bool m_multi_view = false;

    Point m_last_pos;
    bool m_dragging_splitter = false;
    SplitterHit m_active_splitter = SplitterHit::None;
    bool m_dragged_since_press = false;
};

} // namespace mne::scene

#endif // SCENE_BRAIN_VIEW_INTERACTION_H
