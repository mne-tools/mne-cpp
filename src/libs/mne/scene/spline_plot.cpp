//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     spline_plot.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of SplinePlot.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spline_plot.h"
#include "colormap.h"
#include "text_metrics.h"
#include "ticks.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void SplinePlot::set_title(std::string title)
{
    m_title = std::move(title);
}

//=============================================================================================================

void SplinePlot::set_x_label(std::string label)
{
    m_xLabel = std::move(label);
}

//=============================================================================================================

void SplinePlot::set_y_label(std::string label)
{
    m_yLabel = std::move(label);
}

//=============================================================================================================

void SplinePlot::set_classes(std::vector<Point> classMarks, double minX, double maxX)
{
    m_marks = std::move(classMarks);
    m_minX = minX;
    m_maxX = maxX;
    m_left = minX;
    m_middle = maxX;
    m_right = maxX;
    m_hasThresholds = false;
}

//=============================================================================================================

void SplinePlot::set_thresholds(double left, double middle, double right)
{
    m_left = left;
    m_middle = middle;
    m_right = right;
    m_hasThresholds = true;
}

//=============================================================================================================

void SplinePlot::set_colormap(std::string name)
{
    m_colormap = std::move(name);
}

//=============================================================================================================

double SplinePlot::max_count() const
{
    double count = 0.0;
    for (const Point& mark : m_marks) {
        count = std::max(count, mark.y);
    }
    return count;
}

//=============================================================================================================

double SplinePlot::data_to_pixel_x(double x, double width) const
{
    const double plotW = width - kLeftMargin - kRightMargin;
    if (m_maxX == m_minX) {
        return kLeftMargin;
    }
    return kLeftMargin + (x - m_minX) / (m_maxX - m_minX) * plotW;
}

//=============================================================================================================

double SplinePlot::data_to_pixel_y(double y, double height) const
{
    const double plotH = height - kTopMargin - kBottomMargin;
    // Headroom for the curve, which overshoots between the class marks
    const double count = max_count();
    const double maxY = count > 0 ? 1.1 * count : 1.0;
    return kTopMargin + plotH - (y / maxY) * plotH;
}

//=============================================================================================================

double SplinePlot::pixel_to_data_x(double px, double width) const
{
    const double plotW = width - kLeftMargin - kRightMargin;
    if (plotW <= 0) {
        return m_minX;
    }
    return m_minX + (px - kLeftMargin) / plotW * (m_maxX - m_minX);
}

//=============================================================================================================

DisplayList SplinePlot::render(double width, double height, const TextMetrics& metrics, const Font& font) const
{
    Canvas canvas(width, height, Color::white());
    draw(canvas, metrics, font);
    return canvas.take();
}

//=============================================================================================================

void SplinePlot::draw(Canvas& canvas, const TextMetrics& /*metrics*/, const Font& font) const
{
    const double w = canvas.width();
    const double h = canvas.height();
    const double mLeft = kLeftMargin;
    const double mTop = kTopMargin;
    const double plotW = w - kLeftMargin - kRightMargin;
    const double plotH = h - kTopMargin - kBottomMargin;
    if (plotW <= 0 || plotH <= 0) {
        return;
    }
    canvas.save();
    canvas.set_antialiasing(true);
    canvas.set_font(font);

    // Colour map between the thresholds
    if (m_hasThresholds && !m_colormap.empty() && m_maxX > m_minX) {
        const double leftNorm = (m_left - m_minX) / (m_maxX - m_minX);
        const double middleNorm = (m_middle - m_minX) / (m_maxX - m_minX);
        const double rightNorm = (m_right - m_minX) / (m_maxX - m_minX);
        std::vector<GradientStop> stops;
        const auto stop = [&](double position, double value) {
            const double at = std::clamp(position, 0.0, 1.0);
            // A later stop at the same position replaces the earlier one, as in QGradient::setColorAt
            std::erase_if(stops, [at](const GradientStop& s) { return s.position == at; });
            stops.push_back({at, map_color(m_colormap, value)});
        };
        const int steps = 25;
        const double stepLeftMiddle = (middleNorm - leftNorm) / steps;
        const double stepMiddleRight = (rightNorm - middleNorm) / steps;
        stop(leftNorm, 0.0);
        for (int i = 1; i < steps; ++i) {
            stop(leftNorm + stepLeftMiddle * i, i * (0.5 / steps));
            stop(middleNorm + stepMiddleRight * i, 0.5 + i * (0.5 / steps));
        }
        stop(rightNorm, 1.0);
        std::stable_sort(stops.begin(), stops.end(), [](const GradientStop& a, const GradientStop& b) { return a.position < b.position; });
        canvas.fill_rect({mLeft, mTop, plotW, plotH}, Brush::linear_gradient({mLeft, 0}, {mLeft + plotW, 0}, std::move(stops)));
    }

    // Axes
    canvas.set_pen(Pen{Color::black(), 1});
    canvas.draw_line({mLeft, mTop}, {mLeft, mTop + plotH});
    canvas.draw_line({mLeft, mTop + plotH}, {mLeft + plotW, mTop + plotH});

    const double count = max_count();
    for (double value : nice_ticks(0.0, count > 0 ? 1.1 * count : 1.0, 5, 1.0)) {
        const double y = data_to_pixel_y(value, h);
        canvas.draw_line({mLeft - 5, y}, {mLeft, y});
        canvas.draw_text(Rect{0, y - 10, mLeft - 8, 20}, Align::Right | Align::VCenter, tick_label(value));
    }
    for (double value : nice_ticks(m_minX, m_maxX, 6)) {
        const double x = data_to_pixel_x(value, w);
        canvas.draw_line({x, mTop + plotH}, {x, mTop + plotH + 5});
        canvas.draw_text(Rect{x - 40, mTop + plotH + 6, 80, 20}, Align::Center, tick_label(value));
    }

    if (!m_title.empty()) {
        Font titleFont = font;
        titleFont.bold = true;
        canvas.set_font(titleFont);
        canvas.draw_text(Rect{0, 0, w, mTop}, Align::Center, m_title);
        canvas.set_font(font);
    }
    if (!m_xLabel.empty()) {
        canvas.draw_text(Rect{mLeft, h - 22, plotW, 20}, Align::Center, m_xLabel);
    }
    if (!m_yLabel.empty()) {
        canvas.save();
        canvas.translate(4, mTop + plotH / 2.0);
        canvas.rotate(-90);
        canvas.draw_text(Rect{-plotH / 2.0, 0, plotH, 20}, Align::Center, m_yLabel);
        canvas.restore();
    }

    if (m_marks.empty()) {
        canvas.restore();
        return;
    }
    const auto pixel = [&](std::ptrdiff_t index) {
        index = std::clamp<std::ptrdiff_t>(index, 0, std::ssize(m_marks) - 1);
        const Point& mark = m_marks[static_cast<std::size_t>(index)];
        return Point{data_to_pixel_x(mark.x, w), data_to_pixel_y(mark.y, h)};
    };
    if (m_marks.size() == 1) {
        canvas.set_brush(Brush::solid({70, 130, 180}));
        canvas.draw_ellipse(pixel(0), 3, 3);
    } else {
        // Catmull-Rom spline as cubic Bezier segments
        std::vector<Point> curve{pixel(0)};
        for (std::ptrdiff_t i = 0; i + 1 < std::ssize(m_marks); ++i) {
            const Point p0 = pixel(i - 1);
            const Point p1 = pixel(i);
            const Point p2 = pixel(i + 1);
            const Point p3 = pixel(i + 2);
            curve.push_back(p1 + (p2 - p0) * (1.0 / 6.0));
            curve.push_back(p2 - (p3 - p1) * (1.0 / 6.0));
            curve.push_back(p2);
        }
        canvas.set_pen(Pen{{70, 130, 180}, 2});
        canvas.set_brush(Brush{});
        canvas.draw_bezier(curve);
    }

    if (m_hasThresholds) {
        const auto thresholdLine = [&](double x, Color color) {
            if (x >= m_minX && x <= m_maxX) {
                const double px = data_to_pixel_x(x, w);
                canvas.set_pen(Pen{color, 2});
                canvas.draw_line({px, mTop}, {px, mTop + plotH});
            }
        };
        thresholdLine(m_left, {255, 0, 0});
        thresholdLine(m_middle, {0, 255, 0});
        thresholdLine(m_right, {0, 0, 255});
    }
    canvas.restore();
}
