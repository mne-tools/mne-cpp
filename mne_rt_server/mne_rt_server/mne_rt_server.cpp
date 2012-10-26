//=============================================================================================================
/**
* @file     mne_rt_server.cpp
* @author   Christoph Dinh <christoph.dinh@live.de>;
* @version  1.0
* @date     October, 2010
*
* @section  LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief    Contains the implementation of the ModuleManager class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_rt_server.h"

#include "fiff_server.h"

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
{
    //
    // Run server
    //
    if (!server.listen()) {
        printf("Unable to start the mne_rt_server: %s\n", server.errorString().toUtf8().constData());
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

    printf("mne_rt_server is running on\n\nIP: %s\nport: %d\n\n",ipAddress.toUtf8().constData(), server.serverPort());

    //
    // Load Connectors
    //
    m_pConnectorManager = new ConnectorManager();
    m_pConnectorManager->loadConnectors(qApp->applicationDirPath()+connectorDir);

    m_pActiveConnector = m_pConnectorManager->getActiveConnector();
}


//*************************************************************************************************************

MNERTServer::~MNERTServer()
{
    if(m_pConnectorManager)
        delete m_pConnectorManager;
}
