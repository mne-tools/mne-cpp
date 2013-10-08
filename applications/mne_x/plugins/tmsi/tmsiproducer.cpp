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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
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

using namespace TMSIPlugin;


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
                     bool bConvertToVolt,
                     bool bUseChExponent,
                     bool bUseUnitGain,
                     bool bUseUnitOffset,
                     bool bWriteToFile,
                     QString sOutputFilePath)
{
    //Initialise device
    if(m_pTMSIDriver->initDevice(iNumberOfChannels,
                              iSamplingFrequency,
                              iSamplesPerBlock,
                              bConvertToVolt,
                              bUseChExponent,
                              bUseUnitGain,
                              bUseUnitOffset,
                              bWriteToFile,
                              sOutputFilePath))
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
    //Stop this (TMSIProducer) thread
    m_bIsRunning = false;

//    if(this->isRunning())
//        QThread::wait();

    //Uinitialise device
    m_pTMSIDriver->uninitDevice();
}


//*************************************************************************************************************

void TMSIProducer::run()
{
    while(m_bIsRunning)
    {
        std::cout<<"TMSIProducer::run()"<<std::endl;
        MatrixXf matRawBuffer(m_pTMSI->m_iNumberOfChannels, m_pTMSI->m_iSamplesPerBlock);

//        //Wait for sampling period to pass
//        float uiSamplePeriod = 1.0/((float)(m_pTMSI->m_iSamplingFreq));
//        usleep(uiSamplePeriod*m_pTMSI->m_iSamplesPerBlock);

        //Get the TMSi EEG data out of the device buffer and write received data to circular buffer
        if(m_pTMSIDriver->getSampleMatrixValue(matRawBuffer))
            m_pTMSI->m_pRawMatrixBuffer_In->push(&matRawBuffer);
    }

    std::cout<<"EXITING - TMSIProducer::run()"<<std::endl;
}


