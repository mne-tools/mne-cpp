//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_markers.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of TraceMarkers.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "trace_markers.h"
#include "../figures/text_metrics.h"
#include <mne/base/string_utils.h>

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

namespace
{

Color with_alpha(Color color, std::uint8_t alpha)
{
    color.a = alpha;
    return color;
}

// Marker lines just outside the drawn width are still drawn, as their pixels may reach into it
bool outside(double x, double width)
{
    return x < -2.0 || x > width + 2.0;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void TraceMarkers::set_events(std::vector<Event> events)
{
    m_events = std::move(events);
}

//=============================================================================================================

void TraceMarkers::set_epoch_markers(std::vector<int> samples)
{
    m_epochMarkers = std::move(samples);
}

//=============================================================================================================

bool TraceMarkers::set_events_visible(bool visible)
{
    return std::exchange(m_eventsVisible, visible) != visible;
}

//=============================================================================================================

bool TraceMarkers::set_annotations_visible(bool visible)
{
    return std::exchange(m_annotationsVisible, visible) != visible;
}

//=============================================================================================================

bool TraceMarkers::set_epoch_markers_visible(bool visible)
{
    return std::exchange(m_epochMarkersVisible, visible) != visible;
}

//=============================================================================================================

void TraceMarkers::draw_annotations(Canvas& canvas, const TextMetrics& metrics, const Font& font,
                                    std::span<const TraceView::Annotation> annotations, double first_sample,
                                    double pixels_per_sample) const
{
    if (!m_annotationsVisible || annotations.empty()) {
        return;
    }
    const double width = canvas.width();
    const double height = canvas.height();
    Font labelFont = font;
    labelFont.point_size = 8.0;
    labelFont.pixel_size = 0.0;
    labelFont.bold = true;

    canvas.save();
    canvas.set_brush(Brush{});
    canvas.set_font(labelFont);
    for (const TraceView::Annotation& annotation : annotations) {
        const double xStart = (static_cast<double>(annotation.start_sample) - first_sample) * pixels_per_sample;
        const double xEnd = (static_cast<double>(annotation.end_sample + 1) - first_sample) * pixels_per_sample;
        if (xEnd < -2.0 || xStart > width + 2.0) {
            continue;
        }
        const double start = std::clamp(xStart, 0.0, width);
        const double end = std::clamp(xEnd, 0.0, width);
        if (end <= start) {
            continue;
        }

        canvas.fill_rect({start, 0.0, end - start, height}, Brush::solid(with_alpha(annotation.color, 48)));
        canvas.set_pen(Pen{with_alpha(annotation.color, 165), 1.0});
        canvas.draw_line({start, 0.0}, {start, height});
        canvas.draw_line({end, 0.0}, {end, height});

        const std::string_view label = mne::trimmed(annotation.label);
        if (!label.empty()) {
            // A pill 6 px wider and 2 px higher than the text on each side, 4 px from the top
            const int pillWidth = static_cast<int>(std::lround(metrics.advance(labelFont, label))) + 12;
            const int pillHeight = static_cast<int>(std::lround(metrics.height(labelFont))) + 4;
            const int labelX = std::max(4, std::min(static_cast<int>(start) + 4, std::max(4, static_cast<int>(width) - pillWidth - 4)));
            const Rect pill{static_cast<double>(labelX), 4.0, static_cast<double>(pillWidth), static_cast<double>(pillHeight)};
            canvas.fill_rect(pill, Brush::solid(with_alpha(annotation.color, 215)));
            canvas.set_pen(Color::white());
            canvas.draw_text(pill, Align::Center, label);
        }
    }
    canvas.restore();
}

//=============================================================================================================

void TraceMarkers::draw_lines(Canvas& canvas, double first_sample, double pixels_per_sample) const
{
    const double width = canvas.width();
    const double height = canvas.height();
    if (m_eventsVisible) {
        for (const Event& event : m_events) {
            const double x = (static_cast<double>(event.sample) - first_sample) * pixels_per_sample;
            if (outside(x, width)) {
                continue;
            }
            canvas.set_pen(Pen{with_alpha(event.color, 180), 1.0});
            canvas.draw_line({x, 0.0}, {x, height});
        }
    }
    if (m_epochMarkersVisible && !m_epochMarkers.empty()) {
        canvas.set_pen(Pen{{100, 100, 100, 140}, 1.0, LineStyle::Dash});
        for (const int sample : m_epochMarkers) {
            const double x = (static_cast<double>(sample) - first_sample) * pixels_per_sample;
            if (outside(x, width)) {
                continue;
            }
            canvas.draw_line({x, 0.0}, {x, height});
        }
    }
}

//=============================================================================================================

void TraceMarkers::draw(Canvas& canvas, const TextMetrics& metrics, const Font& font,
                        std::span<const TraceView::Annotation> annotations, double first_sample,
                        double pixels_per_sample) const
{
    draw_annotations(canvas, metrics, font, annotations, first_sample, pixels_per_sample);
    draw_lines(canvas, first_sample, pixels_per_sample);
}
