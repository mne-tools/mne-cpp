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

#include <com/rt_command/command_manager.h>
#include "connectormanager.h"
#include "commandserver.h"
#include "fiffstreamserver.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS MNERTServer
 *
 * @brief The MNERTServer class provides a Fiff data simulator.
 */
class MNERTServer : public QObject
{
    Q_OBJECT

public:
    MNERTServer();

    //=========================================================================================================
    /**
     * Destroys the MNERTServer.
     */
    ~MNERTServer();

    //=========================================================================================================
    /**
     * Returns the command manager
     */
    inline COMLIB::CommandManager& getCommandManager();

    //=========================================================================================================
    /**
     * Inits the mne_rt_server.
     */
    void init();

signals:
    void closeServer();

private:

    //SLOTS
    //=========================================================================================================
    /**
     * Closes mne_rt_server
     */
    void comClose();

    //=========================================================================================================
    /**
     * Is called when signal help is executed.
     */
    void comHelp(COMLIB::Command p_command);

    FiffStreamServer                    m_fiffStreamServer;     /**< Fiff stream server. */
    CommandServer                       m_commandServer;        /**< Command server. */

    ConnectorManager                    m_connectorManager;     /**< Connector manager. */

    COMLIB::CommandManager    m_commandManager;       /**< The command manager of the mne_rt_server. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline COMLIB::CommandManager& MNERTServer::getCommandManager()
{
    return m_commandManager;
}
} // NAMESPACE

#ifndef metatype_matrixxf
#define metatype_matrixxf
Q_DECLARE_METATYPE(Eigen::MatrixXf);    /**< Provides QT META type declaration of the Eigen::MatrixXf type. For signal/slot usage.*/
#endif

#endif // MNE_RT_SERVER_H
