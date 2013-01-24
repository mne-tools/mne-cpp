//=============================================================================================================
/**
* @file     rtdataclient.h
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
* @brief     declaration of the RtDataClient Class.
*
*/

#ifndef RTDATACLIENT_H
#define RTDATACLIENT_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "rtclient_global.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_tag.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTcpSocket>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTCLIENTLIB
//=============================================================================================================

namespace RTCLIENTLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//=============================================================================================================
/**
* The real-time data client class provides an interface to communicate with the data port 4218 of a running mne_rt_server.
*
* @brief Real-time data client
*/
class RTCLIENTSHARED_EXPORT RtDataClient : public QTcpSocket
{
    Q_OBJECT
public:
    typedef QSharedPointer<RtDataClient> SPtr;               /**< Shared pointer type for RtDataClient. */
    typedef QSharedPointer<const RtDataClient> ConstSPtr;    /**< Const shared pointer type for RtDataClient. */

    //=========================================================================================================
    /**
    * Creates the real-time data client.
    *
    * @param[in] parent     Parent QObject (optional)
    */
    explicit RtDataClient(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Connect to a mne_rt_server using port 4218
    *
    * @param[in] p_sRtServerHostName    The IP address of the mne_rt_server
    */
    void connectToHost(const QString& p_sRtServerHostName);

    //=========================================================================================================
    /**
    * Requests the ID at mne_rt_server and returns it
    *
    * @return the requested id
    */
    qint32 getClientId();

    //=========================================================================================================
    /**
    * Reads fiff measurement information of a data the connection
    *
    * @return the read fiff measurement information
    */
    FiffInfo readInfo();

    //=========================================================================================================
    /**
    * Reads fiff measurement information of a data the connection
    *
    * @param[in] p_nChannels    Number of channels to reshape the received data
    * @param[out] data          The read data - ToDo change this to raw buffer data object
    * @param[out] kind          Data kind
    */
    void readRawBuffer(qint32 p_nChannels, MatrixXf& data, fiff_int_t& kind);

    //=========================================================================================================
    /**
    * Sets the alias of the data client
    *
    * @param[in] p_sAlias    The alias of the data client
    */
    void setClientAlias(const QString &p_sAlias);

private:
    qint32 m_clientID;  /**< Corresponding client id of the data client at mne_rt_server */

signals:
    
public slots:
    
};

} // NAMESPACE

#endif // RTDATACLIENT_H
