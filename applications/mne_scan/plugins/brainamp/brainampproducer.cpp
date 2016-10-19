//=============================================================================================================
/**
* @file     brainampproducer.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch, Viktor Klüber and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the BrainAmpProducer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainampproducer.h"
#include "brainamp.h"
#include "brainampdriver.h"

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BRAINAMPPLUGIN;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainAmpProducer::BrainAmpProducer(BrainAmp* pBrainAmp)
: m_pBrainAmp(pBrainAmp)
, m_pBrainAmpDriver(new BrainAmpDriver(this))
, m_bIsRunning(true)
{
}


//*************************************************************************************************************

BrainAmpProducer::~BrainAmpProducer()
{
    //cout << "BrainAmpProducer::~BrainAmpProducer()" << endl;
}


//*************************************************************************************************************

void BrainAmpProducer::start(int iNumberOfChannels,
                        int iSamplesPerBlock,
                        int iSamplingFrequency,
                        bool bWriteDriverDebugToFile,
                        QString sOutputFilePath,
                        bool bMeasureImpedance)
{
    //Initialise device
    if(m_pBrainAmpDriver->initDevice(iNumberOfChannels,
                                iSamplesPerBlock,
                                iSamplingFrequency,
                                bWriteDriverDebugToFile,
                                sOutputFilePath,
                                bMeasureImpedance))
    {
        m_bIsRunning = true;
        QThread::start();
    }
    else
        m_bIsRunning = false;
}


//*************************************************************************************************************

void BrainAmpProducer::stop()
{
    //Wait until this thread (BrainAmpProducer) is stopped
    m_bIsRunning = false;

    //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the push function (acquire statement)
    m_pBrainAmpDriver->m_pRawMatrixBuffer_In->releaseFromPush();

    while(this->isRunning())
        m_bIsRunning = false;

    //Unitialise device only after the thread stopped
    m_pBrainAmpDriver->uninitDevice();
}


//*************************************************************************************************************

void BrainAmpProducer::run()
{
    while(m_bIsRunning)
    {
        //std::cout<<"BrainAmpProducer::run()"<<std::endl;
        //Get the TMSi EEG data out of the device buffer and write received data to a QList
        MatrixXd matRawBuffer;

        if(m_pBrainAmpDriver->getSampleMatrixValue(matRawBuffer)) {
            m_pBrainAmpDriver->setSampleData(matRawBuffer);
        }
    }

    std::cout<<"EXITING - BrainAmpProducer::run()"<<std::endl;
}


