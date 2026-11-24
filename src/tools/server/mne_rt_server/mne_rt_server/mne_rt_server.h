//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne_rt_server.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Declaration of the MNERTServer Class.
 */

#ifndef MNE_RT_SERVER_H
#define MNE_RT_SERVER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectormanager.h"
#include "commandserver.h"
#include "fiffstreamserver.h"

#include <rtserver_connector/command_manager.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <condition_variable>
#include <mutex>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
/**
 * @brief mne_rt_server: the command port, the FIFF data port and the connectors.
 */
class MNERTServer
{
public:
    //=========================================================================================================
    /**
     * Sets up the commands and the connectors and starts both servers.
     */
    MNERTServer();

    MNERTServer(const MNERTServer&) = delete;
    MNERTServer& operator=(const MNERTServer&) = delete;

    //=========================================================================================================
    /**
     * Disconnects all clients and stops the connectors.
     */
    ~MNERTServer();

    //=========================================================================================================
    /**
     * @return The command manager of the server.
     */
    inline RTSERVER::CommandManager& getCommandManager();

    //=========================================================================================================
    /**
     * Inserts the commands of the server.
     */
    void init();

    //=========================================================================================================
    /**
     * Blocks until a client sends the "close" command.
     */
    void waitForClose();

private:
    void comClose();
    void comHelp(const RTSERVER::Command& p_command);

    RTSERVER::CommandManager m_commandManager; /**< The command manager of the mne_rt_server. */
    FiffStreamServer m_fiffStreamServer;       /**< Fiff stream server. */
    CommandServer m_commandServer;             /**< Command server. */
    ConnectorManager m_connectorManager;       /**< Connector manager. */

    std::mutex m_closeMutex;                  /**< Guards m_bClose. */
    std::condition_variable m_closeCondition; /**< Signals m_bClose. */
    bool m_bClose = false;                    /**< True once "close" was received. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline RTSERVER::CommandManager& MNERTServer::getCommandManager()
{
    return m_commandManager;
}

} // NAMESPACE

#endif // MNE_RT_SERVER_H
