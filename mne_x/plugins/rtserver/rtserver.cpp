//=============================================================================================================
/**
* @file     rtserver.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
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
* @brief    Contains the implementation of the RtServer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserver.h"
#include "rtserverproducer.h"

#include "FormFiles/rtserversetupwidget.h"
#include "FormFiles/rtserverrunwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QtCore/QFile>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RtServerPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtServer::RtServer()
: m_pRTMSA_RtServer(0)
, m_sRtServerClientAlias("mne-x")
, m_pRtCmdClient(NULL)
, m_pRtServerProducer(new RtServerProducer(this))
, m_sRtServerIP("127.0.0.1")//("172.21.16.88")//("127.0.0.1")
, m_bCmdClientIsConnected(false)
, m_iBufferSize(-1)
, m_pRawMatrixBuffer_In(NULL)
{
    m_PLG_ID = PLG_ID::RTSERVER;

    // Start RtServerProducer
    m_pRtServerProducer->start();


    //Convinience CMD connection timer --> ToDo get rid of that -> it interrupts acquistion when not connected
    connect(&m_cmdConnectionTimer, &QTimer::timeout, this, &RtServer::connectCmdClient);

    //init channels when fiff info is available
    connect(this, &RtServer::fiffInfoAvailable, this, &RtServer::init);

    //Start convinience timer
    m_cmdConnectionTimer.start(5000);

    //Try to connect the cmd client on start up using localhost connection
    this->connectCmdClient();
}


//*************************************************************************************************************

RtServer::~RtServer()
{
    if(m_pRtCmdClient)
        delete m_pRtCmdClient;

    if(m_pRawMatrixBuffer_In)
        delete m_pRawMatrixBuffer_In;
}


//*************************************************************************************************************

void RtServer::changeConnector(qint32 p_iNewConnectorId)
{
    if(p_iNewConnectorId != m_iActiveConnectorId)
    {
        // read meas info
        (*m_pRtCmdClient)["selcon"].pValues()[0].setValue(p_iNewConnectorId);
        (*m_pRtCmdClient)["selcon"].send();

        m_iActiveConnectorId = p_iNewConnectorId;

        // clear all and request everything new
        clear();

        //
        // request available commands
        //
        m_pRtCmdClient->requestCommands();

        //
        // Read Info
        //
        if(!m_pFiffInfo)
            requestInfo();

        //
        // Read Buffer Size
        //
        m_iBufferSize = m_pRtCmdClient->requestBufsize();

        emit cmdConnectionChanged(m_bCmdClientIsConnected);
    }
}


//*************************************************************************************************************

void RtServer::clear()
{
    m_pFiffInfo.reset();
    m_iBufferSize = -1;
}


//*************************************************************************************************************

void RtServer::connectCmdClient()
{
    if(!m_pRtCmdClient)
        m_pRtCmdClient = new RtCmdClient();
    else if(m_bCmdClientIsConnected)
        this->disconnectCmdClient();

    m_pRtCmdClient->connectToHost(m_sRtServerIP);
    m_pRtCmdClient->waitForConnected(1000);

    if(m_pRtCmdClient->state() == QTcpSocket::ConnectedState)
    {
        rtServerMutex.lock();

        //Stop convinience timer
        m_cmdConnectionTimer.stop();

        if(!m_bCmdClientIsConnected)
        {
            //
            // request available commands
            //
            m_pRtCmdClient->requestCommands();

            //
            // set cmd client is connected
            //
            m_bCmdClientIsConnected = true;

            //
            // Read Info
            //
            if(!m_pFiffInfo)
                requestInfo();

            //
            // Read Connectors
            //
            if(m_qMapConnectors.size() == 0)
                m_iActiveConnectorId = m_pRtCmdClient->requestConnectors(m_qMapConnectors);

            //
            // Read Buffer Size
            //
            m_iBufferSize = m_pRtCmdClient->requestBufsize();

            emit cmdConnectionChanged(m_bCmdClientIsConnected);
        }
        rtServerMutex.unlock();
    }
}


//*************************************************************************************************************

void RtServer::disconnectCmdClient()
{
    if(m_bCmdClientIsConnected)
    {
        m_pRtCmdClient->disconnectFromHost();
        m_pRtCmdClient->waitForDisconnected();
        rtServerMutex.lock();
        m_bCmdClientIsConnected = false;
        rtServerMutex.unlock();
        emit cmdConnectionChanged(m_bCmdClientIsConnected);
    }
}


//*************************************************************************************************************

void RtServer::requestInfo()
{
    if(m_pRtServerProducer->m_iDataClientId > -1 && m_bCmdClientIsConnected)
    {
        // read meas info
        (*m_pRtCmdClient)["measinfo"].pValues()[0].setValue(m_pRtServerProducer->m_iDataClientId);
        (*m_pRtCmdClient)["measinfo"].send();

        m_pRtServerProducer->producerMutex.lock();
        m_pRtServerProducer->m_bFlagInfoRequest = true;
        m_pRtServerProducer->producerMutex.unlock();
    }
    else
        qWarning() << "RtServerProducer is not connected!";
}


//*************************************************************************************************************

bool RtServer::start()
{
    if(m_bCmdClientIsConnected && m_pFiffInfo)
    {
        // Initialize real time measurements
        init();

        //Set buffer size
        (*m_pRtCmdClient)["bufsize"].pValues()[0].setValue(m_iBufferSize);
        (*m_pRtCmdClient)["bufsize"].send();

        // Buffer
        if(m_pRawMatrixBuffer_In)
            delete m_pRawMatrixBuffer_In;
        m_pRawMatrixBuffer_In = new RawMatrixBuffer(8,m_pFiffInfo->nchan,m_iBufferSize);

        // Start threads
        QThread::start();

        // Start Measurement at rt_Server
        // start measurement
        (*m_pRtCmdClient)["start"].pValues()[0].setValue(m_pRtServerProducer->m_iDataClientId);
        (*m_pRtCmdClient)["start"].send();

        m_pRtServerProducer->producerMutex.lock();
        m_pRtServerProducer->m_bFlagMeasuring = true;
        m_pRtServerProducer->producerMutex.unlock();
        m_pRtServerProducer->start();

        return true;
    }
    else
        return false;
}


//*************************************************************************************************************

bool RtServer::stop()
{
    if(m_bCmdClientIsConnected) //ToDo replace this with is running
    {
        // Stop Measurement at rt_Server
        (*m_pRtCmdClient)["stop-all"].send();

        m_pRtServerProducer->producerMutex.lock();
        m_pRtServerProducer->m_bFlagMeasuring = false;
        m_pRtServerProducer->producerMutex.unlock();
    }

    // Stop threads
    QThread::terminate();
    QThread::wait();

    //Clear Buffers

    return true;
}


//*************************************************************************************************************

Type RtServer::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

const char* RtServer::getName() const
{
    return "RT Server";
}


//*************************************************************************************************************

QWidget* RtServer::setupWidget()
{
    RtServerSetupWidget* widget = new RtServerSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    //init dialog

    return widget;
}


//*************************************************************************************************************

QWidget* RtServer::runWidget()
{
    RtServerRunWidget* widget = new RtServerRunWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return widget;
}


//*************************************************************************************************************
//=============================================================================================================
// Create measurement instances and config them
//=============================================================================================================

void RtServer::init()
{
    qDebug() << "RtServer::init()";

    if(m_pFiffInfo)
    {
        m_pRTMSA_RtServer = addProviderRealTimeMultiSampleArray_New(MSR_ID::MEGRTSERVER_OUTPUT);
        m_pRTMSA_RtServer->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSA_RtServer->setMultiArraySize(10);
        m_pRTMSA_RtServer->setVisibility(true);
    }
}


//*************************************************************************************************************

void RtServer::run()
{
    MatrixXf matValue;
    while(true)
    {
        matValue = m_pRawMatrixBuffer_In->pop();

        for(qint32 i = 0; i < matValue.cols(); ++i)
            m_pRTMSA_RtServer->setValue(matValue.col(i).cast<double>());

//        qDebug() << "matrix popped.";

//        m_pRTSA_ECG_I->setValue(dValue_I);
    }
}
