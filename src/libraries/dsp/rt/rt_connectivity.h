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
 * RtConnectivity drives the mne_connectivity connectivity estimators (coherence,
 * imaginary coherence, phase-locking value, weighted phase-lag index,
 * amplitude envelope correlation, …) from a background @c QThread. Each
 * arriving data block is forwarded to the worker, which updates the
 * windowed connectivity estimate and emits a @ref mne::Network result
 * carrying the current edge weights and node positions ready for
 * visualisation.
 *
 * The actual estimator and its parameters — frequency band, window length,
 * connectivity measure — are configured through a @ref mne::ConnectivitySettings
 * instance, so this header only adds the threading, lifecycle and signal
 * plumbing on top of the existing batch implementation.
 */

#ifndef RT_CONNECTIVITY_RTPROCESSING_H
#define RT_CONNECTIVITY_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"
#include <connectivity/connectivitysettings.h>
#include <connectivity/network/network.h>

#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QThread>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// mne FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Real-time connectivity worker.
 *
 * @brief Background worker thread that computes functional connectivity metrics in real time.
 *
 * @snippet ex_dsp_rt/main.cpp rt_connectivity_usage
 */
class DSPSHARED_EXPORT RtConnectivityWorker : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Perform actual connectivity estimation.
     *
     * @param[in] connectivitySettings    The trials to estimate from; their spectra are cached in place.
     */
    void doWork(const std::shared_ptr<mne::ConnectivitySettings>& connectivitySettings);

signals:
    void resultReady(const std::vector<mne::Network>& connectivityResults, const std::shared_ptr<mne::ConnectivitySettings>& connectivitySettings);
};

//=============================================================================================================
/**
 * Real-time connectivity estimation.
 *
 * @brief Controller that manages RtConnectivityWorker for online connectivity computation.
 *
 * @snippet ex_dsp_rt/main.cpp rt_connectivity_usage
 */
class DSPSHARED_EXPORT RtConnectivity : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Creates the real-time connectivity estimation object.
     *
     * @param[in] parent     Parent QObject (optional).
     */
    explicit RtConnectivity(QObject* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the real-time connectivity estimation object.
     */
    ~RtConnectivity();

    //=========================================================================================================
    /**
     * Slot to receive incoming data.
     *
     * Queues a connectivity estimate on the worker thread. The worker computes on its own
     * snapshot, so the caller may keep changing its settings; move them in to avoid the copy.
     *
     * @param[in] connectivitySettings  Data to estimate the connectivity from.
     */
    void append(mne::ConnectivitySettings connectivitySettings);

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
    QThread m_workerThread; /**< The worker thread. */

signals:
    /**
     * Emitted with the estimates and the settings they were computed from, including the
     * spectra cached during the computation. The worker no longer uses the settings, so a
     * receiver may move from them instead of copying.
     */
    void newConnectivityResultAvailable(const std::vector<mne::Network>& connectivityResults, const std::shared_ptr<mne::ConnectivitySettings>& connectivitySettings);

    void operate(const std::shared_ptr<mne::ConnectivitySettings>& connectivitySettings);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // RT_CONNECTIVITY_RTPROCESSING_H
