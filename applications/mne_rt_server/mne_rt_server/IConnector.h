//=============================================================================================================
/**
 * @file     IConnector.h
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
 * @brief    The connector interface
 *
 */

#ifndef ICONNECTOR_H
#define ICONNECTOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>
#include <communication/rtCommand/commandmanager.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QtPlugin>
#include <QByteArray>
#include <QStringList>
#include <QJsonObject>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================

//=========================================================================================================
/**
 * Connector id enumeration. ToDo: add here IDs when creating a new connector
 */
enum ConnectorID
{
    _FIFFSIMULATOR = 1,                 /**< Connector id of the FIFF file simulator. */
    _default = -1                       /**< Default connector id. */
};

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define RAW_BUFFFER_SIZE  10

//=========================================================================================================
/**
 * The IConnector class is the interface class for all connectors.
 *
 * @brief The IConnector class is the interface class of all plugins.
 */
class IConnector : public QThread
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Creates the IConnector.
     */
    IConnector(): m_bIsActive(false) {};

    //=========================================================================================================
    /**
     * Destroys the IConnector.
     */
    virtual ~IConnector(){};

    //=========================================================================================================
    /**
     * Returns the CommandManager
     *
     * @return the CommandManager.
     */
    inline COMMUNICATIONLIB::CommandManager& getCommandManager();

    //=========================================================================================================
    /**
     * Connects the command manager to the available slots.
     */
    virtual void connectCommandManager() = 0;

    //=========================================================================================================
    /**
     * Returns the unique connector id
     * Pure virtual method.
     *
     * @return the connector ID.
     */
    virtual ConnectorID getConnectorID() const = 0;

    //=========================================================================================================
    /**
     * Returns the plugin name.
     * Pure virtual method.
     *
     * @return the name of plugin.
     */
    virtual const char* getName() const = 0;

    //=========================================================================================================
    /**
     * Returns the activation status of the plugin.
     *
     * @return true if plugin is activated.
     */
    inline bool isActive() const;

    //=========================================================================================================
    /**
     * Starts the IConnector.
     * Pure virtual method.
     *
     * @return true if successful, false otherwise.
     */
    virtual bool start() = 0;// = 0 call is not longer possible - it has to be reimplemented in child;

    //=========================================================================================================
    /**
     * Stops the AbstractPlugin.
     * Pure virtual method.
     *
     * @return true if success, false otherwise.
     */
    virtual bool stop() = 0;

    //=========================================================================================================
    /**
     * Sets itsmeta data of the plugin after it was laoded by the pluginmanager.
     *
     * @param[in] status the new activation status of the plugin.
     */
    inline void setMetaData(QJsonObject& p_MetaData);

    //=========================================================================================================
    /**
     * Sets the activation status of the plugin.
     *
     * @param[in] status    the new activation status of the plugin.
     */
    inline void setStatus(bool status);

    //=========================================================================================================
    /**
     * Request FiffInfo to be released.
     *
     * @param[in] ID    ID of the data client to send to. ToDo Remove this - do this processing somewhere else.
     */
    virtual void info(qint32 ID) = 0;

signals:
    void remitMeasInfo(qint32, FIFFLIB::FiffInfo);

    void remitRawBuffer(QSharedPointer<Eigen::MatrixXf>);

protected:

    //=========================================================================================================
    /**
     * The starting point for the thread. After calling start(), the newly created thread calls this function.
     * Returning from this method will end the execution of the thread.
     * Pure virtual method inherited by QThread
     */
    virtual void run() = 0;

    QJsonObject     m_qJsonObjectMetaData;  /**< The meta data of the plugin defined in Q_PLUGIN_METADATA and the corresponding json file. */

    COMMUNICATIONLIB::CommandManager  m_commandManager;       /**< The CommandManager of the connector. */

private:
    bool        m_bIsActive;                /**< Holds the activation status. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline COMMUNICATIONLIB::CommandManager& IConnector::getCommandManager()
{
    return m_commandManager;
}

//=============================================================================================================

inline bool IConnector::isActive() const
{
    return m_bIsActive;
}

//=============================================================================================================

inline void IConnector::setMetaData(QJsonObject& p_MetaData)
{
    m_qJsonObjectMetaData = p_MetaData;
}

//=============================================================================================================

inline void IConnector::setStatus(bool status)
{
    m_bIsActive = status;
    m_commandManager.setStatus(status);
}
} //Namespace

#ifndef IConnector_iid
#define IConnector_iid "mne_rt_server/1.0"
#endif
Q_DECLARE_INTERFACE(RTSERVER::IConnector, IConnector_iid)

#endif //ICONNECTOR_H
