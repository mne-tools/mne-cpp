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

#include <QPainter>
#include <QPointF>

//*************************************************************************************************************

PlotSignalWidget::PlotSignalWidget(QWidget *parent)
: QWidget(parent)
{
}

PlotSignalWidget::PlotSignalWidget(MatrixXd data, MatrixXd times, QWidget *parent)
: QWidget(parent)
{
//    m_data = data;
//    m_times = times;

    qDebug("size of data matrix is %ix%i",data.rows(),data.cols());
}

//*************************************************************************************************************

void PlotSignalWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.save();

    //Draw background
    painter.setBrush(Qt::white);
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, 0, width(),200);

    painter.restore();

    QPointF point(100,100);
    painter.drawText(point,"test");

}

void PlotSignalWidget::createPath()
{
//    double dValue = 0;
//    double dPositionDifference = 0.0;
//    for(unsigned char i = 0; i < m_data.size(); ++i)
//    {
//        dValue = m_data[i]*m_fScaleFactor - m_dMiddle;
//        dPositionDifference = m_dPosition - (m_dPosX+width());

//        if((dPositionDifference >= 0) || m_bStartFlag)
//        {
//            if(m_bStartFlag)
//                dPositionDifference = 0;

//            m_qPainterPath = QPainterPath();
//                m_dPosition = m_dPosX + dPositionDifference;
//                m_qPainterPath.moveTo(m_dPosition, m_dPosY-dValue);

//        }
//        else
//        {
//            m_qMutex.lock();
//                m_qPainterPath.lineTo(m_dPosition, m_dPosY-dValue);
//            m_qMutex.unlock();
//        }

//        m_dPosition = m_dPosition + m_dSampleWidth;
//    }
}
