//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filter_response_figure.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FilterResponseFigure: the magnitude response of a FilterKernel in dB up to the Nyquist frequency.
 */

#ifndef VIZ_FILTER_RESPONSE_FIGURE_H
#define VIZ_FILTER_RESPONSE_FIGURE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "display_list.h"
#include "filter_response_plot.h"
#include "scene_figure.h"

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{
class FilterKernel;
} // namespace mne

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief The magnitude response of a filter kernel in dB from 0 Hz to the Nyquist frequency, with its cut-off
 * frequencies marked, as a SceneFigure.
 *
 * The figure is not zoomable: it lays the FilterResponsePlot out to the view size. picture() renders it at any
 * size, e.g. to export it with write_svg() or to_raster().
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT FilterResponseFigure : public SceneFigure
{
public:
    //=========================================================================================================
    /**
     * Shows the response of @p filter, titled with its name; filters without coefficients leave the figure as
     * it is.
     *
     * @param[in] filter   The filter kernel.
     */
    void set_filter(const mne::FilterKernel& filter);

    //=========================================================================================================
    /**
     * @param[in] foreground   Colour of the axes, labels and response.
     * @param[in] cutoff       Colour of the cut-off markers.
     */
    void set_colors(Color foreground, Color cutoff = {220, 0, 0});

    //=========================================================================================================
    /**
     * @return The plot with the response shown.
     */
    const FilterResponsePlot& plot() const
    {
        return m_plot;
    }

    //=========================================================================================================
    /**
     * Renders the figure on a white picture of its own.
     *
     * @param[in] width     Width of the picture in pixels.
     * @param[in] height    Height of the picture in pixels.
     * @param[in] metrics   Metrics of the font, to lay out text.
     * @param[in] font      Base font.
     *
     * @return The picture.
     */
    DisplayList picture(double width, double height, const TextMetrics& metrics, const Font& font) const;

    Rect bounds() const override;
    void draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const override;
    bool zoomable() const override;
    void resize(double width, double height) override;

private:
    FilterResponsePlot m_plot; /**< The response, its cut-offs and colours. */
    double m_width = 400.0;    /**< Width of the view. */
    double m_height = 250.0;   /**< Height of the view. */
};

} // namespace mne::viz

#endif // VIZ_FILTER_RESPONSE_FIGURE_H
