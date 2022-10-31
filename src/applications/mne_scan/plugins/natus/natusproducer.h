//=============================================================================================================
/**
 * @file     natusproducer.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the NatusProducer class.
 *
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
