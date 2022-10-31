//=============================================================================================================
/**
 * @file     plotter.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Plotter class declaration.
 *
 */

#ifndef PLOTTER_H
#define PLOTTER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QWidget>
#include <QMap>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE BABYMEGPLUGIN
//=============================================================================================================

namespace BABYMEGPLUGIN
{

//=============================================================================================================
/**
 * The PlotSettings class provides a PlotSettings for the SQUID control widget.
 *
 * @brief The PlotSettings class provides a PlotSettings for the SQUID control widget.
 */
class BABYMEGSHARED_EXPORT PlotSettings
{

public:
    PlotSettings();

    void scroll(int dx, int dy);
    void adjust();
    double spanX() const { return maxX - minX; }
    double spanY() const { return maxY - minY; }

    double      minX;
    double      maxX;
    int         numXTicks;
    int         numYTicks;
    double      minY;
    double      maxY;
    QString     xlabel;
    QString     ylabel;

private:
    static void adjustAxis(double &min, double &max, int &numTicks);
};

//=============================================================================================================
/**
 * The Plotter class provides a Plotter for the SQUID control widget.
 *
 * @brief The Plotter class provides a Plotter for the SQUID control widget.
 */
class BABYMEGSHARED_EXPORT Plotter : public QWidget
{
    Q_OBJECT

public:
    Plotter(QWidget *parent=0);

    void setPlotSettings(const PlotSettings &settings);
    void setCurveData(int id, const QVector <QPointF>  &data);
    void clearCurve(int id);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

//public slots:
//    void zoomIn();
//    void zoomOut();

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    void updateRubberBandRegion();
    void refreshPixmap();
    void drawGrid(QPainter *painter);
    void drawCurve(QPainter *painter);
    void drawRotatedText(QPainter *painter, int x, int y, const QString &text);

    enum {Margin = 30, xMargin = 80};

//    QToolButton *zoomInButton;
//    QToolButton *zoomOutButton;

    QMap<int, QVector<QPointF> >    curveMap;
    QVector<PlotSettings>           zoomStack;
    int                             curZoom;
    bool                            rubberBandIsShown;
    QRect                           rubberBandRect;
    QPixmap                         pixmap;
};
} // NAMESPACE

#endif // PLOTTER_H
