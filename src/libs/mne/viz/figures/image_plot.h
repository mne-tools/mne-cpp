//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     image_plot.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ImagePlot draws a matrix as a colour-mapped image with axes and a colour bar, like imagesc.
 */

#ifndef VIZ_IMAGE_PLOT_H
#define VIZ_IMAGE_PLOT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "canvas.h"
#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class TextMetrics;

/**
 * @brief A matrix as an image: each value coloured by a colour map, with axes, title and a colour bar.
 *
 * Without an extent, the axes count columns and rows; with one, the first and last column and row centres
 * sit at the given data values. The colour range is the data range unless set.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT ImagePlot
{
public:
    ImagePlot();

    /**
     * Sets the matrix; nothing changes if it is empty.
     *
     * @param[in] rows     Rows.
     * @param[in] cols     Columns.
     * @param[in] values   rows * cols values, row by row.
     */
    void set_data(int rows, int cols, std::vector<double> values);

    /** @return The rows of the matrix. */
    int rows() const
    {
        return m_rows;
    }

    /** @return The columns of the matrix. */
    int cols() const
    {
        return m_cols;
    }

    /** @return The matrix as given, row by row. */
    const std::vector<double>& values() const
    {
        return m_values;
    }

    /** @return Whether a matrix is set. */
    bool has_data() const
    {
        return m_rows > 0 && m_cols > 0;
    }

    /**
     * Sets the colour map by name (see map_color()).
     *
     * @param[in] name   Colour map name.
     */
    void set_colormap(std::string name);

    /** @return The colour map name. */
    const std::string& colormap() const
    {
        return m_colormap;
    }

    /**
     * Maps @p min to the low and @p max to the high end of the colour map, instead of the data range.
     *
     * @param[in] min   Value at the low end of the colour map.
     * @param[in] max   Value at the high end of the colour map.
     */
    void set_color_range(double min, double max);

    /** @return The value at the low end of the colour map. */
    double color_min() const
    {
        return m_min;
    }

    /** @return The value at the high end of the colour map. */
    double color_max() const
    {
        return m_max;
    }

    /**
     * Places the first and last column centres at @p left and @p right, the last and first row at @p bottom and @p top.
     *
     * @param[in] left     X value of the first column.
     * @param[in] right    X value of the last column.
     * @param[in] bottom   Y value of the last row.
     * @param[in] top      Y value of the first row.
     */
    void set_extent(double left, double right, double bottom, double top);

    /** @return The extent as left, right, bottom and top; all 0 until set_extent(). */
    std::array<double, 4> extent() const
    {
        return {m_left, m_right, m_bottom, m_top};
    }

    /**
     * Sets the title above the image.
     *
     * @param[in] title   Title text.
     */
    void set_title(std::string title);

    /**
     * Sets the label below the x axis.
     *
     * @param[in] label   Label text.
     */
    void set_x_label(std::string label);

    /**
     * Sets the label left of the y axis.
     *
     * @param[in] label   Label text.
     */
    void set_y_label(std::string label);

    /**
     * Sets the label of the colour bar.
     *
     * @param[in] label   Label text.
     */
    void set_colorbar_label(std::string label);

    /** @return The label of the colour bar. */
    const std::string& colorbar_label() const
    {
        return m_colorbarLabel;
    }

    /** @return The matrix mapped to [0, 1] by the colour range, row by row. */
    const std::vector<double>& normalized() const
    {
        return m_normalized;
    }

    /** @return The colour-mapped matrix, one pixel per value; null without data. */
    const std::shared_ptr<const Image>& image() const
    {
        return m_image;
    }

    /**
     * Draws the plot into the rectangle (0, 0, width, height) of @p canvas; nothing without data.
     *
     * @param[in] canvas    Canvas.
     * @param[in] metrics   Measures the labels.
     * @param[in] font      Font of ticks and labels; the title is bold and 1.25 times larger.
     */
    void draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const;

    /**
     * @param[in] width   Width in pixels.
     * @param[in] height   Height in pixels.
     * @param[in] metrics   Measures the text.
     * @param[in] font   Font of the text.
     *
     * @return The plot as a figure of @p width x @p height on white.
     */
    DisplayList render(double width, double height, const TextMetrics& metrics, const Font& font) const;

private:
    void update();

    int m_rows = 0;
    int m_cols = 0;
    std::vector<double> m_values;
    std::vector<double> m_normalized;
    std::string m_colormap = "Hot";
    double m_min = 0.0;
    double m_max = 0.0;
    bool m_rangeSet = false;
    double m_left = 0.0;
    double m_right = 0.0;
    double m_bottom = 0.0;
    double m_top = 0.0;
    bool m_extentSet = false;
    std::string m_title;
    std::string m_xLabel;
    std::string m_yLabel;
    std::string m_colorbarLabel;
    int m_colorbarWidth = 14;
    int m_colorbarSteps = 256;
    std::shared_ptr<const Image> m_image;
    std::shared_ptr<const Image> m_colorbar;
};

} // namespace mne::viz

#endif // VIZ_IMAGE_PLOT_H
