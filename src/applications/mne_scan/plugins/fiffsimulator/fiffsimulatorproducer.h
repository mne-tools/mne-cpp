//=============================================================================================================
/**
 * @file     fiffsimulatorproducer.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
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
 * @brief    Contains the declaration of the FiffSimulatorProducer class.
 *
 */

#ifndef FIFFSIMULATORPRODUCER_H
#define FIFFSIMULATORPRODUCER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <communication/rtClient/rtdataclient.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE FIFFSIMULATORPLUGIN
//=============================================================================================================

namespace FIFFSIMULATORPLUGIN
{

//=============================================================================================================
// FIFFSIMULATORPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class FiffSimulator;

//=============================================================================================================
/**
 * DECLARE CLASS FiffSimulatorProducer
 *
 * @brief The FiffSimulatorProducer class provides a Fiff data producer for a given sampling rate.
 */
class FiffSimulatorProducer : public QThread
{
    Q_OBJECT

    friend class FiffSimulator;

public:
    //=========================================================================================================
    /**
     * Constructs a FiffSimulatorProducer.
     *
     * @param[in] p_pMneRtClient   a pointer to the corresponding MneRtClient.
     */
    FiffSimulatorProducer(FiffSimulator* p_pFiffSimulator);

    //=========================================================================================================
    /**
     * Destroys the FiffSimulatorProducer.
     */
    ~FiffSimulatorProducer();

    //=========================================================================================================
    /**
     * Connects the data client.
     *
     * @param[in] p_sRtSeverIP   real-time server ip.
     */
    void connectDataClient(QString p_sRtSeverIP);

    //=========================================================================================================
    /**
     * Disconnects the data client.
     */
    void disconnectDataClient();

    //=========================================================================================================
    /**
     * Stops the MneRtClientProducer by stopping the producer's thread.
     */
    void stop();

signals:
    //=========================================================================================================
    /**
     * Emitted when data clients connection status changed
     *
     * @param[in] p_bStatus  connection status.
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
    QMutex                  m_producerMutex;                        /**< The mutex to ensure thread safety.*/

    QSharedPointer<COMMUNICATIONLIB::RtDataClient> m_pRtDataClient; /**< The data client.*/

    FiffSimulator*          m_pFiffSimulator;                       /**< Holds a pointer to corresponding MneRtClient.*/

    bool                    m_bDataClientIsConnected;               /**< If the data client is connected.*/
    bool                    m_bFlagInfoRequest;                     /**< Read Fiff Info flag. */

    qint32                  m_iDataClientId;                        /**< The client id. */
    quint16                 m_iDefaultPortDataClient;               /**< The default port for the rt data client. */
};
} // NAMESPACE

#endif // FIFFSIMULATORPRODUCER_H
