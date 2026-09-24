//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     realtimeevokedset.cpp
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2016
 * @brief    Definition of the RealTimeEvokedSet class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeevokedset.h"

#include <time.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
#include <QTime>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeEvokedSet::RealTimeEvokedSet(QObject *parent)
: Measurement(QMetaType::fromName("RealTimeEvokedSet::SPtr").id(), parent)
, m_pFiffEvokedSet(new FiffEvokedSet)
, m_iPreStimSamples(0)
, m_bInitialized(false)
{
}

//=============================================================================================================

RealTimeEvokedSet::~RealTimeEvokedSet()
{
}

//=============================================================================================================

void RealTimeEvokedSet::init(FiffInfo::SPtr p_fiffInfo)
{
    QMutexLocker locker(&m_qMutex);
    m_qListChInfo.clear();
    m_qListChColors.clear();

    m_pFiffInfo = p_fiffInfo;

    srand(static_cast<uint>(time(Q_NULLPTR)));
    for(qint32 i = 0; i < p_fiffInfo->nchan; ++i)
    {
         m_qListChColors.append(QColor(rand() % 256, rand() % 256, rand() % 256));

        RealTimeSampleArrayChInfo initChInfo;
        initChInfo.setChannelName(p_fiffInfo->chs[i].ch_name);

        // set channel Unit
        initChInfo.setUnit(p_fiffInfo->chs[i].unit);

        //Treat stimulus channels different
        if(p_fiffInfo->chs[i].kind == FIFFV_STIM_CH)
        {
//            initChInfo.setUnit("");
            initChInfo.setMinValue(0);
            initChInfo.setMaxValue(1.0e6);
        }

        // set channel Kind
        initChInfo.setKind(p_fiffInfo->chs[i].kind);

        // set channel coil
        initChInfo.setCoil(p_fiffInfo->chs[i].chpos.coil_type);

        m_qListChInfo.append(initChInfo);
    }
}

//=============================================================================================================

FiffEvokedSet::SPtr& RealTimeEvokedSet::getValue()
{
    QMutexLocker locker(&m_qMutex);
    return m_pFiffEvokedSet;
}

//=============================================================================================================

const QStringList& RealTimeEvokedSet::getResponsibleTriggerTypes()
{
    QMutexLocker locker(&m_qMutex);
    return m_lResponsibleTriggerTypes;
}

//=============================================================================================================

void RealTimeEvokedSet::setValue(const FiffEvokedSet &v,
                                 const FiffInfo::SPtr &p_fiffinfo,
                                 const QStringList &lResponsibleTriggerTypes)
{
    //Store
    m_qMutex.lock();
     *m_pFiffEvokedSet = v;
    m_lResponsibleTriggerTypes = lResponsibleTriggerTypes;
    m_qMutex.unlock();

    if(!m_bInitialized) {
        init(p_fiffinfo);

        m_qMutex.lock();
        m_iPreStimSamples = 0;

        //Take the first evoked iformation to calcualte the pre samples.
        //They all have the same pre sample size as of right now.
        if(!m_pFiffEvokedSet->evoked.isEmpty()) {
            for(qint32 i = 0; i < m_pFiffEvokedSet->evoked.at(0).times.size(); ++i) {
                if(m_pFiffEvokedSet->evoked.at(0).times[i] >= 0) {
                    break;
                } else {
                    ++m_iPreStimSamples;
                }
            }
        }

        m_bInitialized = true;
        m_qMutex.unlock();
    }

    emit notify();
}

//=============================================================================================================
