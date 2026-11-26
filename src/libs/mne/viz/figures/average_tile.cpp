//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     average_tile.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of AverageTile.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "average_tile.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void AverageTile::draw(Canvas& canvas, const Font& font) const
{
    canvas.save();
    canvas.set_antialiasing(true);
    if (bad) {
        canvas.set_opacity(0.20);
    }
    const double centerY = box.y + box.height / 2.0;

    // Stimulus and zero line
    if (columns > 0) {
        const double xStep = box.width / static_cast<double>(columns);
        const double stimX = box.x + std::abs(stim_sample) * xStep;
        canvas.save();
        canvas.set_pen(Pen{{255, 0, 0}, 0});
        canvas.draw_line({stimX, box.y}, {stimX, box.y + box.height});
        canvas.draw_line({box.x, centerY}, {box.x + box.width, centerY});
        canvas.restore();
    }

    const double scaleY = box.height / (2 * static_cast<float>(max_value));
    const double halfH = box.height / 2.0;
    for (const Trace& trace : traces) {
        if (trace.samples.empty()) {
            continue;
        }
        const double xStep = box.width / static_cast<double>(trace.samples.size());
        std::vector<Point> path{{box.x, centerY}};
        path.reserve(trace.samples.size() + 1);
        for (std::size_t i = 0; i < trace.samples.size(); ++i) {
            const double value = std::clamp(trace.samples[i] * scaleY, -halfH, halfH);
            path.push_back({box.x + static_cast<double>(i) * xStep, centerY - value});
        }
        canvas.set_pen(Pen{trace.color, 0});
        canvas.draw_polyline(path);
    }

    // Name above the box
    Font nameFont = font;
    nameFont.pixel_size = 0.0;
    nameFont.point_size = name_size;
    canvas.set_font(nameFont);
    canvas.set_pen(Pen{{255, 0, 0}, 0});
    canvas.draw_text(Rect{box.x, box.y - name_size, 1e4, 1e4}, Align::Left | Align::Top, name);
    canvas.restore();
}
