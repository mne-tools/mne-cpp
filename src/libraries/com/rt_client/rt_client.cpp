//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rt_client.cpp
 * @since 2026
 * @date  April 2026
 * @brief Translation unit for @ref COMLIB::RtClient: the worker-thread loop that pulls raw buffers from @c mne_rt_server.
 *
 * Implements the initial handshake (alias registration on the command
 * port, client-id propagation to the data port, @c FiffInfo retrieval,
 * buffer-size negotiation) and the steady-state @c run() loop that
 * dequeues sample matrices from @ref RtDataClient and re-emits them on
 * @c rawBufferReceived. Stop/start state is coordinated through
 * @c m_bIsRunning and @c m_bIsConnected under @c mutex so the owning
 * thread can request shutdown without racing the network reader.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_client.h"
#include "rt_cmd_client.h"
#include "rt_data_client.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COMLIB;
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
            qInfo("Reading %d ... %d  =  %9.3f ... %9.3f secs...", from, to,
                  static_cast<float>(from)/m_pFiffInfo->sfreq,
                  static_cast<float>(to)/m_pFiffInfo->sfreq);
            from += matData.cols();

            emit rawBufferReceived(matData);
        }
        else if(FIFF_DATA_BUFFER == FIFF_BLOCK_END)
            m_bIsRunning = false;

        qInfo("[done]");
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
