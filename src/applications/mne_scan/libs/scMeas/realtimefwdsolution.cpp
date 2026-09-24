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
#include <mne/mne_forward_solution.h>
#include <time.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
using namespace FIFFLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeFwdSolution::RealTimeFwdSolution(QObject *parent)
: Measurement(QMetaType::fromName("RealTimeFwdSolution::SPtr").id(), parent)
, m_bInitialized(false)
, m_bClustered(false)
, m_pFwdSolution(QSharedPointer<MNEForwardSolution>(new MNEForwardSolution))
{
}

//=============================================================================================================

RealTimeFwdSolution::~RealTimeFwdSolution()
{
}

//=============================================================================================================

void RealTimeFwdSolution::setFiffInfo(QSharedPointer<FiffInfo> pFiffInfo)
{
    m_pFiffInfo = pFiffInfo;
}

//=============================================================================================================

QSharedPointer<FiffInfo> RealTimeFwdSolution::getFiffInfo()
{
    return m_pFiffInfo;
}

//=============================================================================================================

QSharedPointer<MNEForwardSolution> RealTimeFwdSolution::getValue()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFwdSolution;
}

//=============================================================================================================

void RealTimeFwdSolution::setValue(const MNEForwardSolution::SPtr pFwdSolution)
{
    m_qMutex.lock();
    m_pFwdSolution = pFwdSolution;
    m_bInitialized = true;
    m_bClustered = m_pFwdSolution->isClustered();
    m_qMutex.unlock();

    emit notify();
}

//=============================================================================================================

QSharedDataPointer<FiffNamedMatrix>& RealTimeFwdSolution::getSol()
{
    QMutexLocker locker(&m_qMutex);
    return m_pNamedMatSol;
}

//=============================================================================================================

void RealTimeFwdSolution::setSol(const FiffNamedMatrix::SDPtr& pNamedMatSol)
{
    m_qMutex.lock();
    //Store
    m_pNamedMatSol = pNamedMatSol;
    m_bInitialized = true;
    m_qMutex.unlock();

    emit notify();
}
//=============================================================================================================

QSharedDataPointer<FiffNamedMatrix>& RealTimeFwdSolution::getSolGrad()
{
    QMutexLocker locker(&m_qMutex);
    return m_pNamedMatSolGrad;
}

//=============================================================================================================

void RealTimeFwdSolution::setSolGrad(const FiffNamedMatrix::SDPtr& pNamedMatSolGrad)
{
    m_qMutex.lock();
    //Store
    m_pNamedMatSol = pNamedMatSolGrad;
    m_bInitialized = true;
    m_qMutex.unlock();

    emit notify();
}
