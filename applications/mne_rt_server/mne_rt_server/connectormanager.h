//=============================================================================================================
/**
 * @file     connectormanager.h
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
 * @brief     Declaration of the ConnectorManager Class.
 *
 */

#ifndef CONNECTORMANAGER_H
#define CONNECTORMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "IConnector.h"

//=============================================================================================================
// MNELIB INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QPluginLoader>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
// Function Pointers
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffStreamServer;

//=============================================================================================================
/**
 * DECLARE CLASS ConnectorManager
 *
 * @brief The ConnectorManager class provides a dynamic plugin loader. As well as the handling of the loaded plugins.
 */
class ConnectorManager : public QPluginLoader
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a ConnectorManager with the given parent.
     *
     * @param[in] parent pointer to parent Object. (It's normally the default value.).
     */
    ConnectorManager(FiffStreamServer* p_pFiffStreamServer, QObject* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the ConnectorManager.
     */
    virtual ~ConnectorManager();

    //=========================================================================================================
    /**
     * Connect connector manager to mne_rt_server commands
     */
    void connectCommands();

    //=========================================================================================================
    /**
     * Loads plugins from given directory.
     *
     * @param[in] dir the plugin directory.
     */
    void loadConnectors(const QString& dir);

    static void clearConnectorActivation();

    void connectActiveConnector();

    void disconnectActiveConnector();

    //=========================================================================================================
    /**
     * Returns vector containing active AbstractSensor plugins.
     *
     * @return reference to vector containing active AbstractSensor plugins.
     */
    IConnector* getActiveConnector();

    //=========================================================================================================
    /**
     * Returns vector containing all plugins.
     *
     * @return reference to vector containing all plugins.
     */
    static inline const QVector<IConnector*>& getConnectors();

    //=========================================================================================================
    /**
     * Prints a list of all connectors and their status
     *
     * @param[in] p_bFlagJSON    if true, function return JSON formatted (default = false).
     */
    QByteArray getConnectorList(bool p_bFlagJSON = false) const;

    //=========================================================================================================
    /**
     * ToDo
     */
    QByteArray setActiveConnector(qint32 ID);

signals:
    void sendMeasInfo(qint32, FIFFLIB::FiffInfo);
//    void setBufferSize(qint32 ID);
//    void startMeasConnector();
//    void stopMeasConnector();

private:

    //SLOTS
    //=========================================================================================================
    /**
     * Sends the connector list
     *
     * @param[in] p_command  The connector list command.
     */
    void comConlist(COMMUNICATIONLIB::Command p_command);

    //=========================================================================================================
    /**
     * Selects a connector
     *
     * @param[in] p_command  The select connector command.
     */
    void comSelcon(COMMUNICATIONLIB::Command p_command);

    //=========================================================================================================
    /**
     * Starts the Measurement
     *
     * @param[in] p_command  The start command.
     */
    void comStart(COMMUNICATIONLIB::Command p_command);//comMeas

    //=========================================================================================================
    /**
     * Stops all connectors
     *
     * @param[in] p_command  The stop all command.
     */
    void comStopAll(COMMUNICATIONLIB::Command p_command);

    static QVector<IConnector*> s_vecConnectors;       /**< Holds vector of all plugins. */

    FiffStreamServer* m_pFiffStreamServer;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QVector<IConnector*>& ConnectorManager::getConnectors()
{
    return s_vecConnectors;
}
} // NAMESPACE

#endif // CONNECTORMANAGER_H
