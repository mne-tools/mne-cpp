//=============================================================================================================
/**
 * @file     mne_rt_server.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief     Definition of the MNERTServer Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_rt_server.h"

#include "IConnector.h"

#include <stdio.h>
#include <stdlib.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtNetwork>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;
using namespace COMMUNICATIONLIB;

const char* connectorDir = "/mne_rt_server_plugins";        /**< holds directory to connectors.*/

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNERTServer::MNERTServer()
: m_fiffStreamServer(this)
, m_commandServer(this)
, m_connectorManager(&m_fiffStreamServer, this)
{
    qRegisterMetaType<Eigen::MatrixXf>("MatrixXf");
    qRegisterMetaType<QSharedPointer<Eigen::MatrixXf> >("QSharedPointer<Eigen::MatrixXf>");

    //
    // init mne_rt_server
    //
    this->init();

    // connector manager
    m_connectorManager.connectCommands();

    // fiff stream server
    m_fiffStreamServer.connectCommands();

    // command manager
    m_commandServer.registerCommandManager(this->getCommandManager());

    // ### Load everything ###
    //
    // Load Connectors
    //
    m_connectorManager.loadConnectors(qApp->applicationDirPath()+connectorDir);

    // ### Connect everything ###
    //
    // Meas Info
    //
//    QObject::connect(   this->m_pFiffStreamServer, &FiffStreamServer::requestMeasInfo,
//                        this->m_pConnectorManager, &ConnectorManager::forwardMeasInfoRequest);

    m_connectorManager.connectActiveConnector();

    //Register Fiff Sream Server for command parsing
//    m_pCommandServer->registerCommandParser((ICommandParser*)m_pFiffStreamServer);//OLD
//    m_commandServer.registerCommandManager(m_fiffStreamServer.getCommandManager());//NEW

    //Register Command Managers of loaded connectors
    for(qint32 i = 0; i < m_connectorManager.getConnectors().size(); ++i)
        m_commandServer.registerCommandManager(m_connectorManager.getConnectors()[i]->getCommandManager());

    // ### Run everything ###
    //
    // Run instruction server
    //
    if (!m_commandServer.listen(QHostAddress::Any, 4217)) {
        printf("Unable to start the command server: %s\n", m_commandServer.errorString().toUtf8().constData());
        return;
    }
    //
    // Run data server
    //
    if (!m_fiffStreamServer.listen(QHostAddress::Any, 4218)) {
        printf("Unable to start the fiff stream server: %s\n", m_fiffStreamServer.errorString().toUtf8().constData());
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

    printf("mne_rt_server is running on\n\tIP:\t\t%s\n\tcommand port:\t%d\n\tfiff data port:\t%d\n\n",ipAddress.toUtf8().constData(), m_commandServer.serverPort(), m_fiffStreamServer.serverPort());
}

//=============================================================================================================

MNERTServer::~MNERTServer()
{
    qDebug() << "MNERTServer destructed";
}

//=============================================================================================================

void MNERTServer::comClose()
{
    emit closeServer();
}

//=============================================================================================================

void MNERTServer::comHelp(Command p_command)
{
    bool t_bCommandIsJson = p_command.isJson();
    //
    // Generate help
    //
    UTILSLIB::Subject::t_Observers::Iterator itObservers;

    //Map to store all commands & merging multi occurence
    QMap<QString, Command> t_qMapCommands;

    QMap<QString, Command>::ConstIterator itCommands;

    for(itObservers = m_commandServer.getCommandParser().observers().begin(); itObservers != m_commandServer.getCommandParser().observers().end(); ++itObservers)
    {
        CommandManager* t_pCommandManager = static_cast<CommandManager*> (*itObservers);

        for(itCommands = t_pCommandManager->commandMap().begin(); itCommands != t_pCommandManager->commandMap().end(); ++itCommands)
        {
            if(t_qMapCommands.keys().contains(itCommands.key()))
            {
                //ToDo merge
                qDebug() << "Merge has to be performed.";
            }
            else
                t_qMapCommands.insert(itCommands.key(), itCommands.value());
        }
    }

    //
    //create JSON help object
    //
    QJsonObject t_qJsonObjectCommands;

    for(itCommands = t_qMapCommands.begin(); itCommands != t_qMapCommands.end(); ++itCommands)
        t_qJsonObjectCommands.insert(itCommands.key(),QJsonValue(itCommands.value().toJsonObject()));
    QJsonObject t_qJsonObjectRoot;
    t_qJsonObjectRoot.insert("commands", t_qJsonObjectCommands);
    QJsonDocument p_qJsonDocument(t_qJsonObjectRoot);

    if(!t_bCommandIsJson)
    {
        //
        //create string formatted help and print
        //
        QString p_sOutput("");

        qint32 t_maxSize = 72;
        qint32 t_maxSizeCommand = 0;
        qint32 t_maxSizeParameters = 0;
        qint32 t_maxSizeDescriptions = 0;

        //get max sizes
        for(itCommands = t_qMapCommands.begin(); itCommands != t_qMapCommands.end(); ++itCommands)
        {
            QStringList t_sCommandList = itCommands.value().toStringList();

            if(t_sCommandList[0].size() > t_maxSizeCommand)
                t_maxSizeCommand = t_sCommandList[0].size();

            if(t_sCommandList[1].size() > t_maxSizeParameters)
                t_maxSizeParameters = t_sCommandList[1].size();
        }

        t_maxSizeDescriptions = t_maxSizeCommand + t_maxSizeParameters + 2;
        t_maxSizeDescriptions = t_maxSizeDescriptions < t_maxSize ? t_maxSize - t_maxSizeDescriptions : 20;

        //Format output
        for(itCommands = t_qMapCommands.begin(); itCommands != t_qMapCommands.end(); ++itCommands)
        {
            QStringList t_sCommandList = itCommands.value().toStringList();
            QString t_sCommand;
            qint32 i = 0;
            //Command
            t_sCommand.append(QString("\t%1 ").arg(t_sCommandList[0]));
            //Spaces
            for(i = 0; i < t_maxSizeCommand - t_sCommandList[0].size(); ++i)
                t_sCommand.append(QString(" "));
            //Parameters
            t_sCommand.append(QString("%1 ").arg(t_sCommandList[1]));
            //Spaces
            for(i = 0; i < t_maxSizeParameters - t_sCommandList[1].size(); ++i)
                t_sCommand.append(QString(" "));
            //Description
            qint32 lines = (int)ceil((double)t_sCommandList[2].size() / (double)t_maxSizeDescriptions);
            for(i = 0; i < lines; ++i)
            {
                t_sCommand.append(QString("%1").arg(t_sCommandList[2].mid(i * t_maxSizeDescriptions, t_maxSizeDescriptions)));
                t_sCommand.append(QString("\n\r"));
                //Spaces
                if(i < lines-1)
                {
                    t_sCommand.append(QString("\t"));
                    for(qint32 j = 0; j < t_maxSizeCommand + t_maxSizeParameters + 2; ++j)
                        t_sCommand.append(QString(" "));
                }
            }
            p_sOutput.append(t_sCommand);
        }
        m_commandManager["help"].reply(p_sOutput);
    }
    else
        m_commandManager["help"].reply(p_qJsonDocument.toJson());
}

//=============================================================================================================

void MNERTServer::init()
{
    //insert commands
//    //OPTION 1
//    QStringList t_qListParamNames;
//    QList<QVariant> t_qListParamValues;
//    QStringList t_qListParamDescription;

//    t_qListParamNames.append("id");
//    t_qListParamValues.append(QVariant(QVariant::String));
//    t_qListParamDescription.append("ID/Alias");

//    m_commandManager.insert("measinfo", Command("measinfo", "sends the measurement info to the specified FiffStreamClient.", t_qListParamNames, t_qListParamValues, t_qListParamDescription));
//    m_commandManager.insert("meas", Command("meas", "adds specified FiffStreamClient to raw data buffer receivers. If acquisition is not already strated, it is triggered.", t_qListParamNames, t_qListParamValues, t_qListParamDescription));
//    m_commandManager.insert("stop", Command("stop", "removes specified FiffStreamClient from raw data buffer receivers.", t_qListParamNames, t_qListParamValues, t_qListParamDescription));
//    t_qListParamNames.clear(); t_qListParamValues.clear();t_qListParamDescription.clear();
//    m_commandManager.insert(QString("stop-all"), QString("stops the whole acquisition process."));

//    m_commandManager.insert(QString("conlist"), QString("prints and sends all available connectors"));

//    t_qListParamNames.append("ConID");
//    t_qListParamValues.append(QVariant(QVariant::Int));
//    t_qListParamDescription.append("Connector ID");
//    m_commandManager.insert("\tselcon", Command("\tselcon", "selects a new connector, if a measurement is running it will be stopped.", t_qListParamNames, t_qListParamValues, t_qListParamDescription));

//    m_commandManager.insert(QString("help"), QString("prints and sends this list"));

//    m_commandManager.insert(QString("close"), QString("closes mne_rt_server"));

    //OPTION 2
    QString t_sJsonCommand =
            "{"
            "   \"commands\": {"
            "       \"clist\": {"
            "           \"description\": \"Prints and sends all available FiffStreamClients.\","
            "           \"parameters\": {}"
            "        },"
            "       \"close\": {"
            "           \"description\": \"Closes mne_rt_server.\","
            "           \"parameters\": {}"
            "        },"
            "       \"conlist\": {"
            "           \"description\": \"Prints and sends all available connectors.\","
            "           \"parameters\": {}"
            "        },"
            "       \"help\": {"
            "           \"description\": \"Prints and sends this list.\","
            "           \"parameters\": {}"
            "        },"
            "       \"measinfo\": {"
            "           \"description\": \"Sends the measurement info to the specified FiffStreamClient.\","
            "           \"parameters\": {"
            "               \"id\": {"
            "                   \"description\": \"ID/Alias\","
            "                   \"type\": \"QString\" "
            "               }"
            "           }"
            "       },"
            "       \"selcon\": {"
            "           \"description\": \"Selects a new connector, if a measurement is running it will be stopped.\","
            "           \"parameters\": {"
            "               \"ConID\": {"
            "                   \"description\": \"Connector ID\","
            "                   \"type\": \"int\" "
            "               }"
            "           }"
            "        },"
            "       \"start\": {"
            "           \"description\": \"Adds specified FiffStreamClient to raw data buffer receivers. If acquisition is not already started, it is triggered.\","
            "           \"parameters\": {"
            "               \"id\": {"
            "                   \"description\": \"ID/Alias\","
            "                   \"type\": \"QString\" "
            "               }"
            "           }"
            "        },"
            "       \"stop\": {"
            "           \"description\": \"Removes specified FiffStreamClient from raw data buffer receivers.\","
            "           \"parameters\": {"
            "               \"id\": {"
            "                   \"description\": \"ID/Alias\","
            "                   \"type\": \"QString\" "
            "               }"
            "           }"
            "        },"
            "       \"stop-all\": {"
            "           \"description\": \"Stops the whole acquisition process.\","
            "           \"parameters\": {}"
            "        }"
            "    }"
            "}";

    QJsonDocument t_jsonDocumentOrigin = QJsonDocument::fromJson(t_sJsonCommand.toUtf8());
    m_commandManager.insert(t_jsonDocumentOrigin);

    //connect slots
    QObject::connect(&m_commandManager["help"], &Command::executed, this, &MNERTServer::comHelp);
    QObject::connect(&m_commandManager["close"], &Command::executed, this, &MNERTServer::comClose);
}
