//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     figure_view.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FigureView class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "figure_view.h"
#include "../figures/canvas.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

// Full-target quad as a strip: (x, y) in clip space, (u, v) with v = 0 at the top of the texture
constexpr std::array<float, 16> kQuad = {-1.f, 1.f, 0.f, 0.f, -1.f, -1.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f, 1.f, -1.f, 1.f, 1.f};

// Zoom per wheel notch
constexpr double kWheelZoom = 1.15;
// Angle of one wheel notch, in eighths of a degree
constexpr double kWheelNotch = 120.0;
// Scroll per wheel notch of figures that are not zoomable
constexpr double kScrollPerNotch = 48.0;
// Gap around fitted contents
constexpr double kFitMargin = 2.0;
// Opacity of the rubber band's fill
constexpr std::uint8_t kBandAlpha = 60;

Rect normalized(Point a, Point b)
{
    return {std::min(a.x, b.x), std::min(a.y, b.y), std::abs(b.x - a.x), std::abs(b.y - a.y)};
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FigureView::FigureView(SceneFigure& figure, Theme theme)
: m_figure(figure)
, m_theme(std::move(theme))
{
    m_listener = m_figure.add_listener([this](SceneFigure::Change change) {
        figure_changed(change);
    });

    GpuPipeline overlay;
    overlay.shader = "screen_overlay";
    overlay.bindings = {{4 * sizeof(float), false}};
    overlay.attributes = {{0, 0, GpuVertexFormat::Float2, 0}, {1, 0, GpuVertexFormat::Float2, 2 * sizeof(float)}};
    overlay.topology = GpuTopology::TriangleStrip;
    overlay.blend = GpuBlend::Alpha;
    m_pipeline = m_resources.create_pipeline(std::move(overlay));
    m_quad = m_resources.create_buffer(GpuBufferUsage::Vertex);
    m_resources.set_buffer_values(m_quad, std::span<const float>(kQuad));
}

//=============================================================================================================

FigureView::~FigureView()
{
    m_figure.remove_listener(m_listener);
}

//=============================================================================================================

void FigureView::set_theme(Theme theme)
{
    m_theme = std::move(theme);
    update();
}

//=============================================================================================================

void FigureView::set_background(Color color)
{
    m_background = color;
    update();
}

//=============================================================================================================

void FigureView::set_font(Font font)
{
    m_font = std::move(font);
    update();
}

//=============================================================================================================

void FigureView::set_text_metrics(const TextMetrics* metrics)
{
    m_metrics = metrics ? metrics : &m_approximateMetrics;
    update();
}

//=============================================================================================================

void FigureView::set_view_size(double width, double height)
{
    m_width = std::max(0.0, width);
    m_height = std::max(0.0, height);
    m_figure.resize(m_width, m_height);
    if (!m_figure.zoomable()) {
        clamp_scroll();
    } else if (m_fitted) {
        apply_fit();
    }
    update();
}

//=============================================================================================================

void FigureView::fit_to_contents()
{
    m_fitted = true;
    if (m_figure.zoomable()) {
        apply_fit();
    } else {
        m_offset = {};
    }
    update();
}

//=============================================================================================================

void FigureView::zoom_by(double factor, Point anchor)
{
    if (!(factor > 0.0) || !m_figure.zoomable()) {
        return;
    }
    const Point fixed = map_to_scene(anchor);
    m_zoom *= factor;
    m_offset = {anchor.x - fixed.x * m_zoom, anchor.y - fixed.y * m_zoom};
    m_fitted = false;
    update();
}

//=============================================================================================================

void FigureView::pan_by(double dx, double dy)
{
    if (m_figure.zoomable()) {
        m_offset = {m_offset.x + dx, m_offset.y + dy};
        m_fitted = false;
    } else {
        m_offset.y += dy;
        clamp_scroll();
    }
    update();
}

//=============================================================================================================

Point FigureView::map_to_scene(Point point) const
{
    return {(point.x - m_offset.x) / m_zoom, (point.y - m_offset.y) / m_zoom};
}

//=============================================================================================================

Point FigureView::map_from_scene(Point point) const
{
    return {point.x * m_zoom + m_offset.x, point.y * m_zoom + m_offset.y};
}

//=============================================================================================================

std::optional<Rect> FigureView::band() const
{
    if (m_drag != Drag::Band) {
        return std::nullopt;
    }
    return normalized(m_bandStart, m_bandEnd);
}

//=============================================================================================================

void FigureView::leave()
{
    m_figure.hover(std::nullopt);
}

//=============================================================================================================

DisplayList FigureView::draw() const
{
    Canvas canvas(m_width, m_height, m_background);
    canvas.save();
    canvas.translate(m_offset.x, m_offset.y);
    canvas.scale(m_zoom, m_zoom);
    m_figure.draw(canvas, *m_metrics, m_font);
    canvas.restore();

    if (const std::optional<Rect> rect = band()) {
        Color fill = m_theme.accent;
        fill.a = kBandAlpha;
        canvas.set_pen(Pen{m_theme.accent, 1.0});
        canvas.set_brush(Brush::solid(fill));
        canvas.draw_rect(*rect);
    }
    return canvas.take();
}

//=============================================================================================================

DisplayList FigureView::picture(double width, double height) const
{
    Canvas canvas(width, height, m_background);
    if (!m_figure.zoomable()) {
        m_figure.resize(width, height);
        m_figure.draw(canvas, *m_metrics, m_font);
        m_figure.resize(m_width, m_height);
        return canvas.take();
    }
    const Rect bounds = m_figure.bounds();
    if (!bounds.empty() && width > 2 * kFitMargin && height > 2 * kFitMargin) {
        const double zoom = std::min((width - 2 * kFitMargin) / bounds.width, (height - 2 * kFitMargin) / bounds.height);
        canvas.translate(width / 2.0 - bounds.center().x * zoom, height / 2.0 - bounds.center().y * zoom);
        canvas.scale(zoom, zoom);
        m_figure.draw(canvas, *m_metrics, m_font);
    }
    return canvas.take();
}

//=============================================================================================================

void FigureView::resize(const SurfaceSize& size)
{
    set_view_size(size.width, size.height);
}

//=============================================================================================================

bool FigureView::handle_pointer(const PointerEvent& event)
{
    const Point at{event.x, event.y};
    switch (event.type) {
        case PointerEvent::Type::Press:
            if (event.button == MouseButton::Right) {
                m_drag = Drag::Pan;
                m_last = at;
                return true;
            }
            if (event.button == MouseButton::Left) {
                if (m_figure.press(map_to_scene(at), event.modifiers)) {
                    m_drag = Drag::Item;
                } else if (m_figure.band_selection_enabled()) {
                    m_drag = Drag::Band;
                    m_bandStart = m_bandEnd = at;
                    m_bandModifiers = event.modifiers;
                    update();
                }
                return true;
            }
            return false;
        case PointerEvent::Type::Move:
            if (m_drag == Drag::Pan) {
                pan_by(at.x - m_last.x, at.y - m_last.y);
                m_last = at;
            } else if (m_drag == Drag::Band) {
                m_bandEnd = at;
                update();
            } else if (m_drag == Drag::None) {
                m_figure.hover(map_to_scene(at));
            }
            return true;
        case PointerEvent::Type::Release: {
            const Drag drag = std::exchange(m_drag, Drag::None);
            if (drag == Drag::Item) {
                m_figure.release(map_to_scene(at));
            } else if (drag == Drag::Band) {
                m_bandEnd = at;
                const Point a = map_to_scene(m_bandStart);
                const Point b = map_to_scene(m_bandEnd);
                m_figure.band_selected(normalized(a, b), m_bandModifiers);
                update();
            }
            return drag != Drag::None;
        }
        case PointerEvent::Type::DoubleClick:
            if (event.button != MouseButton::Left) {
                return false;
            }
            if (!m_figure.double_click(map_to_scene(at)) && m_figure.zoomable()) {
                fit_to_contents();
            }
            return true;
    }
    return false;
}

//=============================================================================================================

bool FigureView::handle_wheel(const WheelEvent& event)
{
    if (event.angle_delta_y == 0.0) {
        return false;
    }
    if (m_figure.zoomable()) {
        zoom_by(event.angle_delta_y > 0.0 ? kWheelZoom : 1.0 / kWheelZoom, {event.x, event.y});
        return true;
    }
    const double before = m_offset.y;
    pan_by(0.0, event.angle_delta_y / kWheelNotch * kScrollPerNotch);
    return m_offset.y != before;
}

//=============================================================================================================

GpuFrame FigureView::frame(const GpuFrameTarget& target)
{
    const double ratio = target.device_pixel_ratio > 0.0 ? target.device_pixel_ratio : 1.0;
    if (m_width <= 0.0 || m_height <= 0.0) {
        set_view_size(target.pixel_width / ratio, target.pixel_height / ratio);
    }

    GpuFrame frame;
    frame.pixel_width = target.pixel_width;
    frame.pixel_height = target.pixel_height;
    frame.device_pixel_ratio = ratio;
    GpuPass pass;
    pass.clear_color = m_background;

    DisplayList list = draw();
    if (m_texture == 0) {
        GpuTexture texture;
        texture.content = std::move(list);
        texture.scale = ratio;
        texture.linear_filter = false;
        m_texture = m_resources.create_texture(std::move(texture));
    } else {
        m_resources.set_texture(m_texture, std::move(list), ratio);
    }
    GpuDraw quad;
    quad.pipeline = m_pipeline;
    quad.vertex_buffers = {m_quad};
    quad.count = 4;
    quad.textures = {{0, m_texture}};
    pass.draws.push_back(std::move(quad));
    frame.passes.push_back(std::move(pass));
    return frame;
}

//=============================================================================================================

const GpuResources& FigureView::resources() const
{
    return m_resources;
}

//=============================================================================================================

void FigureView::apply_fit()
{
    const Rect bounds = m_figure.bounds();
    if (bounds.empty() || m_width <= 2 * kFitMargin || m_height <= 2 * kFitMargin) {
        return;
    }
    m_zoom = std::min((m_width - 2 * kFitMargin) / bounds.width, (m_height - 2 * kFitMargin) / bounds.height);
    m_offset = {m_width / 2.0 - bounds.center().x * m_zoom, m_height / 2.0 - bounds.center().y * m_zoom};
}

//=============================================================================================================

void FigureView::clamp_scroll()
{
    // Figures that are not zoomable draw in view units and only scroll vertically
    m_zoom = 1.0;
    m_offset.x = 0.0;
    const Rect bounds = m_figure.bounds();
    const double overflow = bounds.empty() ? 0.0 : std::max(0.0, bounds.bottom() - m_height);
    m_offset.y = std::clamp(m_offset.y, -overflow, 0.0);
}

//=============================================================================================================

void FigureView::figure_changed(SceneFigure::Change change)
{
    if (change == SceneFigure::Change::Bounds) {
        if (!m_figure.zoomable()) {
            clamp_scroll();
        } else if (m_fitted) {
            apply_fit();
        }
    }
    update();
}

//=============================================================================================================

void FigureView::update() const
{
    if (surface()) {
        surface()->request_update();
    }
}
