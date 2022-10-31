//=============================================================================================================
/**
 * @file     realtimeconnectivityestimate.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the RealTimeConnectivityEstimate class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeconnectivityestimate.h"

#include <connectivity/network/network.h>

#include <mne/mne_forwardsolution.h>

#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
using namespace CONNECTIVITYLIB;
using namespace MNELIB;
using namespace FSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeConnectivityEstimate::RealTimeConnectivityEstimate(QObject *parent)
: Measurement(QMetaType::type("RealTimeConnectivityEstimate::SPtr"), parent)
, m_pAnnotSet(AnnotationSet::SPtr(new AnnotationSet))
, m_pSurfSet(SurfaceSet::SPtr(new SurfaceSet))
, m_pFwdSolution(MNEForwardSolution::SPtr(new MNEForwardSolution))
, m_pNetwork(Network::SPtr(new Network))
, m_bInitialized(false)
{
}

//=============================================================================================================

RealTimeConnectivityEstimate::~RealTimeConnectivityEstimate()
{
}

//=============================================================================================================

QSharedPointer<Network> &RealTimeConnectivityEstimate::getValue()
{
    QMutexLocker locker(&m_qMutex);
    return m_pNetwork;
}

//=============================================================================================================

void RealTimeConnectivityEstimate::setValue(const Network& v)
{
    m_qMutex.lock();

    //Store
     *m_pNetwork = v;

    m_bInitialized = true;

    m_qMutex.unlock();

    emit notify();
}

