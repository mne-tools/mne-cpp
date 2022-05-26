//=============================================================================================================
/**
 * @file     pluginconnectorconnection.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the PluginConnectorConnection class.
 *
 */
#ifndef PLUGINCONNECTORCONNECTION_H
#define PLUGINCONNECTORCONNECTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../scshared_global.h"

#include "../Plugins/abstractplugin.h"

#include "plugininputconnector.h"
#include "pluginoutputconnector.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QMetaObject>
#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=============================================================================================================
/**
 * Connector Data Type
 */
enum ConnectorDataType
{
    _N,         /**< Numeric */
    _RTMSA,     /**< Real-Time Multi Sample Array */
    _RTES,      /**< Real-Time Evoked Set */
    _RTC,       /**< Real-Time Covariance */
    _RTSE,      /**< Real-Time Source Estimate */
    _RTHR,      /**< Real-Time Hpi Result */
    _RTFS,      /**< Real-Time Forward Solution */
    _RTNR,      /**< Real-Time Neurofeedback Result */
    _None,      /**< None */
};

//=============================================================================================================
/**
 * Class implements plug-in connector connections.
 *
 * @brief The PluginConnectorConnection class holds connector connections
 */
class SCSHAREDSHARED_EXPORT PluginConnectorConnection : public QObject
{
    Q_OBJECT

    friend class PluginConnectorConnectionWidget;

public:
    typedef QSharedPointer<PluginConnectorConnection> SPtr;             /**< Shared pointer type for PluginConnectorConnection. */
    typedef QSharedPointer<const PluginConnectorConnection> ConstSPtr;  /**< Const shared pointer type for PluginConnectorConnection. */

    explicit PluginConnectorConnection(AbstractPlugin::SPtr sender, AbstractPlugin::SPtr receiver, QObject *parent = 0);
    
    //=========================================================================================================
    /**
     * Destructor
     */
    virtual ~PluginConnectorConnection();

    //=========================================================================================================
    /**
     * Disconnect connection
     */
    void clearConnection();

    //=========================================================================================================
    /**
     * Create connection
     */
    static inline QSharedPointer<PluginConnectorConnection> create(AbstractPlugin::SPtr sender, AbstractPlugin::SPtr receiver, QObject *parent = 0);

    static ConnectorDataType getDataType(QSharedPointer<PluginConnector> pPluginConnector);

    inline AbstractPlugin::SPtr& getSender();

    inline AbstractPlugin::SPtr& getReceiver();

    inline bool isConnected();

    //=========================================================================================================
    /**
     * The connector connection setup widget
     *
     * @return the setup widget.
     */
    QWidget* setupWidget();

signals:
    
private:
    //=========================================================================================================
    /**
     * Create connection
     */
    bool createConnection();

    AbstractPlugin::SPtr m_pSender;
    AbstractPlugin::SPtr m_pReceiver;

    QHash<QPair<QString, QString>, QMetaObject::Connection> m_qHashConnections; /**< QHash which holds the connections between sender and receiver QHash<QPair<Sender,Receiver>, Connection>. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QSharedPointer<PluginConnectorConnection> PluginConnectorConnection::create(AbstractPlugin::SPtr sender, AbstractPlugin::SPtr receiver, QObject *parent)
{
    QSharedPointer<PluginConnectorConnection> pPluginConnectorConnection(new PluginConnectorConnection(sender, receiver, parent));
    return pPluginConnectorConnection;
}

//=============================================================================================================

inline AbstractPlugin::SPtr& PluginConnectorConnection::getSender()
{
    return m_pSender;
}

//=============================================================================================================

inline AbstractPlugin::SPtr& PluginConnectorConnection::getReceiver()
{
    return m_pReceiver;
}

//=============================================================================================================

inline bool PluginConnectorConnection::isConnected()
{
    return m_qHashConnections.size() > 0 ? true : false;
}
} // NAMESPACE

#endif // PLUGINCONNECTORCONNECTION_H
