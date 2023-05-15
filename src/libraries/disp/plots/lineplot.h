//=============================================================================================================
/**
 * @file     lineplot.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    LinePlot class declaration.
 *
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

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIBFORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Line Plot based on QtCharts
 *
 * @brief Line Plot
 */


#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class DISPSHARED_EXPORT LinePlot : public QtCharts::QChartView
#else
class DISPSHARED_EXPORT LinePlot : public QChartView
#endif
{
    Q_OBJECT

public:
    typedef QSharedPointer<LinePlot> SPtr;            /**< Shared pointer type for LinePlot. */
    typedef QSharedPointer<const LinePlot> ConstSPtr; /**< Const shared pointer type for LinePlot. */

    //=========================================================================================================
    /**
     * Constructs a line series plot
     *
     * @param[in] parent    If parent is Q_NULLPTR, the new widget becomes a window. If parent is another widget, this widget becomes a child window inside parent. The new widget is deleted when its parent is deleted.
     */
    LinePlot(QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Constructs a line series plot
     *
     * @param[in] y         The double data vector.
     * @param[in] title     Plot title.
     * @param[in] parent    If parent is Q_NULLPTR, the new widget becomes a window. If parent is another widget, this widget becomes a child window inside parent. The new widget is deleted when its parent is deleted.
     */
    LinePlot(const QVector<double>& y,
             const QString& title = "",
             QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Constructs a line series plot
     *
     * @param[in] x         X-Axis data to plot.
     * @param[in] y         Y-Axis data to plot.
     * @param[in] title     Plot title.
     * @param[in] parent    If parent is Q_NULLPTR, the new widget becomes a window. If parent is another widget, this widget becomes a child window inside parent. The new widget is deleted when its parent is deleted.
     */
    LinePlot(const QVector<double>& x,
             const QVector<double>& y,
             const QString& title = "",
             QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destructs the line series plot
     */
    virtual ~LinePlot();

    //=========================================================================================================
    /**
     * Sets the scaled image view title.
     *
     * @param[in] p_sTitle   The title.
     */
    void setTitle(const QString &p_sTitle);

    //=========================================================================================================
    /**
     * Sets the label of the y axes
     *
     * @param[in] p_sXLabel   The x axes label.
     */
    void setXLabel(const QString &p_sXLabel);

    //=========================================================================================================
    /**
     * Sets the label of the y axes
     *
     * @param[in] p_sXLabel   The y axes label.
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

private:
    //=========================================================================================================
    /**
     * Updates the plot.
     */
    void update();

private:
    QString                 m_sTitle;           /**< Title. */
    QString                 m_sXLabel;          /**< X axes label. */
    QString                 m_sYLabel;          /**< Y axes label. */



#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QtCharts::QLineSeries*  m_pLineSeries;      /**< Line series. */
    QtCharts::QChart*       m_pChart;           /**< The chart. */
#else
    QLineSeries*            m_pLineSeries;      /**< Line series. */
    QChart*                 m_pChart;           /**< The chart. */
#endif
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // LINEPLOT_H
