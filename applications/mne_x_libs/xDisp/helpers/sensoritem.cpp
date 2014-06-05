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

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorItem::SensorItem(const QString& dispChName, qint32 chNumber, const QPointF& coordinate, const QSizeF& size, QGraphicsItem *parent)
: QGraphicsObject(parent)
, m_sDisplayChName(dispChName)
, m_iChNumber(chNumber)
, m_qPointFCoord(coordinate)
, m_qSizeFDim(size)
, m_bIsSelected(false)
{
    setZValue(m_iChNumber);

    setFlags(ItemIsSelectable);// | ItemIsMovable);
    setAcceptHoverEvents(true);
}


//*************************************************************************************************************

QRectF SensorItem::boundingRect() const
{
    return QRectF(QPointF(0,0), m_qSizeFDim);
}


//*************************************************************************************************************

QPainterPath SensorItem::shape() const
{
    QPainterPath path;
    path.addRect(QRectF(QPointF(0,0), m_qSizeFDim));
    return path;
}


//*************************************************************************************************************

void SensorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget)

    if (option->state & QStyle::State_MouseOver)
        painter->setBrush(Qt::lightGray);

    painter->setPen(m_bIsSelected ? Qt::red : Qt::darkBlue);
    painter->drawRect(QRectF(QPointF(0,0), m_qSizeFDim));

    painter->setFont(QFont("Helvetica [Cronyx]", 6));
    painter->drawText(QPointF(0+2,m_qSizeFDim.height()-3), m_sDisplayChName);
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
    m_bIsSelected = !m_bIsSelected;
    emit itemChanged(this);
    update();
}
