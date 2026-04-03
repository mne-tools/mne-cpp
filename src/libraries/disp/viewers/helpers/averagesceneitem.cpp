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
, m_iFontTextSize(3)
, m_iMaxWidth(120)
, m_iMaxHeigth(60)
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
    m_iFontTextSize = 6;
    emit sceneUpdateRequested();
}

//=============================================================================================================

void AverageSceneItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
    m_iFontTextSize = 3;
    emit sceneUpdateRequested();
}

//=============================================================================================================

void AverageSceneItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

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
    pen.setWidthF(0);
    pen.setCosmetic(true);

    QFont f = painter->font();
    f.setPointSizeF(m_iFontTextSize);
    painter->setFont(f);

    painter->setPen(pen);
    painter->drawStaticText(boundingRect().x(), boundingRect().y() - m_iFontTextSize, staticElectrodeName);
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

    //do for all currently stored evoked set data
    for(int dataIndex = 0; dataIndex < m_lAverageData.size(); ++dataIndex) {
        QString sAvrComment = m_lAverageData.at(dataIndex).first;

        // Default to active when no activation map has been configured
        if(!m_qMapAverageActivation.contains(sAvrComment) || m_qMapAverageActivation[sAvrComment]) {
            const double* averageData = m_lAverageData.at(dataIndex).second.first;
            int totalCols =  m_lAverageData.at(dataIndex).second.second;
            if(totalCols <= 0)
                continue;

            //Calculate X step to fill the full bounding rect width
            double xStep = boundingRect.width() / static_cast<double>(totalCols);

            //Create path starting at the left-center of the bounding rect
            double centerY = boundingRect.y() + boundingRect.height() / 2.0;
            QPainterPath path(QPointF(boundingRect.x(), centerY));
            QPen pen;
            pen.setStyle(Qt::SolidLine);
            pen.setColor(m_colorDefault);

            if(m_qMapAverageColor.contains(sAvrComment)) {
                pen.setColor(m_qMapAverageColor[sAvrComment]);
            }

            pen.setWidthF(0);
            pen.setCosmetic(true);
            painter->setPen(pen);

            for(int i = 0; i < totalCols; ++i) {
                //evoked matrix is stored in column major
                double val = (*(averageData + (i * m_iTotalNumberChannels) + m_iChannelNumber)) * dScaleY;

                //Clamp to bounding rect height
                double halfH = boundingRect.height() / 2.0;
                if(val > halfH) val = halfH;
                else if(val < -halfH) val = -halfH;

                double xPos = boundingRect.x() + i * xStep;
                path.lineTo(QPointF(xPos, centerY - val));
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
    int totalCols = m_lAverageData.first().second.second;
    if(totalCols <= 0)
        return;

    double xStep = boundingRect.width() / static_cast<double>(totalCols);

    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setColor(Qt::red);
    pen.setWidthF(0);
    pen.setCosmetic(true);
    painter->setPen(pen);

    QPainterPath path;

    //Stim line (at sample index corresponding to time=0)
    double stimX = boundingRect.x() + std::abs(m_firstLastSample.first) * xStep;
    path.moveTo(stimX, boundingRect.y());
    path.lineTo(stimX, boundingRect.y() + boundingRect.height());

    //Zero line
    double centerY = boundingRect.y() + boundingRect.height() / 2.0;
    path.moveTo(boundingRect.x(), centerY);
    path.lineTo(boundingRect.x() + boundingRect.width(), centerY);

    painter->drawPath(path);
}

//=============================================================================================================

void AverageSceneItem::setDefaultColor(const QColor &viewColor)
{
    m_colorDefault = viewColor;
}

//=============================================================================================================
