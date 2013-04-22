//=============================================================================================================
/**
* @file     rtserverproducer.cpp
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
* @brief    Contains the implementation of the RtServerProducer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserverproducer.h"
#include "rtserver.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RtServerPlugin;


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

RtServerProducer::RtServerProducer(RtServer* p_pRtServer)
: m_pRtServer(p_pRtServer)
, m_pRtDataClient(NULL)
, m_bDataClientIsConnected(false)
, m_iDataClientId(-1)
, m_bFlagInfoRequest(false)
, m_bFlagMeasuring(false)
{
}


//*************************************************************************************************************

RtServerProducer::~RtServerProducer()
{
    if(m_pRtDataClient)
        delete m_pRtDataClient;
}


//*************************************************************************************************************

void RtServerProducer::connectDataClient(QString p_sRtSeverIP)
{
    if(!m_pRtDataClient)
        m_pRtDataClient = new RtDataClient();
    else if(m_bDataClientIsConnected)
        this->disconnectDataClient();

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
            m_pRtDataClient->setClientAlias(m_pRtServer->m_sRtServerClientAlias); // used in option 2 later on

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

void RtServerProducer::disconnectDataClient()
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

void RtServerProducer::stop()
{
    m_bIsRunning = false;
    QThread::wait();
}


//*************************************************************************************************************

void RtServerProducer::run()
{
    //
    // Connect data client
    //
    this->connectDataClient(m_pRtServer->m_sRtServerIP);

    while(m_pRtDataClient->state() != QTcpSocket::ConnectedState)
    {
        msleep(100);
        this->connectDataClient(m_pRtServer->m_sRtServerIP);
    }

    msleep(1000);

    m_bIsRunning = true;

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
            m_pRtServer->rtServerMutex.lock();
            m_pRtServer->m_fiffInfo = m_pRtDataClient->readInfo();
            emit m_pRtServer->fiffInfoAvailable();
            m_pRtServer->rtServerMutex.unlock();

            producerMutex.lock();
            m_bFlagInfoRequest = false;
            producerMutex.unlock();
        }

        if(m_bFlagMeasuring)
        {
            m_pRtDataClient->readRawBuffer(m_pRtServer->m_fiffInfo.nchan, t_matRawBuffer, kind);

            if(kind == FIFF_DATA_BUFFER)
            {
                to += t_matRawBuffer.cols();
//                printf("Reading %d ... %d  =  %9.3f ... %9.3f secs...", from, to, ((float)from)/m_pRtServer->m_fiffInfo.sfreq, ((float)to)/m_pRtServer->m_fiffInfo.sfreq);
                from += t_matRawBuffer.cols();

                m_pRtServer->m_pRawMatrixBuffer_In->push(&t_matRawBuffer);
            }
            else if(FIFF_DATA_BUFFER == FIFF_BLOCK_END)
                m_bFlagMeasuring = false;

//            printf("[done]\n");
        }
    }
}
