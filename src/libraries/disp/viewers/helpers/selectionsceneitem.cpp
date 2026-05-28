//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file selectionsceneitem.cpp
 * @since 2022
 * @date  October 2022
 * @brief Implementation of the SelectionItem / SelectionSceneItem pair.
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
