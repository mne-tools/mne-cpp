//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     tmsiproducer.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     September, 2013
 * @brief    Contains the implementation of the TMSIProducer class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsiproducer.h"
#include "tmsi.h"
#include "tmsidriver.h"

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPLUGIN;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSIProducer::TMSIProducer(TMSI* pTMSI)
: m_pTMSI(pTMSI)
, m_pTMSIDriver(new TMSIDriver(this))
{
}

//=============================================================================================================

TMSIProducer::~TMSIProducer()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

void TMSIProducer::start(int iNumberOfChannels,
                         int iSamplingFrequency,
                         int iSamplesPerBlock,
                         bool bUseChExponent,
                         bool bUseUnitGain,
                         bool bUseUnitOffset,
                         bool bWriteDriverDebugToFile,
                         bool bUseCommonAverage,
                         bool bMeasureImpedance)
{
    //Initialise device
    if(m_pTMSIDriver->initDevice(iNumberOfChannels,
                              iSamplingFrequency,
                              iSamplesPerBlock,
                              bUseChExponent,
                              bUseUnitGain,
                              bUseUnitOffset,
                              bWriteDriverDebugToFile,
                              bUseCommonAverage,
                              bMeasureImpedance)) {
        QThread::start();
    }
}

//=============================================================================================================

void TMSIProducer::stop()
{
    requestInterruption();
    wait();
}

//=============================================================================================================

void TMSIProducer::run()
{
    MatrixXf matData(m_pTMSI->m_iNumberOfChannels, m_pTMSI->m_iSamplesPerBlock);

    while(!isInterruptionRequested()) {
        if(m_pTMSIDriver->getSampleMatrixValue(matData)) {
            while(!m_pTMSI->m_pCircularBuffer->push(matData) && !isInterruptionRequested()) {
                //Do nothing until the circular buffer is ready to accept new data again
            }
        }
    }

    //Unitialise device only after the thread stopped
    m_pTMSIDriver->uninitDevice();
}
