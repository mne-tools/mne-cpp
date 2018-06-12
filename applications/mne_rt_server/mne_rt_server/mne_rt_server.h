//=============================================================================================================
/**
* @file     mne_rt_server.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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
* @brief     Declaration of the MNERTServer Class.
*
*/

#ifndef MNE_RT_SERVER_H
#define MNE_RT_SERVER_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <realtime/rtCommand/commandmanager.h>
#include "connectormanager.h"
#include "commandserver.h"
#include "fiffstreamserver.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QObject>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace REALTIMELIB;

//*************************************************************************************************************
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
    inline CommandManager& getCommandManager();

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
    void comHelp(Command p_command);



    FiffStreamServer    m_fiffStreamServer;     /**< Fiff stream server. */
    CommandServer       m_commandServer;        /**< Command server. */

    ConnectorManager    m_connectorManager;     /**< Connector manager. */

    CommandManager      m_commandManager;       /**< The command manager of the mne_rt_server. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline CommandManager& MNERTServer::getCommandManager()
{
    return m_commandManager;
}


} // NAMESPACE

#ifndef metatype_matrixxf
#define metatype_matrixxf
Q_DECLARE_METATYPE(Eigen::MatrixXf);    /**< Provides QT META type declaration of the Eigen::MatrixXf type. For signal/slot usage.*/
#endif

#endif // MNE_RT_SERVER_H
