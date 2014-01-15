//=============================================================================================================
/**
* @file     plotsignalwidget.cpp
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implements the PlotSignalWidget function of mne_browse_raw_qt
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "plotsignalwidget.h"

#include <QPointF>
#include <QPainter>

//*************************************************************************************************************

PlotSignalWidget::PlotSignalWidget(QWidget *parent)
: QWidget(parent)
, m_dPlotHeight(70)
{
}

PlotSignalWidget::PlotSignalWidget(MatrixXd data, QWidget *parent)
: QWidget(parent)
, m_dPlotHeight(70)
, m_data(data)
, m_dMaxValue(data.row(0).cwiseAbs().maxCoeff())
, m_dScaleY(m_dPlotHeight/(2*m_dMaxValue))
, m_dDx(1)
, m_nhlines(6)
{
    qDebug("size of data matrix is %ix%i",data.rows(),data.cols());
}

//*************************************************************************************************************

void PlotSignalWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.save();

    //Draw white background-box
    painter.setBrush(Qt::white);
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, 0, width(),m_dPlotHeight);

    painter.restore();

    //painter settings
    painter.translate(0,m_dPlotHeight/2);
    painter.setRenderHint(QPainter::Antialiasing, true);

    //create and draw PainterPaths
    QPainterPath t_qPlotPath;
    createPlotPath(t_qPlotPath);
    painter.drawPath(t_qPlotPath);

    QPainterPath t_qGridPath;
    createGridPath(t_qGridPath);
    painter.save();

    painter.setRenderHint(QPainter::Antialiasing, false);
    QPen pen;
    pen.setStyle(Qt::DotLine);
    pen.setWidthF(0.5);
    painter.setPen(pen);

    painter.setBrush(QColor(Qt::darkGray));
    painter.drawPath(t_qGridPath);

}

void PlotSignalWidget::createPlotPath(QPainterPath& path)
{
    double dValue;
    QPointF qSamplePosition;

    //create lines from one to the next sample
    for(qint32 i=0; i < m_data.cols(); ++i)
    {
        dValue = m_data(0,i)*m_dScaleY;

        qSamplePosition.setY(dValue);
        qSamplePosition.setX(path.currentPosition().x()+m_dDx);

        path.lineTo(qSamplePosition);

        path.moveTo(qSamplePosition);
    }

    qDebug("Plot-PainterPath created!");
}

void PlotSignalWidget::createGridPath(QPainterPath& path)
{
    //horizontal lines
    qint8 m_nhlines = 6;
    double distance = m_dPlotHeight/m_nhlines;

    path.moveTo(0,-m_dPlotHeight/2+distance);

    for(qint8 i=0; i < m_nhlines-1; ++i) {
        QPointF endpoint(this->width(),path.currentPosition().y());
        path.lineTo(endpoint);
        path.moveTo(0,path.currentPosition().y()+distance);
    }

    qDebug("Grid-PainterPath created!");
}

QSize PlotSignalWidget::sizeHint() const
{
    return QSize(m_dDx*m_data.cols(),m_dPlotHeight);
}
