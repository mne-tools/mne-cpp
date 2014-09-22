//=============================================================================================================
/**
* @file     ChannelSceneItem.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     June, 2014
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
* @brief    Contains the implementation of the ChannelSceneItem class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channelsceneitem.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;
using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelSceneItem::ChannelSceneItem(QString electrodeName, QPointF electrodePosition, QColor electrodeColor)
: m_sElectrodeName(electrodeName)
, m_qpElectrodePosition(electrodePosition)
, m_cElectrodeColor(electrodeColor)
, m_bHighlight(false)
{
    this->setAcceptHoverEvents(true);
    this->setFlag(QGraphicsItem::ItemIsSelectable, true);
}

//*************************************************************************************************************

QRectF ChannelSceneItem::boundingRect() const
{
    return QRectF(-25, -35, 50, 70);
}

//*************************************************************************************************************

void ChannelSceneItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Plot shadow
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::darkGray);
    painter->drawEllipse(-12, -12, 30, 30);

    // Plot colored circle
    painter->setPen(QPen(Qt::black, 1));
    if(this->isSelected())
        painter->setBrush(QBrush(Qt::red));
    else
        painter->setBrush(QBrush(m_cElectrodeColor));
    painter->drawEllipse(-15, -15, 30, 30);

    // Plot electrode name
    QStaticText staticElectrodeName = QStaticText(m_sElectrodeName);
    QSizeF sizeText = staticElectrodeName.size();
    painter->drawStaticText(-15+((30-sizeText.width())/2), -32, staticElectrodeName);

    this->setPos(25*m_qpElectrodePosition.x(), -25*m_qpElectrodePosition.y());
}

//*************************************************************************************************************

void ChannelSceneItem::setColor(QColor electrodeColor)
{
    m_cElectrodeColor = electrodeColor;
}

//*************************************************************************************************************

QString ChannelSceneItem::getElectrodeName()
{
    return m_sElectrodeName;
}

//*************************************************************************************************************

void ChannelSceneItem::setPosition(QPointF newPosition)
{
    m_qpElectrodePosition = newPosition;
}

//*************************************************************************************************************

QPointF ChannelSceneItem::getPosition()
{
    return m_qpElectrodePosition;
}


//*************************************************************************************************************

void ChannelSceneItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    m_bHighlight = true;
    this->update();
}


//*************************************************************************************************************

void ChannelSceneItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    m_bHighlight = false;
    this->update();
}











