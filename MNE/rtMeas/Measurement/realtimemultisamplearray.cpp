//=============================================================================================================
/**
* @file		realtimemultisamplearray.cpp
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
* @brief	Contains the implementation of the RealTimeMultiSampleArray class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimemultisamplearray.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CSART;
//using namespace IOBuffer;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeMultiSampleArray::RealTimeMultiSampleArray(unsigned int uiNumChannels)
: Measurement()
, m_dMinValue(0)
, m_dMaxValue(65535)
, m_dSamplingRate(0)
, m_qString_Unit("")
, m_uiNumChannels(uiNumChannels)
, m_ucMultiArraySize(10)

{

}


//*************************************************************************************************************

RealTimeMultiSampleArray::~RealTimeMultiSampleArray()
{

}


//*************************************************************************************************************

QVector<double> RealTimeMultiSampleArray::getVector() const
{
    return m_vecValue;
}


//*************************************************************************************************************

void RealTimeMultiSampleArray::setVector(QVector<double> v)
{
    if(v.size() != m_uiNumChannels)
        qDebug() << "Error Occured in RealTimeMultiSampleArray::setVector: Vector size does not matche the number of channels! ";

    for(QVector<double>::iterator it = v.begin(); it != v.end(); ++it)
    {
        if(*it < m_dMinValue) *it = m_dMinValue;
        else if(*it > m_dMaxValue) *it = m_dMaxValue;
    }

    m_vecValue = v;
    m_matSamples.push_back(m_vecValue);
    if(m_matSamples.size() >= m_ucMultiArraySize && notifyEnabled)
    {
        notify();
        m_matSamples.clear();
    }
}
