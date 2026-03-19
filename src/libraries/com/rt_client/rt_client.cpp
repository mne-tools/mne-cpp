//=============================================================================================================
/**
 * @file     rtclient.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     Definition of the RtClient Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtclient.h"
#include "rtcmdclient.h"
#include "rtdataclient.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COMMUNICATIONLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtClient::RtClient(QString p_sRtServerHostname, QString p_sClientAlias, QObject *parent)
: QThread(parent)
, m_bIsConnected(false)
, m_bIsMeasuring(false)
, m_bIsRunning(false)
, m_sClientAlias(p_sClientAlias)
, m_sRtServerHostName(p_sRtServerHostname)
, m_iDefaultPort(4217)
{
}

//=============================================================================================================

RtClient::~RtClient()
{
    stop();
}

//=============================================================================================================

bool RtClient::getConnectionStatus()
{
    return m_bIsConnected;
}

//=============================================================================================================

bool RtClient::stop()
{
    m_bIsRunning = false;
    //QThread::wait();

    return true;
}

//=============================================================================================================

void RtClient::run()
{
    m_bIsRunning = true;

    //
    // Connect Clients
    //
    RtCmdClient t_cmdClient;
    t_cmdClient.connectToHost(m_sRtServerHostName,m_iDefaultPort);
    t_cmdClient.waitForConnected(1000);

    while(t_cmdClient.state() != QTcpSocket::ConnectedState)
    {
        msleep(100);
        t_cmdClient.connectToHost(m_sRtServerHostName,m_iDefaultPort);
        t_cmdClient.waitForConnected(1000);
    }

    RtDataClient t_dataClient;
    t_dataClient.connectToHost(m_sRtServerHostName,m_iDefaultPort);
    t_dataClient.waitForConnected();

    mutex.lock();
    m_bIsConnected = true;
    mutex.unlock();

    emit connectionChanged(m_bIsConnected);

    msleep(1000);

    //
    // get client ID
    //
    qint32 clientId = t_dataClient.getClientId();

    //
    // request available commands
    //
    t_cmdClient.requestCommands();

    //
    // Inits
    //
    MatrixXf matData;

    fiff_int_t kind;

    qint32 from = 0;
    qint32 to = -1;

    // set data client alias -> for convinience (optional)
    t_dataClient.setClientAlias(m_sClientAlias); // used in option 2 later on

//    // example commands
//    t_cmdClient["help"].send();
//    t_cmdClient.waitForDataAvailable(1000);
//    qDebug() << t_cmdClient.readAvailableData();
//    t_cmdClient["clist"].send();
//    t_cmdClient.waitForDataAvailable(1000);
//    qDebug() << t_cmdClient.readAvailableData();
//    t_cmdClient["conlist"].send();
//    t_cmdClient.waitForDataAvailable(1000);
//    qDebug() << t_cmdClient.readAvailableData();

    // read meas info
    t_cmdClient["measinfo"].pValues()[0].setValue(clientId);
    t_cmdClient["measinfo"].send();

    m_pFiffInfo = t_dataClient.readInfo();

    // start measurement
    t_cmdClient["start"].pValues()[0].setValue(clientId);
    t_cmdClient["start"].send();

    while(m_bIsRunning)
    {

//        while(m_bIsMeasuring)

        t_dataClient.readRawBuffer(m_pFiffInfo->nchan, matData, kind);

        if(kind == FIFF_DATA_BUFFER)
        {
            to += matData.cols();
            printf("Reading %d ... %d  =  %9.3f ... %9.3f secs...", from, to, ((float)from)/m_pFiffInfo->sfreq, ((float)to)/m_pFiffInfo->sfreq);
            from += matData.cols();

            emit rawBufferReceived(matData);
        }
        else if(FIFF_DATA_BUFFER == FIFF_BLOCK_END)
            m_bIsRunning = false;

        printf("[done]\n");
    }

    //
    // Disconnect Stuff
    //
    t_cmdClient.disconnectFromHost();
    t_dataClient.disconnectFromHost();

    mutex.lock();
    m_bIsConnected = false;
    mutex.unlock();

    emit connectionChanged(m_bIsConnected);
}
