//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     butterfly_plot.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ButterflyPlot overlays the evoked responses of many channels, with stimulus, baseline and time markers.
 */

#ifndef VIZ_BUTTERFLY_PLOT_H
#define VIZ_BUTTERFLY_PLOT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "canvas.h"
#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief Evoked responses of many channels drawn on top of each other, as in MNE-Python's butterfly plot.
 *
 * Each channel's traces are scaled so that its range fills half the height above and below the zero line; bad
 * channels are drawn faint. A dashed red line marks the stimulus, a light red band the baseline interval and
 * dashed grey lines every 50 ms, labelled at the bottom.
 *
 * Geometry follows the single-precision arithmetic of the QPainter code it replaces, so the QPainter backend
 * draws the same pixels as before.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT ButterflyPlot
{
public:
    /** One average of a channel. */
    struct Trace
    {
        std::vector<double> samples; /**< Values, one per sample. */
        Color color;                 /**< Line colour. */
    };

    /** One channel. */
    struct Channel
    {
        std::vector<Trace> traces; /**< Averages, drawn in order. */
        double max_value = 1.0;    /**< Value drawn at the top edge. */
        bool bad = false;          /**< Bad channels are drawn faint. */
    };

    /**
     * Sets the time axis.
     *
     * @param[in] samples          Samples per trace.
     * @param[in] preStimSamples   Samples before the stimulus.
     * @param[in] sfreq            Sampling frequency in Hz.
     */
    void set_timing(int samples, int preStimSamples, float sfreq);

    /**
     * Sets the baseline interval in seconds relative to the stimulus.
     *
     * @param[in] from   Start in seconds.
     * @param[in] to     End in seconds.
     */
    void set_baseline(float from, float to);

    /**
     * Shows or hides the lines every 50 ms.
     *
     * @param[in] on   Whether the lines are shown.
     */
    void set_time_spacers(bool on);

    /**
     * Sets the background colour.
     *
     * @param[in] color   Background colour.
     */
    void set_background(Color color);

    /**
     * Draws the traces half transparent, to compare a frozen average with live data.
     *
     * @param[in] frozen   Whether the traces are frozen.
     */
    void set_frozen(bool frozen);

    /**
     * Sets the channels, drawn in order.
     *
     * @param[in] channels   Channels in drawing order.
     */
    void set_channels(std::vector<Channel> channels);

    /** @return The channels. */
    const std::vector<Channel>& channels() const
    {
        return m_channels;
    }

    /**
     * Draws the plot into the rectangle (0, 0, width, height) of @p canvas.
     *
     * @param[in] canvas   Canvas.
     * @param[in] font     Font of the time labels.
     */
    void draw(Canvas& canvas, const Font& font) const;

    /**
     * @param[in] width   Width in pixels.
     * @param[in] height   Height in pixels.
     * @param[in] font   Font of the time labels.
     *
     * @return The plot as a figure of @p width x @p height.
     */
    DisplayList render(double width, double height, const Font& font) const;

private:
    void drawChannel(Canvas& canvas, const Channel& channel, int width, int height) const;

    int m_samples = 0;
    int m_preStimSamples = 0;
    float m_sfreq = 1.0f;
    float m_baselineFrom = 0.0f;
    float m_baselineTo = 0.0f;
    bool m_timeSpacers = true;
    bool m_frozen = false;
    Color m_background = Color::white();
    std::vector<Channel> m_channels;
};

} // namespace mne::viz

#endif // VIZ_BUTTERFLY_PLOT_H
