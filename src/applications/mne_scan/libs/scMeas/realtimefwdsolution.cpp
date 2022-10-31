//=============================================================================================================
/**
 * @file     realtimefwdsolution.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.1
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    Definition of the RealTimeFwdSolution class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimefwdsolution.h"
#include <mne/mne_forwardsolution.h>
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
: Measurement(QMetaType::type("RealTimeFwdSolution::SPtr"), parent)
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
