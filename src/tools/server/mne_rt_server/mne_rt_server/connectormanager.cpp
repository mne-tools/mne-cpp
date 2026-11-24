//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     connectormanager.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the ConnectorManager Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectormanager.h"
#include "fiffstreamserver.h"

#include <fiffsimulator/fiffsimulator.h>

#include <mne/base/json.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <fstream>
#include <optional>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectorManager::ConnectorManager(FiffStreamServer* p_pFiffStreamServer, CommandManager& p_commandManager)
: m_pFiffStreamServer(p_pFiffStreamServer)
, m_commandManager(p_commandManager)
{
}

//=============================================================================================================

ConnectorManager::~ConnectorManager()
{
    stopConnectors();
}

//=============================================================================================================

void ConnectorManager::clearConnectorActivation()
{
    // deactivate activated connectors
    for (const auto& connector : m_vecConnectors) {
        if (connector->isActive()) {
            connector->setStatus(false);
        }
    }
}

//=============================================================================================================

void ConnectorManager::comConlist(const Command& p_command)
{
    m_commandManager["conlist"].reply(getConnectorList(p_command.isJson()));
}

//=============================================================================================================

void ConnectorManager::comSelcon(const Command& p_command)
{
    bool t_bIsInt = false;

    const std::int32_t t_id = p_command.pValues()[0].toInt(&t_bIsInt);
    if (t_bIsInt) {
        const std::string t_sActivated = this->setActiveConnector(t_id);

        mne::Log::debug("{}", t_sActivated);

        m_commandManager["selcon"].reply(getConnectorList(p_command.isJson()));
    }
}

//=============================================================================================================

void ConnectorManager::comStart(const Command& /*p_command*/)
{
    if (IConnector* t_pConnector = getActiveConnector()) {
        t_pConnector->start();
    }
    m_commandManager["start"].reply("Starting active connector.\n");
}

//=============================================================================================================

void ConnectorManager::comStopAll(const Command& /*p_command*/)
{
    if (IConnector* t_pConnector = getActiveConnector()) {
        t_pConnector->stop();
    }
    m_commandManager["stop-all"].reply("Stoping all connectors.\r\n");
}

//=============================================================================================================

void ConnectorManager::connectActiveConnector()
{
    IConnector* t_activeConnector = getActiveConnector();

    if (t_activeConnector) {
        FiffStreamServer* t_pServer = m_pFiffStreamServer;

        // Meas Info: the FIFF stream server requests it, the connector answers
        t_pServer->setMeasInfoRequestCallback([t_activeConnector](std::int32_t ID) { t_activeConnector->info(ID); });
        t_activeConnector->setMeasInfoCallback([t_pServer](std::int32_t ID, const mne::FiffInfo& info) { t_pServer->forwardMeasInfo(ID, info); });

        // Raw Data
        t_activeConnector->setRawBufferCallback([t_pServer](std::shared_ptr<const Eigen::MatrixXf> buffer) { t_pServer->forwardRawBuffer(buffer); });
    } else {
        mne::Log::info("Error: Can't connect, no connector active!");
    }
}

//=============================================================================================================

void ConnectorManager::disconnectActiveConnector()
{
    IConnector* t_activeConnector = getActiveConnector();

    if (t_activeConnector) {
        m_pFiffStreamServer->setMeasInfoRequestCallback({});
        t_activeConnector->setMeasInfoCallback({});
        t_activeConnector->setRawBufferCallback({});
    } else {
        mne::Log::info("Error: Can't connect, no connector active!");
    }
}

//=============================================================================================================

IConnector* ConnectorManager::getActiveConnector()
{
    for (const auto& connector : m_vecConnectors) {
        if (connector->isActive()) {
            return connector.get();
        }
    }

    return nullptr;
}

//=============================================================================================================

std::string ConnectorManager::getConnectorList(bool p_bFlagJSON) const
{
    std::string t_blockConnectorList;

    if (p_bFlagJSON) {
        mne::JsonObject t_qJsonObjectConnectors;

        for (const auto& connector : m_vecConnectors) {
            mne::JsonObject t_qJsonObjectConnector;

            //insert id
            t_qJsonObjectConnector.insert("id", static_cast<int>(connector->getConnectorID()));

            //insert isActive
            t_qJsonObjectConnector.insert("active", connector->isActive());

            //insert Connector JsonObject
            t_qJsonObjectConnectors.insert(connector->getName(), t_qJsonObjectConnector);
        }

        mne::JsonObject t_qJsonObjectRoot;
        t_qJsonObjectRoot.insert("connectors", t_qJsonObjectConnectors);

        t_blockConnectorList += mne::JsonDocument(t_qJsonObjectRoot).to_json();
    } else {
        if (!m_vecConnectors.empty()) {
            for (const auto& connector : m_vecConnectors) {
                t_blockConnectorList += std::format("{}({}) {}\r\n", connector->isActive() ? "  *  " : "     ", static_cast<int>(connector->getConnectorID()), connector->getName());
            }
        } else {
            t_blockConnectorList += " - no connector loaded - \r\n";
        }
        t_blockConnectorList += "\r\n";
    }
    return t_blockConnectorList;
}

//=============================================================================================================

void ConnectorManager::connectCommands()
{
    m_commandManager["conlist"].connectExecuted([this](const Command& command) { comConlist(command); });
    m_commandManager["selcon"].connectExecuted([this](const Command& command) { comSelcon(command); });
    m_commandManager["start"].connectExecuted([this](const Command& command) { comStart(command); });
    m_commandManager["stop-all"].connectExecuted([this](const Command& command) { comStopAll(command); });
}

//=============================================================================================================

void ConnectorManager::loadConnectors()
{
    clearConnectorActivation();

    // The known connectors, created directly
    std::vector<std::unique_ptr<IConnector>> t_connectors;
    t_connectors.push_back(std::make_unique<FIFFSIMULATORRTSERVERPLUGIN::FiffSimulator>());

    for (auto& t_pIConnector : t_connectors) {
        t_pIConnector->setStatus(false);

        //Add the connector meta data
        const mne::JsonDocument t_jsonDocumentOrigin = mne::JsonDocument::from_json(t_pIConnector->getMetaData());
        t_pIConnector->setMetaData(t_jsonDocumentOrigin.object());
        t_pIConnector->getCommandManager().insert(t_jsonDocumentOrigin);
        t_pIConnector->connectCommandManager();

        mne::Log::info("[ConnectorManager::loadConnectors] Loading {} done", t_pIConnector->getName());
        m_vecConnectors.push_back(std::move(t_pIConnector));
    }

    //
    // search config for default connector
    //
    std::int32_t configConnector = -1;
    const std::string configFileName("plugin.cfg");
    const std::string configFilePath = std::format("{}/../resources/mne_rt_server/plugins/{}", mne::utf8_from_path(mne::executable_dir()), configFileName);
    std::ifstream configFile(mne::path_from_utf8(configFilePath));
    if (!configFile) {
        mne::Log::info("Not able to read config file... {}", configFilePath);
    } else {
        mne::Log::info("\tReading {}...", configFileName);

        std::string line;
        while (std::getline(configFile, line)) {
            const mne::StringList list = mne::split(line, ':');

            if (std::string(mne::trimmed(list[0])) == "defaultConnector") {
                configConnector = list.size() > 1 ? mne::to_int(list[1]).value_or(0) : 0;
                break;
            }
        }
    }
    if (!m_vecConnectors.empty()) {
        bool activated = false;

        if (configConnector != -1) {
            for (const auto& connector : m_vecConnectors) {
                if (connector->getConnectorID() == configConnector) {
                    connector->setStatus(true);
                    mne::Log::info("activate {}... [done]", connector->getName());
                    activated = true;
                    break;
                }
            }
        }

        //default
        if (!activated) {
            m_vecConnectors[0]->setStatus(true);
        }
    }

    //print
    mne::Log::info("Connector list");
    mne::Log::info("{}", getConnectorList());
}

//=============================================================================================================

std::string ConnectorManager::setActiveConnector(std::int32_t ID)
{
    std::string p_blockClientList;
    IConnector* t_pActiveConnector = getActiveConnector();

    if (t_pActiveConnector == nullptr || ID != t_pActiveConnector->getConnectorID()) {
        IConnector* t_pNewActiveConnector = nullptr;
        for (const auto& connector : m_vecConnectors) {
            if (connector->getConnectorID() == ID) {
                t_pNewActiveConnector = connector.get();
            }
        }

        if (t_pNewActiveConnector) {
            //Stop and disconnect active connector
            if (t_pActiveConnector) {
                t_pActiveConnector->stop();
                this->disconnectActiveConnector();
                t_pActiveConnector->setStatus(false);
            }

            //set new active connector
            t_pNewActiveConnector->setStatus(true);
            this->connectActiveConnector();

            p_blockClientList += std::format("\t{} activated.\r\n\n", t_pNewActiveConnector->getName());
        } else {
            p_blockClientList += std::format("\tID {} doesn't match a connector ID.\r\n\n", ID);
            p_blockClientList += getConnectorList();
        }
    } else {
        p_blockClientList += std::format("\t{} is already active.\r\n\n", t_pActiveConnector->getName());
    }

    return p_blockClientList;
}

//=============================================================================================================

void ConnectorManager::stopConnectors()
{
    for (const auto& connector : m_vecConnectors) {
        connector->stop();
    }
}
