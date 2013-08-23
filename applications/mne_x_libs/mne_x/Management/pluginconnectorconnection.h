//=============================================================================================================
/**
* @file     pluginconnectorconnection.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_x_global.h"

#include "plugininputconnector.h"
#include "pluginoutputconnector.h"
#include "plugininputconnector.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QObject>
#include <QMetaObject>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{


//=============================================================================================================
/**
* Class implements plug-in connector connections.
*
* @brief The PluginConnectorConnection class holds connector
*/
class MNE_X_SHARED_EXPORT PluginConnectorConnection : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<PluginConnectorConnection> SPtr;             /**< Shared pointer type for PluginConnectorConnection. */
    typedef QSharedPointer<const PluginConnectorConnection> ConstSPtr;  /**< Const shared pointer type for PluginConnectorConnection. */

    explicit PluginConnectorConnection(PluginOutputConnector::SPtr sender, PluginInputConnector::SPtr receiver, QObject *parent = 0);
    
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
    void createConnection(PluginOutputConnector::SPtr sender, PluginInputConnector::SPtr receiver);

    PluginOutputConnector::SPtr getSender();

    PluginInputConnector::SPtr getreceiver();


signals:
    
private:
    PluginOutputConnector::SPtr m_pSender;
    PluginInputConnector::SPtr m_pReceiver;

    QMetaObject::Connection m_con;
};

} // NAMESPACE

#endif // PLUGINCONNECTORCONNECTION_H
