//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     tmsielectrodeitem.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2014
 * @brief    Contains the implementation of the TmsiElectrodeItem class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsielectrodeitem.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPLUGIN;
using namespace std;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSIElectrodeItem::TMSIElectrodeItem(QString electrodeName, QPointF electrodePosition, QColor electrodeColor, int channelIndex)
: m_sElectrodeName(electrodeName)
, m_qpElectrodePosition(electrodePosition)
, m_cElectrodeColor(electrodeColor)
, m_dImpedanceValue(0.0)
, m_iChannelIndex(channelIndex)
{
}

//=============================================================================================================

QRectF TMSIElectrodeItem::boundingRect() const
{
    return QRectF(-25, -35, 50, 70);
}

//=============================================================================================================

void TMSIElectrodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Plot shadow
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::darkGray);
    painter->drawEllipse(-12, -12, 30, 30);

    // Plot colored circle
    painter->setPen(QPen(Qt::black, 1));
    painter->setBrush(QBrush(m_cElectrodeColor));
    painter->drawEllipse(-15, -15, 30, 30);

    // Plot electrode name
    QStaticText staticElectrodeName = QStaticText(m_sElectrodeName);
    QSizeF sizeText = staticElectrodeName.size();
    painter->drawStaticText(-15+((30-sizeText.width())/2), -32, staticElectrodeName);

    // Plot electrodes impedance value
    QString impedanceValueToString;
    QStaticText staticElectrodeValue = QStaticText(QString("%1 %2").arg(impedanceValueToString.setNum(m_dImpedanceValue/1000)).arg(/*"kOhm"*/"k")); // transform to kilo ohm (divide by 1000)
    QSizeF sizeValue = staticElectrodeValue.size();
    painter->drawStaticText(-15+((30-sizeValue.width())/2), 19, staticElectrodeValue);

    this->setPos(m_qpElectrodePosition);
}

//=============================================================================================================

void TMSIElectrodeItem::setColor(QColor electrodeColor)
{
    m_cElectrodeColor = electrodeColor;
}

//=============================================================================================================

QString TMSIElectrodeItem::getElectrodeName()
{
    return m_sElectrodeName;
}

//=============================================================================================================

void TMSIElectrodeItem::setImpedanceValue(double impedanceValue)
{
    m_dImpedanceValue = impedanceValue;
}

//=============================================================================================================

double TMSIElectrodeItem::getImpedanceValue()
{
    return m_dImpedanceValue;
}

//=============================================================================================================

void TMSIElectrodeItem::setPosition(QPointF newPosition)
{
    m_qpElectrodePosition = newPosition;
}

//=============================================================================================================

QPointF TMSIElectrodeItem::getPosition()
{
    return m_qpElectrodePosition;
}

//=============================================================================================================

int TMSIElectrodeItem::getChannelIndex()
{
    return m_iChannelIndex;
}

