//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     connectormanager.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Declaration of the ConnectorManager Class.
 */

#ifndef CONNECTORMANAGER_H
#define CONNECTORMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <rtserver_connector/iconnector.h>

#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffStreamServer;

//=============================================================================================================
/**
 * @brief Creates the known connectors, selects the active one and connects it to the FIFF stream server.
 */
class ConnectorManager
{
public:
    //=========================================================================================================
    /**
     * @param[in] p_pFiffStreamServer    The FIFF stream server the active connector feeds.
     * @param[in] p_commandManager       The command manager of mne_rt_server.
     */
    ConnectorManager(FiffStreamServer* p_pFiffStreamServer, RTSERVER::CommandManager& p_commandManager);

    ConnectorManager(const ConnectorManager&) = delete;
    ConnectorManager& operator=(const ConnectorManager&) = delete;

    ~ConnectorManager();

    //=========================================================================================================
    /**
     * Connects the connector commands of mne_rt_server to their handlers.
     */
    void connectCommands();

    //=========================================================================================================
    /**
     * Creates the known connectors and activates the default one of plugin.cfg, or the first one.
     */
    void loadConnectors();

    //=========================================================================================================
    /**
     * Deactivates all connectors.
     */
    void clearConnectorActivation();

    //=========================================================================================================
    /**
     * Connects the active connector to the FIFF stream server.
     */
    void connectActiveConnector();

    //=========================================================================================================
    /**
     * Disconnects the active connector from the FIFF stream server.
     */
    void disconnectActiveConnector();

    //=========================================================================================================
    /**
     * @return The active connector, nullptr if there is none.
     */
    IConnector* getActiveConnector();

    //=========================================================================================================
    /**
     * @return The connectors.
     */
    inline const std::vector<std::unique_ptr<IConnector>>& getConnectors() const;

    //=========================================================================================================
    /**
     * @param[in] p_bFlagJSON    JSON instead of text.
     *
     * @return The list of connectors.
     */
    std::string getConnectorList(bool p_bFlagJSON = false) const;

    //=========================================================================================================
    /**
     * Activates another connector; the active one is stopped.
     *
     * @param[in] ID     Connector ID.
     *
     * @return The report of the change.
     */
    std::string setActiveConnector(std::int32_t ID);

    //=========================================================================================================
    /**
     * Stops all connectors.
     */
    void stopConnectors();

private:
    void comConlist(const RTSERVER::Command& p_command);
    void comSelcon(const RTSERVER::Command& p_command);
    void comStart(const RTSERVER::Command& p_command);
    void comStopAll(const RTSERVER::Command& p_command);

    std::vector<std::unique_ptr<IConnector>> m_vecConnectors; /**< The connectors. */
    FiffStreamServer* m_pFiffStreamServer;                    /**< The FIFF stream server, not owned. */
    RTSERVER::CommandManager& m_commandManager;               /**< Commands of mne_rt_server. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const std::vector<std::unique_ptr<IConnector>>& ConnectorManager::getConnectors() const
{
    return m_vecConnectors;
}

} // NAMESPACE

#endif // CONNECTORMANAGER_H
