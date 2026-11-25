//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     lineplot.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Simple x/y line-series widget, an mne::scene::LineChart drawn with QPainter.
 *
 * LinePlot accepts one or several @c std::vector<double> y-series (with an
 * optional matching x-series) and renders them as connected polylines
 * with auto-scaled axes, a title and x/y labels. It is intentionally
 * lightweight (no Qt Charts / OpenGL dependency) and is the fallback
 * plotting primitive used by HPI / coregistration dialogs and by
 * diagnostic test apps that just need a small embedded line chart.
 */

#ifndef LINEPLOT_H
#define LINEPLOT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_widgets_global.h"
#include <mne/scene/line_chart.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QWidget>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

//=============================================================================================================
// mne::disp FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Simple x / y line-series QWidget rendered with QPainter (no Qt Charts dependency).
 *
 * Supports multiple series, a settable title and x / y labels and
 * auto-ranging axes. Series are stored as @c std::vector<double> pairs
 * and re-drawn during @c paintEvent.
 *
 * @snippet ex_disp_plots/main.cpp line_plot_usage
 */
class DISPWIDGETSSHARED_EXPORT LinePlot : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a line series plot
     *
     * @param[in] parent    If parent is nullptr, the new widget becomes a window.
     */
    LinePlot(QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Constructs a line series plot
     *
     * @param[in] y         The double data vector.
     * @param[in] title     Plot title.
     * @param[in] parent    If parent is nullptr, the new widget becomes a window.
     */
    LinePlot(const std::vector<double>& y,
             const QString& title = "",
             QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Constructs a line series plot
     *
     * @param[in] x         X-Axis data to plot.
     * @param[in] y         Y-Axis data to plot.
     * @param[in] title     Plot title.
     * @param[in] parent    If parent is nullptr, the new widget becomes a window.
     */
    LinePlot(const std::vector<double>& x,
             const std::vector<double>& y,
             const QString& title = "",
             QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Destructs the line series plot
     */
    virtual ~LinePlot();

    //=========================================================================================================
    /**
     * Sets the plot title.
     *
     * @param[in] p_sTitle   The title.
     */
    void setTitle(const QString& p_sTitle);

    //=========================================================================================================
    /**
     * Sets the label of the x axis
     *
     * @param[in] p_sXLabel   The x axis label.
     */
    void setXLabel(const QString& p_sXLabel);

    //=========================================================================================================
    /**
     * Sets the label of the y axis
     *
     * @param[in] p_sYLabel   The y axis label.
     */
    void setYLabel(const QString& p_sYLabel);

    //=========================================================================================================
    /**
     * Updates the plot using a given double vector without given X data.
     *
     * @param[in] y          The double data vector.
     */
    void updateData(const std::vector<double>& y);

    //=========================================================================================================
    /**
     * Updates the plot using the given vectors.
     *
     * @param[in] x         X-Axis data to plot.
     * @param[in] y         Y-Axis data to plot.
     */
    void updateData(const std::vector<double>& x,
                    const std::vector<double>& y);

    //=========================================================================================================
    /**
     * Adds a series to the plot; the axes grow to include it and a legend lists the labelled series.
     *
     * @param[in] x         X-Axis data to plot.
     * @param[in] y         Y-Axis data to plot.
     * @param[in] label     Legend entry; series without a label are not listed.
     * @param[in] color     Line colour; an invalid colour takes the first palette colour no other series uses.
     * @param[in] style     Line style, e.g. Qt::DashLine for a series that may coincide with another.
     */
    void addSeries(const std::vector<double>& x,
                   const std::vector<double>& y,
                   const QString& label = QString(),
                   const QColor& color = QColor(),
                   Qt::PenStyle style = Qt::SolidLine);

    //=========================================================================================================
    /**
     * Returns the number of series in the plot.
     *
     * @return The number of series.
     */
    int seriesCount() const;

protected:
    //=========================================================================================================
    /**
     * Paints the line plot.
     *
     * @param[in] event  The paint event.
     */
    void paintEvent(QPaintEvent* event) override;

private:
    scene::LineChart m_plot; /**< The plot, drawn through the QPainter backend. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // LINEPLOT_H
