//=============================================================================================================
/**
* @file     tmsielectrodeitem.cpp
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
* @brief    Contains the implementation of the TmsiElectrodeItem class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsielectrodeitem.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSIElectrodeItem::TMSIElectrodeItem(QString electrodeName, QPointF electrodePosition, QColor electrodeColor)
: m_sElectrodeName(electrodeName)
, m_qpElectrodePosition(electrodePosition)
, m_cElectrodeColor(electrodeColor)
, m_dImpedanceValue(0.0)
{
}

//*************************************************************************************************************

QRectF TMSIElectrodeItem::boundingRect() const
{
    return QRectF(-25, -35, 50, 60);
}

//*************************************************************************************************************

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
    painter->setPen(QPen(Qt::black, 1));
    painter->setFont(QFont("Arial", 10));
    painter->drawText(-10, -20, m_sElectrodeName);

    // Plot electrodes impedance value
    QString impedanceValueToString;
    painter->setPen(QPen(Qt::white, 1));
    painter->setFont(QFont("Arial", 10));
    painter->drawText(-3, 5, impedanceValueToString.setNum(m_dImpedanceValue));

    this->setPos(m_qpElectrodePosition);
}

//*************************************************************************************************************

void TMSIElectrodeItem::setColor(QColor electrodeColor)
{
    m_cElectrodeColor = electrodeColor;
}

//*************************************************************************************************************

QString TMSIElectrodeItem::getElectrodeName()
{
    return m_sElectrodeName;
}

//*************************************************************************************************************

void TMSIElectrodeItem::setImpedanceValue(double impedanceValue)
{
    m_dImpedanceValue = impedanceValue;
}














