//=============================================================================================================
/**
 * @file     averagesceneitem.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the AverageSceneItem class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagesceneitem.h"
#include "../scalingview.h"

#include <fiff/fiff_types.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QDebug>
#include <QStaticText>

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

AverageSceneItem::AverageSceneItem(const QString& channelName,
                                   int channelNumber,
                                   const QPointF &channelPosition,
                                   int channelKind,
                                   int channelUnit,
                                   const QColor &color)
: m_sChannelName(channelName)
, m_iChannelNumber(channelNumber)
, m_iChannelKind(channelKind)
, m_iChannelUnit(channelUnit)
, m_iTotalNumberChannels(0)
, m_iFontTextSize(15)
, m_iMaxWidth(1500)
, m_iMaxHeigth(150)
, m_bIsBad(false)
, m_qpChannelPosition(channelPosition)
, m_colorDefault(color)
{
    m_rectBoundingRect = QRectF(-m_iMaxWidth/2, -m_iMaxHeigth/2, m_iMaxWidth, m_iMaxHeigth);
}

//=============================================================================================================

QRectF AverageSceneItem::boundingRect() const
{
    return m_rectBoundingRect;
}

//=============================================================================================================

void AverageSceneItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
    m_iFontTextSize = 150;
    emit sceneUpdateRequested();
}

//=============================================================================================================

void AverageSceneItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
    m_iFontTextSize = 15;
    emit sceneUpdateRequested();
}

//=============================================================================================================

void AverageSceneItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //set posistion
    this->setPos(75*m_qpChannelPosition.x(), -75*m_qpChannelPosition.y());

    painter->setRenderHint(QPainter::Antialiasing, true);

    if(m_bIsBad) {
        painter->setOpacity(0.20);
    }

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
    painter->save();
    QPen pen;
    pen.setColor(Qt::red);
    pen.setWidthF(5);

    QFont f = painter->font();
    f.setPointSizeF(m_iFontTextSize);
    painter->setFont(f);

    painter->setPen(pen);
    painter->drawStaticText(boundingRect().x(), boundingRect().y()-(2*m_iFontTextSize), staticElectrodeName);
    painter->restore();

    //Plot bounding rect
//    painter->save();
//    pen.setColor(Qt::red);
//    painter->setPen(pen);
//    painter->drawRect(this->boundingRect());
//    painter->restore();
}

//=============================================================================================================

void AverageSceneItem::paintAveragePath(QPainter *painter)
{
    if(m_lAverageData.size() == 0)
        return;

    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)

    float fMaxValue = DISPLIB::getScalingValue(m_scaleMap, m_iChannelKind, m_iChannelUnit);

    //Plot averaged data
    QRectF boundingRect = this->boundingRect();
    double dScaleY = (boundingRect.height())/(2*fMaxValue);
    QPointF qSamplePosition;

    //do for all currently stored evoked set data
    for(int dataIndex = 0; dataIndex < m_lAverageData.size(); ++dataIndex) {
        QString sAvrComment = m_lAverageData.at(dataIndex).first;

        if(m_qMapAverageActivation[sAvrComment]) {
            //plot data from averaged data m_lAverageData with the calculated downsample factor
            const double* averageData = m_lAverageData.at(dataIndex).second.first;
            int totalCols =  m_lAverageData.at(dataIndex).second.second;

            if(totalCols < m_iMaxWidth) {
                m_rectBoundingRect = QRectF(-totalCols/2, -m_iMaxHeigth/2, totalCols, m_iMaxHeigth);
            }

            //Calculate downsampling factor of averaged data in respect to the items width
            int dsFactor;
            totalCols / boundingRect.width()<1 ? dsFactor = 1 : dsFactor = totalCols / boundingRect.width();
            if(dsFactor == 0) {
                dsFactor = 1;
            }

            //Create path
            //float offset = (*(averageData+(0*m_iTotalNumberChannels)+m_iChannelNumber)); //choose offset to be the signal value at first sample
            float offset = 0;
            QPainterPath path = QPainterPath(QPointF(boundingRect.x(), boundingRect.y() + boundingRect.height()/2));
            QPen pen;
            pen.setStyle(Qt::SolidLine);
            pen.setColor(m_colorDefault);

            if(m_qMapAverageColor.contains(sAvrComment)) {
                pen.setColor(m_qMapAverageColor[sAvrComment]);
            }

            pen.setWidthF(3);
            painter->setPen(pen);

            for(int i = 0; i < totalCols && path.elementCount() <= boundingRect.width(); i += dsFactor) {
                //evoked matrix is stored in column major
                double val = ((*(averageData+(i*m_iTotalNumberChannels)+m_iChannelNumber))-offset) * dScaleY;

                //Cut plotting if six times bigger than m_iMaxHeigth
                if(std::fabs(val) > 6*m_iMaxHeigth && m_bIsBad) {
                    qSamplePosition.setY(-(val/val) * m_iMaxHeigth); //(val/val) used to retrieve sign of val
                    qSamplePosition.setX(path.currentPosition().x()+1);
                } else {
                    qSamplePosition.setY(-val);
                    qSamplePosition.setX(path.currentPosition().x()+1);
                }

                path.lineTo(qSamplePosition);
            }

            painter->drawPath(path);
        }
    }
}

//=============================================================================================================

void AverageSceneItem::paintStimLine(QPainter *painter)
{
    if(m_lAverageData.size() == 0)
        return;

    //Plot vertical and horizontal lines
    QRectF boundingRect = this->boundingRect();
    QPainterPath path = QPainterPath(QPointF(boundingRect.x(), boundingRect.y() + boundingRect.height()/2));

    int dsFactor = 1;
    int totalCols =  m_lAverageData.first().second.second;
    totalCols / boundingRect.width()<1 ? dsFactor = 1 : dsFactor = totalCols / boundingRect.width();

    if(dsFactor == 0)
        dsFactor = 1;

    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setColor(Qt::red);
    pen.setWidthF(3);
    painter->setPen(pen);

    //Stim line
    path.moveTo(boundingRect.x() + std::abs(m_firstLastSample.first)/dsFactor, boundingRect.y());
    path.lineTo(boundingRect.x() + std::abs(m_firstLastSample.first)/dsFactor, boundingRect.y() + boundingRect.height());

    //zero line
    path.moveTo(boundingRect.x(), boundingRect.y() + boundingRect.height()/2);
    path.lineTo(boundingRect.x() + m_lAverageData.first().second.second/dsFactor, boundingRect.y() + boundingRect.height()/2);

    painter->drawPath(path);
}

//=============================================================================================================

void AverageSceneItem::setDefaultColor(const QColor &viewColor)
{
    m_colorDefault = viewColor;
}

//=============================================================================================================
