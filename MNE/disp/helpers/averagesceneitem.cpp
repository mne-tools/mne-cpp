//=============================================================================================================
/**
* @file     averagesceneitem.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     October, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the AverageSceneItem class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagesceneitem.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageSceneItem::AverageSceneItem(QString channelName, int channelNumber, QPointF channelPosition, int channelKind, int channelUnit, const QColor &color)
: m_sChannelName(channelName)
, m_iChannelNumber(channelNumber)
, m_qpChannelPosition(channelPosition)
, m_iChannelKind(channelKind)
, m_iChannelUnit(channelUnit)
{
    m_lAverageColors.append(color);
}


//*************************************************************************************************************

QRectF AverageSceneItem::boundingRect() const
{
    int height = 80;
    int width = 1000;
    return QRectF(-width/2, -height/2, width, height);
}


//*************************************************************************************************************

void AverageSceneItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //set posistion
    this->setPos(75*m_qpChannelPosition.x(), -75*m_qpChannelPosition.y());

    painter->setRenderHint(QPainter::Antialiasing, true);

//    //Plot bounding rect / drawing region of this item
//    painter->drawRect(this->boundingRect());

    //Plot stim time
    painter->save();
    paintStimLine(painter);
    painter->restore();

    //Plot average data
    painter->save();
    paintAveragePath(painter);
    painter->restore();

    // Plot channel name
    QStaticText staticElectrodeName = QStaticText(m_sChannelName);
    QSizeF sizeText = staticElectrodeName.size();
    painter->save();
    QPen pen;
    pen.setColor(Qt::yellow);
    if(!m_lAverageColors.isEmpty()) {
        pen.setColor(m_lAverageColors.first());
    }
    pen.setWidthF(5);
    painter->setPen(pen);
    painter->drawStaticText(boundingRect().x(), boundingRect().y(), staticElectrodeName);
    painter->restore();
}


//*************************************************************************************************************

void AverageSceneItem::setSignalColorForAllChannels(const QColor& color)
{
    for(int i = 0; i<m_lAverageColors.size(); i++) {
        m_lAverageColors[i] = color;
    }

    update();
}


//*************************************************************************************************************

void AverageSceneItem::paintAveragePath(QPainter *painter)
{
    if(m_lAverageData.size() == 0)
        return;

    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    float dMaxValue = 1e-9f;

    switch(m_iChannelKind) {
        case FIFFV_MEG_CH: {
            if(m_iChannelUnit == FIFF_UNIT_T_M) { //gradiometers
                dMaxValue = 1e-10f;
                if(m_scaleMap.contains(FIFF_UNIT_T_M))
                    dMaxValue = m_scaleMap[FIFF_UNIT_T_M];
            }
            else if(m_iChannelUnit == FIFF_UNIT_T) //magnitometers
            {
                dMaxValue = 1e-11f;

                if(m_scaleMap.contains(FIFF_UNIT_T))
                    dMaxValue = m_scaleMap[FIFF_UNIT_T];
            }
            break;
        }

        case FIFFV_REF_MEG_CH: {  /*11/04/14 Added by Limin: MEG reference channel */
            dMaxValue = 1e-11f;
            if(m_scaleMap.contains(FIFF_UNIT_T))
                dMaxValue = m_scaleMap[FIFF_UNIT_T];
            break;
        }
        case FIFFV_EEG_CH: {
            dMaxValue = 1e-4f;
            if(m_scaleMap.contains(FIFFV_EEG_CH))
                dMaxValue = m_scaleMap[FIFFV_EEG_CH];
            break;
        }
        case FIFFV_EOG_CH: {
            dMaxValue = 1e-3f;
            if(m_scaleMap.contains(FIFFV_EOG_CH))
                dMaxValue = m_scaleMap[FIFFV_EOG_CH];
            break;
        }
        case FIFFV_STIM_CH: {
            dMaxValue = 5;
            if(m_scaleMap.contains(FIFFV_STIM_CH))
                dMaxValue = m_scaleMap[FIFFV_STIM_CH];
            break;
        }
        case FIFFV_MISC_CH: {
            dMaxValue = 1e-3f;
            if(m_scaleMap.contains(FIFFV_MISC_CH))
                dMaxValue = m_scaleMap[FIFFV_MISC_CH];
            break;
        }
    }

    //Plot averaged data
    QRectF boundingRect = this->boundingRect();
    double dScaleY = (boundingRect.height()*10)/(2*dMaxValue);
    QPointF qSamplePosition;

    //do for all currently stored evoked set data
    for(int dataIndex = 0; dataIndex<m_lAverageData.size(); dataIndex++) {
        //plot data from averaged data m_lAverageData with the calculated downsample factor
        const double* averageData = m_lAverageData.at(dataIndex).first;
        int totalCols =  m_lAverageData.at(dataIndex).second;

        //Calculate downsampling factor of averaged data in respect to the items width
        int dsFactor;
        totalCols / boundingRect.width()<1 ? dsFactor = 1 : dsFactor = totalCols / boundingRect.width();
        if(dsFactor == 0)
            dsFactor = 1;

        //Create path
        //float offset = (*(averageData+(abs(m_firstLastSample.first)*m_iTotalNumberChannels)+m_iChannelNumber)); //choose offset to be the signal value at time instance 0
        QPainterPath path = QPainterPath(QPointF(boundingRect.x(), boundingRect.y() + boundingRect.height()/2));
        QPen pen;
        pen.setStyle(Qt::SolidLine);
        if(!m_lAverageColors.isEmpty() && dataIndex<m_lAverageColors.size()) {
            pen.setColor(m_lAverageColors.at(dataIndex));
        }

        pen.setWidthF(5);
        painter->setPen(pen);

        for(int i = 0; i < totalCols && path.elementCount() <= boundingRect.width(); i += dsFactor) {
            //evoked matrix is stored in column major
            double val = ((*(averageData+(i*m_iTotalNumberChannels)+m_iChannelNumber))/*-offset*/) * dScaleY;

            qSamplePosition.setY(-val);
            qSamplePosition.setX(path.currentPosition().x()+1);

            path.lineTo(qSamplePosition);
        }

        painter->drawPath(path);
    }
}


//*************************************************************************************************************

void AverageSceneItem::paintStimLine(QPainter *painter)
{
    if(m_lAverageData.size() == 0)
        return;

    //Plot averaged data
    QRectF boundingRect = this->boundingRect();
    QPainterPath path = QPainterPath(QPointF(boundingRect.x(), boundingRect.y() + boundingRect.height()/2));

    int dsFactor = 1;
    int totalCols =  m_lAverageData.first().second;
    totalCols / boundingRect.width()<1 ? dsFactor = 1 : dsFactor = totalCols / boundingRect.width();

    if(dsFactor == 0)
        dsFactor = 1;

    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setColor(Qt::red);
    pen.setWidthF(3);
    painter->setPen(pen);

    path.moveTo(boundingRect.x()+abs(m_firstLastSample.first)/dsFactor, boundingRect.y());
    path.lineTo(boundingRect.x()+abs(m_firstLastSample.first)/dsFactor, boundingRect.y()+boundingRect.height());

    path.moveTo(boundingRect.x(),boundingRect.y()+boundingRect.height()/2);
    path.lineTo(boundingRect.x()+m_lAverageData.first().second/dsFactor, boundingRect.y()+boundingRect.height()/2);

    painter->drawPath(path);
}

