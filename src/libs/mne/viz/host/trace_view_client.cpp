//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_view_client.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Implementation of TraceViewClient.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "trace_view_client.h"
#include "../views/trace_source.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TraceViewClient::TraceViewClient()
{
    install_effects();
}

//=============================================================================================================

void TraceViewClient::set_source(const TraceSource* source)
{
    m_source = source;
    m_view.set_source(source);
    data_changed();
}

//=============================================================================================================

void TraceViewClient::data_changed()
{
    m_builder.geometry().invalidate();
    update();
}

//=============================================================================================================

void TraceViewClient::set_effects(TraceView::Effects effects)
{
    m_effects = std::move(effects);
}

//=============================================================================================================

void TraceViewClient::set_text_metrics(const TextMetrics* metrics)
{
    m_metrics = metrics ? metrics : &m_approximateMetrics;
    m_builder.invalidate_overlay();
    update();
}

//=============================================================================================================

void TraceViewClient::resize(const SurfaceSize& size)
{
    m_view.set_view_size(size.width, size.height);
    update();
}

//=============================================================================================================

bool TraceViewClient::handle_pointer(const PointerEvent& event)
{
    return m_view.handle_pointer(event);
}

//=============================================================================================================

bool TraceViewClient::handle_wheel(const WheelEvent& event)
{
    return m_view.handle_wheel(event);
}

//=============================================================================================================

bool TraceViewClient::tick(std::int64_t now_ms)
{
    return m_animator.tick(now_ms, m_view);
}

//=============================================================================================================

GpuFrame TraceViewClient::frame(const GpuFrameTarget& target)
{
    if (!m_source) {
        return {};
    }
    return m_builder.build(m_view, *m_source, m_style, m_markers, *m_metrics, target);
}

//=============================================================================================================

const GpuResources& TraceViewClient::resources() const
{
    return m_builder.resources();
}

//=============================================================================================================

void TraceViewClient::install_effects()
{
    // Each entry first does what every host needs, then calls the application's callback
    TraceView::Effects effects;
    effects.scroll_changed = [this](double sample) {
        update();
        if (m_effects.scroll_changed) {
            m_effects.scroll_changed(sample);
        }
    };
    effects.zoom_changed = [this](double samples_per_pixel) {
        // Another zoom decimates differently and moves the overlay's time labels
        m_builder.geometry().invalidate();
        m_builder.invalidate_overlay();
        update();
        if (m_effects.zoom_changed) {
            m_effects.zoom_changed(samples_per_pixel);
        }
    };
    effects.channel_offset_changed = [this](int first_channel) {
        m_builder.geometry().invalidate();
        update();
        if (m_effects.channel_offset_changed) {
            m_effects.channel_offset_changed(first_channel);
        }
    };
    effects.animate_scroll = [this](const TraceView::ScrollAnimation& animation) {
        m_animator.start_scroll(animation, now());
        update();
    };
    effects.animate_zoom = [this](const TraceView::ZoomAnimation& animation) {
        m_animator.start_zoom(animation, now());
        update();
    };
    effects.stop_animations = [this] {
        m_animator.stop_scroll();
    };
    effects.sample_clicked = [this](int sample) {
        if (m_effects.sample_clicked) {
            m_effects.sample_clicked(sample);
        }
    };
    effects.range_selected = [this](int start, int end) {
        if (m_effects.range_selected) {
            m_effects.range_selected(start, end);
        }
    };
    effects.annotation_boundary_moved = [this](int index, bool start, int sample) {
        if (m_effects.annotation_boundary_moved) {
            m_effects.annotation_boundary_moved(index, start, sample);
        }
    };
    effects.annotations_changed = [this] {
        m_builder.invalidate_overlay();
        update();
        if (m_effects.annotations_changed) {
            m_effects.annotations_changed();
        }
    };
    effects.channel_double_clicked = [this](int channel) {
        if (m_effects.channel_double_clicked) {
            m_effects.channel_double_clicked(channel);
        }
    };
    effects.cursor_changed = [this](CursorShape shape) {
        if (surface()) {
            surface()->set_cursor(shape);
        }
        if (m_effects.cursor_changed) {
            m_effects.cursor_changed(shape);
        }
    };
    effects.cursor_data_changed = [this](const TraceView::CursorData& data) {
        if (m_effects.cursor_data_changed) {
            m_effects.cursor_data_changed(data);
        }
    };
    effects.redraw = [this] {
        update();
        if (m_effects.redraw) {
            m_effects.redraw();
        }
    };
    m_view.set_effects(std::move(effects));
}

//=============================================================================================================

std::int64_t TraceViewClient::now() const
{
    return surface() ? surface()->now_ms() : 0;
}

//=============================================================================================================

void TraceViewClient::update() const
{
    if (surface()) {
        surface()->request_update();
    }
}
