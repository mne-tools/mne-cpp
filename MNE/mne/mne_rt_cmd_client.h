//=============================================================================================================
/**
* @file     mne_rt_cmd_client.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
*           To Be continued...
*
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
* @brief     declaration of the MNERtCmdClient Class.
*
*/

#ifndef MNE_RT_CMD_CLIENT_H
#define MNE_RT_CMD_CLIENT_H


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "mne_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTcpSocket>
#include <QDataStream>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{


//=============================================================================================================
/**
* The MNE real-time command client class provides an interface to communicate with the command port 4217 of a running mne_rt_server.
*
* @brief MNE real-time client
*/
class MNESHARED_EXPORT MNERtCmdClient : public QTcpSocket
{
    Q_OBJECT
public:
    typedef QSharedPointer<MNERtCmdClient> SPtr;            /**< Shared pointer type for MNERtCmdClient. */
    typedef QSharedPointer<const MNERtCmdClient> ConstSPtr; /**< Const shared pointer type for MNERtCmdClient. */

    //=========================================================================================================
    /**
    * Creates the real-time command client.
    *
    * @param[in] parent     Parent QObject (optional)
    */
    explicit MNERtCmdClient(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Connect to a mne_rt_server using port 4217
    *
    * @param[in] p_sRtServerHostname    The IP address of the mne_rt_server
    */
    void connectToHost(QString &p_sRtServerHostName);

    //=========================================================================================================
    /**
    * Sends a command to a connected mne_rt_server
    *
    * @param[in] p_sCommand    The command to send
    *
    * @return mne_rt_server reply
    */
    QString sendCommand(const QString &p_sCommand);

    //=========================================================================================================
    /**
    * Request measurement information to send it to a specified (id) client
    *
    * @param[in] p_id   ID of the client to send the measurement information to
    */
    void requestMeasInfo(qint32 p_id);

    //=========================================================================================================
    /**
    * Request measurement information to send it to a specified (alias) client
    *
    * @param[in] p_Alias    Alias of the client to send the measurement information to
    */
    void requestMeasInfo(const QString &p_Alias);

    //=========================================================================================================
    /**
    * Request measurement and send raw data to a specified (id) client
    *
    * @param[in] p_id   ID of the client to send the measurement to
    */
    void requestMeas(qint32 p_id);

    //=========================================================================================================
    /**
    * Request measurement and send raw data to a specified (alias) client
    *
    * @param[in] p_Alias   Alias of the client to send the measurement to
    */
    void requestMeas(QString p_Alias);

    //=========================================================================================================
    /**
    * stop data acquistion and sending measurements to all clients
    */
    void stopAll();

signals:
    
public slots:
    
};

} // NAMESPACE

#endif // MNE_RT_CMD_CLIENT_H
