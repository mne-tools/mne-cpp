//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_rt_server.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the MNERTServer Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_rt_server.h"

#include <rtserver_connector/iconnector.h>

#include <mne/base/json.h>
#include <mne/base/log.h>
#include <mne/base/process.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <format>
#include <map>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNERTServer::MNERTServer()
: m_fiffStreamServer(m_commandManager)
, m_connectorManager(&m_fiffStreamServer, m_commandManager)
{
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
    m_connectorManager.loadConnectors();

    // ### Connect everything ###
    m_connectorManager.connectActiveConnector();

    //Register Command Managers of loaded connectors
    for (const auto& connector : m_connectorManager.getConnectors()) {
        m_commandServer.registerCommandManager(connector->getCommandManager());
    }

    // ### Run everything ###
    //
    // Run instruction server
    //
    std::string t_sError;
    if (!m_commandServer.listen(4217, t_sError)) {
        mne::Log::info("Unable to start the command server: {}", t_sError);
        return;
    }
    //
    // Run data server
    //
    if (!m_fiffStreamServer.listen(4218, t_sError)) {
        mne::Log::info("Unable to start the fiff stream server: {}", t_sError);
        return;
    }

    // Both servers listen on all interfaces
    mne::Log::info("mne_rt_server is running on\n\tHost:\t\t{} (all interfaces)\n\tcommand port:\t{}\n\tfiff data port:\t{}\n", mne::host_name(), m_commandServer.serverPort(), m_fiffStreamServer.serverPort());
}

//=============================================================================================================

MNERTServer::~MNERTServer()
{
    // No command runs and no buffer arrives while the members are destroyed
    m_commandServer.close();
    m_connectorManager.stopConnectors();
    m_fiffStreamServer.close();
    mne::Log::debug("MNERTServer destructed");
}

//=============================================================================================================

void MNERTServer::waitForClose()
{
    std::unique_lock lock(m_closeMutex);
    m_closeCondition.wait(lock, [this] { return m_bClose; });
}

//=============================================================================================================

void MNERTServer::comClose()
{
    {
        const std::scoped_lock lock(m_closeMutex);
        m_bClose = true;
    }
    m_closeCondition.notify_all();
}

//=============================================================================================================

void MNERTServer::comHelp(const Command& p_command)
{
    const bool t_bCommandIsJson = p_command.isJson();
    //
    // Generate help
    //
    //Map to store all commands & merging multi occurence
    std::map<std::string, Command> t_qMapCommands;

    for (RTSERVER::IObserver* observer : m_commandServer.getCommandParser().observers()) {
        auto* t_pCommandManager = static_cast<CommandManager*>(observer);

        for (const auto& [key, command] : t_pCommandManager->commandMap()) {
            if (t_qMapCommands.contains(key)) {
                //ToDo merge
                mne::Log::debug("Merge has to be performed.");
            } else {
                t_qMapCommands.emplace(key, command);
            }
        }
    }

    if (!t_bCommandIsJson) {
        //
        //create string formatted help and print
        //
        std::string p_sOutput;

        const std::size_t t_maxSize = 72;
        std::size_t t_maxSizeCommand = 0;
        std::size_t t_maxSizeParameters = 0;

        //get max sizes
        for (const auto& [key, command] : t_qMapCommands) {
            const mne::StringList t_sCommandList = command.toStringList();
            t_maxSizeCommand = std::max(t_maxSizeCommand, t_sCommandList[0].size());
            t_maxSizeParameters = std::max(t_maxSizeParameters, t_sCommandList[1].size());
        }

        std::size_t t_maxSizeDescriptions = t_maxSizeCommand + t_maxSizeParameters + 2;
        t_maxSizeDescriptions = t_maxSizeDescriptions < t_maxSize ? t_maxSize - t_maxSizeDescriptions : 20;

        //Format output
        for (const auto& [key, command] : t_qMapCommands) {
            const mne::StringList t_sCommandList = command.toStringList();
            //Command
            std::string t_sCommand = std::format("\t{} ", t_sCommandList[0]);
            //Spaces
            t_sCommand.append(t_maxSizeCommand - t_sCommandList[0].size(), ' ');
            //Parameters
            t_sCommand += std::format("{} ", t_sCommandList[1]);
            //Spaces
            t_sCommand.append(t_maxSizeParameters - t_sCommandList[1].size(), ' ');
            //Description
            const std::string& t_sDescription = t_sCommandList[2];
            const std::size_t lines = (t_sDescription.size() + t_maxSizeDescriptions - 1) / t_maxSizeDescriptions;
            for (std::size_t i = 0; i < lines; ++i) {
                t_sCommand += t_sDescription.substr(i * t_maxSizeDescriptions, t_maxSizeDescriptions);
                t_sCommand += "\n\r";
                //Spaces
                if (i + 1 < lines) {
                    t_sCommand += "\t";
                    t_sCommand.append(t_maxSizeCommand + t_maxSizeParameters + 2, ' ');
                }
            }
            p_sOutput += t_sCommand;
        }
        m_commandManager["help"].reply(p_sOutput);
    } else {
        //
        //create JSON help object
        //
        mne::JsonObject t_qJsonObjectCommands;
        for (const auto& [key, command] : t_qMapCommands) {
            t_qJsonObjectCommands.insert(key, command.toJsonObject());
        }
        mne::JsonObject t_qJsonObjectRoot;
        t_qJsonObjectRoot.insert("commands", t_qJsonObjectCommands);

        m_commandManager["help"].reply(mne::JsonDocument(t_qJsonObjectRoot).to_json());
    }
}

//=============================================================================================================

void MNERTServer::init()
{
    const char* t_sJsonCommand =
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

    m_commandManager.insert(mne::JsonDocument::from_json(t_sJsonCommand));

    //connect handlers
    m_commandManager["help"].connectExecuted([this](const Command& command) { comHelp(command); });
    m_commandManager["close"].connectExecuted([this](const Command& /*command*/) { comClose(); });
}
