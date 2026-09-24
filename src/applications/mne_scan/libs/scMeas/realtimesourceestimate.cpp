//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     realtimesourceestimate.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the RealTimeSourceEstimate class.
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
using namespace INVLIB;
using namespace FSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeSourceEstimate::RealTimeSourceEstimate(QObject *parent)
: Measurement(QMetaType::fromName("RealTimeSourceEstimate::SPtr").id(), parent)
, m_pAnnotSet(FsAnnotationSet::SPtr(new FsAnnotationSet))
, m_pSurfSet(FsSurfaceSet::SPtr(new FsSurfaceSet))
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

QList<InvSourceEstimate::SPtr>& RealTimeSourceEstimate::getValue()
{
    QMutexLocker locker(&m_qMutex);
    return m_pMNEStc;
}

//=============================================================================================================

void RealTimeSourceEstimate::setValue(InvSourceEstimate& v)
{
    m_qMutex.lock();

    //Store
    InvSourceEstimate::SPtr pMNESourceEstimate = InvSourceEstimate::SPtr::create(v);
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

