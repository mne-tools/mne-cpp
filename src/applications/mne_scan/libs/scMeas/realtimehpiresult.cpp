//=============================================================================================================
/**
 * @file     realtimehpiresult.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the RealTimeHpiResult class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimehpiresult.h"

#include <time.h>
#include <fiff/c/fiff_digitizer_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeHpiResult::RealTimeHpiResult(QObject *parent)
: Measurement(QMetaType::type("RealTimeHpiResult::SPtr"), parent)
, m_bInitialized(false)
, m_pHpiFitResult(QSharedPointer<HpiFitResult>(new HpiFitResult))
{
}

//=============================================================================================================

RealTimeHpiResult::~RealTimeHpiResult()
{
}

//=============================================================================================================

QSharedPointer<INVERSELIB::HpiFitResult>& RealTimeHpiResult::getValue()
{
    QMutexLocker locker(&m_qMutex);
    return m_pHpiFitResult;
}

//=============================================================================================================

void RealTimeHpiResult::setFiffInfo(QSharedPointer<FiffInfo> pFiffInfo)
{
    m_pFiffInfo = pFiffInfo;
}

//=============================================================================================================

QSharedPointer<FiffInfo> RealTimeHpiResult::getFiffInfo()
{
    return m_pFiffInfo;
}

//=============================================================================================================

void RealTimeHpiResult::setValue(const HpiFitResult& v)
{
    m_qMutex.lock();
    //Store
     *m_pHpiFitResult = v;
    m_bInitialized = true;
    m_qMutex.unlock();

    emit notify();
}

//=============================================================================================================

void RealTimeHpiResult::setDigitizerData(QSharedPointer<FIFFLIB::FiffDigitizerData> digData)
{
    QMutexLocker lock(&m_qMutex);
    m_pFiffDigData = digData;
}
