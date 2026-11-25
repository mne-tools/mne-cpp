//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_geometry.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the trace geometry, TraceSource::groups() and draw_trace_figure().
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "trace_geometry.h"
#include "text_metrics.h"
#include "trace_markers.h"
#include "trace_view.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

namespace
{

// Lane of each type in butterfly mode, -1 if the type has none
int lane_of(const std::vector<TraceSource::Group>& groups, const std::string& type)
{
    for (int lane = 0; lane < std::ssize(groups); ++lane) {
        if (groups[lane].type_label == type) {
            return lane;
        }
    }
    return -1;
}

TraceUniforms uniforms_for(const std::optional<TraceSource::Channel>& channel, bool hidden, float yCenter, float yRange,
                           const TraceView& view, const TraceStyle& style)
{
    const bool bad = channel && channel->bad;
    const Color color = hidden ? style.background : (bad ? TraceStyle::bad_color() : channel->color);
    TraceUniforms u;
    u.color[0] = static_cast<float>(color.r) / 255.f;
    u.color[1] = static_cast<float>(color.g) / 255.f;
    u.color[2] = static_cast<float>(color.b) / 255.f;
    u.color[3] = static_cast<float>(color.a) / 255.f;
    u.scroll_sample = static_cast<float>(view.scroll_sample());
    u.samples_per_pixel = static_cast<float>(view.samples_per_pixel());
    u.view_width = static_cast<float>(view.view_width());
    u.view_height = static_cast<float>(view.view_height());
    u.channel_y_center = yCenter;
    u.channel_y_range = hidden ? 0.f : yRange;
    u.amplitude_max = style.z_score ? 4.f : (channel ? channel->amplitude_max : 0.f);
    u.show_clipping = (style.show_clipping && !bad && !style.z_score) ? 1.f : 0.f;
    return u;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::vector<TraceSource::Group> TraceSource::groups() const
{
    std::vector<Group> result;
    const int count = channel_count();
    for (int index = 0; index < count; ++index) {
        const std::optional<Channel> ch = channel(index);
        if (ch && lane_of(result, ch->type_label) < 0) {
            result.push_back({ch->type_label, ch->amplitude_max});
        }
    }
    return result;
}

//=============================================================================================================

void TraceGeometry::set_prefetch_factor(double factor)
{
    m_prefetchFactor = std::max(factor, 0.1);
}

//=============================================================================================================

bool TraceGeometry::needs_stream(const TraceView& view) const
{
    const double visible = view.view_width() * view.samples_per_pixel();
    const double margin = m_prefetchFactor * visible;
    const double scroll = view.scroll_sample();
    return m_dirty || scroll < m_windowFirst + margin || scroll + visible > m_windowLast - margin;
}

//=============================================================================================================

bool TraceGeometry::stream(const TraceView& view, const TraceSource& source, bool z_score)
{
    const double spp = view.samples_per_pixel();
    const double visible = view.view_width() * spp;
    const double scroll = view.scroll_sample();
    const int dataFirst = source.first_sample();
    const int first = std::max(static_cast<int>(scroll - m_prefetchFactor * visible), dataFirst);
    const int last = std::min(static_cast<int>(scroll + (1.0 + m_prefetchFactor) * visible), dataFirst + source.sample_count());
    m_dirty = false;
    if (first >= last) {
        return false;
    }
    m_windowFirst = first;
    m_windowLast = last;

    // Decimated to the view's scale over the whole window
    const int pixelWidth = static_cast<int>((last - first) / spp);
    const int count = source.channel_count();
    m_channels.assign(static_cast<std::size_t>(std::max(count, 0)), TraceVertices{});
    for (int index = 0; index < count; ++index) {
        TraceVertices vertices = source.vertices(index, first, last, pixelWidth);
        if (z_score) {
            ::mne::scene::z_score(vertices.xy);
        }
        m_channels[static_cast<std::size_t>(index)] = std::move(vertices);
    }
    return true;
}

//=============================================================================================================

int TraceGeometry::slot_count(const TraceView& view, const TraceSource& source)
{
    const int total = source.channel_count();
    const int slots = view.butterfly_mode() ? total : std::min(view.visible_channel_count(), total - view.first_visible_channel());
    return std::clamp(slots, 1, kMaxTraces);
}

//=============================================================================================================

std::vector<TraceDraw> TraceGeometry::draws(const TraceView& view, const TraceSource& source, const TraceStyle& style) const
{
    std::vector<TraceDraw> result;
    const int total = source.channel_count();
    const auto firstSampleOf = [this](int index) {
        return index < std::ssize(m_channels) ? static_cast<float>(m_channels[static_cast<std::size_t>(index)].first_sample) : 0.f;
    };

    if (view.butterfly_mode()) {
        // Every channel, in the lane of its type
        const std::vector<TraceSource::Group> groups = source.groups();
        const int lanes = static_cast<int>(groups.size());
        if (lanes <= 0) {
            return result;
        }
        const float laneRange = 2.f / static_cast<float>(lanes);
        const int count = std::min(total, kMaxTraces);
        for (int index = 0; index < count; ++index) {
            const std::optional<TraceSource::Channel> channel = source.channel(index);
            const int lane = channel ? lane_of(groups, channel->type_label) : -1;
            const bool hidden = !channel || (style.hide_bad_channels && channel->bad) || lane < 0;
            const float yCenter = lane >= 0 ? 1.f - laneRange * (static_cast<float>(lane) + 0.5f) : 0.f;
            TraceDraw draw{index, uniforms_for(channel, hidden, yCenter, laneRange, view, style)};
            draw.uniforms.first_sample = firstSampleOf(index);
            result.push_back(draw);
        }
        return result;
    }

    // The visible rows, top (NDC +1) to bottom (NDC -1)
    const int first = std::clamp(view.first_visible_channel(), 0, std::max(total, 0));
    const int rows = std::min({view.visible_channel_count(), total - first, kMaxTraces});
    if (rows <= 0) {
        return result;
    }
    const float laneRange = 2.f / static_cast<float>(rows);
    for (int row = 0; row < rows; ++row) {
        const int index = first + row;
        const std::optional<TraceSource::Channel> channel = source.channel(index);
        const bool hidden = !channel || (style.hide_bad_channels && channel->bad);
        TraceDraw draw{index, uniforms_for(channel, hidden, 1.f - laneRange * (static_cast<float>(row) + 0.5f), laneRange, view, style)};
        draw.uniforms.first_sample = firstSampleOf(index);
        result.push_back(draw);
    }
    return result;
}

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::vector<float> mne::scene::decimate_trace(std::span<const float> samples, int pixel_width, Detrend detrend)
{
    const int nSamples = static_cast<int>(samples.size());
    if (pixel_width <= 0 || nSamples <= 0) {
        return {};
    }

    // Trend over the window: its mean, or the least-squares line over t = 0 .. n - 1
    float dcOffset = 0.f;
    float slope = 0.f;
    float intercept = 0.f;
    if (detrend == Detrend::Mean) {
        double sum = 0.0;
        for (const float value : samples) {
            sum += value;
        }
        dcOffset = static_cast<float>(sum / nSamples);
    } else if (detrend == Detrend::Linear) {
        double sumX = 0.0;
        double sumY = 0.0;
        double sumXX = 0.0;
        double sumXY = 0.0;
        for (int i = 0; i < nSamples; ++i) {
            const double x = static_cast<double>(i);
            const double y = static_cast<double>(samples[static_cast<std::size_t>(i)]);
            sumX += x;
            sumY += y;
            sumXX += x * x;
            sumXY += x * y;
        }
        const double denom = nSamples * sumXX - sumX * sumX;
        if (std::abs(denom) > 1e-30) {
            slope = static_cast<float>((nSamples * sumXY - sumX * sumY) / denom);
            intercept = static_cast<float>((sumY - slope * sumX) / nSamples);
        }
    }
    const auto trendAt = [&](float t) {
        return detrend == Detrend::Mean ? dcOffset : detrend == Detrend::Linear ? slope * t + intercept
                                                                                : 0.f;
    };

    std::vector<float> result;
    if (nSamples <= pixel_width * 2) {
        // One vertex per sample
        result.reserve(static_cast<std::size_t>(nSamples) * 2);
        for (int i = 0; i < nSamples; ++i) {
            result.push_back(static_cast<float>(i));
            result.push_back(samples[static_cast<std::size_t>(i)] - trendAt(static_cast<float>(i)));
        }
        return result;
    }

    // Min/max envelope: (x, max), (x, min) per pixel bin; as a line strip a spike per column, diagonals between
    result.reserve(static_cast<std::size_t>(pixel_width) * 4);
    const float spp = static_cast<float>(nSamples) / static_cast<float>(pixel_width);
    for (int px = 0; px < pixel_width; ++px) {
        int begin = static_cast<int>(static_cast<float>(px) * spp);
        const int end = std::min(static_cast<int>(static_cast<float>(px + 1) * spp), nSamples);
        if (begin >= end) {
            begin = std::max(end - 1, 0);
        }
        float minV = samples[static_cast<std::size_t>(begin)];
        float maxV = minV;
        for (int s = begin + 1; s < end; ++s) {
            minV = std::min(minV, samples[static_cast<std::size_t>(s)]);
            maxV = std::max(maxV, samples[static_cast<std::size_t>(s)]);
        }
        const float trend = trendAt(static_cast<float>(begin) + static_cast<float>(end - begin) * 0.5f);
        const float x = static_cast<float>(px) * spp;
        result.push_back(x);
        result.push_back(maxV - trend);
        result.push_back(x);
        result.push_back(minV - trend);
    }
    return result;
}

//=============================================================================================================

void mne::scene::z_score(std::vector<float>& xy)
{
    const int n = static_cast<int>(xy.size()) / 2;
    if (n <= 1) {
        return;
    }
    double sum = 0.0;
    double sumSq = 0.0;
    for (int v = 0; v < n; ++v) {
        const double a = static_cast<double>(xy[static_cast<std::size_t>(v) * 2 + 1]);
        sum += a;
        sumSq += a * a;
    }
    const float mean = static_cast<float>(sum / n);
    const double var = sumSq / n - static_cast<double>(mean) * mean;
    const float sd = var > 0.0 ? static_cast<float>(std::sqrt(var)) : 1.f;
    for (int v = 0; v < n; ++v) {
        float& y = xy[static_cast<std::size_t>(v) * 2 + 1];
        y = (y - mean) / sd;
    }
}

//=============================================================================================================

std::vector<int> mne::scene::visible_channels(int channel_count, std::span<const int> filter, bool hide_bad,
                                              const std::function<bool(int)>& is_bad)
{
    std::vector<int> indices;
    if (filter.empty()) {
        indices.reserve(static_cast<std::size_t>(std::max(channel_count, 0)));
        for (int channel = 0; channel < channel_count; ++channel) {
            indices.push_back(channel);
        }
    } else {
        indices.assign(filter.begin(), filter.end());
    }
    if (!hide_bad) {
        return indices;
    }
    std::erase_if(indices, [&](int channel) { return channel < 0 || (is_bad && is_bad(channel)); });
    return indices;
}

//=============================================================================================================

void mne::scene::draw_trace_figure(Canvas& canvas, const TextMetrics& metrics, const Font& font, const TraceView& view,
                                   const TraceSource& source, const TraceMarkers& markers, const TraceStyle& style)
{
    const int pw = static_cast<int>(canvas.width());
    const int ph = static_cast<int>(canvas.height());
    if (pw <= 0 || ph <= 0 || view.samples_per_pixel() <= 0.0) {
        return;
    }

    // Same rows and time span as on screen at any output width
    const float spp = static_cast<float>(view.samples_per_pixel()) * static_cast<float>(std::max(view.view_width(), 1)) / static_cast<float>(pw);
    const float scrollSample = static_cast<float>(view.scroll_sample());
    const float sfreq = static_cast<float>(view.sfreq());
    const int firstFileSample = view.first_file_sample();

    // (logical channel, lane) of every trace, laid out like TraceGeometry::draws()
    std::vector<std::pair<int, int>> traces;
    int laneCount = 0;
    const int total = source.channel_count();
    if (view.butterfly_mode()) {
        const std::vector<TraceSource::Group> groups = source.groups();
        laneCount = static_cast<int>(groups.size());
        for (int lane = 0; lane < laneCount; ++lane) {
            for (int index = 0; index < total; ++index) {
                const std::optional<TraceSource::Channel> channel = source.channel(index);
                if (channel && channel->type_label == groups[lane].type_label && !(style.hide_bad_channels && channel->bad)) {
                    traces.emplace_back(index, lane);
                }
            }
        }
    } else {
        const int first = std::clamp(view.first_visible_channel(), 0, std::max(total, 0));
        laneCount = std::min(view.visible_channel_count(), total - first);
        for (int lane = 0; lane < laneCount; ++lane) {
            traces.emplace_back(first + lane, lane);
        }
    }
    if (laneCount <= 0) {
        return;
    }

    canvas.save();
    canvas.set_antialiasing(false);
    canvas.set_brush(Brush{});

    const float laneH = static_cast<float>(ph) / static_cast<float>(laneCount);
    const int firstSample = static_cast<int>(scrollSample);
    const int lastSample = firstSample + static_cast<int>(static_cast<float>(pw) * spp) + 1;

    // Alternating per-second bands: odd seconds darkened by 8 %, as in overlay.frag
    if (style.grid && sfreq > 0.f) {
        const float firstBound = std::floor((scrollSample - static_cast<float>(firstFileSample)) / sfreq) * sfreq + static_cast<float>(firstFileSample);
        bool oddBand = (static_cast<long long>((firstBound - static_cast<float>(firstFileSample)) / sfreq) & 1) != 0;
        const Color& bg = style.background;
        const Color altColor{static_cast<std::uint8_t>(std::lround(bg.r * 0.92)), static_cast<std::uint8_t>(std::lround(bg.g * 0.92)),
                             static_cast<std::uint8_t>(std::lround(bg.b * 0.92)), bg.a};
        for (int band = 0;; ++band, oddBand = !oddBand) {
            const float s = firstBound + static_cast<float>(band) * sfreq;
            if (s >= static_cast<float>(lastSample)) {
                break;
            }
            if (!oddBand) {
                continue;
            }
            const float xStart = std::clamp((s - scrollSample) / spp, 0.f, static_cast<float>(pw));
            const float xEnd = std::clamp((s - scrollSample) / spp + sfreq / spp, 0.f, static_cast<float>(pw));
            if (xEnd > xStart) {
                canvas.fill_rect({xStart, 0.0, static_cast<double>(xEnd - xStart), static_cast<double>(ph)}, Brush::solid(altColor));
            }
        }
    }

    // Grid: lane separators, ±45 % guides, centre lines and time ticks at least 80 px apart
    if (style.grid) {
        for (int i = 0; i < laneCount; ++i) {
            const float yMid = (static_cast<float>(i) + 0.5f) * laneH;
            const float yTop = static_cast<float>(i) * laneH;
            if (i > 0) {
                canvas.set_pen(Pen{{205, 205, 215}, 1.0});
                canvas.draw_line({0.0, yTop}, {static_cast<double>(pw), yTop});
            }
            canvas.set_pen(Pen{{228, 228, 235}, 1.0, LineStyle::Dot});
            canvas.draw_line({0.0, yMid - laneH * 0.44f}, {static_cast<double>(pw), yMid - laneH * 0.44f});
            canvas.draw_line({0.0, yMid + laneH * 0.44f}, {static_cast<double>(pw), yMid + laneH * 0.44f});
            canvas.set_pen(Pen{{210, 210, 218}, 1.0});
            canvas.draw_line({0.0, yMid}, {static_cast<double>(pw), yMid});
        }
        if (sfreq > 0.f) {
            static constexpr std::array<float, 10> kNiceIntervals = {0.05f, 0.1f, 0.2f, 0.5f, 1.f, 2.f, 5.f, 10.f, 30.f, 60.f};
            const float pxPerSecond = sfreq / spp;
            float tickIntervalS = kNiceIntervals[0];
            for (const float interval : kNiceIntervals) {
                tickIntervalS = interval;
                if (interval * pxPerSecond >= 80.f) {
                    break;
                }
            }
            const float tickSamples = tickIntervalS * sfreq;
            const float origin = static_cast<float>(firstFileSample);
            const float firstTick = std::ceil((scrollSample - origin) / tickSamples) * tickSamples + origin;
            canvas.set_pen(Pen{{205, 205, 210}, 1.0});
            for (int tick = 0;; ++tick) {
                const float s = firstTick + static_cast<float>(tick) * tickSamples;
                if (s >= static_cast<float>(lastSample)) {
                    break;
                }
                const double x = (s - scrollSample) / spp;
                canvas.draw_line({x, 0.0}, {x, static_cast<double>(ph)});
            }
        }
    }

    markers.draw_annotations(canvas, metrics, font, view.annotations(), scrollSample, 1.0 / spp);

    // Traces, with the mapping of channeldata.vert / .frag
    for (const auto& [index, lane] : traces) {
        const std::optional<TraceSource::Channel> channel = source.channel(index);
        if (!channel) {
            continue;
        }
        const TraceVertices vertices = source.vertices(index, firstSample, lastSample, pw);
        const int n = vertices.vertex_count();
        if (n < 2) {
            continue;
        }

        // Z-score: (y - mean) / std, with ±4 std filling the row like the GPU path
        float offset = 0.f;
        float amplitudeMax = channel->amplitude_max;
        if (style.z_score) {
            double sum = 0.0;
            double sumSq = 0.0;
            for (int v = 0; v < n; ++v) {
                const double amp = static_cast<double>(vertices.xy[static_cast<std::size_t>(v) * 2 + 1]);
                sum += amp;
                sumSq += amp * amp;
            }
            const double mean = sum / n;
            const double var = sumSq / n - mean * mean;
            offset = static_cast<float>(mean);
            amplitudeMax = 4.f * (var > 0.0 ? static_cast<float>(std::sqrt(var)) : 1.f);
        }

        const Pen normalPen{channel->bad ? TraceStyle::bad_color() : channel->color, 1.2};
        const Pen clipPen{{255, 0, 0}, 1.6};
        const bool doClip = style.show_clipping && !channel->bad && !style.z_score;
        const float yMid = (static_cast<float>(lane) + 0.5f) * laneH;

        std::vector<Point> segment;
        segment.reserve(static_cast<std::size_t>(n));
        bool prevClipped = false;
        for (int v = 0; v < n; ++v) {
            const float y = vertices.xy[static_cast<std::size_t>(v) * 2 + 1];
            const float norm = amplitudeMax > 0.f ? std::clamp((y - offset) / amplitudeMax, -2.f, 2.f) : 0.f;
            const Point pt{(static_cast<float>(vertices.first_sample) + vertices.xy[static_cast<std::size_t>(v) * 2] - scrollSample) / spp,
                           yMid - norm * 0.45f * laneH};
            const bool clipped = doClip && std::abs(norm) >= 0.95f;
            if (v > 0 && clipped != prevClipped) {
                segment.push_back(pt);
                canvas.set_pen(prevClipped ? clipPen : normalPen);
                canvas.draw_polyline(segment);
                segment.clear();
            }
            segment.push_back(pt);
            prevClipped = clipped;
        }
        if (segment.size() > 1) {
            canvas.set_pen(prevClipped ? clipPen : normalPen);
            canvas.draw_polyline(segment);
        }
    }

    markers.draw_lines(canvas, scrollSample, 1.0 / spp);
    canvas.restore();
}
