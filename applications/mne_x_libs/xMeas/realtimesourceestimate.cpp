//=============================================================================================================
/**
* @file     realtimesourceestimate.cpp
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
* @brief    Contains the implementation of the RealTimeSourceEstimate class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimesourceestimate.h"


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

RealTimeSourceEstimate::RealTimeSourceEstimate(QObject *parent)
: NewMeasurement(QMetaType::type("RealTimeSourceEstimate::SPtr"), parent)
, m_ucArraySize(200)
{

}


//*************************************************************************************************************

RealTimeSourceEstimate::~RealTimeSourceEstimate()
{

}


//*************************************************************************************************************

VectorXd RealTimeSourceEstimate::getValue() const
{
    return m_vecValue;
}


//*************************************************************************************************************

void RealTimeSourceEstimate::setValue(VectorXd v)
{
//    //check vector size
//    if(v.size() != m_qListChInfo.size())
//        qCritical() << "Error Occured in RealTimeMultiSampleArrayNew::setVector: Vector size does not matche the number of channels! ";

//    //Check if maximum exceeded //ToDo speed this up
//    for(qint32 i = 0; i < v.size(); ++i)
//    {
//        if(v[i] < m_qListChInfo[i].getMinValue()) v[i] = m_qListChInfo[i].getMinValue();
//        else if(v[i] > m_qListChInfo[i].getMaxValue()) v[i] = m_qListChInfo[i].getMaxValue();
//    }

    //Store
    m_vecValue = v;
    m_matSamples.push_back(m_vecValue);
    if(m_matSamples.size() >= m_ucArraySize)
    {
        emit notify();
        m_matSamples.clear();
    }
}

