//=============================================================================================================
/**
* @file     fiffsimulatorproducer.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the FiffSimulatorProducer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulatorproducer.h"
#include "fiffsimulator.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FiffSimulatorPlugin;


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

FiffSimulatorProducer::FiffSimulatorProducer(FiffSimulator* p_pFiffSimulator)
: m_pFiffSimulator(p_pFiffSimulator)
, m_pRtDataClient(0)
, m_bDataClientIsConnected(false)
, m_iDataClientId(-1)
, m_bFlagInfoRequest(false)
, m_bFlagMeasuring(false)
{
}


//*************************************************************************************************************

FiffSimulatorProducer::~FiffSimulatorProducer()
{

}


//*************************************************************************************************************

void FiffSimulatorProducer::connectDataClient(QString p_sRtSeverIP)
{
    if(m_pRtDataClient.isNull())
        m_pRtDataClient = QSharedPointer<RtDataClient>(new RtDataClient);
    else if(m_bDataClientIsConnected)
        return;

    m_pRtDataClient->connectToHost(p_sRtSeverIP);
    m_pRtDataClient->waitForConnected(1000);

    if(m_pRtDataClient->state() == QTcpSocket::ConnectedState)
    {
        producerMutex.lock();
        if(!m_bDataClientIsConnected)
        {
            //
            // get client ID
            //
            m_iDataClientId = m_pRtDataClient->getClientId();

            //
            // set data client alias -> for convinience (optional)
            //
            m_pRtDataClient->setClientAlias(m_pFiffSimulator->m_sFiffSimulatorClientAlias); // used in option 2 later on

            //
            // set new state
            //
            m_bDataClientIsConnected = true;
            emit dataConnectionChanged(m_bDataClientIsConnected);
        }
        producerMutex.unlock();
    }
}


//*************************************************************************************************************

void FiffSimulatorProducer::disconnectDataClient()
{
    if(m_bDataClientIsConnected)
    {
        m_pRtDataClient->disconnectFromHost();
        m_pRtDataClient->waitForDisconnected();
        producerMutex.lock();
        m_iDataClientId = -1;
        m_bDataClientIsConnected = false;
        producerMutex.unlock();
        emit dataConnectionChanged(m_bDataClientIsConnected);
    }
}


//*************************************************************************************************************

void FiffSimulatorProducer::stop()
{
    //Wait until this thread (Producer) is stopped
    m_bIsRunning = false;
    m_bFlagMeasuring = false;

    if(m_pFiffSimulator->m_bCmdClientIsConnected) //ToDo replace this with is running
    {
        // Stop Measurement at rt_Server
        (*m_pFiffSimulator->m_pRtCmdClient)["stop-all"].send();
    }

    //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the push function (acquire statement)
    m_pFiffSimulator->m_pRawMatrixBuffer_In->releaseFromPush();

    while(this->isRunning())
        m_bIsRunning = false;

    this->disconnectDataClient();
}


//*************************************************************************************************************

void FiffSimulatorProducer::run()
{
    //
    // Connect data client
    //
    this->connectDataClient(m_pFiffSimulator->m_sFiffSimulatorIP);

    while(m_pRtDataClient->state() != QTcpSocket::ConnectedState)
    {
        msleep(100);
        this->connectDataClient(m_pFiffSimulator->m_sFiffSimulatorIP);
    }

    msleep(1000);

    m_bIsRunning = true;
    m_bFlagMeasuring = true;

    //
    // Inits
    //
    MatrixXf t_matRawBuffer;

    fiff_int_t kind;

    qint32 from = 0;
    qint32 to = -1;

    while(m_bIsRunning)
    {
        if(m_bFlagInfoRequest)
        {
            m_pFiffSimulator->rtServerMutex.lock();
            m_pFiffSimulator->m_pFiffInfo = m_pRtDataClient->readInfo();
            emit m_pFiffSimulator->fiffInfoAvailable();
            m_pFiffSimulator->rtServerMutex.unlock();

            producerMutex.lock();
            m_bFlagInfoRequest = false;
            producerMutex.unlock();
        }

        if(m_bFlagMeasuring)
        {
            m_pRtDataClient->readRawBuffer(m_pFiffSimulator->m_pFiffInfo->nchan, t_matRawBuffer, kind);

            if(kind == FIFF_DATA_BUFFER)
            {
                to += t_matRawBuffer.cols();
                from += t_matRawBuffer.cols();
                m_pFiffSimulator->m_pRawMatrixBuffer_In->push(&t_matRawBuffer);
                qDebug() << "Pushed Data" << t_matRawBuffer.rows() << "x" << t_matRawBuffer.cols();
            }
            else if(FIFF_DATA_BUFFER == FIFF_BLOCK_END)
                m_bFlagMeasuring = false;
        }
    }
}
