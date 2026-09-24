//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     eegosportsproducer.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Contains the implementation of the EEGoSportsProducer class.
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
