//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     realtimecov.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    Definition of the RealTimeCov class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimecov.h"

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

RealTimeCov::RealTimeCov(QObject* parent)
: Measurement(QMetaType::fromName("RealTimeCov::SPtr").id(), parent)
, m_pFiffCov(std::make_shared<FiffCov>())
, m_pFiffInfo(std::make_shared<FiffInfo>())
, m_bInitialized(false)
{
}

//=============================================================================================================

RealTimeCov::~RealTimeCov()
{
}

//=============================================================================================================

std::shared_ptr<FiffCov>& RealTimeCov::getValue()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffCov;
}

//=============================================================================================================

void RealTimeCov::setFiffInfo(std::shared_ptr<FiffInfo> pFiffInfo)
{
    m_pFiffInfo = std::move(pFiffInfo);
}

//=============================================================================================================

std::shared_ptr<FiffInfo> RealTimeCov::getFiffInfo()
{
    return m_pFiffInfo;
}

//=============================================================================================================

void RealTimeCov::setValue(const FiffCov& v)
{
    m_qMutex.lock();
    //Store
    *m_pFiffCov = v;
    m_bInitialized = true;
    m_qMutex.unlock();

    emit notify();
}
