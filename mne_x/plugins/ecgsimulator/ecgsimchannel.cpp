//=============================================================================================================
/**
* @file		ecgsimchannel.cpp
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the implementation of the ECGChannel class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecgsimchannel.h"


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

using namespace ECGSimulatorModule;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ECGSimChannel::ECGSimChannel(QString ResourceDataPath, QString ChannelFile, bool enabled, bool visible)
: m_qStringResourceDataPath(ResourceDataPath)
, m_qStringChannelFile(ChannelFile)
, m_bIsEnabled(enabled)
, m_bIsVisible(visible)
{

}


//*************************************************************************************************************

ECGSimChannel::~ECGSimChannel()
{
}


//*************************************************************************************************************

void ECGSimChannel::initChannel()
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

void ECGSimChannel::clear()
{
    m_vecBuffer.clear();
}
