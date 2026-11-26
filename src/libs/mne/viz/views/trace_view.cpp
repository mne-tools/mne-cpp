//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_view.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of TraceView: viewport, input handling and overlays of a raw-data trace view.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "trace_view.h"
#include "../figures/text_metrics.h"
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <format>
#include <map>
#include <string_view>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

namespace
{

constexpr int kBoundaryHitPx = 5;               // Annotation boundary hit-test tolerance
constexpr int kDragThresholdPx = 5;             // Left-button movement before a drag scrolls
constexpr std::int64_t kVelocityWindowMs = 100; // Drag history used for the release velocity
constexpr int kSnapThresholdPx = 8;             // Ruler movement before it locks to an axis

// Rounds a pointer position to whole pixels, as QPointF::toPoint() does
int to_px(double value)
{
    return static_cast<int>(std::lround(value));
}

std::string fixed(double value, int decimals)
{
    return mne::format_number(value, 'f', decimals);
}

// "µ" and "Δ" in UTF-8
constexpr std::string_view kMicro = "\xC2\xB5";
constexpr std::string_view kDelta = "\xCE\x94";

std::string format_amplitude(float amp, const std::string& unit)
{
    const float absAmp = std::abs(amp);
    if (absAmp == 0.f) {
        return "0 " + unit;
    }
    if (absAmp < 1e-9f) {
        return fixed(amp * 1e12f, 1) + " p" + unit;
    }
    if (absAmp < 1e-6f) {
        return fixed(amp * 1e9f, 1) + " n" + unit;
    }
    if (absAmp < 1e-3f) {
        return fixed(amp * 1e6f, 1) + " " + std::string(kMicro) + unit;
    }
    if (absAmp < 1.f) {
        return fixed(amp * 1e3f, 1) + " m" + unit;
    }
    return fixed(amp, 3) + " " + unit;
}

std::string format_ruler_amplitude(float amp, const std::string& unit)
{
    const float absAmp = std::abs(amp);
    if (absAmp < 1e-6f) {
        return fixed(amp * 1e9f, 3) + " n" + unit;
    }
    if (absAmp < 1e-3f) {
        return fixed(amp * 1e6f, 3) + " " + std::string(kMicro) + unit;
    }
    if (absAmp < 1.f) {
        return fixed(amp * 1e3f, 3) + " m" + unit;
    }
    return fixed(amp, 3) + " " + unit;
}

std::string format_ruler_time(float sec)
{
    if (std::abs(sec) < 1.f) {
        return fixed(sec * 1000.f, 1) + " ms";
    }
    return fixed(sec, 3) + " s";
}

std::string unit_for_type(std::string_view typeLabel)
{
    if (typeLabel == "MEG grad") {
        return "T/m";
    }
    if (typeLabel == "MEG mag" || typeLabel == "MEG") {
        return "T";
    }
    if (typeLabel == "EEG" || typeLabel == "EOG" || typeLabel == "ECG" || typeLabel == "EMG") {
        return "V";
    }
    return "AU";
}

Font sized_font(const Font& base, double points, bool bold)
{
    Font font = base;
    font.point_size = points;
    font.pixel_size = 0.0;
    font.bold = bold;
    return font;
}

// A text box as QFontMetrics::boundingRect() gives it: whole pixels, the advance wide and the line high
struct TextBox
{
    int width = 0;
    int height = 0;
    int ascent = 0;
};

TextBox measure(const TextMetrics& metrics, const Font& font, std::string_view text)
{
    return {to_px(metrics.advance(font, text)), to_px(metrics.height(font)), to_px(metrics.ascent(font))};
}

Point pt(int x, int y)
{
    return {static_cast<double>(x), static_cast<double>(y)};
}

Rect rect(int x, int y, int width, int height)
{
    return {static_cast<double>(x), static_cast<double>(y), static_cast<double>(width), static_cast<double>(height)};
}

// The rectangle QRect(QPoint(left, top), QPoint(right, bottom)) covers: both corners inclusive
Rect inclusive_rect(int left, int top, int right, int bottom)
{
    return rect(left, top, right - left + 1, bottom - top + 1);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void TraceView::set_effects(Effects effects)
{
    m_effects = std::move(effects);
}

//=============================================================================================================

void TraceView::set_source(const TraceSource* source)
{
    m_source = source;
}

//=============================================================================================================

void TraceView::set_view_size(int width, int height)
{
    m_width = std::max(0, width);
    m_height = std::max(0, height);
}

//=============================================================================================================

double TraceView::clamp_scroll_sample(double sample) const
{
    // Never before the first available sample
    sample = std::max(sample, static_cast<double>(m_source ? m_source->first_sample() : 0));

    // Never past the file end, once it is known
    if (m_lastFileSample >= 0) {
        double maxScroll = static_cast<double>(m_lastFileSample - visible_sample_count() + 1);
        maxScroll = std::max(maxScroll, static_cast<double>(m_firstFileSample));
        sample = std::min(sample, maxScroll);
    }
    return sample;
}

//=============================================================================================================

bool TraceView::set_scroll_sample(double sample)
{
    sample = clamp_scroll_sample(sample);
    // Exact: a relative fuzzy compare ignores steps below 1e-5 of the position (10 samples at 10^6)
    if (sample == m_scrollSample) {
        return false;
    }
    m_scrollSample = sample;
    if (m_effects.scroll_changed) {
        m_effects.scroll_changed(m_scrollSample);
    }
    return true;
}

//=============================================================================================================

bool TraceView::set_samples_per_pixel(double spp)
{
    spp = std::max(spp, 1e-4);
    if (std::abs(m_samplesPerPixel - spp) * 100000.0 <= std::min(std::abs(m_samplesPerPixel), std::abs(spp))) {
        return false;
    }
    m_samplesPerPixel = spp;
    if (m_effects.zoom_changed) {
        m_effects.zoom_changed(m_samplesPerPixel);
    }
    return true;
}

//=============================================================================================================

void TraceView::scroll_to(double target, int duration_ms)
{
    // A running animation would keep writing the position and fight the new one
    stop_scroll_animations();
    if (duration_ms <= 0) {
        set_scroll_sample(target);
        return;
    }
    m_scrollTarget = clamp_scroll_sample(target);
    m_scrollAnimating = true;
    if (m_effects.animate_scroll) {
        m_effects.animate_scroll({m_scrollSample, m_scrollTarget, duration_ms, false});
    }
}

//=============================================================================================================

void TraceView::scroll_by(double delta, int duration_ms)
{
    scroll_to((m_scrollAnimating ? m_scrollTarget : m_scrollSample) + delta, duration_ms);
}

//=============================================================================================================

void TraceView::zoom_to(double spp, int duration_ms)
{
    spp = std::max(spp, 1e-4);
    if (duration_ms <= 0) {
        set_samples_per_pixel(spp);
        return;
    }
    if (m_effects.animate_zoom) {
        m_effects.animate_zoom({m_samplesPerPixel, spp, duration_ms});
    }
}

//=============================================================================================================

void TraceView::stop_scroll_animations()
{
    m_scrollAnimating = false;
    if (m_effects.stop_animations) {
        m_effects.stop_animations();
    }
}

//=============================================================================================================

void TraceView::scroll_animation_finished()
{
    m_scrollAnimating = false;
}

//=============================================================================================================

int TraceView::visible_first_sample() const
{
    return static_cast<int>(m_scrollSample);
}

//=============================================================================================================

int TraceView::visible_sample_count() const
{
    return static_cast<int>(m_width * m_samplesPerPixel);
}

//=============================================================================================================

double TraceView::sample_at(double x) const
{
    return m_scrollSample + x * m_samplesPerPixel;
}

//=============================================================================================================

void TraceView::set_sfreq(double sfreq)
{
    m_sfreq = std::max(sfreq, 0.0);
}

//=============================================================================================================

bool TraceView::set_first_file_sample(int first)
{
    if (first == m_firstFileSample) {
        return false;
    }
    m_firstFileSample = first;
    return true;
}

//=============================================================================================================

void TraceView::set_last_file_sample(int last)
{
    m_lastFileSample = last;
}

//=============================================================================================================

int TraceView::total_channels() const
{
    return m_source ? m_source->channel_count() : 0;
}

//=============================================================================================================

bool TraceView::set_first_visible_channel(int channel)
{
    const int maxFirst = std::max(0, total_channels() - m_visibleChannelCount);
    channel = std::clamp(channel, 0, maxFirst);
    if (channel == m_firstVisibleChannel) {
        return false;
    }
    m_firstVisibleChannel = channel;
    if (m_effects.channel_offset_changed) {
        m_effects.channel_offset_changed(m_firstVisibleChannel);
    }
    return true;
}

//=============================================================================================================

bool TraceView::set_visible_channel_count(int count)
{
    count = std::max(1, count);
    if (count == m_visibleChannelCount) {
        return false;
    }
    m_visibleChannelCount = count;
    return true;
}

//=============================================================================================================

int TraceView::visible_rows() const
{
    return std::min(m_visibleChannelCount, total_channels() - m_firstVisibleChannel);
}

//=============================================================================================================

bool TraceView::set_butterfly_mode(bool enabled)
{
    if (enabled == m_butterflyMode) {
        return false;
    }
    m_butterflyMode = enabled;
    return true;
}

//=============================================================================================================

void TraceView::set_frozen(bool frozen)
{
    m_frozen = frozen;
    if (m_frozen) {
        stop_scroll_animations();
    }
}

//=============================================================================================================

void TraceView::set_wheel_scrolls_channels(bool channels)
{
    m_wheelScrollsChannels = channels;
}

//=============================================================================================================

void TraceView::set_scroll_speed_factor(double factor)
{
    m_scrollSpeedFactor = std::clamp(factor, 0.25, 4.0);
}

//=============================================================================================================

void TraceView::set_annotations(std::vector<Annotation> annotations)
{
    m_annotations = std::move(annotations);
}

//=============================================================================================================

void TraceView::set_annotation_selection_enabled(bool enabled)
{
    m_annotationSelectionEnabled = enabled;
}

//=============================================================================================================

int TraceView::hit_test_annotation_boundary(double x, bool& is_start) const
{
    const int px = to_px(x);
    for (int i = 0; i < std::ssize(m_annotations); ++i) {
        const double xStart = (static_cast<double>(m_annotations[i].start_sample) - m_scrollSample) / m_samplesPerPixel;
        const double xEnd = (static_cast<double>(m_annotations[i].end_sample + 1) - m_scrollSample) / m_samplesPerPixel;
        if (std::abs(px - static_cast<int>(xStart)) <= kBoundaryHitPx) {
            is_start = true;
            return i;
        }
        if (std::abs(px - static_cast<int>(xEnd)) <= kBoundaryHitPx) {
            is_start = false;
            return i;
        }
    }
    return -1;
}

//=============================================================================================================

bool TraceView::set_crosshair_enabled(bool enabled)
{
    if (enabled == m_crosshairEnabled) {
        return false;
    }
    m_crosshairEnabled = enabled;
    if (!enabled) {
        m_crosshairX = m_crosshairY = -1;
    }
    return true;
}

//=============================================================================================================

void TraceView::set_clock_time_format(bool clock)
{
    m_clockTime = clock;
}

//=============================================================================================================

bool TraceView::set_scalebars_visible(bool visible)
{
    if (visible == m_scalebarsVisible) {
        return false;
    }
    m_scalebarsVisible = visible;
    return true;
}

//=============================================================================================================

TraceView::RulerSnap TraceView::ruler_snap(int dx, int dy)
{
    dx = std::abs(dx);
    dy = std::abs(dy);
    if (dx < kSnapThresholdPx && dy < kSnapThresholdPx) {
        return RulerSnap::Free;
    }
    if (dx > dy * 2) {
        return RulerSnap::Horizontal;
    }
    if (dy > dx * 2) {
        return RulerSnap::Vertical;
    }
    return RulerSnap::Free;
}

//=============================================================================================================

bool TraceView::overlays_visible() const
{
    return (m_crosshairEnabled && m_crosshairX >= 0 && m_crosshairY >= 0) || m_scalebarsVisible || m_ruler.active ||
        m_selection.active;
}

//=============================================================================================================

std::optional<TraceView::CursorData> TraceView::cursor_data() const
{
    if (m_crosshairX < 0 || m_crosshairY < 0 || !m_source || total_channels() == 0) {
        return std::nullopt;
    }

    const int sample = static_cast<int>(sample_at(m_crosshairX));
    CursorData data;
    data.time_sec = m_sfreq > 0.0 ? static_cast<float>(sample - m_firstFileSample) / static_cast<float>(m_sfreq) : 0.f;

    if (m_butterflyMode) {
        // Lanes are the channel types
        const std::vector<TraceSource::Group> groups = m_source->groups();
        const int lanes = static_cast<int>(groups.size());
        if (lanes <= 0) {
            return std::nullopt;
        }
        const float laneH = static_cast<float>(m_height) / static_cast<float>(lanes);
        const int lane = std::clamp(static_cast<int>(static_cast<float>(m_crosshairY) / laneH), 0, lanes - 1);
        data.channel_name = groups[lane].type_label;
        data.unit_label = unit_for_type(groups[lane].type_label);
        return data;
    }

    const int rows = visible_rows();
    if (rows <= 0) {
        return std::nullopt;
    }
    const float laneH = static_cast<float>(m_height) / static_cast<float>(rows);
    const int row = std::clamp(static_cast<int>(static_cast<float>(m_crosshairY) / laneH), 0, rows - 1);
    const int index = m_firstVisibleChannel + row;
    const std::optional<TraceSource::Channel> channel = m_source->channel(index);
    if (!channel) {
        return std::nullopt;
    }
    data.amplitude = m_source->value_at(index, sample);
    data.channel_name = channel->name;
    data.unit_label = unit_for_type(channel->type_label);
    return data;
}

//=============================================================================================================

void TraceView::draw_overlays(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    canvas.save();
    canvas.set_antialiasing(false);
    canvas.set_brush(Brush{});
    if (m_crosshairEnabled) {
        draw_crosshair(canvas, metrics, font);
    }
    if (m_scalebarsVisible) {
        draw_scalebars(canvas, metrics, font);
    }
    if (m_ruler.active) {
        draw_ruler(canvas, metrics, font);
    }
    if (m_selection.active) {
        draw_selection(canvas, metrics, font);
    }
    canvas.restore();
}

//=============================================================================================================

DisplayList TraceView::render_overlays(const TextMetrics& metrics, const Font& font) const
{
    Canvas canvas(m_width, m_height);
    draw_overlays(canvas, metrics, font);
    return canvas.take();
}

//=============================================================================================================

void TraceView::draw_crosshair(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    const int cx = m_crosshairX;
    const int cy = m_crosshairY;
    if (cx < 0 || cy < 0 || !m_source || total_channels() == 0) {
        return;
    }

    canvas.set_pen(Pen{{80, 80, 80, 160}, 1.0, LineStyle::Dash});
    canvas.draw_line(pt(cx, 0), pt(cx, m_height));
    canvas.draw_line(pt(0, cy), pt(m_width, cy));

    const std::optional<CursorData> data = cursor_data();
    if (!data) {
        return;
    }

    std::string timeText;
    if (m_clockTime && data->time_sec >= 0.f) {
        const int totalMs = static_cast<int>(std::lround(data->time_sec * 1000.f));
        timeText = std::format("{:02}:{:02}.{:03}", totalMs / 60000, (totalMs % 60000) / 1000, totalMs % 1000);
    } else {
        timeText = fixed(static_cast<double>(data->time_sec), 3) + " s";
    }
    const std::string label = data->channel_name + "  " + timeText + "  " + format_amplitude(data->amplitude, data->unit_label);

    const Font labelFont = sized_font(font, 8.5, false);
    const TextBox box = measure(metrics, labelFont, label);
    int lx = cx + 10;
    int ly = cy - 10;
    if (lx + box.width + 8 > m_width) {
        lx = cx - box.width - 18;
    }
    if (ly - box.height < 4) {
        ly = cy + box.height + 6;
    }
    const Rect labelRect = rect(lx, ly - box.height, box.width, box.height).adjusted(-4, -2, 4, 2);
    canvas.fill_rect(labelRect, Brush::solid({255, 255, 255, 220}));
    canvas.set_pen(Color{30, 30, 30});
    canvas.set_font(labelFont);
    canvas.draw_text(labelRect, Align::Center, label);
}

//=============================================================================================================

void TraceView::draw_scalebars(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    if (!m_source || total_channels() == 0) {
        return;
    }

    std::vector<TraceSource::Group> groups;
    int lanes = 0;
    if (m_butterflyMode) {
        groups = m_source->groups();
        lanes = static_cast<int>(groups.size());
    } else {
        lanes = visible_rows();
    }
    if (lanes <= 0) {
        return;
    }
    const float laneH = static_cast<float>(m_height) / static_cast<float>(lanes);

    // The amplitude scale of each channel type in view
    std::map<std::string, float> typeScales;
    if (m_butterflyMode) {
        for (const TraceSource::Group& group : groups) {
            if (group.amplitude_max > 0.f) {
                typeScales[group.type_label] = group.amplitude_max;
            }
        }
    } else {
        for (int i = 0; i < lanes; ++i) {
            const std::optional<TraceSource::Channel> channel = m_source->channel(m_firstVisibleChannel + i);
            if (channel && !typeScales.contains(channel->type_label) && channel->amplitude_max > 0.f) {
                typeScales[channel->type_label] = channel->amplitude_max;
            }
        }
    }
    if (typeScales.empty()) {
        return;
    }

    const Font labelFont = sized_font(font, 8.0, false);
    canvas.set_font(labelFont);
    const int lineHeight = to_px(metrics.height(labelFont));
    const int ascent = to_px(metrics.ascent(labelFont));

    // Stacked upwards from the bottom-right corner; amplitude_max spans 45 % of a row, so a bar of barHeight
    // pixels stands for amplitude_max * barHeight / scalePx
    const int margin = 12;
    const float scalePx = laneH * 0.45f;
    const int barHeight = std::clamp(static_cast<int>(std::lround(scalePx)), 20, 60);
    const int x = m_width - margin;
    int y = m_height - margin;

    for (auto it = typeScales.crbegin(); it != typeScales.crend(); ++it) {
        const float ampValue = it->second * static_cast<float>(barHeight) / scalePx;
        const std::string label = it->first + ": " + format_amplitude(ampValue, unit_for_type(it->first));
        const int textW = to_px(metrics.advance(labelFont, label));
        const int barX = x - textW - 14;
        const int barTop = y - barHeight;

        canvas.fill_rect(rect(barX - 4, y - barHeight - lineHeight - 4, textW + 22, barHeight + lineHeight + 8),
                         Brush::solid({255, 255, 255, 200}));

        canvas.set_pen(Pen{{40, 40, 40}, 2.0});
        canvas.draw_line(pt(barX + 4, barTop), pt(barX + 4, y));
        canvas.draw_line(pt(barX, barTop), pt(barX + 8, barTop));
        canvas.draw_line(pt(barX, y), pt(barX + 8, y));

        canvas.set_pen(Color{30, 30, 30});
        canvas.draw_text(pt(barX + 14, y - barHeight / 2 + ascent / 2), label);

        y -= barHeight + lineHeight + 16;
    }
}

//=============================================================================================================

void TraceView::draw_ruler(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    const int x0 = m_ruler.x0;
    const int y0 = m_ruler.y0;
    const int x1 = m_ruler.x1;
    const int y1 = m_ruler.y1;
    const bool snapH = m_ruler.snap == RulerSnap::Horizontal;
    const bool snapV = m_ruler.snap == RulerSnap::Vertical;

    const Color activeColor{40, 120, 200, 220};
    const Color dimColor{130, 160, 200, 120};
    const int tickLen = 5;

    // Light veil with a border over the measured area
    Rect measured;
    if (snapH) {
        measured = inclusive_rect(std::min(x0, x1), 0, std::max(x0, x1), m_height);
    } else if (snapV) {
        measured = inclusive_rect(0, std::min(y0, y1), m_width, std::max(y0, y1));
    } else {
        measured = inclusive_rect(std::min(x0, x1), std::min(y0, y1), std::max(x0, x1), std::max(y0, y1));
    }
    canvas.fill_rect(measured, Brush::solid({255, 255, 255, 60}));
    canvas.set_pen(Pen{{40, 120, 200, 80}, 1.0});
    canvas.draw_rect(measured);

    // Vertical guides at the two x positions
    canvas.set_pen(Pen{snapV ? dimColor : activeColor, 1.0, LineStyle::Dash});
    canvas.draw_line(pt(x0, 0), pt(x0, m_height));
    if (!snapV) {
        canvas.draw_line(pt(x1, 0), pt(x1, m_height));
    }

    // Horizontal guides at the two y positions while measuring amplitude only
    if (snapV) {
        canvas.set_pen(Pen{activeColor, 1.0, LineStyle::Dash});
        canvas.draw_line(pt(0, y0), pt(m_width, y0));
        canvas.draw_line(pt(0, y1), pt(m_width, y1));
    }

    // Time span at y0 and amplitude span at x0
    canvas.set_pen(Pen{snapV ? dimColor : activeColor, snapH ? 2.0 : 1.0});
    if (!snapV) {
        canvas.draw_line(pt(std::min(x0, x1), y0), pt(std::max(x0, x1), y0));
    }
    canvas.set_pen(Pen{snapH ? dimColor : activeColor, snapV ? 2.0 : 1.0});
    if (!snapH) {
        canvas.draw_line(pt(x0, std::min(y0, y1)), pt(x0, std::max(y0, y1)));
    }

    // End ticks
    canvas.set_pen(Pen{activeColor, 1.0});
    if (!snapV) {
        canvas.draw_line(pt(x0 - tickLen, y0), pt(x0 + tickLen, y0));
        canvas.draw_line(pt(x1 - tickLen, y0), pt(x1 + tickLen, y0));
    }
    if (!snapH) {
        canvas.draw_line(pt(x0, y0 - tickLen), pt(x0, y0 + tickLen));
        canvas.draw_line(pt(x0, y1 - tickLen), pt(x0, y1 + tickLen));
    }

    // Measured values: time from the pixels, amplitude at the scale of the channel row of the press
    const float deltaSamples = static_cast<float>(x1 - x0) * static_cast<float>(m_samplesPerPixel);
    const float deltaSec = m_sfreq > 0.0 ? deltaSamples / static_cast<float>(m_sfreq) : 0.f;
    float deltaAmp = 0.f;
    std::string ampUnit = "AU";
    const int rows = visible_rows();
    if (m_source && total_channels() > 0 && rows > 0) {
        const float laneH = static_cast<float>(m_height) / static_cast<float>(rows);
        const int row = std::clamp(static_cast<int>(static_cast<float>(y0) / laneH), 0, rows - 1);
        std::optional<TraceSource::Channel> channel = m_source->channel(m_firstVisibleChannel + row);
        if (!channel) {
            channel = m_source->channel(0);
        }
        if (channel && channel->amplitude_max > 0.f) {
            const float yScale = channel->amplitude_max / (laneH * 0.45f);
            deltaAmp = -static_cast<float>(y1 - y0) * yScale;
            ampUnit = unit_for_type(channel->type_label);
        }
    }
    const std::string timeLabel = std::string(kDelta) + "T = " + format_ruler_time(deltaSec);
    const std::string ampLabel = std::string(kDelta) + "A = " + format_ruler_amplitude(deltaAmp, ampUnit);

    const Font labelFont = sized_font(font, 9.0, true);
    canvas.set_font(labelFont);
    const Color textColor{20, 80, 160};
    const Brush pill = Brush::solid({255, 255, 255, 210});

    if (!snapV) {
        const int labelX = (x0 + x1) / 2;
        int labelY = y0 - 6;
        if (labelY < 14) {
            labelY = y0 + 16;
        }
        const TextBox box = measure(metrics, labelFont, timeLabel);
        const Rect r = rect(labelX - (box.width - 1) / 2, labelY - (box.height - 1) / 2, box.width, box.height).adjusted(-4, -2, 4, 2);
        canvas.fill_rect(r, pill);
        canvas.set_pen(textColor);
        canvas.draw_text(r, Align::Center, timeLabel);
    }
    if (!snapH) {
        const int labelX = x0 + 8;
        const int labelY = (y0 + y1) / 2;
        const TextBox box = measure(metrics, labelFont, ampLabel);
        const Rect r = rect(labelX + box.width / 2 - (box.width - 1) / 2, labelY - (box.height - 1) / 2, box.width, box.height)
                           .adjusted(-4, -2, 4, 2);
        canvas.fill_rect(r, pill);
        canvas.set_pen(textColor);
        canvas.draw_text(r, Align::Center, ampLabel);
    }
}

//=============================================================================================================

void TraceView::draw_selection(Canvas& canvas, const TextMetrics& metrics, const Font& font) const
{
    const int x0 = std::min(m_selection.x0, m_selection.x1);
    const int x1 = std::max(m_selection.x0, m_selection.x1);

    canvas.fill_rect(rect(x0, 0, x1 - x0, m_height), Brush::solid({210, 60, 60, 50}));
    canvas.set_pen(Pen{{210, 60, 60, 180}, 2.0});
    canvas.draw_line(pt(x0, 0), pt(x0, m_height));
    canvas.draw_line(pt(x1, 0), pt(x1, m_height));

    // Duration pill at the top
    if (m_sfreq > 0.0 && m_samplesPerPixel > 0.0) {
        const float deltaSamples = static_cast<float>(x1 - x0) * static_cast<float>(m_samplesPerPixel);
        const float deltaSec = deltaSamples / static_cast<float>(m_sfreq);
        const std::string label = deltaSec < 1.f ? fixed(deltaSec * 1000.f, 0) + " ms" : fixed(deltaSec, 2) + " s";

        const Font labelFont = sized_font(font, 8.0, true);
        const TextBox box = measure(metrics, labelFont, label);
        const Rect r = rect(x0 + 4, 4, box.width + 12, box.height + 4);
        canvas.fill_rect(r, Brush::solid({210, 60, 60, 215}));
        canvas.set_pen(Color::white());
        canvas.set_font(labelFont);
        canvas.draw_text(r, Align::Center, label);
    }
}

//=============================================================================================================

bool TraceView::handle_wheel(const WheelEvent& event)
{
    const double dx = event.angle_delta_x;
    const double dy = event.angle_delta_y;

    if (has_flag(event.modifiers, Modifier::Ctrl)) {
        // Zoom the time axis
        zoom_to(m_samplesPerPixel * (dy > 0.0 ? 0.8 : 1.25), 150);
    } else if (std::abs(dx) > std::abs(dy)) {
        // Mostly horizontal (touch-pad swipe): scroll time by 10 % of the view
        if (!m_frozen) {
            const double step = m_width * m_samplesPerPixel * 0.1 * m_scrollSpeedFactor * (dx > 0.0 ? -1.0 : 1.0);
            scroll_by(step, 100);
        }
    } else if (m_wheelScrollsChannels) {
        // Vertical wheel scrolls channel rows: up = earlier
        const int maxFirst = std::max(0, total_channels() - m_visibleChannelCount);
        set_first_visible_channel(std::clamp(m_firstVisibleChannel + (dy > 0.0 ? -1 : 1), 0, maxFirst));
    } else if (!m_frozen) {
        // Vertical wheel scrolls time by 15 % of the view
        const double step = m_width * m_samplesPerPixel * 0.15 * m_scrollSpeedFactor * (dy > 0.0 ? -1.0 : 1.0);
        scroll_by(step, 100);
    }
    return true;
}

//=============================================================================================================

bool TraceView::handle_pointer(const PointerEvent& event)
{
    switch (event.type) {
        case PointerEvent::Type::Press:
            return handle_press(event);
        case PointerEvent::Type::Move:
            return handle_move(event);
        case PointerEvent::Type::Release:
            return handle_release(event);
        case PointerEvent::Type::DoubleClick:
            return handle_double_click(event);
    }
    return false;
}

//=============================================================================================================

bool TraceView::handle_press(const PointerEvent& event)
{
    const int px = to_px(event.x);
    const int py = to_px(event.y);

    // Right button: annotation range selection in annotation mode, the ruler otherwise
    if (event.button == MouseButton::Right) {
        stop_scroll_animations();
        if (m_annotationSelectionEnabled) {
            m_selection = {true, px, px};
        } else {
            m_ruler = {true, RulerSnap::Free, px, py, px, py};
        }
        redraw();
        return true;
    }

    // Middle button or Alt + left: plain drag scroll
    if (!m_frozen && (event.button == MouseButton::Middle || (event.button == MouseButton::Left && has_flag(event.modifiers, Modifier::Alt)))) {
        m_dragging = true;
        m_dragStartX = px;
        m_dragStartScroll = m_scrollSample;
        return true;
    }

    if (event.button == MouseButton::Left) {
        stop_scroll_animations();

        // Grab an annotation boundary for resizing
        if (m_annotationSelectionEnabled && !m_annotations.empty()) {
            bool isStart = false;
            const int hit = hit_test_annotation_boundary(event.x, isStart);
            if (hit >= 0) {
                m_annDragging = true;
                m_annDragIndex = hit;
                m_annDragIsStart = isStart;
                return true;
            }
        }

        if (m_frozen) {
            // A click still reports its sample, but nothing scrolls
            if (m_effects.sample_clicked) {
                m_effects.sample_clicked(static_cast<int>(sample_at(event.x)));
            }
            return true;
        }

        // The drag starts once the pointer moved past the threshold
        m_leftButtonDown = true;
        m_leftDragActivated = false;
        m_leftDownX = px;
        m_leftDownScroll = m_scrollSample;
        m_dragStartTime = event.timestamp_ms;
        m_velocityHistory.clear();
        m_velocityHistory.push_back({px, 0});
        return true;
    }
    return false;
}

//=============================================================================================================

bool TraceView::handle_move(const PointerEvent& event)
{
    const int px = to_px(event.x);
    const int py = to_px(event.y);

    if (m_annDragging) {
        if (m_annDragIndex >= 0 && m_annDragIndex < std::ssize(m_annotations)) {
            const int sample = clamped_boundary_sample(px);
            if (m_annDragIsStart) {
                m_annotations[m_annDragIndex].start_sample = sample;
            } else {
                m_annotations[m_annDragIndex].end_sample = sample;
            }
            if (m_effects.annotations_changed) {
                m_effects.annotations_changed();
            }
        }
        return true;
    }

    if (m_selection.active) {
        m_selection.x1 = px;
        redraw();
        return true;
    }

    if (m_ruler.active) {
        m_ruler.snap = ruler_snap(px - m_ruler.x0, py - m_ruler.y0);
        apply_ruler_end(px, py);
        redraw();
        return true;
    }

    if (m_dragging) {
        set_scroll_sample(m_dragStartScroll - static_cast<double>(px - m_dragStartX) * m_samplesPerPixel);
        return true;
    }

    if (m_leftButtonDown) {
        const int dx = px - m_leftDownX;
        if (!m_leftDragActivated && std::abs(dx) > kDragThresholdPx) {
            m_leftDragActivated = true;
        }
        if (m_leftDragActivated) {
            set_scroll_sample(m_leftDownScroll - static_cast<double>(dx) * m_samplesPerPixel);

            // Keep the last 100 ms of positions for the release velocity
            const std::int64_t now = event.timestamp_ms - m_dragStartTime;
            m_velocityHistory.push_back({px, now});
            while (m_velocityHistory.size() > 1 && now - m_velocityHistory.front().t > kVelocityWindowMs) {
                m_velocityHistory.erase(m_velocityHistory.begin());
            }
            return true;
        }
    }

    // Hover: crosshair tracking
    if (m_crosshairEnabled) {
        m_crosshairX = px;
        m_crosshairY = py;
        redraw();
        if (m_effects.cursor_data_changed) {
            if (const std::optional<CursorData> data = cursor_data()) {
                m_effects.cursor_data_changed(*data);
            }
        }
    }

    // Hover: resize cursor over annotation boundaries
    if (m_annotationSelectionEnabled && !m_annotations.empty()) {
        bool isStart = false;
        const int hit = hit_test_annotation_boundary(event.x, isStart);
        if (hit >= 0) {
            if (m_annHoverIndex != hit || m_annHoverIsStart != isStart) {
                m_annHoverIndex = hit;
                m_annHoverIsStart = isStart;
                if (m_effects.cursor_changed) {
                    m_effects.cursor_changed(CursorShape::SizeHorizontal);
                }
            }
        } else if (m_annHoverIndex >= 0) {
            m_annHoverIndex = -1;
            if (m_effects.cursor_changed) {
                m_effects.cursor_changed(CursorShape::Default);
            }
        }
    }
    return false;
}

//=============================================================================================================

bool TraceView::handle_release(const PointerEvent& event)
{
    const int px = to_px(event.x);
    const int py = to_px(event.y);

    if (m_annDragging && event.button == MouseButton::Left) {
        if (m_effects.annotation_boundary_moved) {
            m_effects.annotation_boundary_moved(m_annDragIndex, m_annDragIsStart, clamped_boundary_sample(px));
        }
        m_annDragging = false;
        m_annDragIndex = -1;
        return true;
    }

    if (m_selection.active && event.button == MouseButton::Right) {
        m_selection.x1 = px;
        m_selection.active = false;
        redraw();
        const int x0 = std::min(m_selection.x0, m_selection.x1);
        const int x1 = std::max(m_selection.x0, m_selection.x1);
        if (x1 - x0 > 3) {
            const int startSample = std::max(static_cast<int>(sample_at(x0)), m_firstFileSample);
            int endSample = static_cast<int>(sample_at(x1));
            if (m_lastFileSample >= 0) {
                endSample = std::min(endSample, m_lastFileSample);
            }
            if (endSample >= startSample && m_effects.range_selected) {
                m_effects.range_selected(startSample, endSample);
            }
        }
        return true;
    }

    if (m_ruler.active && event.button == MouseButton::Right) {
        apply_ruler_end(px, py);
        m_ruler.active = false;
        redraw();
        return true;
    }

    if (m_dragging && (event.button == MouseButton::Middle || event.button == MouseButton::Left)) {
        m_dragging = false;
        return true;
    }

    if (event.button == MouseButton::Left && m_leftButtonDown) {
        if (!m_leftDragActivated) {
            // A tap: report the sample, no inertia
            if (m_effects.sample_clicked) {
                m_effects.sample_clicked(static_cast<int>(m_leftDownScroll + event.x * m_samplesPerPixel));
            }
        } else if (m_velocityHistory.size() >= 2) {
            // Glide on with the velocity of the last 100 ms
            const VelocitySample oldest = m_velocityHistory.front();
            const VelocitySample newest = m_velocityHistory.back();
            const double dt = static_cast<double>(newest.t - oldest.t);
            if (dt > 5.0) {
                // Dragging right scrolls back in time
                const double velocity = -(static_cast<double>(newest.x - oldest.x) / dt) * m_samplesPerPixel;
                const double speed = std::abs(velocity);
                if (speed > 0.3) {
                    // Cubic ease-out starts at 3 times its mean speed: travel = v * duration / 3
                    const double durationMs = std::clamp(speed, 500.0, 5000.0);
                    const double target = std::max(m_scrollSample + velocity * durationMs / 3.0, static_cast<double>(m_firstFileSample));
                    if (m_effects.animate_scroll) {
                        m_effects.animate_scroll({m_scrollSample, target, static_cast<int>(durationMs), true});
                    }
                }
            }
        }
        m_leftButtonDown = false;
        m_leftDragActivated = false;
        return true;
    }
    return false;
}

//=============================================================================================================

bool TraceView::handle_double_click(const PointerEvent& event)
{
    if (event.button != MouseButton::Left) {
        return false;
    }
    const int rows = visible_rows();
    if (rows <= 0) {
        return true;
    }
    const double laneH = static_cast<double>(m_height) / rows;
    const int row = static_cast<int>(event.y / laneH);
    if (row >= 0 && row < rows && m_effects.channel_double_clicked) {
        m_effects.channel_double_clicked(m_firstVisibleChannel + row);
    }
    return true;
}

//=============================================================================================================

int TraceView::clamped_boundary_sample(int x) const
{
    int sample = std::max(static_cast<int>(sample_at(x)), m_firstFileSample);
    if (m_lastFileSample >= 0) {
        sample = std::min(sample, m_lastFileSample);
    }
    return sample;
}

//=============================================================================================================

void TraceView::apply_ruler_end(int x, int y)
{
    switch (m_ruler.snap) {
        case RulerSnap::Horizontal:
            m_ruler.x1 = x;
            m_ruler.y1 = m_ruler.y0;
            break;
        case RulerSnap::Vertical:
            m_ruler.x1 = m_ruler.x0;
            m_ruler.y1 = y;
            break;
        case RulerSnap::Free:
            m_ruler.x1 = x;
            m_ruler.y1 = y;
            break;
    }
}

//=============================================================================================================

void TraceView::redraw() const
{
    if (m_effects.redraw) {
        m_effects.redraw();
    }
}
