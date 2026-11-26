//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_source.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TraceSource, the channels and samples a raw-data trace view shows.
 */

#ifndef VIZ_TRACE_SOURCE_H
#define VIZ_TRACE_SOURCE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "../figures/style.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief Vertices of one channel trace over a sample window.
 *
 * @c xy holds interleaved (x, y) pairs: x is the sample offset from @c first_sample (fractional for decimated
 * traces), y the value in physical units. This is the vertex layout of the trace shader (vec2 per vertex,
 * drawn as a line strip).
 *
 * @since 3.0.0
 */
struct TraceVertices
{
    int first_sample = 0;  /**< Absolute sample at x = 0. */
    std::vector<float> xy; /**< Interleaved x, y. */

    /** @return The number of vertices. */
    int vertex_count() const
    {
        return static_cast<int>(xy.size() / 2);
    }
};

/**
 * @brief The channels a trace view shows: their description, values and trace vertices.
 *
 * Channels are addressed by their logical index: the position in the list of channels the view scrolls
 * through, after any channel filter of the host.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT TraceSource
{
public:
    /** A channel as the view draws and describes it. */
    struct Channel
    {
        std::string name;          /**< Channel name, e.g. "MEG 0111". */
        std::string type_label;    /**< Channel type, e.g. "MEG grad", "EEG". */
        float amplitude_max = 0.f; /**< Amplitude that fills 45 % of a row, in physical units. */
        Color color;               /**< Trace colour. */
        bool bad = false;          /**< Whether the channel is marked bad. */
    };

    /** A lane of the butterfly mode: all channels of one type overlaid. */
    struct Group
    {
        std::string type_label;    /**< Channel type of the lane. */
        float amplitude_max = 0.f; /**< Amplitude that fills 45 % of the lane. */
    };

    virtual ~TraceSource() = default;

    /** @return The number of logical channels the view scrolls through. */
    virtual int channel_count() const = 0;

    /**
     * @param[in] index   Logical channel index.
     *
     * @return The channel, or nothing if @p index maps to no channel.
     */
    virtual std::optional<Channel> channel(int index) const = 0;

    /**
     * @param[in] index    Logical channel index.
     * @param[in] sample   Absolute sample index.
     *
     * @return The raw value of the channel at @p sample in physical units, 0 if unknown.
     */
    virtual float value_at(int index, int sample) const = 0;

    /**
     * @return The lanes of the butterfly mode, in display order. By default one lane per channel type in the
     *         order of the types' first channels, with the scale of that channel.
     */
    virtual std::vector<Group> groups() const;

    /** @return The first sample that can be scrolled to (the first loaded sample). */
    virtual int first_sample() const
    {
        return 0;
    }

    /** @return The number of loaded samples from first_sample() on. */
    virtual int sample_count() const
    {
        return 0;
    }

    /**
     * Returns the trace of a channel over a sample window, decimated to min/max pairs when the window has more
     * than two samples per pixel (see decimate_trace()).
     *
     * @param[in] index         Logical channel index.
     * @param[in] first         Absolute first sample.
     * @param[in] last          Absolute end sample (exclusive).
     * @param[in] pixel_width   Pixels the window spans.
     *
     * @return The vertices, empty if there is no data; first_sample is clamped to the loaded data.
     */
    virtual TraceVertices vertices([[maybe_unused]] int index, int first, [[maybe_unused]] int last,
                                   [[maybe_unused]] int pixel_width) const
    {
        return {first, {}};
    }
};

} // namespace mne::viz

#endif // VIZ_TRACE_SOURCE_H
