//=============================================================================================================
/**
 * @file     eegosportsproducer.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Viktor Klueber, Johannes Vorwerk. All rights reserved.
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
 * @brief    Contains the implementation of the EEGoSportsProducer class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportsproducer.h"
#include "eegosports.h"
#include "eegosportsdriver.h"

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGOSPORTSPLUGIN;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsProducer::EEGoSportsProducer(EEGoSports* pEEGoSports)
: m_pEEGoSports(pEEGoSports)
, m_pEEGoSportsDriver(new EEGoSportsDriver(this))
{
}

//=============================================================================================================

EEGoSportsProducer::~EEGoSportsProducer()
{
}

//=============================================================================================================

bool EEGoSportsProducer::init(bool bWriteDriverDebugToFile,
                              bool bMeasureImpedance)
{
    //Initialise device
    if(m_pEEGoSportsDriver->initDevice(bWriteDriverDebugToFile,
                                       bMeasureImpedance)) {
        m_bIsConnected = true;
    } else {
        m_bIsConnected = false;
        return false;
    }

    //Return number of channels
    m_pEEGoSports->setNumberOfChannels(m_pEEGoSportsDriver->getNumberOfChannels(),
                                       m_pEEGoSportsDriver->getNumberOfEEGChannels(),
                                       m_pEEGoSportsDriver->getNumberOfBipolarChannels());

    return true;
}

//=============================================================================================================

void EEGoSportsProducer::start(int iSamplesPerBlock,
                               int iSamplingFrequency,
                               bool bMeasureImpedance)
{
    //Initialise device
    if(m_bIsConnected && m_pEEGoSportsDriver->startRecording(iSamplesPerBlock,
                                iSamplingFrequency,
                                bMeasureImpedance)) {
        QThread::start();
    }
}

//=============================================================================================================

void EEGoSportsProducer::stop()
{    
    requestInterruption();
    wait();

    //Unitialise device only after the thread stopped
    m_pEEGoSportsDriver->uninitDevice();

    m_bIsConnected = false;
}

//=============================================================================================================

QList<uint> EEGoSportsProducer::getChannellist()
{
    return m_pEEGoSportsDriver->getChannellist();
}

//=============================================================================================================

void EEGoSportsProducer::run()
{
    MatrixXd matRawBuffer;

    while(!isInterruptionRequested()) {
        //Get the EEG data out of the device buffer and send it to main thread
        if(m_pEEGoSportsDriver->getSampleMatrixValue(matRawBuffer)) {
            m_pEEGoSports->setSampleData(matRawBuffer);
        }
    }
}
