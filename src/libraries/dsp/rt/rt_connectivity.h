//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_connectivity.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Real-time worker that re-estimates a connectivity network from incoming data epochs.
 *
 * RtConnectivity drives the CONNECTIVITYLIB connectivity estimators (coherence,
 * imaginary coherence, phase-locking value, weighted phase-lag index,
 * amplitude envelope correlation, …) from a background @c QThread. Each
 * arriving data block is forwarded to the worker, which updates the
 * windowed connectivity estimate and emits a @ref CONNECTIVITYLIB::Network result
 * carrying the current edge weights and node positions ready for
 * visualisation.
 *
 * The actual estimator and its parameters — frequency band, window length,
 * connectivity measure — are configured through a @ref CONNECTIVITYLIB::ConnectivitySettings
 * instance, so this header only adds the threading, lifecycle and signal
 * plumbing on top of the existing batch implementation.
 */

#ifndef RT_CONNECTIVITY_RTPROCESSING_H
#define RT_CONNECTIVITY_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"
#include <connectivity/network/network.h>

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
 * @brief Background worker thread that computes functional connectivity metrics in real time.
 */
class DSPSHARED_EXPORT RtConnectivityWorker : public QObject
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
 * @brief Controller that manages RtConnectivityWorker for online connectivity computation.
 */
class DSPSHARED_EXPORT RtConnectivity : public QObject
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

#endif // RT_CONNECTIVITY_RTPROCESSING_H
