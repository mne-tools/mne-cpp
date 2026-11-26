//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     butterfly_plot.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of ButterflyPlot.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "butterfly_plot.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <format>
#include <string>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void ButterflyPlot::set_timing(int samples, int preStimSamples, float sfreq)
{
    m_samples = samples;
    m_preStimSamples = preStimSamples;
    m_sfreq = sfreq;
}

//=============================================================================================================

void ButterflyPlot::set_baseline(float from, float to)
{
    m_baselineFrom = from;
    m_baselineTo = to;
}

//=============================================================================================================

void ButterflyPlot::set_time_spacers(bool on)
{
    m_timeSpacers = on;
}

//=============================================================================================================

void ButterflyPlot::set_background(Color color)
{
    m_background = color;
}

//=============================================================================================================

void ButterflyPlot::set_frozen(bool frozen)
{
    m_frozen = frozen;
}

//=============================================================================================================

void ButterflyPlot::set_channels(std::vector<Channel> channels)
{
    m_channels = std::move(channels);
}

//=============================================================================================================

DisplayList ButterflyPlot::render(double width, double height, const Font& font) const
{
    Canvas canvas(width, height);
    draw(canvas, font);
    return canvas.take();
}

//=============================================================================================================

void ButterflyPlot::draw(Canvas& canvas, const Font& font) const
{
    const int w = static_cast<int>(canvas.width());
    const int h = static_cast<int>(canvas.height());
    canvas.save();
    canvas.set_font(font);

    // Background, its outline just outside the figure
    canvas.set_pen(Color::black());
    canvas.set_brush(Brush::solid(m_background));
    canvas.draw_rect({-1, -1, static_cast<double>(w + 2), static_cast<double>(h + 2)});
    canvas.set_brush(Brush{});
    canvas.set_antialiasing(true);
    if (m_samples == 0) {
        canvas.restore();
        return;
    }
    const float fDx = static_cast<float>(w) / static_cast<float>(m_samples);
    const Pen dashedRed{{255, 0, 0}, 1, LineStyle::Dash};

    // Baseline interval
    {
        canvas.save();
        canvas.set_pen(dashedRed);
        canvas.set_brush(Brush::solid({255, 0, 0}));
        canvas.set_opacity(0.1);
        const float fromSample = m_baselineFrom * m_sfreq + static_cast<float>(m_preStimSamples);
        const float toSample = m_baselineTo * m_sfreq + static_cast<float>(m_preStimSamples);
        canvas.draw_rect({static_cast<double>(static_cast<int>(fDx * fromSample)), 0,
                          static_cast<double>(static_cast<int>(fDx * (toSample - fromSample))), static_cast<double>(h)});
        canvas.restore();
    }

    // Stimulus
    {
        canvas.save();
        canvas.set_pen(dashedRed);
        const float posX = fDx * static_cast<float>(m_preStimSamples);
        const double x = static_cast<int>(posX);
        canvas.draw_line({x, 1}, {x, static_cast<double>(h)});
        canvas.draw_text(Point{posX + 5.0f, static_cast<double>(h - 1 - 5)}, "0ms / Stimulus");
        canvas.restore();
    }

    // A dashed line every 50 ms
    if (m_timeSpacers) {
        canvas.save();
        canvas.set_pen(Pen{{0, 0, 0, 128}, 1, LineStyle::Dash});
        const double yEnd = h - 1;
        const float timeDistanceMSec = 50.0f;
        const float timeDistanceSamples = (timeDistanceMSec / 1000.0f) * m_sfreq;
        const auto spacer = [&](float sampleCounter, const std::string& label) {
            const float x = fDx * sampleCounter;
            const double xi = static_cast<int>(x);
            canvas.draw_line({xi, 0}, {xi, static_cast<double>(static_cast<int>(yEnd))});
            canvas.draw_text(Point{x + 5.0f, yEnd - 5}, label);
        };
        float sampleCounter = static_cast<float>(m_preStimSamples);
        for (int counter = 1; sampleCounter - timeDistanceSamples > 0; ++counter) {
            sampleCounter -= timeDistanceSamples;
            spacer(sampleCounter, std::format("-{}ms", timeDistanceMSec * static_cast<float>(counter)));
        }
        sampleCounter = static_cast<float>(m_preStimSamples);
        for (int counter = 1; sampleCounter + timeDistanceSamples < static_cast<float>(m_samples); ++counter) {
            sampleCounter += timeDistanceSamples;
            spacer(sampleCounter, std::format("{}ms", timeDistanceMSec * static_cast<float>(counter)));
        }
        canvas.restore();
    }

    // Zero line
    canvas.save();
    canvas.set_pen(Pen{Color::black(), 1, LineStyle::Dash});
    // The zero line sits on a whole pixel
    const int zeroY = h / 2;
    canvas.draw_line({0, static_cast<double>(zeroY)}, {static_cast<double>(w), static_cast<double>(zeroY)});
    canvas.restore();

    canvas.translate(0, zeroY);
    for (const Channel& channel : m_channels) {
        canvas.save();
        drawChannel(canvas, channel, w, h);
        canvas.restore();
    }
    canvas.restore();
}

//=============================================================================================================

void ButterflyPlot::drawChannel(Canvas& canvas, const Channel& channel, int width, int height) const
{
    canvas.set_opacity(channel.bad ? 0.20 : 0.75);
    const auto fMaxValue = static_cast<float>(channel.max_value);
    const float fScaleY = static_cast<float>(height) / (2 * fMaxValue);
    const float fWinMaxVal = (static_cast<float>(height) - 2) / 2.0f;
    const float fDx = static_cast<float>(width - 2) / (static_cast<float>(m_samples) - 1.0f);

    for (const Trace& trace : channel.traces) {
        Color color = trace.color;
        if (m_frozen) {
            color.a = 128;
        }
        canvas.set_pen(Pen{color, 1});

        const int cols = static_cast<int>(trace.samples.size());
        const int dsFactor = std::max(1, cols / width);
        std::vector<Point> path{{1, 0}};
        if (cols > 0) {
            const float fValue = static_cast<float>(trace.samples[0]) * fScaleY;
            path[0] = {1, -static_cast<double>(fValue)};
        }
        // At most `width` segments, as many as the widget has pixels
        for (int i = 1; i < cols && static_cast<int>(path.size()) <= width; i += dsFactor) {
            float fValue = static_cast<float>(trace.samples[static_cast<std::size_t>(i)]) * fScaleY;
            // Cut at the edges of the figure
            fValue = fValue > fWinMaxVal ? fWinMaxVal : fValue < -fWinMaxVal ? -fWinMaxVal
                                                                             : fValue;
            path.push_back({1.0 + static_cast<double>(fDx * static_cast<float>(i)), -static_cast<double>(fValue)});
        }
        canvas.draw_polyline(path);
    }
}
