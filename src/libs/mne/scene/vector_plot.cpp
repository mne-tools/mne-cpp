//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     vector_plot.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of VectorPlot.
 *
 * Positions use the integer arithmetic of the QPainter code this replaces, so the QPainter backend draws the
 * same pixels as before.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "vector_plot.h"
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

void VectorPlot::set_data(const std::vector<double>& values)
{
    if (values.empty()) {
        return;
    }
    m_data.clear();
    m_minY = m_maxY = m_maxX = 0.0;
    add_data(values);
}

//=============================================================================================================

void VectorPlot::add_data(const std::vector<double>& values)
{
    if (values.empty()) {
        return;
    }
    const auto [lo, hi] = std::minmax_element(values.begin(), values.end());
    m_maxX = std::max(m_maxX, static_cast<double>(values.size() - 1));
    m_minY = std::min(m_minY, *lo);
    m_maxY = std::max(m_maxY, *hi);
    m_data.push_back(values);
}

//=============================================================================================================

void VectorPlot::set_title(std::string title)
{
    m_title = std::move(title);
}

//=============================================================================================================

void VectorPlot::set_x_label(std::string label)
{
    m_xLabel = std::move(label);
}

//=============================================================================================================

void VectorPlot::set_y_label(std::string label)
{
    m_yLabel = std::move(label);
}

//=============================================================================================================

Font VectorPlot::axes_font()
{
    return Font{"", 0.0, 12.0};
}

//=============================================================================================================

Font VectorPlot::title_font()
{
    return Font{"", 0.0, 20.0, true};
}

//=============================================================================================================

DisplayList VectorPlot::render(double width, double height, const TextMetrics& metrics) const
{
    Canvas canvas(width, height, Color::white());
    draw(canvas, metrics);
    return canvas.take();
}

//=============================================================================================================

void VectorPlot::draw(Canvas& canvas, const TextMetrics& metrics) const
{
    if (m_data.empty()) {
        return;
    }
    const int widgetW = static_cast<int>(canvas.width());
    const int widgetH = static_cast<int>(canvas.height());
    const int border = static_cast<int>(kBorderLeftRight);
    const int borderTB = static_cast<int>(kBorderTopBottom);
    const double left = border;
    const double top = borderTB;
    const double plotW = widgetW - 2 * border;
    const double plotH = widgetH - 2 * borderTB;

    canvas.save();
    canvas.fill_rect({left, top, plotW, plotH}, Brush::solid(Color::white()));
    canvas.set_pen(Color::black());
    canvas.draw_rect({left - 1, top - 1, plotW + 1, plotH + 1});

    // Data: x from 0 to the last index over the width, y upwards over 90 % of the height
    canvas.set_pen(Pen{{0, 0, 255}, 1});
    const double minX = 0.0;
    const double rangeX = m_maxX > minX ? m_maxX - minX : 1.0;
    const double rangeY = m_maxY > m_minY ? m_maxY - m_minY : 1.0;
    const double scaleX = plotW / rangeX;
    const double scaleY = 0.9 * plotH / rangeY;
    const double bottom = top + 0.95 * plotH;
    for (const std::vector<double>& values : m_data) {
        for (std::size_t k = 1; k < values.size(); ++k) {
            canvas.draw_line({left + (k - 1 - minX) * scaleX, bottom - (values[k - 1] - m_minY) * scaleY},
                             {left + (k - minX) * scaleX, bottom - (values[k] - m_minY) * scaleY});
        }
    }

    // Ticks
    const Font axes = axes_font();
    canvas.set_pen(Color::black());
    canvas.set_font(axes);
    const double line = metrics.height(axes);
    const double frameBottom = top + plotH;
    for (double v : nice_ticks(minX, minX + rangeX, 8)) {
        const double px = left + (v - minX) * scaleX;
        canvas.draw_line({px, frameBottom}, {px, frameBottom + 4});
        canvas.draw_text(Rect{px - 50, frameBottom + 5, 100, line}, Align::HCenter | Align::Top, tick_label(v));
    }
    const double padY = 0.05 * plotH / scaleY;
    for (double v : nice_ticks(m_minY - padY, m_minY + rangeY + padY, 6)) {
        const double py = bottom - (v - m_minY) * scaleY;
        canvas.draw_line({left - 5, py}, {left - 1, py});
        canvas.draw_text(Rect{0, py - line, left - 8, 2 * line}, Align::Right | Align::VCenter, tick_label(v));
    }

    // Title and axis labels in boxes as wide as the frame and 100 pixels high
    const int labelWidth = widgetW - 2 * border;
    const int labelHeight = 100;
    if (!m_title.empty()) {
        canvas.save();
        canvas.set_font(title_font());
        canvas.translate((widgetW - labelWidth) / 2, (widgetH - static_cast<int>(plotH)) / 2 - borderTB * 1.5);
        canvas.draw_text(Rect{0, 0, static_cast<double>(labelWidth), labelHeight}, Align::Center, m_title);
        canvas.restore();
    }
    if (!m_xLabel.empty()) {
        canvas.save();
        // Centred 12 px above the bottom edge, below the tick labels
        canvas.translate((widgetW - labelWidth) / 2, widgetH - 12 - labelHeight / 2);
        canvas.draw_text(Rect{0, 0, static_cast<double>(labelWidth), labelHeight}, Align::Center, m_xLabel);
        canvas.restore();
    }
    if (!m_yLabel.empty()) {
        canvas.save();
        canvas.rotate(270);
        // Centred 16 px from the left edge, left of the tick labels
        canvas.translate(-(widgetH + labelWidth) / 2, 16 - labelHeight / 2);
        canvas.draw_text(Rect{0, 0, static_cast<double>(labelWidth), labelHeight}, Align::Center, m_yLabel);
        canvas.restore();
    }
    canvas.restore();
}
