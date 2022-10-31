//=============================================================================================================
/**
 * @file     selectionsceneitem.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2014
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
 * @brief    Definition of the SelectionSceneItem class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "selectionsceneitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
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

SelectionSceneItem::SelectionSceneItem(QString channelName,
                                       int channelNumber,
                                       QPointF channelPosition,
                                       int channelKind,
                                       int channelUnit,
                                       QColor channelColor,
                                       bool bIsBadChannel)
: m_sChannelName(channelName)
, m_iChannelNumber(channelNumber)
, m_qpChannelPosition(channelPosition)
, m_cChannelColor(channelColor)
, m_bHighlightItem(false)
, m_iChannelKind(channelKind)
, m_iChannelUnit(channelUnit)
, m_bIsBadChannel(bIsBadChannel)
{
    this->setAcceptHoverEvents(true);
    this->setFlag(QGraphicsItem::ItemIsSelectable, true);
}

//=============================================================================================================

QRectF SelectionSceneItem::boundingRect() const
{
    return QRectF(-25, -30, 50, 50);
}

//=============================================================================================================

void SelectionSceneItem::paint(QPainter *painter,
                               const QStyleOptionGraphicsItem *option,
                               QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    this->setPos(10*m_qpChannelPosition.x(), -10*m_qpChannelPosition.y());

    // Plot shadow
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::darkGray);
    painter->drawEllipse(-12, -12, 30, 30);

    //Plot red if bad
    if(m_bIsBadChannel) {
        painter->setBrush(Qt::red);
        painter->drawEllipse(-15, -15, 30, 30);
    } else {
        painter->setBrush(m_cChannelColor);
        painter->drawEllipse(-15, -15, 30, 30);
    }

    //Plot selected item
    if(this->isSelected()){
        //painter->setPen(QPen(QColor(255,84,22), 5));
        painter->setPen(QPen(Qt::red, 5));
        painter->drawEllipse(-15, -15, 30, 30);
    }

    //OLD
//    //Plot selected item
//    if(this->isSelected())
//        painter->setBrush(QBrush(QColor(93,177,47)));
//    else
//        painter->setBrush(QBrush(m_cChannelColor));

//    //Plot highlighted selected item
//    if(m_bHighlightItem) {
//        painter->setPen(QPen(Qt::red, 4));
//        painter->drawEllipse(-15, -15, 30, 30);
//    }
//    else {
//        painter->setPen(QPen(Qt::black, 1));
//        painter->drawEllipse(-15, -15, 30, 30);
//    }

    // Plot electrode name
    painter->setPen(QPen(Qt::black, 1));
    QStaticText staticElectrodeName = QStaticText(m_sChannelName);
    QSizeF sizeText = staticElectrodeName.size();
    painter->drawStaticText(-15+((30-sizeText.width())/2), -32, staticElectrodeName);
}

//=============================================================================================================

SelectionItem::SelectionItem(const SelectionItem &other)
{
    m_sChannelName = other.m_sChannelName;
    m_iChannelNumber = other.m_iChannelNumber;
    m_iChannelKind = other.m_iChannelKind;
    m_iChannelUnit = other.m_iChannelUnit;
    m_qpChannelPosition = other.m_qpChannelPosition;
}
