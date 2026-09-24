//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiffsimulatorproducer.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the FiffSimulatorProducer class.
 */

#ifndef FIFFSIMULATORPRODUCER_H
#define FIFFSIMULATORPRODUCER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <com/rt_client/rt_data_client.h>

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
     * @param[in] p_pMneRtClient   a pointer to the corresponding MNERtClient.
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
     * Stops the MNERtClientProducer by stopping the producer's thread.
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

    QSharedPointer<COMLIB::RtDataClient> m_pRtDataClient; /**< The data client.*/

    FiffSimulator*          m_pFiffSimulator;                       /**< Holds a pointer to corresponding MNERtClient.*/

    bool                    m_bDataClientIsConnected;               /**< If the data client is connected.*/
    bool                    m_bFlagInfoRequest;                     /**< Read Fiff Info flag. */

    qint32                  m_iDataClientId;                        /**< The client id. */
    quint16                 m_iDefaultPortDataClient;               /**< The default port for the rt data client. */
};
} // NAMESPACE

#endif // FIFFSIMULATORPRODUCER_H
