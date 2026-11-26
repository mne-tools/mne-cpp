//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_geometry.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Trace vertices, their streaming window and per-trace shader parameters of a raw-data trace view.
 */

#ifndef SCENE_TRACE_GEOMETRY_H
#define SCENE_TRACE_GEOMETRY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "canvas.h"
#include "scene_global.h"
#include "style.h"
#include "trace_source.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <span>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

class TextMetrics;
class TraceMarkers;
class TraceView;

/**
 * @brief Trend removed from a trace window before it is drawn.
 *
 * @since 3.0.0
 */
enum class Detrend
{
    None,  /**< Raw values. */
    Mean,  /**< The window mean. */
    Linear /**< The least-squares line over the window. */
};

/**
 * Turns a window of samples into trace vertices (interleaved x, y with x the offset from the first sample).
 *
 * Up to two samples per pixel every sample is a vertex. Beyond that the window is split into @p pixel_width
 * bins, each drawn as its maximum and then its minimum at the bin start, so that a line strip draws the
 * min/max envelope (2 vertices per pixel).
 *
 * @param[in] samples       The window.
 * @param[in] pixel_width   Pixels the window spans; nothing is returned if it is not positive.
 * @param[in] detrend       Trend to subtract; for bins it is taken at the bin centre.
 *
 * @return The vertices.
 *
 * @since 3.0.0
 */
SCENESHARED_EXPORT std::vector<float> decimate_trace(std::span<const float> samples, int pixel_width, Detrend detrend = Detrend::None);

/**
 * Replaces the y values of trace vertices by their z-scores (y - mean) / standard deviation; a constant trace
 * becomes 0. Traces with fewer than two vertices are left alone.
 *
 * @param[in, out] xy   Interleaved x, y.
 *
 * @since 3.0.0
 */
SCENESHARED_EXPORT void z_score(std::vector<float>& xy);

/**
 * @param[in] channel_count   Channels of the data.
 * @param[in] filter          Channels to show in this order; empty for all.
 * @param[in] hide_bad        Whether bad channels (and invalid filter entries) are left out.
 * @param[in] is_bad          Tells whether a channel is bad.
 *
 * @return The data channel of every logical channel of the view.
 *
 * @since 3.0.0
 */
SCENESHARED_EXPORT std::vector<int> visible_channels(int channel_count, std::span<const int> filter, bool hide_bad,
                                                     const std::function<bool(int)>& is_bad);

/**
 * @brief Shader parameters of one drawn trace.
 *
 * The layout is the uniform block of channeldata.vert / channeldata.frag (std140, 52 bytes): copy the struct
 * as is into a uniform buffer slot.
 *
 * @since 3.0.0
 */
struct TraceUniforms
{
    float color[4] = {0.f, 0.f, 0.f, 0.f}; /**< Offset 0: RGBA in 0..1. */
    float first_sample = 0.f;              /**< Offset 16: absolute sample at vertex x = 0. */
    float scroll_sample = 0.f;             /**< Offset 20: sample at the left edge. */
    float samples_per_pixel = 1.f;         /**< Offset 24: horizontal scale. */
    float view_width = 0.f;                /**< Offset 28: view width in logical pixels. */
    float view_height = 0.f;               /**< Offset 32: view height in logical pixels. */
    float channel_y_center = 0.f;          /**< Offset 36: lane centre in normalised device coordinates (+1 = top). */
    float channel_y_range = 0.f;           /**< Offset 40: lane height in normalised device coordinates (0 hides it). */
    float amplitude_max = 0.f;             /**< Offset 44: value that spans 45 % of the lane. */
    float show_clipping = 0.f;             /**< Offset 48: 1 to draw clipped segments in red. */
};

static_assert(sizeof(TraceUniforms) == 52, "TraceUniforms must match the uniform block of channeldata.vert");

/**
 * @brief One trace to draw: the logical channel whose vertices it uses and its shader parameters.
 *
 * @since 3.0.0
 */
struct TraceDraw
{
    int channel = 0;        /**< Logical channel. */
    TraceUniforms uniforms; /**< Shader parameters. */
};

/**
 * @brief How traces are coloured and scaled.
 *
 * @since 3.0.0
 */
struct TraceStyle
{
    Color background{250, 250, 250}; /**< Colour of hidden traces (they are drawn flat in the background). */
    bool z_score = false;            /**< Whether the vertices hold z-scores (amplitude scale 4). */
    bool show_clipping = true;       /**< Whether clipped segments of good channels are red. */
    bool hide_bad_channels = false;  /**< Whether bad channels are hidden. */
    bool grid = true;                /**< Whether the image export draws the grid and the per-second bands. */

    /** @return The colour of bad channels. */
    static constexpr Color bad_color()
    {
        return {200, 60, 60, 180};
    }
};

/**
 * @brief The vertices of all traces over a window around the view, and what to draw of them.
 *
 * stream() fetches the vertices of every logical channel for the visible samples plus a prefetch of
 * prefetch_factor() view widths on each side, decimated to the view's scale; scrolling needs no new vertices
 * until the view uses up half of the prefetch on a side where the window does not end with the data. draws() lists the
 * traces of the current frame with their shader parameters.
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT TraceGeometry
{
public:
    /** Most traces drawn in one frame. */
    static constexpr int kMaxTraces = 1024;

    //=========================================================================================================
    /**
     * Sets how many view widths are fetched on each side of the view.
     *
     * @param[in] factor   Prefetch factor, at least 0.1.
     */
    void set_prefetch_factor(double factor);

    /** @return The prefetch factor. */
    double prefetch_factor() const
    {
        return m_prefetchFactor;
    }

    //=========================================================================================================
    /**
     * Marks the vertices as outdated, e.g. after a zoom or a data change.
     */
    void invalidate()
    {
        m_dirty = true;
    }

    //=========================================================================================================
    /**
     * @param[in] view   The view.
     *
     * @return Whether stream() has to run: outdated vertices or a view near or past the window edges.
     */
    bool needs_stream(const TraceView& view) const;

    //=========================================================================================================
    /**
     * Fetches the vertices of every logical channel over the window around @p view.
     *
     * @param[in] view      The view.
     * @param[in] source    The channels.
     * @param[in] z_score   Whether to replace the values by their z-scores.
     *
     * @return Whether new vertices were fetched; nothing is fetched if no data lies in the window.
     */
    bool stream(const TraceView& view, const TraceSource& source, bool z_score);

    /** @return The vertices by logical channel, from the last stream(). */
    const std::vector<TraceVertices>& channels() const
    {
        return m_channels;
    }

    /** @return The first sample of the streamed window. */
    int window_first() const
    {
        return m_windowFirst;
    }

    /** @return The end sample (exclusive) of the streamed window. */
    int window_last() const
    {
        return m_windowLast;
    }

    //=========================================================================================================
    /**
     * @param[in] view     The view.
     * @param[in] source   The channels.
     *
     * @return The number of uniform slots a frame needs: every channel in butterfly mode, otherwise the visible
     *         rows; at least 1, at most kMaxTraces.
     */
    static int slot_count(const TraceView& view, const TraceSource& source);

    //=========================================================================================================
    /**
     * Lists the traces of a frame; the uniform slot of a trace is its position in the list.
     *
     * In butterfly mode every logical channel is drawn in the lane of its type, otherwise the visible rows top
     * to bottom. Bad channels are red, hidden or missing ones are flat and of the background colour.
     *
     * @param[in] view      The view.
     * @param[in] source    The channels.
     * @param[in] style     Colours and scales.
     *
     * @return The traces.
     */
    std::vector<TraceDraw> draws(const TraceView& view, const TraceSource& source, const TraceStyle& style) const;

private:
    std::vector<TraceVertices> m_channels;
    double m_prefetchFactor = 1.0;
    int m_windowFirst = 0;
    int m_windowLast = 0;
    bool m_windowAtDataFirst = false; /**< Whether the window starts at the first sample of the data. */
    bool m_windowAtDataLast = false;  /**< Whether the window ends at the last sample of the data. */
    bool m_dirty = true;
};

/**
 * Draws the view as an image would show it, into (0, 0, width, height) of @p canvas: the per-second bands and
 * the grid (if style.grid), the annotations, the traces as polylines (clipped segments in red) and the event
 * and epoch lines. The time span of @p view is kept at any canvas width; the traces are fetched from
 * @p source at the canvas resolution.
 *
 * @param[in] canvas    Canvas, usually with the background colour.
 * @param[in] metrics   Measures the annotation labels.
 * @param[in] font      Base font of the annotation labels.
 * @param[in] view      The view: scroll, scale, channel window, butterfly mode, annotations.
 * @param[in] source    The channels.
 * @param[in] markers   Events and epoch markers.
 * @param[in] style     Colours and scales.
 *
 * @since 3.0.0
 */
SCENESHARED_EXPORT void draw_trace_figure(Canvas& canvas, const TextMetrics& metrics, const Font& font, const TraceView& view,
                                          const TraceSource& source, const TraceMarkers& markers, const TraceStyle& style);

} // namespace mne::scene

#endif // SCENE_TRACE_GEOMETRY_H
