//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Andreas Griesshammer <ag@fieldlineinc.com>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file lineplot.h
 * @since 2022
 * @date  February 2026
 * @brief Simple x/y line-series widget drawn with QPainter.
 *
 * LinePlot accepts one or several @c QVector<double> y-series (with an
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

#include "../disp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSharedPointer>
#include <QVector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Simple x / y line-series QWidget rendered with QPainter (no Qt Charts dependency).
 *
 * Supports multiple series, a settable title and x / y labels and
 * auto-ranging axes. Series are stored as @c QVector<double> pairs
 * and re-drawn during @c paintEvent.
 */
class DISPSHARED_EXPORT LinePlot : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<LinePlot> SPtr;            /**< Shared pointer type for LinePlot. */
    typedef QSharedPointer<const LinePlot> ConstSPtr; /**< Const shared pointer type for LinePlot. */

    //=========================================================================================================
    /**
     * Constructs a line series plot
     *
     * @param[in] parent    If parent is nullptr, the new widget becomes a window.
     */
    LinePlot(QWidget *parent = nullptr);

    //=========================================================================================================
    /**
     * Constructs a line series plot
     *
     * @param[in] y         The double data vector.
     * @param[in] title     Plot title.
     * @param[in] parent    If parent is nullptr, the new widget becomes a window.
     */
    LinePlot(const QVector<double>& y,
             const QString& title = "",
             QWidget *parent = nullptr);

    //=========================================================================================================
    /**
     * Constructs a line series plot
     *
     * @param[in] x         X-Axis data to plot.
     * @param[in] y         Y-Axis data to plot.
     * @param[in] title     Plot title.
     * @param[in] parent    If parent is nullptr, the new widget becomes a window.
     */
    LinePlot(const QVector<double>& x,
             const QVector<double>& y,
             const QString& title = "",
             QWidget *parent = nullptr);

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
    void setTitle(const QString &p_sTitle);

    //=========================================================================================================
    /**
     * Sets the label of the x axis
     *
     * @param[in] p_sXLabel   The x axis label.
     */
    void setXLabel(const QString &p_sXLabel);

    //=========================================================================================================
    /**
     * Sets the label of the y axis
     *
     * @param[in] p_sYLabel   The y axis label.
     */
    void setYLabel(const QString &p_sYLabel);

    //=========================================================================================================
    /**
     * Updates the plot using a given double vector without given X data.
     *
     * @param[in] y          The double data vector.
     */
    void updateData(const QVector<double>& y);

    //=========================================================================================================
    /**
     * Updates the plot using the given vectors.
     *
     * @param[in] x         X-Axis data to plot.
     * @param[in] y         Y-Axis data to plot.
     */
    void updateData(const QVector<double>& x,
                    const QVector<double>& y);

protected:
    //=========================================================================================================
    /**
     * Paints the line plot.
     *
     * @param[in] event  The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

private:
    QString                 m_sTitle;           /**< Title. */
    QString                 m_sXLabel;          /**< X axis label. */
    QString                 m_sYLabel;          /**< Y axis label. */
    QVector<double>         m_vecXData;         /**< X data points. */
    QVector<double>         m_vecYData;         /**< Y data points. */
    double                  m_dMinX;            /**< Minimum X value. */
    double                  m_dMaxX;            /**< Maximum X value. */
    double                  m_dMinY;            /**< Minimum Y value. */
    double                  m_dMaxY;            /**< Maximum Y value. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // LINEPLOT_H
