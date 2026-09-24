//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     natusproducer.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2018
 * @brief    Contains the declaration of the NatusProducer class.
 */

#ifndef NATUSPRODUCER_H
#define NATUSPRODUCER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "natus_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QObject>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE NATUSPLUGIN
//=============================================================================================================

namespace NATUSPLUGIN
{

//=============================================================================================================
// NATUSPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * The NatusProducer class.
 *
 * @brief The NatusProducer class provides producer to receive data from the connected Natus amplifier and forward it to the main plugin class.
 */
class NATUSSHARED_EXPORT NatusProducer : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a NatusProducer which is a child of parent.
     *
     * @param[in] iBlockSize The block size to init the data matrix with.
     * @param[in] iChannelSize The channel size to init the data matrix with.
     * @param[in] parent pointer to parent widget.
     */
    explicit NatusProducer(int iBlockSize,
                           int iChannelSize,
                           QObject *parent = 0);

protected:
    //=========================================================================================================
    /**
     * Called whenever a new datagram was received.
     */
    void readPendingDatagrams();

    //=========================================================================================================
    /**
     * Parsed the received datagram.
     *
     * @param[in] datagram The received datagram.
     */
    void processDatagram(const QNetworkDatagram &datagram);

    QSharedPointer<QUdpSocket>          m_pUdpSocket;                   /**< A pointer to the UDP socket.*/
    Eigen::MatrixXd                     m_matData;                      /**< The data matrix storing the received data.*/

    int                                 m_iMatDataSampleIterator;       /**< The current iterator of the current data matrix.*/
    float                               m_fSampleFreq;                  /**< The current sample frequency.*/
    float                               m_fChannelSize;                 /**< The current channel size.*/

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever a new data matrix is available.
     *
     * @param[in] matData The newly parsed data.
     */
    void newDataAvailable(const Eigen::MatrixXd &matData);
};
} // NAMESPACE

#endif // NATUSPRODUCER_H
