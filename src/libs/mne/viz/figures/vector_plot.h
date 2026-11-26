//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     vector_plot.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    VectorPlot draws vectors over their sample index in a fixed frame, like MATLAB's plot(y).
 */

#ifndef VIZ_VECTOR_PLOT_H
#define VIZ_VECTOR_PLOT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "canvas.h"
#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class TextMetrics;

/**
 * @brief Vectors as blue lines over their sample index, in a frame 100 pixels from the sides and 50 from top
 * and bottom, with ticks, a 20 pixel bold title and 12 pixel axis labels.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT VectorPlot
{
public:
    static constexpr double kBorderLeftRight = 100.0; /**< Space left and right of the frame. */
    static constexpr double kBorderTopBottom = 50.0;  /**< Space above and below the frame. */

    /**
     * Replaces the plotted vectors by @p values; nothing changes if it is empty.
     *
     * @param[in] values   Values over their index.
     */
    void set_data(const std::vector<double>& values);

    /**
     * Adds @p values to the plotted vectors; nothing changes if it is empty.
     *
     * @param[in] values   Values over their index.
     */
    void add_data(const std::vector<double>& values);

    /** @return The plotted vectors. */
    const std::vector<std::vector<double>>& data() const
    {
        return m_data;
    }

    /**
     * Sets the title above the frame.
     *
     * @param[in] title   Title text.
     */
    void set_title(std::string title);

    /**
     * Sets the label below the frame.
     *
     * @param[in] label   Label text.
     */
    void set_x_label(std::string label);

    /**
     * Sets the label left of the frame.
     *
     * @param[in] label   Label text.
     */
    void set_y_label(std::string label);

    /**
     * Draws the plot into the rectangle (0, 0, width, height) of @p canvas; nothing without data.
     *
     * @param[in] canvas    Canvas.
     * @param[in] metrics   Measures the axis font.
     */
    void draw(Canvas& canvas, const TextMetrics& metrics) const;

    /**
     * @param[in] width   Width in pixels.
     * @param[in] height   Height in pixels.
     * @param[in] metrics   Measures the text.
     *
     * @return The plot as a figure of @p width x @p height on white.
     */
    DisplayList render(double width, double height, const TextMetrics& metrics) const;

    /** @return The font of ticks and axis labels, 12 pixels. */
    static Font axes_font();

    /** @return The font of the title, 20 pixels bold. */
    static Font title_font();

private:
    std::vector<std::vector<double>> m_data;
    double m_minY = 0.0;
    double m_maxY = 0.0;
    double m_maxX = 0.0;
    std::string m_title;
    std::string m_xLabel;
    std::string m_yLabel;
};

} // namespace mne::viz

#endif // VIZ_VECTOR_PLOT_H
