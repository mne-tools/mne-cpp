//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bar_chart.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    BarChart draws counts per category as bars with a y axis and category labels.
 */

#ifndef VIZ_BAR_CHART_H
#define VIZ_BAR_CHART_H

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
 * @brief Counts per category as bars, e.g. a histogram.
 *
 * Category labels sit under their bars, rotated by 45 degrees when they are wider than a bar.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT BarChart
{
public:
    /**
     * Sets the title above the bars.
     *
     * @param[in] title   Title text.
     */
    void set_title(std::string title);

    /**
     * Sets the label below the categories.
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
     * Sets the bars.
     *
     * @param[in] categories   Label of each bar.
     * @param[in] counts       Height of each bar; the bars are as many as counts.
     */
    void set_bars(std::vector<std::string> categories, std::vector<int> counts);

    /** @return The bar heights. */
    const std::vector<int>& counts() const
    {
        return m_counts;
    }

    /** @return The category labels. */
    const std::vector<std::string>& categories() const
    {
        return m_categories;
    }

    /**
     * Draws the chart into the rectangle (0, 0, width, height) of @p canvas.
     *
     * @param[in] canvas    Canvas.
     * @param[in] metrics   Measures the category labels.
     * @param[in] font      Font of the labels; the title is bold and 2 points larger, categories 1 point smaller.
     */
    void draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const;

    /**
     * @param[in] width   Width in pixels.
     * @param[in] height   Height in pixels.
     * @param[in] metrics   Measures the text.
     * @param[in] font   Font of the text.
     *
     * @return The chart as a figure of @p width x @p height on white.
     */
    DisplayList render(double width, double height, const TextMetrics& metrics, const Font& font) const;

private:
    std::string m_title;
    std::string m_xLabel;
    std::string m_yLabel;
    std::vector<std::string> m_categories;
    std::vector<int> m_counts;
};

} // namespace mne::viz

#endif // VIZ_BAR_CHART_H
