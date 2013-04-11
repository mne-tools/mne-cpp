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
: m_sRtServerClientAlias("mne-x")
, m_pRtCmdClient(NULL)
, m_pRtServerProducer(new RtServerProducer(this))
, m_sRtServerIP("127.0.0.1")
, m_bCmdClientIsConnected(false)
{
    m_MDL_ID = MDL_ID::RTSERVER;

    //Try to connect the cmd client on start up using localhost connection
    this->connectCmdClient(m_sRtServerIP);

    // Start RtServerProducer
    m_pRtServerProducer->start();
}


//*************************************************************************************************************

RtServer::~RtServer()
{
    if(m_pRtCmdClient)
        delete m_pRtCmdClient;
}


//*************************************************************************************************************

void RtServer::connectCmdClient(QString p_sRtSeverIP)
{
    if(!m_pRtCmdClient)
        m_pRtCmdClient = new RtCmdClient();
    else if(m_bCmdClientIsConnected)
        this->disconnectCmdClient();

    m_pRtCmdClient->connectToHost(p_sRtSeverIP);
    m_pRtCmdClient->waitForConnected(1000);

    if(m_pRtCmdClient->state() == QTcpSocket::ConnectedState)
    {
        mutex.lock();
        m_sRtServerIP = p_sRtSeverIP;
        if(!m_bCmdClientIsConnected)
        {
            //
            // request available commands
            //
            m_pRtCmdClient->requestCommands();

            m_bCmdClientIsConnected = true;
            emit cmdConnectionChanged(m_bCmdClientIsConnected);
        }
        mutex.unlock();
    }
}


//*************************************************************************************************************

void RtServer::disconnectCmdClient()
{
    if(m_bCmdClientIsConnected)
    {
        m_pRtCmdClient->disconnectFromHost();
        m_pRtCmdClient->waitForDisconnected();
        mutex.lock();
        m_bCmdClientIsConnected = false;
        mutex.unlock();
        emit cmdConnectionChanged(m_bCmdClientIsConnected);
    }
}


//*************************************************************************************************************

void RtServer::requestInfo()
{
    if(m_pRtServerProducer->m_iDataClientId > -1)
    {
        qDebug() << "in reqeust Info";
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
    // Initialize real time measurements
    init();

    // Start threads
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool RtServer::stop()
{
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
}


//*************************************************************************************************************

void RtServer::run()
{
    while(true)
    {

    }
}
