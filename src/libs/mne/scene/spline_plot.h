//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     spline_plot.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SplinePlot draws a histogram as a smooth curve with three threshold lines over a colour map.
 */

#ifndef SCENE_SPLINE_PLOT_H
#define SCENE_SPLINE_PLOT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "canvas.h"
#include "scene_global.h"

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
 * @brief A histogram as a Catmull-Rom curve through the class marks, with left, middle and right thresholds.
 *
 * Once thresholds are set, the background between them shows a colour map: its lower half from the left to
 * the middle threshold, its upper half from the middle to the right one.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT SplinePlot
{
public:
    static constexpr double kLeftMargin = 60.0;   /**< Left margin in pixels. */
    static constexpr double kRightMargin = 20.0;  /**< Right margin in pixels. */
    static constexpr double kTopMargin = 30.0;    /**< Top margin in pixels. */
    static constexpr double kBottomMargin = 60.0; /**< Bottom margin in pixels. */

    /**
     * Sets the title above the plot.
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
     * Sets the histogram and clears the thresholds (left at the minimum, middle and right at the maximum).
     *
     * @param[in] classMarks   Centre and count of each class.
     * @param[in] minX         Lower limit of the first class.
     * @param[in] maxX         Upper limit of the last class.
     */
    void set_classes(std::vector<Point> classMarks, double minX, double maxX);

    /** @return The class marks. */
    const std::vector<Point>& class_marks() const
    {
        return m_marks;
    }

    /** @return The lower end of the x axis. */
    double min_x() const
    {
        return m_minX;
    }

    /** @return The upper end of the x axis. */
    double max_x() const
    {
        return m_maxX;
    }

    /**
     * Sets the three thresholds and shows them.
     *
     * @param[in] left     X value of the left threshold.
     * @param[in] middle   X value of the middle threshold.
     * @param[in] right    X value of the right threshold.
     */
    void set_thresholds(double left, double middle, double right);

    /** @return Whether thresholds are shown. */
    bool has_thresholds() const
    {
        return m_hasThresholds;
    }

    /** @return The left threshold. */
    double left_threshold() const
    {
        return m_left;
    }

    /** @return The middle threshold. */
    double middle_threshold() const
    {
        return m_middle;
    }

    /** @return The right threshold. */
    double right_threshold() const
    {
        return m_right;
    }

    /**
     * Sets the colour map of the background, by name (see map_color()); none if empty.
     *
     * @param[in] name   Colour map name, none if empty.
     */
    void set_colormap(std::string name);

    /**
     * @param[in] x       X value.
     * @param[in] width   Width of the plot in pixels.
     *
     * @return The horizontal pixel of @p x in a plot of @p width.
     */
    double data_to_pixel_x(double x, double width) const;

    /**
     * @param[in] y        Count.
     * @param[in] height   Height of the plot in pixels.
     *
     * @return The vertical pixel of count @p y in a plot of @p height.
     */
    double data_to_pixel_y(double y, double height) const;

    /**
     * @param[in] px      Horizontal pixel.
     * @param[in] width   Width of the plot in pixels.
     *
     * @return The x value at horizontal pixel @p px in a plot of @p width.
     */
    double pixel_to_data_x(double px, double width) const;

    /**
     * Draws the plot into the rectangle (0, 0, width, height) of @p canvas.
     *
     * @param[in] canvas    Canvas.
     * @param[in] metrics   Measures the labels.
     * @param[in] font      Font of the labels; the title is bold.
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
    double max_count() const;

    std::string m_title;
    std::string m_xLabel;
    std::string m_yLabel;
    std::string m_colormap;
    std::vector<Point> m_marks;
    double m_minX = 0.0;
    double m_maxX = 0.0;
    double m_left = 0.0;
    double m_middle = 0.0;
    double m_right = 0.0;
    bool m_hasThresholds = false;
};

} // namespace mne::scene

#endif // SCENE_SPLINE_PLOT_H
