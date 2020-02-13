//=============================================================================================================
/**
 * @file     neuromagproducer.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     April, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the NeuromagProducer class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromagproducer.h"
#include "neuromag.h"

#include <communication/rtClient/rtcmdclient.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NEUROMAGPLUGIN;
using namespace IOBUFFER;
using namespace COMMUNICATIONLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

NeuromagProducer::NeuromagProducer(Neuromag* p_pNeuromag)
: m_pNeuromag(p_pNeuromag)
, m_pRtDataClient(0)
, m_bDataClientIsConnected(false)
, m_iDataClientId(-1)
, m_bFlagInfoRequest(false)
, m_bFlagMeasuring(false)
, m_bIsRunning(false)
{
}


//*************************************************************************************************************

NeuromagProducer::~NeuromagProducer()
{

}


//*************************************************************************************************************

void NeuromagProducer::connectDataClient(QString p_sRtSeverIP)
{
    if(m_pRtDataClient.isNull())
        m_pRtDataClient = QSharedPointer<RtDataClient>(new RtDataClient);
    else if(m_bDataClientIsConnected)
        return;

    m_pRtDataClient->connectToHost(p_sRtSeverIP);
    m_pRtDataClient->waitForConnected(1000);

    if(m_pRtDataClient->state() == QTcpSocket::ConnectedState)
    {
        m_mutex.lock();
        if(!m_bDataClientIsConnected)
        {
            //
            // get client ID
            //
            m_iDataClientId = m_pRtDataClient->getClientId();

            //
            // set data client alias -> for convinience (optional)
            //
            m_pRtDataClient->setClientAlias(m_pNeuromag->m_sNeuromagClientAlias); // used in option 2 later on

            //
            // set new state
            //
            m_bDataClientIsConnected = true;
            emit dataConnectionChanged(m_bDataClientIsConnected);
        }
        m_mutex.unlock();
    }
}


//*************************************************************************************************************

void NeuromagProducer::disconnectDataClient()
{
    if(m_bDataClientIsConnected)
    {
        m_pRtDataClient->disconnectFromHost();
        m_pRtDataClient->waitForDisconnected();
        m_mutex.lock();
        m_iDataClientId = -1;
        m_bDataClientIsConnected = false;
        m_mutex.unlock();
        emit dataConnectionChanged(m_bDataClientIsConnected);
    }
}


//*************************************************************************************************************

void NeuromagProducer::stop()
{
    //Wait until this thread (Producer) is stopped
    m_bIsRunning = false;
    m_bFlagMeasuring = false;

    if(m_pNeuromag->m_bCmdClientIsConnected) //ToDo replace this with is running
    {
        // Stop Measurement at rt_Server
        (*m_pNeuromag->m_pRtCmdClient)["stop-all"].send();
    }

    if(m_pNeuromag->m_pRawMatrixBuffer_In)
    {
        //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the push function (acquire statement)
        m_pNeuromag->m_pRawMatrixBuffer_In->releaseFromPush();
    }

    this->disconnectDataClient();

    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning())
//        QThread::wait();
}


//*************************************************************************************************************

void NeuromagProducer::run()
{
    m_bIsRunning = true;
    //
    // Connect data client
    //
    this->connectDataClient(m_pNeuromag->m_sNeuromagIP);

    qint32 count = 0;
    while(m_pRtDataClient->state() != QTcpSocket::ConnectedState)
    {
        msleep(100);
        this->connectDataClient(m_pNeuromag->m_sNeuromagIP);
        ++count;
        if(count > 10 || !m_bIsRunning)
            return;
    }

    msleep(1000);

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
            m_pNeuromag->m_mutex.lock();
            m_pNeuromag->m_pFiffInfo = m_pRtDataClient->readInfo();
            m_pNeuromag->m_mutex.unlock();

            m_mutex.lock();
            if(m_pNeuromag->m_pFiffInfo) {
                m_bFlagInfoRequest = false;
                emit m_pNeuromag->fiffInfoAvailable();
            }
            m_mutex.unlock();
        }

        if(m_bFlagMeasuring && !m_bFlagInfoRequest)
        {
            m_pRtDataClient->readRawBuffer(m_pNeuromag->m_pFiffInfo->nchan, t_matRawBuffer, kind);

            if(kind == FIFF_DATA_BUFFER)
            {
                to += t_matRawBuffer.cols();
                from += t_matRawBuffer.cols();

                m_pNeuromag->m_pRawMatrixBuffer_In->push(&t_matRawBuffer);
            }
            else if(FIFF_DATA_BUFFER == FIFF_BLOCK_END)
                m_bFlagMeasuring = false;
        }
    }
}
