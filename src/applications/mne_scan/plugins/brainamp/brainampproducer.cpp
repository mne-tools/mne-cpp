//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     brainampproducer.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Contains the implementation of the BrainAMPProducer class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainampproducer.h"
#include "brainamp.h"
#include "brainampdriver.h"

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BRAINAMPPLUGIN;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainAMPProducer::BrainAMPProducer(BrainAMP* pBrainAmp)
: m_pBrainAmp(pBrainAmp)
, m_pBrainAmpDriver(new BrainAMPDriver(this))
, m_bIsRunning(true)
{
}

//=============================================================================================================

BrainAMPProducer::~BrainAMPProducer()
{
}

//=============================================================================================================

void BrainAMPProducer::start(int iSamplesPerBlock,
                             int iSamplingFrequency)
{
    //Initialise device
    if(m_pBrainAmpDriver->initDevice(iSamplesPerBlock,
                                     iSamplingFrequency)) {
        m_bIsRunning = true;
        QThread::start();
    } else {
        m_bIsRunning = false;
    }
}

//=============================================================================================================

void BrainAMPProducer::stop()
{
    //Wait until this thread (BrainAMPProducer) is stopped
    m_bIsRunning = false;

    while(this->isRunning())
        m_bIsRunning = false;

    //Unitialise device only after the thread stopped
    m_pBrainAmpDriver->uninitDevice();
}

//=============================================================================================================

void BrainAMPProducer::run()
{
    while(m_bIsRunning)
    {
        //std::cout<<"BrainAMPProducer::run()"<<std::endl;
        //Get the TMSi EEG data out of the device buffer and write received data to a QList
        MatrixXd matRawBuffer;

        if(m_pBrainAmpDriver->getSampleMatrixValue(matRawBuffer)) {
            m_pBrainAmp->setSampleData(matRawBuffer);
        }
    }

    std::cout<<"EXITING - BrainAMPProducer::run()"<<std::endl;
}

