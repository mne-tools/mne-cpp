//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     line_chart.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    LineChart draws line series with axes, grid, labels and a legend into a Canvas.
 */

#ifndef VIZ_LINE_CHART_H
#define VIZ_LINE_CHART_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "canvas.h"
#include "../viz_global.h"
#include "style.h"

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

class TextMetrics;

/**
 * @brief Line series over shared axes, like matplotlib's plot().
 *
 * The axes span the data with 5 % padding in y; ticks fall on round numbers with a light grid. Series
 * without colour take the first palette colour no series uses yet. Labelled series get a legend in the corner
 * of the axes that hides the least of the lines, or to the right of the axes if every corner hides some.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT LineChart
{
public:
    /** One series. */
    struct Series
    {
        std::vector<double> x;              /**< X values. */
        std::vector<double> y;              /**< Y values, as many as x. */
        std::string label;                  /**< Legend entry, none if empty. */
        Color color;                        /**< Line colour. */
        LineStyle style = LineStyle::Solid; /**< Dash pattern. */
    };

    /**
     * Sets the title above the axes.
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

    /** @return The title. */
    const std::string& title() const
    {
        return m_title;
    }

    /** @return The x axis label. */
    const std::string& x_label() const
    {
        return m_xLabel;
    }

    /** @return The y axis label. */
    const std::string& y_label() const
    {
        return m_yLabel;
    }

    /**
     * Replaces all series by @p y over x = 0, 1, 2, ...
     *
     * @param[in] y   Y values.
     */
    void set_data(const std::vector<double>& y);

    /**
     * Replaces all series by one series.
     *
     * @param[in] x   X values.
     * @param[in] y   Y values; the shorter of @p x and @p y sets the length.
     */
    void set_data(const std::vector<double>& x, const std::vector<double>& y);

    /**
     * Adds a series; the shorter of @p x and @p y sets its length.
     *
     * @param[in] x       X values.
     * @param[in] y       Y values.
     * @param[in] label   Legend entry, none if empty.
     * @param[in] color   Line colour, or the next unused palette colour.
     * @param[in] style   Dash pattern.
     */
    void add_series(const std::vector<double>& x, const std::vector<double>& y, std::string label = {},
                    std::optional<Color> color = std::nullopt, LineStyle style = LineStyle::Solid);

    /** Removes all series; title and labels stay. */
    void clear();

    /** @return The series. */
    const std::vector<Series>& series() const
    {
        return m_series;
    }

    /**
     * Draws the plot into the rectangle (0, 0, width, height) of @p canvas.
     *
     * @param[in] canvas    Canvas.
     * @param[in] metrics   Measures the labels.
     * @param[in] font      Font of ticks and labels; the title is bold and 1.25 times larger.
     */
    void draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const;

    /**
     * @param[in] width     Width in pixels.
     * @param[in] height    Height in pixels.
     * @param[in] metrics   Measures the labels.
     * @param[in] font      Font of ticks and labels.
     *
     * @return The plot as a figure on white.
     */
    DisplayList render(double width, double height, const TextMetrics& metrics, const Font& font) const;

private:
    std::string m_title;
    std::string m_xLabel;
    std::string m_yLabel;
    std::vector<Series> m_series;
    double m_minX = 0.0;
    double m_maxX = 0.0;
    double m_minY = 0.0;
    double m_maxY = 0.0;
};

} // namespace mne::viz

#endif // VIZ_LINE_CHART_H
