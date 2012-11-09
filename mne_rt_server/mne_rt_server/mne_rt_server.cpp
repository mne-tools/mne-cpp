//=============================================================================================================
/**
* @file     mne_rt_server.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the MNERTServer Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_rt_server.h"

#include "commandserver.h"
#include "fiffstreamserver.h"

#include "IConnector.h"
#include "connectormanager.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <stdio.h>
#include <stdlib.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtNetwork>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MSERVER;


const char* connectorDir = "/mne_rt_server_plugins";        /**< holds directory to connectors.*/



//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNERTServer::MNERTServer()
: m_pFiffStreamServer(new FiffStreamServer(this))
, m_pCommandServer(new CommandServer(this))
, m_pConnectorManager(new ConnectorManager(this))
{

    // ### Load everything ###
    //
    // Load Connectors
    //
    m_pConnectorManager->loadConnectors(qApp->applicationDirPath()+connectorDir);


    // ### Connect everything ###
    //
    // Meas Info
    //
    // connect command server and connector manager
    QObject::connect(   m_pCommandServer, &CommandServer::requestMeasInfo,
                        m_pConnectorManager, &ConnectorManager::getActiveMeasInfo);
    // connect connector manager and fiff stream server
    QObject::connect(   m_pConnectorManager, &ConnectorManager::sendMeasInfo,
                        m_pFiffStreamServer, &FiffStreamServer::forwardMeasInfo);

    //
    // Connect instruction and data server
    //
//    QObject::connect(   m_pCommandServer, &CommandServer::sendFiffStreamServerInstruction,
//                        m_pFiffStreamServer, &FiffStreamServer::readCommandServerInstruction);


    // ### Run everything ###
    //
    // Run instruction server
    //
    if (!m_pCommandServer->listen()) {
        printf("Unable to start the command server: %s\n", m_pCommandServer->errorString().toUtf8().constData());
        return;
    }
    //
    // Run data server
    //
    if (!m_pFiffStreamServer->listen()) {
        printf("Unable to start the fiff stream server: %s\n", m_pFiffStreamServer->errorString().toUtf8().constData());
        return;
    }

    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();

    printf("mne_rt_server is running on\n\tIP:\t\t%s\n\tcommand port:\t%d\n\tfiff data port:\t%d\n\n",ipAddress.toUtf8().constData(), m_pCommandServer->serverPort(), m_pFiffStreamServer->serverPort());
}


//*************************************************************************************************************

MNERTServer::~MNERTServer()
{
    if(m_pCommandServer)
        delete m_pCommandServer;
    if(m_pFiffStreamServer)
        delete m_pFiffStreamServer;
    if(m_pConnectorManager)
        delete m_pConnectorManager;
}
