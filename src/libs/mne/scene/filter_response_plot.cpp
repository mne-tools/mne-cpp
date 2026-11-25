//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filter_response_plot.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of FilterResponsePlot.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filter_response_plot.h"
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

void FilterResponsePlot::set_response(std::vector<double> magnitude, double nyquist)
{
    m_magnitude = std::move(magnitude);
    m_nyquist = nyquist;
}

//=============================================================================================================

void FilterResponsePlot::set_cutoffs(std::vector<double> frequencies)
{
    m_cutoffs = std::move(frequencies);
}

//=============================================================================================================

void FilterResponsePlot::set_title(std::string title)
{
    m_title = std::move(title);
}

//=============================================================================================================

void FilterResponsePlot::set_floor_db(double db)
{
    m_floorDb = std::min(db, -1.0);
}

//=============================================================================================================

void FilterResponsePlot::set_colors(Color foreground, Color cutoff)
{
    m_foreground = foreground;
    m_cutoff = cutoff;
}

//=============================================================================================================

std::vector<double> FilterResponsePlot::magnitude_db() const
{
    double largest = 0.0;
    for (double value : m_magnitude) {
        largest = std::max(largest, std::abs(value));
    }
    std::vector<double> db;
    db.reserve(m_magnitude.size());
    for (double value : m_magnitude) {
        const double relative = largest > 0.0 ? std::abs(value) / largest : 0.0;
        db.push_back(relative > 0.0 ? std::max(20.0 * std::log10(relative), m_floorDb) : m_floorDb);
    }
    return db;
}

//=============================================================================================================

void FilterResponsePlot::draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    const double width = canvas.width();
    const double height = canvas.height();
    const double line = metrics.height(font);
    const std::vector<double> yTicks = nice_ticks(m_floorDb, 0.0, 6);
    const double nyquist = m_nyquist > 0.0 ? m_nyquist : 1.0;
    const std::vector<double> xTicks = nice_ticks(0.0, nyquist, 6);
    const auto dbLabel = [](double v) {
        return tick_label(v) + " dB";
    };
    const auto hzLabel = [](double v) {
        return tick_label(v) + " Hz";
    };

    double tickWidth = 0.0;
    for (double v : yTicks) {
        tickWidth = std::max(tickWidth, metrics.advance(font, dbLabel(v)));
    }
    const double lastLabel = xTicks.empty() ? 0.0 : metrics.advance(font, hzLabel(xTicks.back())) / 2.0;
    const double mLeft = tickWidth + 14;
    const double mRight = std::max(16.0, lastLabel + 4);
    const double mTop = m_title.empty() ? line : 2 * line + 8;
    const double mBottom = line + 14;
    const Rect area{mLeft, mTop, width - mLeft - mRight, height - mTop - mBottom};
    if (area.empty()) {
        return;
    }
    const auto toPixelX = [&](double hz) {
        return area.left() + hz / nyquist * area.width;
    };
    const auto toPixelY = [&](double db) {
        return area.top() + db / m_floorDb * area.height;
    };

    canvas.save();
    canvas.set_antialiasing(true);
    if (!m_title.empty()) {
        Font titleFont = scaled_font(font, 1.25, metrics);
        titleFont.bold = true;
        canvas.set_font(titleFont);
        canvas.set_pen(m_foreground);
        canvas.draw_text(Rect{0, 4, width, mTop - 4}, Align::Center, m_title);
    }

    Color grid = m_foreground;
    grid.a = 90;
    canvas.set_font(font);
    for (double v : yTicks) {
        const double py = toPixelY(v);
        canvas.set_pen(Pen{grid, 1, LineStyle::Dot, LineCap::Flat});
        canvas.draw_line({area.left(), py}, {area.right(), py});
        canvas.set_pen(m_foreground);
        canvas.draw_text(Rect{0, py - line, area.left() - 8, 2 * line}, Align::Right | Align::VCenter, dbLabel(v));
    }
    for (double v : xTicks) {
        const double px = toPixelX(v);
        canvas.set_pen(Pen{grid, 1, LineStyle::Dot, LineCap::Flat});
        canvas.draw_line({px, area.top()}, {px, area.bottom()});
        canvas.set_pen(m_foreground);
        canvas.draw_text(Rect{px - 60, area.bottom() + 6, 120, line}, Align::HCenter | Align::Top, hzLabel(v));
    }

    canvas.save();
    canvas.clip_rect(area);
    const std::vector<double> db = magnitude_db();
    if (db.size() >= 2) {
        const double last = static_cast<double>(db.size() - 1);
        std::vector<Point> points;
        if (db.size() <= static_cast<std::size_t>(2 * area.width)) {
            points.reserve(db.size());
            for (std::size_t i = 0; i < db.size(); ++i) {
                points.push_back({toPixelX(static_cast<double>(i) / last * nyquist), toPixelY(db[i])});
            }
        } else {
            // One pixel column per step: its lowest and highest value, in the order they occur
            const int columns = std::max(1, static_cast<int>(area.width));
            points.reserve(2 * static_cast<std::size_t>(columns));
            for (int c = 0; c < columns; ++c) {
                const auto from = static_cast<std::size_t>(static_cast<double>(c) / columns * last);
                const auto to = std::max(from + 1, static_cast<std::size_t>(static_cast<double>(c + 1) / columns * last));
                const auto [low, high] = std::minmax_element(db.begin() + static_cast<std::ptrdiff_t>(from),
                                                             db.begin() + static_cast<std::ptrdiff_t>(std::min(to, db.size())));
                const double px = area.left() + (c + 0.5);
                const bool lowFirst = low < high;
                points.push_back({px, toPixelY(lowFirst ? *low : *high)});
                points.push_back({px, toPixelY(lowFirst ? *high : *low)});
            }
        }
        canvas.set_pen(Pen{m_foreground, 2, LineStyle::Solid, LineCap::Round, LineJoin::Round});
        canvas.draw_polyline(points);
    }
    canvas.set_pen(Pen{m_cutoff, 2, LineStyle::Solid, LineCap::Flat});
    for (double hz : m_cutoffs) {
        if (hz >= 0.0 && hz <= nyquist) {
            canvas.draw_line({toPixelX(hz), area.top()}, {toPixelX(hz), area.bottom()});
        }
    }
    canvas.restore();

    canvas.set_pen(Pen{m_foreground, 1});
    canvas.set_brush(Brush{});
    canvas.draw_rect(area);
    canvas.restore();
}

//=============================================================================================================

DisplayList FilterResponsePlot::render(double width, double height, const TextMetrics& metrics, const Font& font) const
{
    Canvas canvas(width, height, Color::white());
    draw(canvas, metrics, font);
    return canvas.take();
}
