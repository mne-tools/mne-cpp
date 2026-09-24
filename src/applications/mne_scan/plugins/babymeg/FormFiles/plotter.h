//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     plotter.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Plotter class declaration.
 */

#ifndef PLOTTER_H
#define PLOTTER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../babymeg_global.h"

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
    void setCurveData(int id, const QVector <QPointF>  &curveData);
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
