//=============================================================================================================
/**
* @file     sensoritem.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
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
* @brief    Implementation of the SensorItem Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensoritem.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStaticText>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorItem::SensorItem(const QString& dispChName, qint32 chNumber, const QPointF& coordinate, const QColor& channelColor, QGraphicsItem *parent)
: QGraphicsObject(parent)
, m_sDisplayChName(dispChName)
, m_iChNumber(chNumber)
, m_qPointFCoord(coordinate)
, m_qColorChannel(channelColor)
, m_bIsHighlighted(false)
{
    setZValue(m_iChNumber);

    setFlags(ItemIsSelectable);// | ItemIsMovable);
    setAcceptHoverEvents(true);
}


//*************************************************************************************************************

QRectF SensorItem::boundingRect() const
{
    return QRectF(-25, -30, 50, 50);//QRectF(QPointF(0,0), m_qSizeFDim);
}


//*************************************************************************************************************

QPainterPath SensorItem::shape() const
{
    QPainterPath path;
    path.addRect(-25, -30, 50, 50);
    return path;
}


//*************************************************************************************************************

void SensorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget)

    // Plot shadow
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::darkGray);
    painter->drawEllipse(-12, -12, 30, 30);

    if (option->state & QStyle::State_MouseOver)
        painter->setBrush(Qt::lightGray);
    else
        painter->setBrush(this->isSelected() ? Qt::yellow : m_qColorChannel);

    painter->setPen(Qt::black);
    painter->drawEllipse(-15, -15, 30, 30);

    // Plot channel name
    painter->setPen(QPen(Qt::black, 1));
    QStaticText staticChName = QStaticText(m_sDisplayChName);
    QSizeF sizeText = staticChName.size();
    painter->drawStaticText(-15+((30-sizeText.width())/2), -32, staticChName);
}


//*************************************************************************************************************

void SensorItem::setColor(const QColor& channelColor)
{
    m_qColorChannel = channelColor;
}


//*************************************************************************************************************

void SensorItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mousePressEvent(event);
    update();
}


//*************************************************************************************************************

void SensorItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
//    if (event->modifiers() & Qt::ShiftModifier) {
//        stuff << event->pos();
//        update();
//        return;
//    }
    QGraphicsItem::mouseMoveEvent(event);
}


//*************************************************************************************************************

void SensorItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseReleaseEvent(event);
//    m_bIsSelected = !m_bIsSelected;
    emit itemChanged(this);
    update();
}
