//=============================================================================================================
/**
* @file     tmsichannel.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the TMSIChannel class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsichannel.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QTextStream>
#include <QtCore/QFile>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSIChannel::TMSIChannel(QString ResourceDataPath, QString ChannelFile, bool enabled, bool visible)
: m_qStringResourceDataPath(ResourceDataPath)
, m_qStringChannelFile(ChannelFile)
, m_bIsEnabled(enabled)
, m_bIsVisible(visible)
{

}


//*************************************************************************************************************

TMSIChannel::~TMSIChannel()
{
}


//*************************************************************************************************************

void TMSIChannel::initChannel()
{
    QFile file;
    file.setFileName(m_qStringResourceDataPath+m_qStringChannelFile);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        double value;

        while(!in.atEnd())
        {
            in >> value;

            //init min and max with first value
            if(m_vecBuffer.size() == 0)
            {
                m_dMin = value;
                m_dMax = value;
            }

            if(value < m_dMin)
                m_dMin = value;

            if(value > m_dMax)
                m_dMax = value;

            m_vecBuffer.push_back(value);

        }
        file.close();
    }
}


//*************************************************************************************************************

void TMSIChannel::clear()
{
    m_vecBuffer.clear();
}
