/**
* @file     spline.cpp
* @author   Ricky Tjen <ricky270@student.sgu.ac.id>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     April, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Ricky Tjen and Matti Hamalainen. All rights reserved.
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
* @brief    implementation of spline class
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <spline.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGui/QResizeEvent>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsTextItem>
#include <QtGui/QMouseEvent>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Spline::Spline(const QString& title, QWidget* parent)
:  QWidget(parent),
   m_coordX(0),
   m_coordY(0)

{
    m_pChart = new QChart();
    m_pChart->setTitle(title);
    m_pChart->setAnimationOptions(QChart::SeriesAnimations);
    m_pChart->setAcceptHoverEvents(false);

    QChartView *chartView = new QChartView(m_pChart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QGridLayout* layout = new QGridLayout();

    layout->addWidget(chartView, 0, 0);
    this->setLayout(layout);
}


//*************************************************************************************************************

void Spline::mousePressEvent(QMouseEvent *event)
{
    QXYSeries *shadowSeries = qobject_cast<QXYSeries *>(sender());
    QLineSeries *verticalLine = new QLineSeries();
    QPointF point = event->pos();
    point.setX(point.x()-10);                   //-10 needed to correctly position the line at mouse position
    point.setY(0);
    QPointF localX = m_pChart->mapToValue(point, shadowSeries);
    verticalLine -> append(localX.x(), 0);
    verticalLine -> append(localX.x(), maximumFrequency);
    double boundaryX = double(localX.x());       //casting localX.x() from float to double for comparison with minAxisX and maxAxisX

    if((boundaryX >= float(minAxisX)) && (boundaryX <= float(maxAxisX)))  //this condition ensures that threshold lines can only be created within the boundary of the chart
    {
        QVector<QPointF> middlePoint = middleThreshold->pointsVector(); //Point values need to be updated before tested and displayed on the widget
        QVector<QPointF> rightPoint = rightThreshold->pointsVector();
        QVector<QPointF> leftPoint = leftThreshold->pointsVector();

        double emitLeft = (leftPoint[0].x() * (pow(10, resultExponentValues[0])));
        double emitMiddle = (middlePoint[0].x() * (pow(10, resultExponentValues[0])));
        double emitRight = (rightPoint[0].x() * (pow(10, resultExponentValues[0])));

        if (event->buttons() == Qt::LeftButton)
        {
            leftPoint = verticalLine->pointsVector();
            if((leftPoint[0].x() < middlePoint[0].x()) && (leftPoint[0].x()  < rightPoint[0].x()))
            {
                m_pChart->removeSeries(leftThreshold);
                leftThreshold=verticalLine;
                leftThreshold->setColor("red");
                leftThreshold->setName("left threshold");
                m_pChart->addSeries(leftThreshold);
                m_pChart->legend()->markers().at(m_pChart->legend()->markers().size()-1)->setVisible(false);
                m_pChart->createDefaultAxes();
                emitLeft = (leftPoint[0].x() * (pow(10, resultExponentValues[0])));
                emit borderChanged(emitLeft, emitMiddle, emitRight);
                qDebug()<< "Border changed = " << emitLeft << " , " << emitMiddle << " , " << emitRight;
            }
        }
        if (event->buttons() == Qt::MiddleButton)
        {
            middlePoint = verticalLine->pointsVector();
            if((middlePoint[0].x() > leftPoint[0].x()) && (middlePoint[0].x()  < rightPoint[0].x()))
            {
                m_pChart->removeSeries(middleThreshold);
                middleThreshold=verticalLine;
                middleThreshold->setColor("green");
                m_pChart->addSeries(middleThreshold);

                m_pChart->legend()->markers().at(m_pChart->legend()->markers().size()-1)->setVisible(false);

                m_pChart->createDefaultAxes();
                emitMiddle = (middlePoint[0].x() * (pow(10, resultExponentValues[0])));
                emit borderChanged(emitLeft, emitMiddle, emitRight);
                qDebug()<< "Border changed = " << emitLeft << " , " << emitMiddle << " , " << emitRight;
            }
        }
        if (event->buttons() == Qt::RightButton)
        {
            rightPoint = verticalLine->pointsVector();
            if((rightPoint[0].x() > leftPoint[0].x()) && (rightPoint[0].x()  > middlePoint[0].x()))
            {
                m_pChart->removeSeries(rightThreshold);
                rightThreshold=verticalLine;
                rightThreshold->setColor("blue");
                m_pChart->addSeries(rightThreshold);

                m_pChart->legend()->markers().at(m_pChart->legend()->markers().size()-1)->setVisible(false);

                m_pChart->createDefaultAxes();
                emitRight = (rightPoint[0].x() * (pow(10, resultExponentValues[0])));
                emit borderChanged(emitLeft, emitMiddle, emitRight);
                qDebug()<< "Border changed = " << emitLeft << " , " << emitMiddle << " , " << emitRight;
            }
        }
    }

    QWidget::mousePressEvent(event);
}


//*************************************************************************************************************
