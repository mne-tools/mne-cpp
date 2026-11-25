//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     image_plot.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of ImagePlot.
 *
 * Rectangles follow the integer conventions of the QPainter code this replaces (QRect: right = left + width - 1),
 * so the QPainter backend draws the same pixels as before.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "image_plot.h"
#include "colormap.h"
#include "text_metrics.h"
#include "ticks.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ImagePlot::ImagePlot() = default;

//=============================================================================================================

void ImagePlot::set_data(int rows, int cols, std::vector<double> values)
{
    if (rows <= 0 || cols <= 0 || values.size() != static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols)) {
        return;
    }
    m_rows = rows;
    m_cols = cols;
    m_values = std::move(values);
    if (!m_rangeSet) {
        const auto [lo, hi] = std::minmax_element(m_values.begin(), m_values.end());
        m_min = *lo;
        m_max = *hi;
    }
    update();
}

//=============================================================================================================

void ImagePlot::set_colormap(std::string name)
{
    m_colormap = std::move(name);
    update();
}

//=============================================================================================================

void ImagePlot::set_color_range(double min, double max)
{
    m_min = min;
    m_max = max;
    m_rangeSet = true;
    update();
}

//=============================================================================================================

void ImagePlot::set_extent(double left, double right, double bottom, double top)
{
    m_left = left;
    m_right = right;
    m_bottom = bottom;
    m_top = top;
    m_extentSet = true;
}

//=============================================================================================================

void ImagePlot::set_title(std::string title)
{
    m_title = std::move(title);
}

//=============================================================================================================

void ImagePlot::set_x_label(std::string label)
{
    m_xLabel = std::move(label);
}

//=============================================================================================================

void ImagePlot::set_y_label(std::string label)
{
    m_yLabel = std::move(label);
}

//=============================================================================================================

void ImagePlot::set_colorbar_label(std::string label)
{
    m_colorbarLabel = std::move(label);
}

//=============================================================================================================

void ImagePlot::update()
{
    if (!has_data()) {
        return;
    }
    m_normalized = m_values;
    for (double& v : m_normalized) {
        v -= m_min;
        // A constant matrix has no range: keep it at 0 instead of dividing 0 by 0
        if (m_max > m_min) {
            v /= m_max - m_min;
        }
        v = std::clamp(v, 0.0, 1.0);
    }
    const auto opaque = [](Color c) {
        c.a = 255;
        return c;
    };
    auto image = std::make_shared<Image>(Image::transparent(m_cols, m_rows));
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            image->set_pixel(c, r, opaque(map_color(m_colormap, m_normalized[static_cast<std::size_t>(r) * static_cast<std::size_t>(m_cols) + static_cast<std::size_t>(c)])));
        }
    }
    m_image = std::move(image);
    auto colorbar = std::make_shared<Image>(Image::transparent(1, m_colorbarSteps));
    for (int j = 0; j < m_colorbarSteps; ++j) {
        colorbar->set_pixel(0, j, opaque(map_color(m_colormap, static_cast<double>(m_colorbarSteps - 1 - j) / (m_colorbarSteps - 1))));
    }
    m_colorbar = std::move(colorbar);
}

//=============================================================================================================

DisplayList ImagePlot::render(double width, double height, const TextMetrics& metrics, const Font& font) const
{
    Canvas canvas(width, height, Color::white());
    draw(canvas, metrics, font);
    return canvas.take();
}

//=============================================================================================================

void ImagePlot::draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    if (!m_image) {
        return;
    }
    const double width = canvas.width();
    const double height = canvas.height();
    const Color text{51, 51, 51};
    const double line = metrics.height(font);

    // Column and row centres in data units: without an extent, column and row indices
    const double left = m_extentSet ? m_left : 0.0;
    const double right = m_extentSet ? m_right : m_cols - 1;
    const double top = m_extentSet ? m_top : 0.0;
    const double bottom = m_extentSet ? m_bottom : m_rows - 1;
    // Half a cell beyond the first and last centre, so each cell is centred on its value
    const double halfX = m_cols > 1 ? (right - left) / (m_cols - 1) / 2.0 : 0.5;
    const double halfY = m_rows > 1 ? (bottom - top) / (m_rows - 1) / 2.0 : 0.5;
    const double x0 = left - halfX;
    const double x1 = right + halfX;
    const double y0 = top - halfY;
    const double y1 = bottom + halfY;

    // No ticks between cell centres: whole numbers for indices, at least one cell apart in data units
    const std::vector<double> xTicks = nice_ticks(std::min(x0, x1), std::max(x0, x1), 8, std::fabs(2.0 * halfX));
    const std::vector<double> yTicks = nice_ticks(std::min(y0, y1), std::max(y0, y1), 6, std::fabs(2.0 * halfY));
    const std::vector<double> cTicks = nice_ticks(m_min, m_max, 6);
    double yTickWidth = 0.0;
    for (double v : yTicks) {
        yTickWidth = std::max(yTickWidth, metrics.advance(font, tick_label(v)));
    }
    double cTickWidth = 0.0;
    for (double v : cTicks) {
        cTickWidth = std::max(cTickWidth, metrics.advance(font, tick_label(v)));
    }
    const double mLeft = yTickWidth + 14 + (m_yLabel.empty() ? 0 : line + 6);
    const double mRight = 16 + m_colorbarWidth + 14 + cTickWidth + (m_colorbarLabel.empty() ? 0 : line + 6);
    const double mTop = m_title.empty() ? 12 : 2 * line + 8;
    const double mBottom = line + 14 + (m_xLabel.empty() ? 0 : line + 6);
    const double areaW = std::floor(width - mLeft - mRight);
    const double areaH = std::floor(height - mTop - mBottom);
    if (areaW <= 0 || areaH <= 0) {
        return;
    }
    const Rect area{mLeft, mTop, areaW, areaH};
    // Last pixel row and column, and the centre, as QRect reports them
    const double areaRight = area.x + areaW - 1;
    const double areaBottom = area.y + areaH - 1;
    const double areaCenterY = std::floor((area.y + areaBottom) / 2);

    const auto toPixelX = [&](double x) {
        return area.x + (x - x0) / (x1 - x0) * areaW;
    };
    const auto toPixelY = [&](double y) {
        return area.y + (y - y0) / (y1 - y0) * areaH;
    };

    canvas.save();
    canvas.set_font(font);
    canvas.draw_image(area, m_image);
    canvas.set_pen(Pen{{136, 136, 136}, 1});
    canvas.set_brush(Brush{});
    canvas.draw_rect(area.adjusted(0, 0, -1, -1));

    canvas.set_pen(text);
    for (double v : xTicks) {
        const double px = toPixelX(v);
        canvas.draw_line({px, areaBottom}, {px, areaBottom + 4});
        canvas.draw_text(Rect{px - 50, areaBottom + 6, 100, line}, Align::HCenter | Align::Top, tick_label(v));
    }
    for (double v : yTicks) {
        const double py = toPixelY(v);
        canvas.draw_line({area.x - 4, py}, {area.x, py});
        canvas.draw_text(Rect{0, py - line, area.x - 8, 2 * line}, Align::Right | Align::VCenter, tick_label(v));
    }

    const Rect bar{areaRight + 16, area.y, static_cast<double>(m_colorbarWidth), areaH};
    const double barRight = bar.x + bar.width - 1;
    const double barBottom = bar.y + bar.height - 1;
    canvas.draw_image(bar, m_colorbar);
    canvas.set_pen(Pen{{136, 136, 136}, 1});
    canvas.draw_rect(bar.adjusted(0, 0, -1, -1));
    canvas.set_pen(text);
    if (m_max > m_min) {
        for (double v : cTicks) {
            const double py = barBottom - (v - m_min) / (m_max - m_min) * bar.height;
            canvas.draw_line({barRight, py}, {barRight + 4, py});
            canvas.draw_text(Rect{barRight + 7, py - line, cTickWidth + 4, 2 * line}, Align::Left | Align::VCenter, tick_label(v));
        }
    }
    if (!m_colorbarLabel.empty()) {
        canvas.save();
        canvas.translate(barRight + 13 + cTickWidth, std::floor((bar.y + barBottom) / 2));
        canvas.rotate(-90);
        canvas.draw_text(Rect{-bar.height / 2.0, 0, bar.height, line}, Align::Center, m_colorbarLabel);
        canvas.restore();
    }

    if (!m_title.empty()) {
        Font titleFont = scaled_font(font, 1.25, metrics);
        titleFont.bold = true;
        canvas.set_font(titleFont);
        canvas.draw_text(Rect{0, 4, width, mTop - 4}, Align::Center, m_title);
        canvas.set_font(font);
    }
    if (!m_xLabel.empty()) {
        canvas.draw_text(Rect{area.x, height - line - 6, areaW, line}, Align::Center, m_xLabel);
    }
    if (!m_yLabel.empty()) {
        canvas.save();
        canvas.translate(6, areaCenterY);
        canvas.rotate(-90);
        canvas.draw_text(Rect{-areaH / 2.0, 0, areaH, line}, Align::Center, m_yLabel);
        canvas.restore();
    }
    canvas.restore();
}
