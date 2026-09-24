//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     realtimemultisamplearray.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the RealTimeMultiSampleArray class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimemultisamplearray.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_digitizer_data.h>

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeMultiSampleArray::RealTimeMultiSampleArray(QObject *parent)
: Measurement(QMetaType::fromName("RealTimeMultiSampleArray::SPtr").id(), parent)
, m_pFiffInfo_orig(nullptr)
, m_pFiffDigitizerData_orig(nullptr)
, m_fSamplingRate(0)
, m_iMultiArraySize(10)
, m_bChInfoIsInit(false)
{
}

//=============================================================================================================

RealTimeMultiSampleArray::~RealTimeMultiSampleArray()
{
}

//=============================================================================================================

void RealTimeMultiSampleArray::init(QList<RealTimeSampleArrayChInfo> &chInfo)
{
    QMutexLocker locker(&m_qMutex);
    m_qListChInfo = chInfo;

    m_bChInfoIsInit = true;

//    m_qListChInfo.clear();
//    for(quint32 i = 0; i < uiNumChannels; ++i)
//    {
//        RealTimeSampleArrayChInfo initChInfo;
//        QString string;
//        initChInfo.setChannelName(string.number(i+1));
//        m_qListChInfo.append(initChInfo);
//    }
}

//=============================================================================================================

void RealTimeMultiSampleArray::initFromFiffInfo(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo)
{
    QMutexLocker locker(&m_qMutex);
    m_qListChInfo.clear();
    m_bChInfoIsInit = false;

    for(qint32 i = 0; i < pFiffInfo->nchan; ++i)
    {
        RealTimeSampleArrayChInfo initChInfo;
        initChInfo.setChannelName(pFiffInfo->chs[i].ch_name);

        // set channel Unit
        initChInfo.setUnit(pFiffInfo->chs[i].unit);

        //Treat stimulus channels different
        if(pFiffInfo->chs[i].kind == FIFFV_STIM_CH)
        {
//            initChInfo.setUnit("");
            initChInfo.setMinValue(0);
            initChInfo.setMaxValue(1.0e6);
        }

        // set channel Kind
        initChInfo.setKind(pFiffInfo->chs[i].kind);

        // set channel coil
        initChInfo.setCoil(pFiffInfo->chs[i].chpos.coil_type);

        m_qListChInfo.append(initChInfo);
    }

    //Sampling rate
    m_fSamplingRate = pFiffInfo->sfreq;

    m_pFiffInfo_orig = pFiffInfo;

    m_bChInfoIsInit = true;
}

//=============================================================================================================

void RealTimeMultiSampleArray::setValue(const MatrixXd& mat)
{
    if(!m_bChInfoIsInit)
        return;

    m_qMutex.lock();
    //check vector size
    if(mat.rows() != m_qListChInfo.size())
        qCritical() << "Error Occured in RealTimeMultiSampleArray::setVector: Vector size does not match the number of channels! ";

    //Store
    m_matSamples.push_back(mat);

    m_qMutex.unlock();
    if(m_matSamples.size() >= m_iMultiArraySize)
    {
        emit notify();
        m_qMutex.lock();
        m_matSamples.clear();
        m_qMutex.unlock();
    }
}

//=============================================================================================================

void RealTimeMultiSampleArray::setDigitizerData(QSharedPointer<FIFFLIB::FiffDigitizerData> digData)
{
    QMutexLocker locker(&m_qMutex);
    m_pFiffDigitizerData_orig = digData;
}
