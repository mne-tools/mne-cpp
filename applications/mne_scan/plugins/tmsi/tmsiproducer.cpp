//=============================================================================================================
/**
* @file     tmsiproducer.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the TMSIProducer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsiproducer.h"
#include "tmsi.h"
#include "tmsidriver.h"

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPLUGIN;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSIProducer::TMSIProducer(TMSI* pTMSI)
: m_pTMSI(pTMSI)
, m_pTMSIDriver(new TMSIDriver(this))
, m_bIsRunning(true)
{
}


//*************************************************************************************************************

TMSIProducer::~TMSIProducer()
{
    //cout << "TMSIProducer::~TMSIProducer()" << endl;
}


//*************************************************************************************************************

void TMSIProducer::start(int iNumberOfChannels,
                     int iSamplingFrequency,
                     int iSamplesPerBlock,
                     bool bUseChExponent,
                     bool bUseUnitGain,
                     bool bUseUnitOffset,
                     bool bWriteDriverDebugToFile,
                     QString sOutputFilePath,
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
                              sOutputFilePath,
                              bUseCommonAverage,
                              bMeasureImpedance))
    {
        m_bIsRunning = true;
        QThread::start();
    }
    else
        m_bIsRunning = false;
}


//*************************************************************************************************************

void TMSIProducer::stop()
{
    //Wait until this thread (TMSIProducer) is stopped
    m_bIsRunning = false;

    //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the push function (acquire statement)
    m_pTMSI->m_pRawMatrixBuffer_In->releaseFromPush();

    while(this->isRunning())
        m_bIsRunning = false;

    //Unitialise device only after the thread stopped
    m_pTMSIDriver->uninitDevice();
}


//*************************************************************************************************************

void TMSIProducer::run()
{
    MatrixXf matRawBuffer(m_pTMSI->m_iNumberOfChannels, m_pTMSI->m_iSamplesPerBlock);

    while(m_bIsRunning)
    {
        //std::cout<<"TMSIProducer::run()"<<std::endl;
        //Get the TMSi EEG data out of the device buffer and write received data to circular buffer
        if(m_pTMSIDriver->getSampleMatrixValue(matRawBuffer))
            m_pTMSI->m_pRawMatrixBuffer_In->push(&matRawBuffer);
    }

    //std::cout<<"EXITING - TMSIProducer::run()"<<std::endl;
}


