//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     eegosportselectrodeitem.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at;>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Contains the implementation of the EEGoSportsElectrodeItem class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportselectrodeitem.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGOSPORTSPLUGIN;
using namespace std;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsElectrodeItem::EEGoSportsElectrodeItem(const QString& electrodeName, 
                                                 const QPointF& electrodePosition,
                                                 const QColor& electrodeColor, 
                                                 int channelIndex)
: m_sElectrodeName(electrodeName)
, m_qpElectrodePosition(electrodePosition)
, m_cElectrodeColor(electrodeColor)
, m_dImpedanceValue(0.0)
, m_iChannelIndex(channelIndex)
{
}

//=============================================================================================================

QRectF EEGoSportsElectrodeItem::boundingRect() const
{
    return QRectF(-25, -35, 50, 70);
}

//=============================================================================================================

void EEGoSportsElectrodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
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
    QStaticText staticElectrodeValue;
    if (m_dImpedanceValue > 0){
        QString impedanceValueToString;
        staticElectrodeValue = QStaticText(QString("%1 %2").arg(impedanceValueToString.setNum(m_dImpedanceValue/1000,'f',2)).arg(/*"kOhm"*/"k")); // transform to kilo ohm (divide by 1000)
    } else {
        staticElectrodeValue = QStaticText("HIGH");
    }

    QSizeF sizeValue = staticElectrodeValue.size();
    painter->drawStaticText(-15+((30-sizeValue.width())/2), 19, staticElectrodeValue);

    this->setPos(m_qpElectrodePosition);
}

//=============================================================================================================

void EEGoSportsElectrodeItem::setColor(const QColor& electrodeColor)
{
    m_cElectrodeColor = electrodeColor;
}

//=============================================================================================================

QString EEGoSportsElectrodeItem::getElectrodeName()
{
    return m_sElectrodeName;
}

//=============================================================================================================

void EEGoSportsElectrodeItem::setImpedanceValue(double impedanceValue)
{
    m_dImpedanceValue = impedanceValue;
}

//=============================================================================================================

double EEGoSportsElectrodeItem::getImpedanceValue()
{
    return m_dImpedanceValue;
}

//=============================================================================================================

void EEGoSportsElectrodeItem::setPosition(const QPointF& newPosition)
{
    m_qpElectrodePosition = newPosition;
}

//=============================================================================================================

QPointF EEGoSportsElectrodeItem::getPosition()
{
    return m_qpElectrodePosition;
}

//=============================================================================================================

int EEGoSportsElectrodeItem::getChannelIndex()
{
    return m_iChannelIndex;
}