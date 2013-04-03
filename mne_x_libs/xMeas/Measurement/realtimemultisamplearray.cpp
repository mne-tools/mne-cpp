//=============================================================================================================
/**
* @file     realtimemultisamplearray.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the RealTimeMultiSampleArray class.
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

using namespace XMEASLIB;
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
