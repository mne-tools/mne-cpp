//=============================================================================================================
/**
 * @file     rtconnectivity.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
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
 * @brief     RtConnectivity class declaration.
 *
 */

#ifndef RTCONNECTIVITY_RTPROCESSING_H
#define RTCONNECTIVITY_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QThread>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

namespace CONNECTIVITYLIB {
    class ConnectivitySettings;
    class Network;
}

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
// CONNECTIVITYLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Real-time connectivity worker.
 *
 * @brief Real-time connectivity worker.
 */
class RTPROCESINGSHARED_EXPORT RtConnectivityWorker : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Perform actual connectivity estimation.
     *
     * @param[in] connectivitySettings           The connectivity settings to be used during connectivity estimation.
     */
    void doWork(const CONNECTIVITYLIB::ConnectivitySettings& connectivitySettings);

signals:
    void resultReady(const  QList<CONNECTIVITYLIB::Network>& connectivityResults, const CONNECTIVITYLIB::ConnectivitySettings& connectivitySettings);
};

//=============================================================================================================
/**
 * Real-time connectivity estimation.
 *
 * @brief Real-time connectivity estimation.
 */
class RTPROCESINGSHARED_EXPORT RtConnectivity : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtConnectivity> SPtr;             /**< Shared pointer type for RtConnectivity. */
    typedef QSharedPointer<const RtConnectivity> ConstSPtr;  /**< Const shared pointer type for RtConnectivity. */

    //=========================================================================================================
    /**
     * Creates the real-time connectivity estimation object.
     *
     * @param[in] parent     Parent QObject (optional).
     */
    explicit RtConnectivity(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the real-time connectivity estimation object.
     */
    ~RtConnectivity();

    //=========================================================================================================
    /**
     * Slot to receive incoming data.
     *
     * @param[in] data  Data to estimate the connectivity from.
     */
    void append(const CONNECTIVITYLIB::ConnectivitySettings& connectivitySettings);

    //=========================================================================================================
    /**
     * Restarts the thread by interrupting its computation queue, quitting, waiting and then starting it again.
     */
    void restart();

    //=========================================================================================================
    /**
     * Stops the thread by interrupting its computation queue, quitting and waiting.
     */
    void stop();

protected:
    QThread             m_workerThread;         /**< The worker thread. */

signals:
    void newConnectivityResultAvailable(const QList<CONNECTIVITYLIB::Network>& connectivityResults, const CONNECTIVITYLIB::ConnectivitySettings& connectivitySettings);

    void operate(const CONNECTIVITYLIB::ConnectivitySettings& connectivitySettings);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // RTCONNECTIVITY_RTPROCESSING_H
