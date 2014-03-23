//=============================================================================================================
/**
* @file     roi.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2014
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
* @brief    Implementation of the Roi Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "roi.h"
#include "roiselectionwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOption>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Roi::Roi(const QString & fileName, RoiSelectionWidget *roiSelectionWidget)
: m_pRoiSelectionWidget(roiSelectionWidget)
{
    m_pGraphicsSvgItem = new QGraphicsSvgItem(fileName+".svg", this);
    m_pGraphicsSvgItem->setVisible(true);

    m_pGraphicsSvgItem->setFlags(QGraphicsItem::ItemClipsToShape);
    m_pGraphicsSvgItem->setCacheMode(QGraphicsItem::NoCache);
    m_pGraphicsSvgItem->setZValue(0);

    m_pGraphicsSvgItemActive = new QGraphicsSvgItem(fileName+"_active.svg", this);
    m_pGraphicsSvgItemActive->setVisible(false);

//    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsGeometryChanges);
    setCacheMode(DeviceCoordinateCache);
    setZValue(-1);
}


//*************************************************************************************************************

QRectF Roi::boundingRect() const
{
//    qreal adjust = 2;
    return m_pGraphicsSvgItem->boundingRect();//QRectF( -30 - adjust, -30 - adjust, 60 + adjust, 60 + adjust);
}


//*************************************************************************************************************

QPainterPath Roi::shape() const
{
    return m_pGraphicsSvgItem->shape();
}


//*************************************************************************************************************

void Roi::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    Q_UNUSED(painter);

//    painter->setPen(Qt::NoPen);
//    painter->setBrush(Qt::darkGray);
//    painter->drawPath( m_pGraphicsSvgItem->shape());

    if (option->state & QStyle::State_Sunken)
    {
        m_pGraphicsSvgItem->setVisible(false); //painter->setBrush(QColor(Qt::blue).light(180));
        m_pGraphicsSvgItemActive->setVisible(true);
    }
    else
    {
        m_pGraphicsSvgItem->setVisible(true); //painter->setBrush(Qt::white);
        m_pGraphicsSvgItemActive->setVisible(false);
    }

//    painter->setPen(QPen(Qt::black, 0));
//    painter->drawPath( m_pGraphicsSvgItem->shape());

    Q_UNUSED(painter);

}


//*************************************************************************************************************

QVariant Roi::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
        case ItemPositionHasChanged:
//            m_pRoiSelectionWidget->itemMoved();
            break;
        default:
            break;
    };

    return QGraphicsItem::itemChange(change, value);
}


//*************************************************************************************************************

void Roi::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    update();
    QGraphicsItem::mousePressEvent(event);
}


//*************************************************************************************************************

void Roi::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    update();
    QGraphicsItem::mouseReleaseEvent(event);
}
