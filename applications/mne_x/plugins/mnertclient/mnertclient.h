//=============================================================================================================
/**
* @file     mnertclient.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
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
* @brief    Contains the declaration of the MneRtClient class.
*
*/

#ifndef MNERTCLIENT_H
#define MNERTCLIENT_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mnertclient_global.h"

#include <mne_x/Interfaces/ISensor.h>
#include <generics/circularbuffer_old.h>
#include <generics/circularmatrixbuffer.h>
#include <xMeas/Measurement/realtimemultisamplearray_new.h>


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <rtClient/rtcmdclient.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QVector>
#include <QTimer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MneRtClientPlugin
//=============================================================================================================

namespace MneRtClientPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace IOBuffer;
using namespace RTCLIENTLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MneRtClientProducer;
//class ECGChannel;


//=============================================================================================================
/**
* DECLARE CLASS MneRtClient
*
* @brief The MneRtClient class provides a RT server connection.
*/
class MNERTCLIENTSHARED_EXPORT MneRtClient : public ISensor
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "mnertclient.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::ISensor)

    friend class MneRtClientProducer;
    friend class MneRtClientSetupWidget;
    friend class MneRtClientRunWidget;

public:

    //=========================================================================================================
    /**
    * Constructs a MneRtClient.
    */
    MneRtClient();

    //=========================================================================================================
    /**
    * Destroys the MneRtClient.
    */
    virtual ~MneRtClient();

    //=========================================================================================================
    /**
    * Clears the rt server
    */
    void clear();

    virtual bool start();
    virtual bool stop();

    virtual Type getType() const;
    virtual const char* getName() const;

    virtual QWidget* setupWidget();
    virtual QWidget* runWidget();

//slots:
    //=========================================================================================================
    /**
    * Change connector
    *
    * @param[in] p_iNewConnectorId      new connector ID
    */
    void changeConnector(qint32 p_iNewConnectorId);

    //=========================================================================================================
    /**
    * Connects the cmd client.
    */
    void connectCmdClient();

    //=========================================================================================================
    /**
    * Disconnects the cmd client.
    */
    void disconnectCmdClient();

    //=========================================================================================================
    /**
    * Request FiffInfo using cmd client and producer (data client)
    */
    void requestInfo();

signals:
    //=========================================================================================================
    /**
    * Emitted when command clients connection status changed
    *
    * @param[in] p_bStatus  connection status
    */
    void cmdConnectionChanged(bool p_bStatus);

    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Initialise the MneRtClient.
    */
    void init();


    QMutex rtServerMutex;


    QString m_sMneRtClientClientAlias;     /**< The rt server client alias.*/

//    float           m_fSamplingRate;                /**< Holds the sampling rate.*/
//    int             m_iDownsamplingFactor;          /**< Holds the down sampling factor.*/

    RealTimeMultiSampleArrayNew*    m_pRTMSA_MneRtClient;      /**< Holds the RealTimeMultiSampleArray to provide the rt_server Channels.*/

    RtCmdClient*       m_pRtCmdClient;      /**< The command client.*/
    bool m_bCmdClientIsConnected;           /**< If the command client is connected.*/

    QString     m_sMneRtClientIP;              /**< The IP Adress of mne_rt_server.*/

    MneRtClientProducer*   m_pMneRtClientProducer;/**< Holds the MneRtClientProducer.*/


    QMap<qint32, QString> m_qMapConnectors; /**< Connector map.*/
    qint32 m_iActiveConnectorId;            /**< The active connector.*/

    FiffInfo::SPtr m_pFiffInfo;             /**< Fiff measurement info.*/
    qint32 m_iBufferSize;                   /**< The raw data buffer size.*/

    QTimer m_cmdConnectionTimer;            /**< Timer for convinient command client connection. When timer times out a connection is tried to be established. */


    RawMatrixBuffer*      m_pRawMatrixBuffer_In;         /**< Holds incoming raw data. */


};

} // NAMESPACE

#endif // MNERTCLIENT_H
