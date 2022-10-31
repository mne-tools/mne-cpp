//=============================================================================================================
/**
 * @file     tmsielectrodeitem.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the implementation of the TmsiElectrodeItem class.
 *
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

