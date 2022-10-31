//=============================================================================================================
/**
 * @file     connectormanager.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Felix Arndt, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief     Definition of the ConnectorManager Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectormanager.h"
#include "mne_rt_server.h"
#include "commandserver.h"
#include "fiffstreamserver.h"

#include "IConnector.h"

#include <communication/rtCommand/commandmanager.h>

#ifdef STATICBUILD
#include <../plugins/fiffsimulator/fiffsimulator.h>
#endif

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QCoreApplication>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;
using namespace COMMUNICATIONLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectorManager::ConnectorManager(FiffStreamServer* p_pFiffStreamServer, QObject *parent)
: QPluginLoader(parent)
, m_pFiffStreamServer(p_pFiffStreamServer)
{
}

//=============================================================================================================

ConnectorManager::~ConnectorManager()
{
    QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
    for( ; it != s_vecConnectors.end(); ++it)
        delete (*it);
}

//=============================================================================================================

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

//=============================================================================================================

void ConnectorManager::comConlist(Command p_command)
{
    bool t_bCommandIsJson = p_command.isJson();

    MNERTServer* t_pMNERTServer = qobject_cast<MNERTServer*> (this->parent());

    if(!t_bCommandIsJson)
        t_pMNERTServer->getCommandManager()["conlist"].reply(this->getConnectorList());
    else
        t_pMNERTServer->getCommandManager()["conlist"].reply(this->getConnectorList(true));
}

//=============================================================================================================

void ConnectorManager::comSelcon(Command p_command)
{
    bool t_bIsInt;

    qint32 t_id = p_command.pValues()[0].toInt(&t_bIsInt);
    if(t_bIsInt)
    {
//        qobject_cast<MNERTServer*> (this->parent())->getCommandManager()["selcon"].reply(this->setActiveConnector(t_id));
        QString t_sActivated = this->setActiveConnector(t_id);

        qDebug() << t_sActivated;

        bool t_bCommandIsJson = p_command.isJson();

        if(!t_bCommandIsJson)
            qobject_cast<MNERTServer*> (this->parent())->getCommandManager()["selcon"].reply(this->getConnectorList());
        else
            qobject_cast<MNERTServer*> (this->parent())->getCommandManager()["selcon"].reply(this->getConnectorList(true));
    }
}

//=============================================================================================================

void ConnectorManager::comStart(Command p_command)//comMeas
{
    getActiveConnector()->start();
    qobject_cast<MNERTServer*>(this->parent())->getCommandManager()["start"].reply("Starting active connector.\n");

    Q_UNUSED(p_command);
}

//=============================================================================================================

void ConnectorManager::comStopAll(Command p_command)
{
    getActiveConnector()->stop();
    qobject_cast<MNERTServer*>(this->parent())->getCommandManager()["stop-all"].reply("Stoping all connectors.\r\n");

    Q_UNUSED(p_command);
}

//=============================================================================================================

void ConnectorManager::connectActiveConnector()
{
    IConnector* t_activeConnector = ConnectorManager::getActiveConnector();

    if(t_activeConnector)
    {
        // use signal slots instead of call backs
        //Consulting the Signal/Slot documentation describes why the Signal/Slot approach is better:
        //    Callbacks have two fundamental flaws: Firstly, they are not type-safe. We can never be certain
        //    that the processing function will call the callback with the correct arguments.
        //    Secondly, the callback is strongly coupled to the processing function since the processing
        //    function must know which callback to call.
        //Do be aware of the following though:
        //    Compared to callbacks, signals and slots are slightly slower because of the increased
        //    flexibility they provide
        //The speed probably doesn't matter for most cases, but there may be some extreme cases of repeated
        //calling that makes a difference.

        //
        // Meas Info
        //
        // connect command server and connector manager
        QObject::connect(   this->m_pFiffStreamServer, &FiffStreamServer::requestMeasInfo,
                            t_activeConnector, &IConnector::info);

        // connect connector manager and fiff stream server
        QObject::connect(   t_activeConnector, &IConnector::remitMeasInfo,
                            this->m_pFiffStreamServer, &FiffStreamServer::forwardMeasInfo);

        //
        // Raw Data
        //
        // connect command server and connector manager

        // connect connector manager and fiff stream server
        QObject::connect(   t_activeConnector, &IConnector::remitRawBuffer,
                            this->m_pFiffStreamServer, &FiffStreamServer::forwardRawBuffer);
    }
    else
    {
        printf("Error: Can't connect, no connector active!\n");
    }
}

//=============================================================================================================

void ConnectorManager::disconnectActiveConnector()
{
    IConnector* t_activeConnector = ConnectorManager::getActiveConnector();

    if(t_activeConnector)
    {
        // use signal slots instead of call backs
        //Consulting the Signal/Slot documentation describes why the Signal/Slot approach is better:
        //    Callbacks have two fundamental flaws: Firstly, they are not type-safe. We can never be certain
        //    that the processing function will call the callback with the correct arguments.
        //    Secondly, the callback is strongly coupled to the processing function since the processing
        //    function must know which callback to call.
        //Do be aware of the following though:
        //    Compared to callbacks, signals and slots are slightly slower because of the increased
        //    flexibility they provide
        //The speed probably doesn't matter for most cases, but there may be some extreme cases of repeated
        //calling that makes a difference.

        //
        // Meas Info
        //
        this->disconnect(t_activeConnector);
        //
        t_activeConnector->disconnect(this->m_pFiffStreamServer);

        this->m_pFiffStreamServer->disconnect(t_activeConnector);

        //
        // Raw Data
        //
//        t_pMNERTServer->m_pCommandServer->disconnect(t_activeConnector);
        //
//        t_activeConnector->disconnect(t_pMNERTServer->m_pFiffStreamServer);
        // connect command server and connector manager
//        t_pMNERTServer->m_pCommandServer->disconnect(t_activeConnector);

        //
        // Reset Raw Buffer
        //
//        t_pMNERTServer->m_pCommandServer->disconnect(t_activeConnector);
    }
    else
    {
        printf("Error: Can't connect, no connector active!\n");
    }
}

//=============================================================================================================

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

//=============================================================================================================

QByteArray ConnectorManager::getConnectorList(bool p_bFlagJSON) const
{
    QByteArray t_blockConnectorList;

    if(p_bFlagJSON)
    {
        QJsonObject t_qJsonObjectConnectors;

        QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
        for( ; it != s_vecConnectors.end(); ++it)
        {
            QJsonObject t_qJsonObjectConnector;

            //insert id
            t_qJsonObjectConnector.insert(QString("id"), QJsonValue((*it)->getConnectorID()));

            //insert isActive
            t_qJsonObjectConnector.insert(QString("active"), QJsonValue((*it)->isActive()));

            //insert Connector JsonObject
            t_qJsonObjectConnectors.insert((*it)->getName(),t_qJsonObjectConnector);//QJsonObject());//QJsonValue());

        }

        QJsonObject t_qJsonObjectRoot;
        t_qJsonObjectRoot.insert("connectors", t_qJsonObjectConnectors);
        QJsonDocument p_qJsonDocument(t_qJsonObjectRoot);

        t_blockConnectorList.append(p_qJsonDocument.toJson());
    }
    else
    {
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
    }
    return t_blockConnectorList;
}

//=============================================================================================================

void ConnectorManager::connectCommands()
{
    //Connect slots
    MNERTServer* t_pMNERTServer = qobject_cast<MNERTServer*> (this->parent());

    QObject::connect(&t_pMNERTServer->getCommandManager()["conlist"], &Command::executed, this, &ConnectorManager::comConlist);
    QObject::connect(&t_pMNERTServer->getCommandManager()["selcon"], &Command::executed, this, &ConnectorManager::comSelcon);
    QObject::connect(&t_pMNERTServer->getCommandManager()["start"], &Command::executed, this, &ConnectorManager::comStart);
    QObject::connect(&t_pMNERTServer->getCommandManager()["stop-all"], &Command::executed, this, &ConnectorManager::comStopAll);
}

//=============================================================================================================

void ConnectorManager::loadConnectors(const QString& dir)
{
    clearConnectorActivation();

#ifdef STATICBUILD
    Q_UNUSED(dir)

    // In case of a static build we have to load plugins manually.
    const auto staticInstances = QPluginLoader::staticPlugins();
    QString sJSONFile;

    for(QStaticPlugin plugin : staticInstances) {
        // AbstractPlugin
        if(plugin.instance()) {
            if(IConnector* t_pIConnector = qobject_cast<IConnector*>(plugin.instance())) {
                t_pIConnector->setStatus(false);

                //Add the curent plugin meta data
                QJsonObject t_qJsonObjectMetaData = plugin.metaData().value("MetaData").toObject();
                t_pIConnector->setMetaData(t_qJsonObjectMetaData);
                QJsonDocument t_jsonDocumentOrigin(t_qJsonObjectMetaData);
                t_pIConnector->getCommandManager().insert(t_jsonDocumentOrigin);
                t_pIConnector->connectCommandManager();

                s_vecConnectors.push_back(t_pIConnector);
                qInfo() << "[ConnectorManager::loadConnectors] Loading " << t_pIConnector->getName() << "done";
            } else {
                qWarning() << "[ConnectorManager::loadConnectors] Loading plugin failed";
            }
        }
    }
#else
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
            IConnector* t_pIConnector = qobject_cast<IConnector*>(pConnector);
            t_pIConnector->setStatus(false);

            //Add the curent plugin meta data
            QJsonObject t_qJsonObjectMetaData = this->metaData().value("MetaData").toObject();
            t_pIConnector->setMetaData(t_qJsonObjectMetaData);
            QJsonDocument t_jsonDocumentOrigin(t_qJsonObjectMetaData);
            t_pIConnector->getCommandManager().insert(t_jsonDocumentOrigin);
            t_pIConnector->connectCommandManager();

            s_vecConnectors.push_back(t_pIConnector);

            qInfo() << "[ConnectorManager::loadConnectors] Loading " << fileName << "done";
        } else {
            qWarning() << "[ConnectorManager::loadConnectors] Loading plugin failed" << fileName << "failed";
        }
    }
#endif

    //
    // search config for default connector
    //
    qint32 configConnector = -1;
    QString configFileName("plugin.cfg");
    QFile configFile(QString("%1/resources/mne_rt_server/plugins/"+configFileName).arg(QCoreApplication::applicationDirPath()));
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
    printf("%s", getConnectorList().data());
}

//=============================================================================================================

QByteArray ConnectorManager::setActiveConnector(qint32 ID)
{
    QByteArray p_blockClientList;
    QString str;

    if(ID != getActiveConnector()->getConnectorID())
    {
        IConnector* t_pNewActiveConnector = NULL;
        QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
        for( ; it != s_vecConnectors.end(); ++it)
            if((*it)->getConnectorID() == ID)
                t_pNewActiveConnector = *it;

        if (t_pNewActiveConnector)
        {

           IConnector* t_pActiveConnector = getActiveConnector();

           //Stop and disconnect active connector
           t_pActiveConnector->stop();
           this->disconnectActiveConnector();
           t_pActiveConnector->setStatus(false);

           //set new active connector
           t_pNewActiveConnector->setStatus(true);
           this->connectActiveConnector();

            str = QString("\t%1 activated.\r\n\n").arg(t_pNewActiveConnector->getName());
            p_blockClientList.append(str);
        }
        else
        {
            str = QString("\tID %1 doesn't match a connector ID.\r\n\n").arg(ID);
            p_blockClientList.append(str);
            p_blockClientList.append(getConnectorList());
        }
    }
    else
    {
        str = QString("\t%1 is already active.\r\n\n").arg(getActiveConnector()->getName());
        p_blockClientList.append(str);
    }

    return p_blockClientList;
}

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

QVector<IConnector*>    ConnectorManager::  s_vecConnectors;
