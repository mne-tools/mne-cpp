//=============================================================================================================
/**
 * @file     realtimeneurofeedbackresult.cpp
 * @author   Simon Marxgut <simon.marxgut@umit-tirol.at>
 * @since    0.1.0
 * @date     November, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Simon Marxgut. All rights reserved.
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
 * @brief    Definition of the RealTimeNeurofeedbackResult class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeneurofeedbackresult.h"

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

RealTimeNeurofeedbackResult::RealTimeNeurofeedbackResult(QObject *parent)
: Measurement(QMetaType::type("RealTimeNeurofeedbackResult::SPtr"), parent)
, m_fSamplingRate(0)
, m_iMultiArraySize(10)
, m_bChInfoIsInit(false)
{
}

//=============================================================================================================

RealTimeNeurofeedbackResult::~RealTimeNeurofeedbackResult()
{
}

//=============================================================================================================

void RealTimeNeurofeedbackResult::init(QList<RealTimeSampleArrayChInfo> &chInfo)
{
    qDebug()<<"RealTimeNeurofeedbackResult::init";
    QMutexLocker locker(&m_qMutex);
    m_qListChInfo = chInfo;

    m_bChInfoIsInit = true;
}

//=============================================================================================================

void RealTimeNeurofeedbackResult::initFromFiffInfo(FiffInfo::SPtr pFiffInfo)
{
    qDebug()<<"RealTimeNeurofeedbackResult::initFromFiffInfo";
    QMutexLocker locker(&m_qMutex);
    m_qListChInfo.clear();
    m_bChInfoIsInit = false;

    bool t_bIsBabyMEG = false;

    if(pFiffInfo->acq_pars == "BabyMEG")
        t_bIsBabyMEG = true;

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
//        else
//        {

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

void RealTimeNeurofeedbackResult::setValue(const MatrixXd& mat)
{
    if(!m_bChInfoIsInit)
        return;
    m_qMutex.lock();

    m_Neurolist.push_back(mat);

    m_qMutex.unlock();
    if(m_Neurolist.size() >= m_iMultiArraySize){
        emit notify();
        m_qMutex.lock();
        m_Neurolist.clear();
        m_qMutex.unlock();
    }
}

