//=============================================================================================================
/**
 * @file     eegosportselectrodeitem.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at;
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Christoph Dinh, Matti Hamalainen, Johannes Vorwerk. All rights reserved.
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
 * @brief    Contains the implementation of the EEGoSportsElectrodeItem class.
 *
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