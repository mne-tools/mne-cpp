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
#include <helpers/callout.h>


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
//=============================================================================================================Bar::Bar(const VectorXd& matClassLimitData, VectorXi& matClassFrequencyData, int iPrecisionValue)

Spline::Spline(const QString& title, QWidget* parent)
:  QWidget(parent),
   m_coordX(0),
   m_coordY(0),
   m_tooltip(0)

{
    m_pChart = new QChart();
    m_pChart->setTitle(title);
    m_pChart->setAnimationOptions(QChart::SeriesAnimations);

    m_pAxis = new QBarCategoryAxis();
    m_pChart->legend()->setVisible(true);
    m_pChart->legend()->setAlignment(Qt::AlignBottom);

    QChartView *chartView = new QChartView(m_pChart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QGridLayout* layout = new QGridLayout();

    layout->addWidget(chartView, 0, 0);
    this->setLayout(layout);
}


//*************************************************************************************************************

//Spline::View(QWidget *parent, m_pChart)
//: QWidget(new QGraphicsScene, parent),
//  m_coordX(0),
//  m_coordY(0),
//  m_tooltip(0)
//{
//    setDragMode(QWidget::NoDrag);
//    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

//    m_coordX = new QGraphicsSimpleTextItem(m_pChart);
//    m_coordX->setPos(m_pChart->size().width()/2 - 50, m_pChart->size().height());
//    m_coordX->setText("X: ");
//    m_coordY = new QGraphicsSimpleTextItem(m_pChart);
//    m_coordY->setPos(m_pChart->size().width()/2 + 50, m_pChart->size().height());
//    m_coordY->setText("Y: ");

//    connect(series, SIGNAL(clicked(QPointF)), this, SLOT(keepCallout()));
//    connect(series, SIGNAL(hovered(QPointF, bool)), this, SLOT(tooltip(QPointF,bool)));

//}


//*************************************************************************************************************

//void Spline::mousePressEvent(QMouseEvent *event)
//{
//    qDebug() << "Output global position:" << event->globalPos();
//    qDebug() << "Output window position:" << event->windowPos();
//    qDebug() << "Output m_pChart position:" << m_pChart->pos();
//    qDebug() << "Output m_pChart margins:" << m_pChart->margins();

//    QWidget::mousePressEvent(event);
//}


//*************************************************************************************************************

void Spline::keepCallout()
{
    m_tooltip = new Callout(m_pChart);
    //emit borderChanged(left, middle, right);
    //qDebug() << "Left Mouse Button: " << left;
}


//*************************************************************************************************************

void Spline::tooltip(QPointF point, bool state)
{
    if (m_tooltip == 0)
        m_tooltip = new Callout(m_pChart);

    if (state) {
        m_tooltip->setText(QString("X: %1 \nY: %2 ").arg(point.x()).arg(point.y()));
        QXYSeries *series = qobject_cast<QXYSeries *>(sender());
        m_tooltip->setAnchor(m_pChart->mapToPosition(point, series));
        m_tooltip->setPos(m_pChart->mapToPosition(point, series) + QPoint(10, -50));
        m_tooltip->setZValue(11);
        m_tooltip->show();
    } else {
        m_tooltip->hide();
    }
}


//*************************************************************************************************************


void Spline::mouseMoveEvent(QMouseEvent *event)
{
    m_coordX->setText(QString("X: %1").arg(m_pChart->mapToValue(event->pos()).x()));
    m_coordY->setText(QString("Y: %1").arg(m_pChart->mapToValue(event->pos()).y()));
    QWidget::mouseMoveEvent(event);
}


//*************************************************************************************************************

void Spline::resizeEvent(QResizeEvent *event)
{
    m_pChart->resize(event->size());
    m_coordX->setPos(m_pChart->size().width()/2 - 50, m_pChart->size().height() - 20);
    m_coordY->setPos(m_pChart->size().width()/2 + 50, m_pChart->size().height() - 20);
    QWidget::resizeEvent(event);
}


//*************************************************************************************************************
