//=============================================================================================================
/**
 * @file     realtimesamplearray.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the RealTimeSampleArray class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimesamplearray.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
//using namespace IOBUFFER;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeSampleArray::RealTimeSampleArray(QObject *parent)
: Measurement(QMetaType::type("RealTimeSampleArray::SPtr"), parent)
, m_dMinValue(0)
, m_dMaxValue(65535)
, m_dSamplingRate(0)
, m_qString_Unit("")
, m_dValue(0)
, m_ucArraySize(10)

{

}

//=============================================================================================================

RealTimeSampleArray::~RealTimeSampleArray()
{

}

//=============================================================================================================

double RealTimeSampleArray::getValue() const
{
    QMutexLocker locker(&m_qMutex);
    return m_dValue;
}

//=============================================================================================================

void RealTimeSampleArray::setValue(double v)
{
    m_qMutex.lock();
    if(v < m_dMinValue) v = m_dMinValue;
    else if(v > m_dMaxValue) v = m_dMaxValue;
    m_dValue = v;
    m_vecSamples.push_back(m_dValue);
    m_qMutex.unlock();
    if(m_vecSamples.size() >= m_ucArraySize)
    {
        emit notify();
        m_qMutex.lock();
        m_vecSamples.clear();
        m_qMutex.unlock();
    }
}
