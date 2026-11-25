//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filter_response_plot.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FilterResponsePlot draws the magnitude response of a filter in dB with its cut-off frequencies.
 */

#ifndef SCENE_FILTER_RESPONSE_PLOT_H
#define SCENE_FILTER_RESPONSE_PLOT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "canvas.h"
#include "scene_global.h"
#include "style.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

class TextMetrics;

/**
 * @brief Magnitude response of a filter from 0 Hz to the Nyquist frequency, in dB relative to its maximum.
 *
 * The response is normalised to its largest value, so the pass band sits at 0 dB, and cut at the floor (-100 dB
 * by default). Cut-off frequencies are vertical marker lines. When the response has more points than the plot has
 * pixels, each pixel column draws the lowest and highest value it covers, so narrow notches stay visible.
 *
 * @snippet ex_scene/main.cpp filter_response_usage
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT FilterResponsePlot
{
public:
    /**
     * Sets the response.
     *
     * @param[in] magnitude   Linear magnitude |H(f)| at frequencies spaced evenly from 0 Hz to @p nyquist, both
     *                        included.
     * @param[in] nyquist     Nyquist frequency in Hz, half the sampling frequency.
     */
    void set_response(std::vector<double> magnitude, double nyquist);

    /**
     * Sets the cut-off frequencies drawn as markers.
     *
     * @param[in] frequencies   Frequencies in Hz; those outside 0 Hz to the Nyquist frequency are not drawn.
     */
    void set_cutoffs(std::vector<double> frequencies);

    /**
     * Sets the title above the plot, e.g. the filter's name.
     *
     * @param[in] title   Title text.
     */
    void set_title(std::string title);

    /**
     * Sets the lowest level shown; lower values are drawn at it.
     *
     * @param[in] db   Floor in dB, below 0 (default -100).
     */
    void set_floor_db(double db);

    /**
     * Sets the colours.
     *
     * @param[in] foreground   Text, frame, grid and response (default black).
     * @param[in] cutoff       Cut-off markers (default red).
     */
    void set_colors(Color foreground, Color cutoff = {220, 0, 0});

    /** @return The response in dB relative to its maximum, cut at the floor; empty without a response. */
    std::vector<double> magnitude_db() const;

    /** @return The Nyquist frequency in Hz. */
    double nyquist() const
    {
        return m_nyquist;
    }

    /** @return The cut-off frequencies in Hz. */
    const std::vector<double>& cutoffs() const
    {
        return m_cutoffs;
    }

    /**
     * Draws the plot onto @p canvas, filling its size.
     *
     * @param[in, out] canvas    Canvas to draw on.
     * @param[in]      metrics   Text metrics of the backend, for the margins.
     * @param[in]      font      Font of the tick labels; the title is larger and bold.
     */
    void draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const;

    /**
     * @param[in] width     Width of the figure in pixels.
     * @param[in] height    Height of the figure in pixels.
     * @param[in] metrics   Text metrics of the backend.
     * @param[in] font      Font of the tick labels.
     *
     * @return The plot as a display list.
     */
    DisplayList render(double width, double height, const TextMetrics& metrics, const Font& font) const;

private:
    std::vector<double> m_magnitude;
    std::vector<double> m_cutoffs;
    std::string m_title;
    double m_nyquist = 0.0;
    double m_floorDb = -100.0;
    Color m_foreground = Color::black();
    Color m_cutoff{220, 0, 0};
};

} // namespace mne::scene

#endif // SCENE_FILTER_RESPONSE_PLOT_H
