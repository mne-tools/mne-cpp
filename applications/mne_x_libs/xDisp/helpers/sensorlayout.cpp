//=============================================================================================================
/**
* @file     sensorLlayout.cpp
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
* @brief    Implementation of the SensorLayout Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensorlayout.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorLayout::SensorLayout()
{
}


//*************************************************************************************************************

SensorLayout SensorLayout::parseSensorLayout(const QDomElement &sensorLayoutElement)
{
    SensorLayout layout;

    qint32 t_iNumChannels = sensorLayoutElement.attribute("NumChannels", 0).toInt();

    Q_UNUSED(t_iNumChannels)

    layout.m_sName = sensorLayoutElement.attribute("Type", "");

    QDomElement childSensor = sensorLayoutElement.firstChildElement("Sensor");
    while (!childSensor.isNull()) {
        QString chName = layout.m_sName.isEmpty() ? childSensor.attribute("ChannelNumber") : QString("%1%2").arg(layout.m_sName).arg(childSensor.attribute("ChannelNumber"));
        layout.m_qListFullChannelNames.append(chName);
        layout.m_qListShortChannelNames.append(childSensor.attribute("ChannelNumber"));
        float plot_x = childSensor.attribute("plot_x").toFloat()*5; //mm to pixel
        float plot_y = childSensor.attribute("plot_y").toFloat()*5; //mm to pixel
        layout.m_qListLocations.append(QPointF(plot_x,plot_y));
        childSensor = childSensor.nextSiblingElement("Sensor");
    }

//    qDebug() << "layout.m_qListChannels" << layout.m_qListChannels;
//    qDebug() << "layout.m_qListLocations" << layout.m_qListLocations;

//    if(t_iNumChannels == layout.m_qListChannels.size())
        return layout;
//    else
//    {
//        qWarning() << "Number of channel inconsistency!";
//        return SensorLayout();
//    }
}
