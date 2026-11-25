//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filterplot.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FilterPlot widget: the magnitude response of a filter, an mne::scene::FilterResponsePlot drawn with QPainter.
 *
 * Replaces FilterPlotScene, which drew the same plot as items of a QGraphicsScene. The plot fills the widget,
 * follows the palette's text colour and exports as vector SVG or PNG through figure().
 */

#ifndef FILTERPLOT_H
#define FILTERPLOT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_widgets_global.h"

#include <mne/scene/filter_response_plot.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FilterKernel;
}

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

//=============================================================================================================
/**
 * @brief Magnitude response of a FIR or IIR filter in dB, with its cut-off frequencies marked.
 *
 * @snippet ex_disp_plots/main.cpp filter_plot_usage
 *
 * @since 3.0.0
 */
class DISPWIDGETSSHARED_EXPORT FilterPlot : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs an empty plot.
     *
     * @param[in] parent   Parent widget.
     */
    explicit FilterPlot(QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Shows the response of @p filter: its FFT magnitude from 0 Hz to half its sampling frequency, titled with its
     * name, with its high-pass and low-pass frequencies marked where they lie inside that range.
     *
     * @param[in] filter   The filter; one without coefficients leaves the plot unchanged.
     */
    void setFilter(const mne::FilterKernel& filter);

    //=========================================================================================================
    /**
     * @return The plot model.
     */
    const scene::FilterResponsePlot& plot() const
    {
        return m_plot;
    }

    //=========================================================================================================
    /**
     * The plot as a figure, e.g. to export it with scene::write_svg() or render it with paintScene().
     *
     * @param[in] size   Size of the figure in pixels.
     *
     * @return The figure on a white background, laid out with this widget's font.
     */
    scene::DisplayList figure(const QSize& size) const;

protected:
    //=========================================================================================================
    /**
     * Draws the plot over the widget's background, in the palette's text colour.
     *
     * @param[in] event   The paint event.
     */
    void paintEvent(QPaintEvent* event) override;

private:
    scene::FilterResponsePlot m_plot; /**< The plot, drawn through the QPainter backend. */
};

} // namespace mne::disp

#endif // FILTERPLOT_H
