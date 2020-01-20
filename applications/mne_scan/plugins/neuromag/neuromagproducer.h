//=============================================================================================================
/**
 * @file     neuromagproducer.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the NeuromagProducer class.
 *
 */

#ifndef NEUROMAGPRODUCER_H
#define NEUROMAGPRODUCER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <communication/rtClient/rtdataclient.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NEUROMAGPLUGIN
//=============================================================================================================

namespace NEUROMAGPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COMMUNICATIONLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Neuromag;


//=============================================================================================================
/**
 * DECLARE CLASS NeuromagProducer
 *
 * @brief The NeuromagProducer class provides a Rt Client data producer for a given sampling rate.
 */
class NeuromagProducer : public QThread
{
    Q_OBJECT

    friend class Neuromag;

public:
    //=========================================================================================================
    /**
    * Constructs a NeuromagProducer.
    *
    * @param [in] p_pNeuromag   a pointer to the corresponding Neuromag.
    */
    NeuromagProducer(Neuromag* p_pNeuromag);

    //=========================================================================================================
    /**
    * Destroys the NeuromagProducer.
    */
    ~NeuromagProducer();

    //=========================================================================================================
    /**
    * Connects the data client.
    *
    * @param[in] p_sRtSeverIP   real-time server ip
    */
    void connectDataClient(QString p_sRtSeverIP);

    //=========================================================================================================
    /**
    * Disconnects the data client.
    */
    void disconnectDataClient();

    //=========================================================================================================
    /**
    * Stops the NeuromagProducer by stopping the producer's thread.
    */
    void stop();

signals:
    //=========================================================================================================
    /**
    * Emitted when data clients connection status changed
    *
    * @param[in] p_bStatus  connection status
    */
    void dataConnectionChanged(bool p_bStatus);

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:

    QMutex m_mutex;

    Neuromag*   m_pNeuromag;    /**< Holds a pointer to corresponding Neuromag.*/
    bool        m_bIsRunning;       /**< Whether NeuromagProducer is running.*/

    QSharedPointer<RtDataClient> m_pRtDataClient;   /**< The data client.*/
    bool m_bDataClientIsConnected;                  /**< If the data client is connected.*/

    qint32 m_iDataClientId;

    //Acquisition flags
    bool m_bFlagInfoRequest;    /**< Read Fiff Info flag */
    bool m_bFlagMeasuring;      /**< Read Fiff raw Buffers */
};

} // NAMESPACE

#endif // NEUROMAGPRODUCER_H
