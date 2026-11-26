//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brain_view_interaction.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of BrainViewInteraction.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brain_view_interaction.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

namespace
{

// Pane hit-testing works on whole pixels, as the toolkits report integer mouse positions
Point pixel_of(double x, double y)
{
    return {std::round(x), std::round(y)};
}

template<typename Callback, typename... Args>
void notify(const Callback& callback, Args&&... args)
{
    if (callback) {
        callback(std::forward<Args>(args)...);
    }
}

} // namespace

//=============================================================================================================

void BrainViewInteraction::set_effects(Effects effects)
{
    m_effects = std::move(effects);
}

//=============================================================================================================

void BrainViewInteraction::attach_cameras(PaneCamera* single, std::vector<PaneCamera*> panes)
{
    m_single = single;
    m_panes = std::move(panes);
}

//=============================================================================================================

void BrainViewInteraction::set_panes(bool multi_view, std::vector<int> viewports)
{
    m_multi_view = multi_view;
    m_viewports = std::move(viewports);
    if (m_viewports.empty()) {
        m_viewports.push_back(0);
    }
}

//=============================================================================================================

PaneCamera* BrainViewInteraction::pane_camera(int viewport) const
{
    return (viewport >= 0 && viewport < static_cast<int>(m_panes.size())) ? m_panes[static_cast<std::size_t>(viewport)] : nullptr;
}

//=============================================================================================================

int BrainViewInteraction::viewport_at(Point pos) const
{
    if (!m_multi_view) {
        return 0;
    }
    return m_layout.viewport_index_at(pos, m_viewports, m_size);
}

//=============================================================================================================

void BrainViewInteraction::cancel_splitter_drag()
{
    m_dragging_splitter = false;
    m_active_splitter = SplitterHit::None;
    notify(m_effects.cursor_changed, CursorShape::Default);
}

//=============================================================================================================

void BrainViewInteraction::update_cursor(Point pos)
{
    const SplitterHit hit = m_multi_view ? m_layout.hit_test_splitter(pos, static_cast<int>(m_viewports.size()), m_size)
                                         : SplitterHit::None;
    notify(m_effects.cursor_changed, PaneLayout::cursor_for_hit(hit));
}

//=============================================================================================================

bool BrainViewInteraction::handle_pointer(const PointerEvent& event)
{
    const Point pos = pixel_of(event.x, event.y);
    switch (event.type) {
        case PointerEvent::Type::Press:
            press(pos, event.button);
            return true;
        case PointerEvent::Type::Move:
            move(pos, has_flag(event.buttons, MouseButtons::Left));
            return true;
        case PointerEvent::Type::Release:
            release(pos, event.button);
            return true;
        case PointerEvent::Type::DoubleClick:
            return event.button == MouseButton::Left && m_effects.double_clicked && m_effects.double_clicked(pos);
    }
    return false;
}

//=============================================================================================================

void BrainViewInteraction::press(Point pos, MouseButton button)
{
    m_last_pos = pos;
    if (button != MouseButton::Left) {
        return;
    }
    m_dragged_since_press = false;
    if (!m_multi_view) {
        return;
    }

    const int viewport = viewport_at(pos);
    if (viewport >= 0 && m_effects.hits_viewport_label && m_effects.hits_viewport_label(viewport, pos)) {
        notify(m_effects.viewport_selected, viewport);
        notify(m_effects.preset_menu_requested, viewport, pos);
        return;
    }

    const SplitterHit hit = m_layout.hit_test_splitter(pos, static_cast<int>(m_viewports.size()), m_size);
    if (hit != SplitterHit::None) {
        m_dragging_splitter = true;
        m_active_splitter = hit;
        update_cursor(pos);
        return;
    }

    if (viewport >= 0) {
        notify(m_effects.viewport_selected, viewport);
    }
}

//=============================================================================================================

void BrainViewInteraction::move(Point pos, bool left_held)
{
    if (m_dragging_splitter && left_held) {
        m_layout.drag_splitter(pos, m_active_splitter, m_size);
        m_last_pos = pos;
        notify(m_effects.layout_changed);
        return;
    }

    if (!left_held) {
        if (m_multi_view) {
            update_cursor(pos);
        } else {
            notify(m_effects.cursor_changed, CursorShape::Default);
        }
        notify(m_effects.hovered, pos);
        return;
    }

    const double dx = pos.x - m_last_pos.x;
    const double dy = pos.y - m_last_pos.y;
    m_last_pos = pos;

    if (!m_multi_view) {
        if (m_single) {
            m_single->rotation = OrbitCamera::orbit(m_single->rotation, dx, dy);
        }
        m_dragged_since_press = true;
        notify(m_effects.redraw);
        return;
    }

    PaneCamera* camera = pane_camera(viewport_at(pos));
    if (!camera) {
        return;
    }
    // The perspective pane orbits, the planar panes pan along their view plane
    if (view_preset_is_perspective(clamp_view_preset(camera->preset))) {
        camera->rotation = OrbitCamera::orbit(camera->rotation, dx, dy);
    } else {
        camera->pan = OrbitCamera::panned(camera->pan, dx, dy, m_scene_size);
    }
    m_dragged_since_press = true;
    notify(m_effects.redraw);
}

//=============================================================================================================

void BrainViewInteraction::release(Point pos, MouseButton button)
{
    if (button == MouseButton::Left) {
        if (m_dragging_splitter) {
            m_dragging_splitter = false;
            m_active_splitter = SplitterHit::None;
            notify(m_effects.settings_changed);
            update_cursor(pos);
            return;
        }
        // Rotations and pans of the panes are persisted when the drag ends
        if (m_multi_view) {
            notify(m_effects.settings_changed);
        }
        if (!m_dragged_since_press) {
            notify(m_effects.clicked, pos);
        }
    }

    if (m_multi_view) {
        update_cursor(pos);
    } else {
        notify(m_effects.cursor_changed, CursorShape::Default);
    }
}

//=============================================================================================================

void BrainViewInteraction::handle_wheel(const WheelEvent& event)
{
    const auto delta = static_cast<float>(event.angle_delta_y / 120.0);
    PaneCamera* camera = m_multi_view ? pane_camera(viewport_at(pixel_of(event.x, event.y))) : m_single;
    if (camera) {
        camera->zoom += delta;
        notify(m_effects.settings_changed);
    }
    notify(m_effects.redraw);
}

//=============================================================================================================

bool BrainViewInteraction::handle_key(const KeyEvent& event)
{
    if (event.is_character(U'S')) {
        notify(m_effects.snapshot_requested);
        return true;
    }
    if (event.is_character(U'R')) {
        if (m_single) {
            m_single->rotation = Eigen::Quaternionf::Identity();
        }
        notify(m_effects.settings_changed);
        notify(m_effects.redraw);
        return true;
    }
    return false;
}

//=============================================================================================================

void BrainViewInteraction::orbit_perspective_views(double dx, double dy)
{
    if (m_multi_view) {
        for (PaneCamera* camera : m_panes) {
            if (camera && view_preset_is_perspective(clamp_view_preset(camera->preset))) {
                camera->rotation = OrbitCamera::orbit(camera->rotation, dx, dy);
            }
        }
    } else if (m_single) {
        m_single->rotation = OrbitCamera::orbit(m_single->rotation, dx, dy);
    }
    notify(m_effects.redraw);
}

} // namespace mne::viz
