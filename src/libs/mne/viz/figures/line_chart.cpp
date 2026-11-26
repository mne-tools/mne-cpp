//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     line_chart.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of LineChart.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "line_chart.h"
#include "text_metrics.h"
#include "ticks.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

namespace
{

constexpr std::array<Color, 10> kPalette = {Color{70, 130, 180}, Color{221, 132, 82}, Color{85, 168, 104}, Color{196, 78, 82},
                                            Color{129, 114, 179}, Color{147, 120, 96}, Color{218, 139, 195}, Color{140, 140, 140},
                                            Color{204, 185, 116}, Color{100, 181, 205}};

bool contains(const Rect& r, Point p)
{
    return p.x >= r.left() && p.x <= r.right() && p.y >= r.top() && p.y <= r.bottom();
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void LineChart::set_title(std::string title)
{
    m_title = std::move(title);
}

//=============================================================================================================

void LineChart::set_x_label(std::string label)
{
    m_xLabel = std::move(label);
}

//=============================================================================================================

void LineChart::set_y_label(std::string label)
{
    m_yLabel = std::move(label);
}

//=============================================================================================================

void LineChart::set_data(const std::vector<double>& y)
{
    std::vector<double> x(y.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        x[i] = static_cast<double>(i);
    }
    set_data(x, y);
}

//=============================================================================================================

void LineChart::set_data(const std::vector<double>& x, const std::vector<double>& y)
{
    m_series.clear();
    add_series(x, y);
}

//=============================================================================================================

void LineChart::add_series(const std::vector<double>& x, const std::vector<double>& y, std::string label, std::optional<Color> color,
                           LineStyle style)
{
    const std::size_t n = std::min(x.size(), y.size());
    const bool empty = std::all_of(m_series.cbegin(), m_series.cend(), [](const Series& s) { return s.x.empty(); });
    for (std::size_t i = 0; i < n; ++i) {
        const bool first = empty && i == 0;
        m_minX = first ? x[i] : std::min(m_minX, x[i]);
        m_maxX = first ? x[i] : std::max(m_maxX, x[i]);
        m_minY = first ? y[i] : std::min(m_minY, y[i]);
        m_maxY = first ? y[i] : std::max(m_maxY, y[i]);
    }
    Color lineColor = kPalette[m_series.size() % kPalette.size()];
    if (color) {
        lineColor = *color;
    } else {
        for (const Color& candidate : kPalette) {
            if (std::none_of(m_series.cbegin(), m_series.cend(), [&](const Series& s) { return s.color == candidate; })) {
                lineColor = candidate;
                break;
            }
        }
    }
    m_series.push_back({std::vector<double>(x.begin(), x.begin() + static_cast<std::ptrdiff_t>(n)),
                        std::vector<double>(y.begin(), y.begin() + static_cast<std::ptrdiff_t>(n)), std::move(label), lineColor, style});
}

//=============================================================================================================

void LineChart::clear()
{
    m_series.clear();
    m_minX = m_maxX = m_minY = m_maxY = 0.0;
}

//=============================================================================================================

DisplayList LineChart::render(double width, double height, const TextMetrics& metrics, const Font& font) const
{
    Canvas canvas(width, height, Color::white());
    draw(canvas, metrics, font);
    return canvas.take();
}

//=============================================================================================================

void LineChart::draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    const double width = canvas.width();
    const double height = canvas.height();
    canvas.save();
    canvas.set_antialiasing(true);
    const Color text{51, 51, 51};
    const Color grid{230, 230, 230};

    double minY = m_minY;
    double maxY = m_maxY;
    const double padY = (maxY - minY) * 0.05;
    minY -= padY == 0.0 ? 1.0 : padY;
    maxY += padY == 0.0 ? 1.0 : padY;
    const double minX = m_minX;
    const double maxX = m_maxX > m_minX ? m_maxX : m_minX + 1.0;

    const std::vector<double> yTicks = nice_ticks(minY, maxY, 6);
    const std::vector<double> xTicks = nice_ticks(minX, maxX, 8);
    double tickWidth = 0.0;
    for (double v : yTicks) {
        tickWidth = std::max(tickWidth, metrics.advance(font, tick_label(v)));
    }
    const double line = metrics.height(font);
    const double mLeft = tickWidth + 14 + (m_yLabel.empty() ? 0 : line + 6);
    const double mRight = 24;
    const double mTop = m_title.empty() ? 16 : 2 * line + 8;
    const double mBottom = line + 14 + (m_xLabel.empty() ? 0 : line + 6);
    Rect area{mLeft, mTop, width - mLeft - mRight, height - mTop - mBottom};
    if (area.empty()) {
        canvas.restore();
        return;
    }

    const auto toPixelX = [&](double dx) {
        return area.left() + (dx - minX) / (maxX - minX) * area.width;
    };
    const auto toPixelY = [&](double dy) {
        return area.bottom() - (dy - minY) / (maxY - minY) * area.height;
    };
    // Length of the lines inside a rectangle, sampled every 2 pixels
    const auto hiddenLength = [&](const Rect& rect) {
        double hidden = 0.0;
        for (const Series& series : m_series) {
            for (std::size_t i = 1; i < series.x.size(); ++i) {
                const Point a{toPixelX(series.x[i - 1]), toPixelY(series.y[i - 1])};
                const Point b{toPixelX(series.x[i]), toPixelY(series.y[i])};
                const double length = std::hypot(b.x - a.x, b.y - a.y);
                const int steps = std::max(1, static_cast<int>(length / 2.0));
                for (int s = 0; s <= steps; ++s) {
                    hidden += contains(rect, a + (b - a) * (static_cast<double>(s) / steps)) ? length / (steps + 1) : 0.0;
                }
            }
        }
        return hidden;
    };

    // Legend: in the corner of the axes that hides the least of the lines, else to the right of them
    double labelWidth = 0.0;
    int labelCount = 0;
    for (const Series& series : m_series) {
        if (!series.label.empty()) {
            labelWidth = std::max(labelWidth, metrics.advance(font, series.label));
            ++labelCount;
        }
    }
    const double legendWidth = labelWidth + 38;
    const double legendHeight = labelCount * (line + 2) + 10;
    Rect legend;
    if (labelCount > 0) {
        const std::array<Rect, 4> corners = {Rect{area.right() - legendWidth - 8, area.top() + 8, legendWidth, legendHeight},
                                             Rect{area.left() + 8, area.top() + 8, legendWidth, legendHeight},
                                             Rect{area.right() - legendWidth - 8, area.bottom() - legendHeight - 8, legendWidth, legendHeight},
                                             Rect{area.left() + 8, area.bottom() - legendHeight - 8, legendWidth, legendHeight}};
        double fewest = std::numeric_limits<double>::max();
        for (const Rect& corner : corners) {
            const double hidden = hiddenLength(corner);
            if (hidden < fewest) {
                fewest = hidden;
                legend = corner;
            }
        }
        if (fewest > 0.0 && area.width > 2 * legendWidth) {
            area.width -= legendWidth + 12;
            legend = Rect{area.right() + 12, area.top(), legendWidth, legendHeight};
        }
    }

    if (!m_title.empty()) {
        Font titleFont = scaled_font(font, 1.25, metrics);
        titleFont.bold = true;
        canvas.set_font(titleFont);
        canvas.set_pen(text);
        canvas.draw_text(Rect{0, 4, width, mTop - 4}, Align::Center, m_title);
    }

    canvas.set_font(font);
    for (double v : yTicks) {
        const double py = toPixelY(v);
        canvas.set_pen(Pen{grid, 1});
        canvas.draw_line({area.left(), py}, {area.right(), py});
        canvas.set_pen(text);
        canvas.draw_text(Rect{0, py - line, area.left() - 8, 2 * line}, Align::Right | Align::VCenter, tick_label(v));
    }
    for (double v : xTicks) {
        const double px = toPixelX(v);
        canvas.set_pen(Pen{grid, 1});
        canvas.draw_line({px, area.top()}, {px, area.bottom()});
        canvas.set_pen(text);
        canvas.draw_text(Rect{px - 50, area.bottom() + 6, 100, line}, Align::HCenter | Align::Top, tick_label(v));
    }

    if (!m_xLabel.empty()) {
        canvas.draw_text(Rect{area.left(), height - line - 6, area.width, line}, Align::Center, m_xLabel);
    }
    if (!m_yLabel.empty()) {
        canvas.save();
        canvas.translate(6, area.center().y);
        canvas.rotate(-90);
        canvas.draw_text(Rect{-area.height / 2, 0, area.height, line}, Align::Center, m_yLabel);
        canvas.restore();
    }

    canvas.save();
    canvas.clip_rect(area);
    const double penWidth = m_series.size() > 20 ? 1.0 : 1.75;
    for (const Series& series : m_series) {
        if (series.x.size() < 2) {
            continue;
        }
        std::vector<Point> points;
        points.reserve(series.x.size());
        for (std::size_t i = 0; i < series.x.size(); ++i) {
            points.push_back({toPixelX(series.x[i]), toPixelY(series.y[i])});
        }
        canvas.set_pen(Pen{series.color, penWidth, series.style, series.style == LineStyle::Solid ? LineCap::Round : LineCap::Flat, LineJoin::Round});
        canvas.draw_polyline(points);
    }
    canvas.restore();

    canvas.set_pen(Pen{{136, 136, 136}, 1});
    canvas.set_brush(Brush{});
    canvas.draw_rect(area);

    if (labelCount > 0) {
        canvas.set_pen(Pen{{204, 204, 204}, 1});
        canvas.set_brush(Brush::solid({255, 255, 255, 225}));
        canvas.draw_rounded_rect(legend, 4, 4);
        int row = 0;
        for (const Series& series : m_series) {
            if (series.label.empty()) {
                continue;
            }
            const double y = legend.top() + 5 + row++ * (line + 2) + line / 2.0;
            canvas.set_pen(Pen{series.color, 2.5, series.style, series.style == LineStyle::Solid ? LineCap::Round : LineCap::Flat});
            canvas.draw_line({legend.left() + 8, y}, {legend.left() + 26, y});
            canvas.set_pen(text);
            canvas.draw_text(Rect{legend.left() + 32, y - line / 2.0, labelWidth + 4, line}, Align::Left | Align::VCenter, series.label);
        }
    }
    canvas.restore();
}
