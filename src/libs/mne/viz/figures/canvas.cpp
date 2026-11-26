//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     canvas.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of Canvas and of Brush::color_at.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "canvas.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Color Brush::color_at(double t) const
{
    if (stops.empty()) {
        return color;
    }
    t = std::clamp(t, 0.0, 1.0);
    if (t <= stops.front().position) {
        return stops.front().color;
    }
    for (std::size_t i = 1; i < stops.size(); ++i) {
        const GradientStop& a = stops[i - 1];
        const GradientStop& b = stops[i];
        if (t <= b.position) {
            const double span = b.position - a.position;
            const double f = span > 0.0 ? (t - a.position) / span : 1.0;
            const auto mix = [f](std::uint8_t x, std::uint8_t y) {
                return static_cast<std::uint8_t>(std::lround(x + (y - x) * f));
            };
            return {mix(a.color.r, b.color.r), mix(a.color.g, b.color.g), mix(a.color.b, b.color.b), mix(a.color.a, b.color.a)};
        }
    }
    return stops.back().color;
}

//=============================================================================================================

Canvas::Canvas(double width, double height, std::optional<Color> background)
{
    m_list.width = width;
    m_list.height = height;
    m_list.background = background;
}

//=============================================================================================================

void Canvas::save()
{
    m_saved.push_back(m_state);
}

//=============================================================================================================

void Canvas::restore()
{
    if (!m_saved.empty()) {
        m_state = std::move(m_saved.back());
        m_saved.pop_back();
    }
}

//=============================================================================================================

void Canvas::translate(double dx, double dy)
{
    m_state.transform = Transform::translation(dx, dy) * m_state.transform;
}

//=============================================================================================================

void Canvas::rotate(double degrees)
{
    m_state.transform = Transform::rotation(degrees) * m_state.transform;
}

//=============================================================================================================

void Canvas::scale(double sx, double sy)
{
    m_state.transform = Transform::scaling(sx, sy) * m_state.transform;
}

//=============================================================================================================

void Canvas::clip_rect(const Rect& rect)
{
    m_state.clips.push_back({rect, m_state.transform});
}

//=============================================================================================================

void Canvas::set_antialiasing(bool on)
{
    m_state.antialias = on;
}

//=============================================================================================================

void Canvas::set_opacity(double opacity)
{
    m_state.opacity = std::clamp(opacity, 0.0, 1.0);
}

//=============================================================================================================

void Canvas::set_pen(const Pen& pen)
{
    m_state.pen = pen;
}

//=============================================================================================================

void Canvas::set_pen(Color color)
{
    m_state.pen = Pen{color};
}

//=============================================================================================================

void Canvas::set_brush(const Brush& brush)
{
    m_state.brush = brush;
}

//=============================================================================================================

void Canvas::set_font(const Font& font)
{
    m_state.font = font;
}

//=============================================================================================================

void Canvas::add(Command::Shape shape)
{
    m_list.commands.push_back({std::move(shape), m_state.transform, m_state.clips, m_state.antialias, m_state.opacity});
}

//=============================================================================================================

void Canvas::draw_line(Point from, Point to)
{
    if (m_state.pen.visible()) {
        add(LineShape{from, to, m_state.pen});
    }
}

//=============================================================================================================

void Canvas::draw_polyline(std::span<const Point> points)
{
    if (points.size() >= 2 && m_state.pen.visible()) {
        add(PathShape{std::vector<Point>(points.begin(), points.end()), false, false, m_state.pen, Brush{}});
    }
}

//=============================================================================================================

void Canvas::draw_bezier(std::span<const Point> points)
{
    if (points.size() >= 4 && (points.size() - 1) % 3 == 0 && m_state.pen.visible()) {
        add(PathShape{std::vector<Point>(points.begin(), points.end()), false, true, m_state.pen, Brush{}});
    }
}

//=============================================================================================================

void Canvas::draw_polygon(std::span<const Point> points)
{
    if (points.size() >= 2 && (m_state.pen.visible() || m_state.brush.visible())) {
        add(PathShape{std::vector<Point>(points.begin(), points.end()), true, false, m_state.pen, m_state.brush});
    }
}

//=============================================================================================================

void Canvas::draw_rect(const Rect& rect)
{
    draw_rounded_rect(rect, 0.0, 0.0);
}

//=============================================================================================================

void Canvas::draw_rounded_rect(const Rect& rect, double rx, double ry)
{
    if (m_state.pen.visible() || m_state.brush.visible()) {
        add(RectShape{rect, rx, ry, m_state.pen, m_state.brush});
    }
}

//=============================================================================================================

void Canvas::draw_ellipse(Point center, double rx, double ry)
{
    if (m_state.pen.visible() || m_state.brush.visible()) {
        add(EllipseShape{center, rx, ry, m_state.pen, m_state.brush});
    }
}

//=============================================================================================================

void Canvas::fill_rect(const Rect& rect, const Brush& brush)
{
    if (brush.visible()) {
        add(RectShape{rect, 0.0, 0.0, Pen::none(), brush});
    }
}

//=============================================================================================================

void Canvas::draw_text(const Rect& box, Align align, std::string_view text)
{
    if (!text.empty()) {
        add(TextShape{box, align, std::string(text), m_state.font, m_state.pen.color});
    }
}

//=============================================================================================================

void Canvas::draw_text(Point baseline, std::string_view text)
{
    draw_text(Rect{baseline.x, baseline.y, 0.0, 0.0}, Align::Left | Align::Baseline, text);
}

//=============================================================================================================

void Canvas::draw_image(const Rect& target, std::shared_ptr<const Image> image, bool smooth)
{
    if (image && image->width > 0 && image->height > 0) {
        add(ImageShape{target, std::move(image), smooth});
    }
}

//=============================================================================================================

DisplayList Canvas::take()
{
    DisplayList list = std::move(m_list);
    m_list = DisplayList{list.width, list.height, list.background, {}};
    m_saved.clear();
    m_state = State{};
    return list;
}
