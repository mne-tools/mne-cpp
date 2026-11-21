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
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scan;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeSourceEstimate::RealTimeSourceEstimate(QObject* parent)
: Measurement(QMetaType::fromName("RealTimeSourceEstimate::SPtr").id(), parent)
, m_pAnnotSet(std::make_shared<FsAnnotationSet>())
, m_pSurfSet(std::make_shared<FsSurfaceSet>())
, m_pFwdSolution(std::make_shared<MNEForwardSolution>())
, m_iSourceEstimateSize(1)
, m_bInitialized(false)
{
}

//=============================================================================================================

RealTimeSourceEstimate::~RealTimeSourceEstimate()
{
}

//=============================================================================================================

QList<std::shared_ptr<InvSourceEstimate>>& RealTimeSourceEstimate::getValue()
{
    QMutexLocker locker(&m_qMutex);
    return m_pMNEStc;
}

//=============================================================================================================

void RealTimeSourceEstimate::setValue(InvSourceEstimate& v)
{
    m_qMutex.lock();

    //Store
    std::shared_ptr<InvSourceEstimate> pMNESourceEstimate = std::make_shared<InvSourceEstimate>(v);
    m_pMNEStc.append(pMNESourceEstimate);

    m_bInitialized = true;

    m_qMutex.unlock();

    if (m_pMNEStc.size() >= m_iSourceEstimateSize) {
        emit notify();
        m_qMutex.lock();
        m_pMNEStc.clear();
        m_qMutex.unlock();
    }
}
