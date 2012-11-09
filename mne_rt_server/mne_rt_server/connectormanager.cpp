//=============================================================================================================
/**
* @file     connectormanager.cpp
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
* @brief    Contains the implementation of the ConnectorManager Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectormanager.h"
#include "mne_rt_server.h"
#include "commandserver.h"
#include "fiffstreamserver.h"

#include "IConnector.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QTextStream>



#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MSERVER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectorManager::ConnectorManager(QObject *parent)
: QPluginLoader(parent)
{

}


//*************************************************************************************************************

ConnectorManager::~ConnectorManager()
{

}


//*************************************************************************************************************

void ConnectorManager::loadConnectors(const QString& dir)
{
    clearConnectorActivation();

    QDir ConnectorsDir(dir);

    printf("Loading connectors in directory... %s\n", ConnectorsDir.path().toUtf8().constData() );

    foreach(QString fileName, ConnectorsDir.entryList(QDir::Files))
    {
        if(fileName.compare("README") == 0 || fileName.compare("plugin.cfg") == 0)
            continue;

        this->setFileName(ConnectorsDir.absoluteFilePath(fileName));
        QObject *pConnector = this->instance();

        printf("\tLoading %s... ", fileName.toUtf8().constData() );

        // IModule
        if(pConnector)
        {
            s_vecConnectors.push_back(qobject_cast<IConnector*>(pConnector));
            printf("[done]\n");
        }
        else
        {
            printf("failed!\n");
        }
    }

    //
    // search config for default connector
    //
    qint32 configConnector = -1;
    QString configFileName("plugin.cfg");
    QFile configFile(dir+"/"+configFileName);
    if(!configFile.open(QIODevice::ReadOnly)) {
        printf("Not able to read config file... %s\n", configFile.fileName().toUtf8().constData());
    }
    else
    {
        printf("\tReading %s... ", configFileName.toUtf8().constData());

        QTextStream in(&configFile);
        QString line = in.readLine();
        QStringList list;
        while (!line.isNull()) {
            list = line.split(":");

            if(list[0].simplified().compare("defaultConnector") == 0)
            {
                configConnector = list[1].simplified().toInt();
                break;
            }
            line = in.readLine();
        }
    }
    if(s_vecConnectors.size() > 0)
    {

        bool activated = false;

        if( configConnector != -1)
        {
            for(qint32 i = 0; i < s_vecConnectors.size(); ++i)
            {
                if(s_vecConnectors[i]->getConnectorID() == configConnector)
                {
                    s_vecConnectors[i]->setStatus(true);
                    printf("activate %s... ", s_vecConnectors[i]->getName());
                    activated = true;
                    break;
                }
            }
        }

        printf("[done]\n");

        //default
        if(!activated)
            s_vecConnectors[0]->setStatus(true);
    }

    //print
    printf("Connector list\n");
    printf(getByteArrayConnectorList().data());
}


//*************************************************************************************************************

bool ConnectorManager::parseConnectorCommand(QStringList& p_qListCommand, QByteArray& p_blockOutputInfo)
{
    IConnector* t_pConnector = this->getActiveConnector();

    if(t_pConnector)
        return t_pConnector->parseCommand(p_qListCommand, p_blockOutputInfo);
    else
        return false;

}


//*************************************************************************************************************

bool ConnectorManager::startConnector()
{
//    qDebug() << "ModuleManager::startModules()";
//    // Start ISensor and IRTAlgorithm modules first!
//    bool bFlag = startSensorModules();
//    startRTAlgorithmModules();
//    startRTVisualizationModules();
//    startRTRecordModules();
//    startAlertModules();
//    //ToDo other Modules

    return true;//bFlag;
}


//*************************************************************************************************************

void ConnectorManager::stopConnector()
{
//    // Stop ISensor modules first!
//    qDebug() << "Try stopping sensor modules.";
//    QVector<IModule*>::const_iterator it = s_vecModules.begin();
//    for( ; it != s_vecModules.end(); ++it)
//    {
//        if((*it)->isActive())
//        {
//            if((*it)->getType() == _ISensor)
//            {
//                if(!(*it)->stop())
//                {
//                    qDebug() << "Could not stop IModule: " << (*it)->getName();
//                }
//            }
//        }
//    }

//    // Stop all other modules!
//    qDebug() << "Try stopping all other modules";
//    it = s_vecModules.begin();
//    for( ; it != s_vecModules.end(); ++it)
//    {
//        if((*it)->isActive())
//        {
//            if((*it)->getType() != _ISensor)
//            {
//                if(!(*it)->stop())
//                {
//                    qDebug() << "Could not stop IModule: " << (*it)->getName();
//                }
//            }
//        }
//    }
}


//*************************************************************************************************************

IConnector* ConnectorManager::getActiveConnector()
{
    QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
    for( ; it != s_vecConnectors.end(); ++it)
    {
        if((*it)->isActive())
            return *it;
    }

    return NULL;
}


//*************************************************************************************************************

//void ConnectorManager::getActiveMeasInfo(qint32 ID)
//{
//    IConnector* t_activeConnector = ConnectorManager::getActiveConnector();

//    if(t_activeConnector)
//    {
//        FiffInfo* t_FiffInfo = t_activeConnector->getMeasInfo();
//        emit sendMeasInfo(ID, t_FiffInfo);
//    }
//    else
//    {
//        printf("Error: Can't send measurement info, no connector active!\n");
//    }
//}


//*************************************************************************************************************

void ConnectorManager::connectActiveConnector()
{
    IConnector* t_activeConnector = ConnectorManager::getActiveConnector();

    if(t_activeConnector)
    {
        MNERTServer* t_pMNERTServer = qobject_cast<MNERTServer*>(this->parent());

        //
        // Meas Info
        //
        // connect command server and connector manager
        QObject::connect(   t_pMNERTServer->m_pCommandServer, &CommandServer::requestMeasInfo,
                            t_activeConnector, &IConnector::requestMeasInfo);
        // connect connector manager and fiff stream server
        QObject::connect(   t_activeConnector, &IConnector::remitMeasInfo,
                            t_pMNERTServer->m_pFiffStreamServer, &FiffStreamServer::forwardMeasInfo);
    }
    else
    {
        printf("Error: Can't send measurement info, no connector active!\n");
    }
}


//*************************************************************************************************************

QByteArray ConnectorManager::getByteArrayConnectorList() const
{
    QByteArray t_blockConnectorList;
    if(s_vecConnectors.size() > 0)
    {
        QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
        for( ; it != s_vecConnectors.end(); ++it)
        {
            if((*it)->isActive())
                t_blockConnectorList.append(QString("  *  (%1) %2\r\n").arg((*it)->getConnectorID()).arg((*it)->getName()));
            else
                t_blockConnectorList.append(QString("     (%1) %2\r\n").arg((*it)->getConnectorID()).arg((*it)->getName()));
        }
    }
    else
        t_blockConnectorList.append(" - no connector loaded - \r\n");

    t_blockConnectorList.append("\r\n");
    return t_blockConnectorList;
}


//*************************************************************************************************************

void ConnectorManager::clearConnectorActivation()
{
    // deactivate activated connectors
    if(s_vecConnectors.size() > 0)
    {
        QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
        for( ; it != s_vecConnectors.end(); ++it)
            if((*it)->isActive())
                (*it)->setStatus(false);
    }
}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

QVector<IConnector*>    ConnectorManager::  s_vecConnectors;
