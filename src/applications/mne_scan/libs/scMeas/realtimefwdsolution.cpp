//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     realtimefwdsolution.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.1
 * @date     May, 2020
 * @brief    Definition of the RealTimeFwdSolution class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimefwdsolution.h"

#include <mne/mne/mne_forward_solution.h>
#include <time.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scan;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeFwdSolution::RealTimeFwdSolution(QObject* parent)
: Measurement(QMetaType::fromName("RealTimeFwdSolution::SPtr").id(), parent)
, m_bInitialized(false)
, m_bClustered(false)
, m_pFwdSolution(std::make_shared<MNEForwardSolution>())
{
}

//=============================================================================================================

RealTimeFwdSolution::~RealTimeFwdSolution()
{
}

//=============================================================================================================

void RealTimeFwdSolution::setFiffInfo(std::shared_ptr<FiffInfo> pFiffInfo)
{
    m_pFiffInfo = std::move(pFiffInfo);
}

//=============================================================================================================

std::shared_ptr<FiffInfo> RealTimeFwdSolution::getFiffInfo()
{
    return m_pFiffInfo;
}

//=============================================================================================================

std::shared_ptr<MNEForwardSolution> RealTimeFwdSolution::getValue()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFwdSolution;
}

//=============================================================================================================

void RealTimeFwdSolution::setValue(const std::shared_ptr<MNEForwardSolution>& pFwdSolution)
{
    m_qMutex.lock();
    m_pFwdSolution = pFwdSolution;
    m_bInitialized = true;
    m_bClustered = m_pFwdSolution->isClustered();
    m_qMutex.unlock();

    emit notify();
}
