//=============================================================================================================
/**
 * @file     spline.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of spline class
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spline.h"

#include "helpers/colormap.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QChartView>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Spline::Spline(QWidget* parent, const QString& title)
: QWidget(parent)
, m_dMinAxisX(0)
, m_dMaxAxisX(0)
, m_pLeftThreshold(new QLineSeries())
, m_pMiddleThreshold(new QLineSeries())
, m_pRightThreshold(new QLineSeries())
, m_iMaximumFrequency(0)
{
    Q_UNUSED(title)
    m_pChart = new QChart();
    //m_pChart->setTitle(title);
    m_pChart->setAnimationOptions(QChart::SeriesAnimations);
    m_pChart->setAcceptHoverEvents(false);
    m_pSeries = new QSplineSeries();
    QChartView *chartView = new QChartView(m_pChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    QGridLayout* layout = new QGridLayout();
    layout->addWidget(chartView, 0, 0);
    this->setLayout(layout);
}

//=============================================================================================================

void Spline::mousePressEvent(QMouseEvent *event)
{
    if (m_pSeries->count() == 0)               //protect integrity of the histogram widget in case series contain no data values
    {
        qDebug() << "Data set not found.";  //do nothing
    }

    else
    {
        QXYSeries *shadowSeries = qobject_cast<QXYSeries *>(sender());
        QLineSeries *verticalLine = new QLineSeries();
        QPointF point = event->pos();
        QPointF pointY = point;
        pointY.setX(m_dMinAxisX);
        pointY.setY(pointY.y()-10.5);   //-10.5 needed to correctly position the line at mouse position
        point.setX(point.x()-10.5);
        point.setY(0);

        QPointF localX = m_pChart->mapToValue(point, shadowSeries);
        QPointF localY = m_pChart->mapToValue(pointY, shadowSeries);
        verticalLine->append(localX.x(), 0);
        verticalLine->append(localX.x(), m_iMaximumFrequency);
        double boundaryX = double(localX.x());   //casting localX.x() from float to double for comparison with minAxisX and maxAxisX
        double boundaryY = double(localY.y());   //casting localY.y() from float to double for comparison with 0 and the maximum frequency

        if((boundaryX >= float(m_dMinAxisX)) && (boundaryX <= float(m_dMaxAxisX)))  //this condition ensures that threshold lines can only be created within the boundary of the x-axis
        {
            if((boundaryY >= 0.0) && (boundaryY <= float(m_iMaximumFrequency)))  // this condition ensures that threshold lines can only be created within the boundary of the y-axis
            {
                QVector<QPointF> middlePoint = m_pMiddleThreshold->pointsVector();   //Point values need to be updated before tested and displayed on the widget
                QVector<QPointF> rightPoint = m_pRightThreshold->pointsVector();
                QVector<QPointF> leftPoint = m_pLeftThreshold->pointsVector();

                double emitLeft = (leftPoint[0].x() * (pow(10, m_vecResultExponentValues[0])));
                double emitMiddle = (middlePoint[0].x() * (pow(10, m_vecResultExponentValues[0])));
                double emitRight = (rightPoint[0].x() * (pow(10, m_vecResultExponentValues[0])));

                if (event->buttons() == Qt::LeftButton)
                {
                    leftPoint = verticalLine->pointsVector();
                    if((leftPoint[0].x() < middlePoint[0].x()) && (leftPoint[0].x() < rightPoint[0].x()))
                    {
                        m_pChart->removeSeries(m_pLeftThreshold);
                        m_pLeftThreshold = verticalLine;
                        m_pLeftThreshold->setName("left");
                        updateThreshold(m_pLeftThreshold);
                        emitLeft = (leftPoint[0].x() * (pow(10, m_vecResultExponentValues[0])));
                        emit borderChanged(emitLeft, emitMiddle, emitRight);
                    }
                }

                if (event->buttons() == Qt::MiddleButton)
                {
                    middlePoint = verticalLine->pointsVector();
                    if((middlePoint[0].x() > leftPoint[0].x()) && (middlePoint[0].x() < rightPoint[0].x()))
                    {
                        m_pChart->removeSeries(m_pMiddleThreshold);
                        m_pMiddleThreshold = verticalLine;
                        m_pMiddleThreshold->setName("middle");
                        updateThreshold(m_pMiddleThreshold);
                        emitMiddle = (middlePoint[0].x() * (pow(10, m_vecResultExponentValues[0])));
                        emit borderChanged(emitLeft, emitMiddle, emitRight);
                    }
                }

                if (event->buttons() == Qt::RightButton)
                {
                    rightPoint = verticalLine->pointsVector();
                    if((rightPoint[0].x() > leftPoint[0].x()) && (rightPoint[0].x() > middlePoint[0].x()))
                    {
                        m_pChart->removeSeries(m_pRightThreshold);
                        m_pRightThreshold = verticalLine;
                        m_pRightThreshold->setName("right");
                        updateThreshold(m_pRightThreshold);
                        emitRight = (rightPoint[0].x() * (pow(10, m_vecResultExponentValues[0])));
                        emit borderChanged(emitLeft, emitMiddle, emitRight);
                    }
                }
            }
        }
        setColorMap(m_colorMap);
    }
}

//=============================================================================================================

void Spline::setThreshold(const QVector3D& vecThresholdValues)
{
    float leftThresholdValue = vecThresholdValues.x();
    float middleThresholdValue = vecThresholdValues.y();
    float rightThresholdValue = vecThresholdValues.z();

    QVector3D correctedVectorThreshold = correctionDisplayTrueValue(vecThresholdValues, "up");

    if (m_pSeries->count() == 0)               //protect integrity of the histogram widget in case series contain no data values
    {
        qDebug() << "Data set not found.";
    }

//    the condition below tests the threshold values given and ensures that all three must be within minAxisX and maxAxisX otherwise they will be given either minAxisX or maxAxisX value
    else if (correctedVectorThreshold.x() < m_dMinAxisX || correctedVectorThreshold.y() < m_dMinAxisX || correctedVectorThreshold.z() < m_dMinAxisX || correctedVectorThreshold.x() > m_dMaxAxisX || correctedVectorThreshold.y() > m_dMaxAxisX || correctedVectorThreshold.z() > m_dMaxAxisX)
    {
        qDebug() << "One or more of the values given are out of the minimum and maximum range. Changed to default thresholds.";
        leftThresholdValue = 1.01 * m_dMinAxisX;
        middleThresholdValue = (m_dMinAxisX + m_dMaxAxisX) / 2;
        rightThresholdValue = 0.99 * m_dMaxAxisX;
    }
    else
    {
        if ((correctedVectorThreshold.x() < correctedVectorThreshold.y()) && (correctedVectorThreshold.x() < correctedVectorThreshold.z()))
        {
            leftThresholdValue = correctedVectorThreshold.x();
            if(correctedVectorThreshold.y() < correctedVectorThreshold.z())
            {
                middleThresholdValue = correctedVectorThreshold.y();
                rightThresholdValue = correctedVectorThreshold.z();
            }
            else
            {
                middleThresholdValue = correctedVectorThreshold.z();
                rightThresholdValue = correctedVectorThreshold.y();
            }
        }
        if ((correctedVectorThreshold.y() < correctedVectorThreshold.x()) && (correctedVectorThreshold.y() < correctedVectorThreshold.z()))
        {
            leftThresholdValue = correctedVectorThreshold.y();

            if(correctedVectorThreshold.x() < correctedVectorThreshold.z())
            {
                middleThresholdValue = correctedVectorThreshold.x();
                rightThresholdValue = correctedVectorThreshold.z();
            }
            else
            {
                middleThresholdValue = correctedVectorThreshold.z();
                rightThresholdValue = correctedVectorThreshold.x();
            }
        }
        if ((correctedVectorThreshold.z() < correctedVectorThreshold.x()) && (correctedVectorThreshold.z() < correctedVectorThreshold.y()))
        {
            leftThresholdValue = correctedVectorThreshold.z();

            if(correctedVectorThreshold.x() < correctedVectorThreshold.y())
            {
                middleThresholdValue = correctedVectorThreshold.x();
                rightThresholdValue = correctedVectorThreshold.y();
            }
            else
            {
                middleThresholdValue = correctedVectorThreshold.y();
                rightThresholdValue = correctedVectorThreshold.x();
            }
        }
    }

    QPointF leftThresholdPoint;
    QPointF middleThresholdPoint;
    QPointF rightThresholdPoint;

    leftThresholdPoint.setX(leftThresholdValue);
    middleThresholdPoint.setX(middleThresholdValue);
    rightThresholdPoint.setX(rightThresholdValue);

    m_pLeftThreshold->append(leftThresholdPoint.x(), 0);
    m_pLeftThreshold->append(leftThresholdPoint.x(), m_iMaximumFrequency);
    m_pMiddleThreshold->append(middleThresholdPoint.x(), 0);
    m_pMiddleThreshold->append(middleThresholdPoint.x(), m_iMaximumFrequency);
    m_pRightThreshold->append(rightThresholdPoint.x(), 0);
    m_pRightThreshold->append(rightThresholdPoint.x(), m_iMaximumFrequency);

    updateThreshold(m_pLeftThreshold);
    updateThreshold(m_pMiddleThreshold);
    updateThreshold(m_pRightThreshold);
    setColorMap(m_colorMap);
}

//=============================================================================================================

void Spline::updateThreshold(QLineSeries* lineSeries)
{
    if (lineSeries->name() == "left")
    {
        lineSeries->setColor("red");
    }
    else if (lineSeries->name() == "middle")
    {
        lineSeries->setColor("green");
    }
    else if (lineSeries->name() == "right")
    {
        lineSeries->setColor("blue");
    }
    else
    {
        qDebug()<< "Error: lineSeries->name() is not 'left', 'middle' or 'right'.";
    }
    lineSeries->setVisible(true);
    m_pChart->addSeries(lineSeries);
    m_pChart->legend()->markers().at(m_pChart->legend()->markers().size()-1)->setVisible(false);
    m_pChart->createDefaultAxes();
}

//=============================================================================================================

void Spline::setColorMap(const QString& colorMap)
{
    m_colorMap = colorMap;
    double leftThresholdValue = (m_pLeftThreshold->at(0).x())/ m_dMaxAxisX;
    double middleThresholdValue = (m_pMiddleThreshold->at(0).x())/ m_dMaxAxisX;
    double rightThresholdValue = (m_pRightThreshold->at(0).x())/ m_dMaxAxisX;
    int stepsNumber = 25;
    double stepsSizeLeftMiddle = (middleThresholdValue - leftThresholdValue) / stepsNumber;
    double stepsSizeMiddleRight = (rightThresholdValue - middleThresholdValue) / stepsNumber;
    QLinearGradient plotAreaGradient;
    plotAreaGradient.setStart(QPointF(0, 0));
    plotAreaGradient.setFinalStop(QPointF(1, 0));

    plotAreaGradient.setColorAt(leftThresholdValue, ColorMap::valueToColor(0.0, colorMap));

    for (int i = 1; i < stepsNumber; ++i)
    {
        plotAreaGradient.setColorAt(leftThresholdValue + (stepsSizeLeftMiddle * i), ColorMap::valueToColor((double)i * (0.5 / (double)stepsNumber), colorMap));
        plotAreaGradient.setColorAt(middleThresholdValue + (stepsSizeMiddleRight * i), ColorMap::valueToColor((double)0.5 + (i * (0.5 / (double)stepsNumber)), colorMap));
    }
    plotAreaGradient.setColorAt(rightThresholdValue, ColorMap::valueToColor(1.0, colorMap));

    // Customize plot area background
    plotAreaGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    m_pChart->setPlotAreaBackgroundBrush(plotAreaGradient);
    m_pChart->setPlotAreaBackgroundVisible(true);
}

//=============================================================================================================

const QVector3D& Spline::getThreshold()
{
    QVector<QPointF> middlePoint = m_pMiddleThreshold->pointsVector();   //Point values need to be updated before tested and displayed on the widget
    QVector<QPointF> rightPoint = m_pRightThreshold->pointsVector();
    QVector<QPointF> leftPoint = m_pLeftThreshold->pointsVector();
    float emitLeft = leftPoint[0].x();
    float emitMiddle = middlePoint[0].x();
    float emitRight = rightPoint[0].x();
    QVector3D originalVector;
    originalVector.setX(emitLeft);
    originalVector.setY(emitMiddle);
    originalVector.setZ(emitRight);
    m_vecReturnVector = correctionDisplayTrueValue(originalVector, "down");
    return m_vecReturnVector;
}

//=============================================================================================================

QVector3D Spline::correctionDisplayTrueValue(QVector3D vecOriginalValues, QString upOrDown)
{
    QVector3D returnCorrectedVector;

    if(m_vecResultExponentValues.rows() > 0) {
        int exponent;
        if (upOrDown == "up")
        {
            if (m_vecResultExponentValues[0] < 0)
            {
                exponent = std::abs(m_vecResultExponentValues[0]);
            }
            else if (m_vecResultExponentValues[0] > 0)
            {
                exponent = -(std::abs(m_vecResultExponentValues[0]));
            }
            else
            {
                exponent = 0;
            }
        }
        else if (upOrDown == "down")
        {
            if (m_vecResultExponentValues[0] < 0)
            {
                exponent = -(std::abs(m_vecResultExponentValues[0]));
            }
            else if (m_vecResultExponentValues[0] > 0)
            {
                exponent = std::abs(m_vecResultExponentValues[0]);
            }
            else
            {
                exponent = 0;
            }
        }
        else
        {
            qDebug() << "Spline::correctionDisplayTrueValue error.";
        }

        returnCorrectedVector.setX(vecOriginalValues.x() * (pow(10, exponent)));
        returnCorrectedVector.setY(vecOriginalValues.y() * (pow(10, exponent)));
        returnCorrectedVector.setZ(vecOriginalValues.z() * (pow(10, exponent)));
    }

    return returnCorrectedVector;
}
