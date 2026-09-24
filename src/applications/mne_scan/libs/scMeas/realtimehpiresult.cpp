//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     realtimehpiresult.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March, 2020
 * @brief    Definition of the RealTimeHpiResult class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimehpiresult.h"

#include <time.h>
#include <fiff/fiff_digitizer_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
using namespace FIFFLIB;
using namespace INVLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeHpiResult::RealTimeHpiResult(QObject *parent)
: Measurement(QMetaType::fromName("RealTimeHpiResult::SPtr").id(), parent)
, m_bInitialized(false)
, m_pHpiFitResult(QSharedPointer<HpiFitResult>(new HpiFitResult))
, m_iMaxHeadPositions(1000)
{
}

//=============================================================================================================

RealTimeHpiResult::~RealTimeHpiResult()
{
}

//=============================================================================================================

QSharedPointer<INVLIB::HpiFitResult>& RealTimeHpiResult::getValue()
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

    // Keep the head origin of this fit so the path the head travelled can be
    // drawn later. The device-to-head transform maps device to head, so the
    // head origin expressed in device coordinates is the translation of the
    // inverse transform.
    if(m_iMaxHeadPositions > 0 && !v.devHeadTrans.isEmpty()) {
        m_vecHeadPositions.append(v.devHeadTrans.invtrans.block<3,1>(0,3));

        while(m_vecHeadPositions.size() > m_iMaxHeadPositions) {
            m_vecHeadPositions.removeFirst();
        }
    }
    m_qMutex.unlock();

    emit notify();
}

//=============================================================================================================

QVector<Eigen::Vector3f> RealTimeHpiResult::headPositionHistory() const
{
    QMutexLocker locker(&m_qMutex);
    return m_vecHeadPositions;
}

//=============================================================================================================

void RealTimeHpiResult::clearHeadPositionHistory()
{
    QMutexLocker locker(&m_qMutex);
    m_vecHeadPositions.clear();
}

//=============================================================================================================

void RealTimeHpiResult::setHeadPositionHistorySize(int iMaxPositions)
{
    QMutexLocker locker(&m_qMutex);
    m_iMaxHeadPositions = iMaxPositions > 0 ? iMaxPositions : 0;

    while(m_vecHeadPositions.size() > m_iMaxHeadPositions) {
        m_vecHeadPositions.removeFirst();
    }
}

//=============================================================================================================

void RealTimeHpiResult::setDigitizerData(QSharedPointer<FIFFLIB::FiffDigitizerData> digData)
{
    QMutexLocker lock(&m_qMutex);
    m_pFiffDigData = digData;
}
