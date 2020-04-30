//=============================================================================================================
/**
 * @file     realtimesourceestimate.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the RealTimeSourceEstimate class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimesourceestimate.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
using namespace MNELIB;
using namespace FSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeSourceEstimate::RealTimeSourceEstimate(QObject *parent)
: Measurement(QMetaType::type("RealTimeSourceEstimate::SPtr"), parent)
, m_pAnnotSet(AnnotationSet::SPtr(new AnnotationSet))
, m_pSurfSet(SurfaceSet::SPtr(new SurfaceSet))
, m_pFwdSolution(MNEForwardSolution::SPtr(new MNEForwardSolution))
, m_iSourceEstimateSize(1)
, m_bInitialized(false)
{
}

//=============================================================================================================

RealTimeSourceEstimate::~RealTimeSourceEstimate()
{
}

//=============================================================================================================

QList<MNESourceEstimate::SPtr>& RealTimeSourceEstimate::getValue()
{
    QMutexLocker locker(&m_qMutex);
    return m_pMNEStc;
}

//=============================================================================================================

void RealTimeSourceEstimate::setValue(MNESourceEstimate& v)
{
    m_qMutex.lock();

    //Store
    MNESourceEstimate::SPtr pMNESourceEstimate = MNESourceEstimate::SPtr::create(v);
    m_pMNEStc.append(pMNESourceEstimate);

    m_bInitialized = true;

    m_qMutex.unlock();

    if(m_pMNEStc.size() >= m_iSourceEstimateSize)
    {
        emit notify();
        m_qMutex.lock();
        m_pMNEStc.clear();
        m_qMutex.unlock();
    }
}

