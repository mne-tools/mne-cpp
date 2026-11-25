//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bar_chart.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of BarChart.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bar_chart.h"
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

void BarChart::set_title(std::string title)
{
    m_title = std::move(title);
}

//=============================================================================================================

void BarChart::set_x_label(std::string label)
{
    m_xLabel = std::move(label);
}

//=============================================================================================================

void BarChart::set_y_label(std::string label)
{
    m_yLabel = std::move(label);
}

//=============================================================================================================

void BarChart::set_bars(std::vector<std::string> categories, std::vector<int> counts)
{
    m_categories = std::move(categories);
    m_counts = std::move(counts);
}

//=============================================================================================================

DisplayList BarChart::render(double width, double height, const TextMetrics& metrics, const Font& font) const
{
    Canvas canvas(width, height, Color::white());
    draw(canvas, metrics, font);
    return canvas.take();
}

//=============================================================================================================

void BarChart::draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    const double w = canvas.width();
    const double h = canvas.height();
    const double marginLeft = 60;
    const double marginRight = 20;
    const double marginTop = 40;
    const double marginBottom = 100;
    const double plotW = w - marginLeft - marginRight;
    const double plotH = h - marginTop - marginBottom;
    if (plotW <= 0 || plotH <= 0) {
        return;
    }
    canvas.save();
    canvas.set_antialiasing(true);
    canvas.set_font(font);

    if (!m_title.empty()) {
        canvas.set_pen(Color::black());
        Font titleFont = resized_font(font, 2, metrics);
        titleFont.bold = true;
        canvas.set_font(titleFont);
        canvas.draw_text(Rect{0, 0, w, marginTop}, Align::Center, m_title);
        canvas.set_font(font);
    }

    if (m_counts.empty()) {
        canvas.restore();
        return;
    }

    const int nBars = static_cast<int>(m_counts.size());
    const int maxCount = std::max(1, *std::max_element(m_counts.begin(), m_counts.end()));

    // Axes
    canvas.set_pen(Pen{Color::black(), 1});
    canvas.draw_line({marginLeft, marginTop}, {marginLeft, marginTop + plotH});
    canvas.draw_line({marginLeft, marginTop + plotH}, {marginLeft + plotW, marginTop + plotH});

    for (double value : nice_ticks(0.0, maxCount, 5, 1.0)) {
        const double y = marginTop + plotH - plotH * value / maxCount;
        canvas.draw_line({marginLeft - 5, y}, {marginLeft, y});
        canvas.draw_text(Rect{0, y - 10, marginLeft - 8, 20}, Align::Right | Align::VCenter, tick_label(value));
    }

    const double barWidth = plotW / nBars;
    const double barPadding = barWidth * 0.1;
    for (int i = 0; i < nBars; ++i) {
        const double barH = (static_cast<double>(m_counts[static_cast<std::size_t>(i)]) / maxCount) * plotH;
        canvas.set_brush(Brush::solid({70, 130, 180}));
        canvas.set_pen(Pen{{50, 100, 150}, 1});
        canvas.draw_rect({marginLeft + i * barWidth + barPadding, marginTop + plotH - barH, barWidth - 2.0 * barPadding, barH});
    }

    // Category labels, rotated when wider than their bar
    canvas.set_pen(Color::black());
    const Font small = resized_font(font, -1, metrics);
    canvas.set_font(small);
    for (int i = 0; i < nBars && i < static_cast<int>(m_categories.size()); ++i) {
        const std::string& category = m_categories[static_cast<std::size_t>(i)];
        const double cx = marginLeft + (i + 0.5) * barWidth;
        if (metrics.advance(small, category) < barWidth - 4) {
            canvas.draw_text(Rect{cx - barWidth / 2.0, marginTop + plotH + 6, barWidth, 20}, Align::HCenter | Align::Top, category);
            continue;
        }
        canvas.save();
        canvas.translate(cx, marginTop + plotH + 5);
        canvas.rotate(45);
        canvas.draw_text(Point{0, 0}, category);
        canvas.restore();
    }

    canvas.set_font(font);
    if (!m_xLabel.empty()) {
        canvas.draw_text(Rect{marginLeft, h - 22, plotW, 20}, Align::Center, m_xLabel);
    }
    if (!m_yLabel.empty()) {
        canvas.save();
        canvas.translate(4, marginTop + plotH / 2.0);
        canvas.rotate(-90);
        canvas.draw_text(Rect{-plotH / 2.0, 0, plotH, 20}, Align::Center, m_yLabel);
        canvas.restore();
    }
    canvas.restore();
}
