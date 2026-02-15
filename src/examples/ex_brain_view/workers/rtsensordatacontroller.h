//=============================================================================================================
/**
 * @file     rtsensordatacontroller.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    RtSensorDataController class declaration.
 *
 */

#ifndef BRAINVIEW_RTSENSORDATACONTROLLER_H
#define BRAINVIEW_RTSENSORDATACONTROLLER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QThread;
class QTimer;

namespace BRAINVIEWLIB {
    class RtSensorDataWorker;
}

//=============================================================================================================
/**
 * RtSensorDataController orchestrates real-time sensor data streaming.
 * It manages a background worker thread and a timer that drives the data flow.
 *
 * Unlike the source-estimate controller (which uses sparse interpolation matrices
 * split by hemisphere), this controller uses a single dense mapping matrix
 * produced by FieldMap::computeMeg/EegMapping() that maps sensor measurements
 * directly to surface vertex values.
 *
 * Usage:
 *   1. Create controller
 *   2. Set mapping matrix via setMappingMatrix()
 *   3. Connect newSensorColorsAvailable() to your rendering update slot
 *   4. Call addData() to push sensor measurement vectors
 *   5. Call setStreamingState(true) to start streaming
 *
 * @brief Controller for real-time sensor data streaming.
 */
class RtSensorDataController : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor. Creates the worker and background thread.
     *
     * @param[in] parent     Parent QObject.
     */
    explicit RtSensorDataController(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor. Stops the worker thread and cleans up.
     */
    ~RtSensorDataController() override;

    //=========================================================================================================
    /**
     * Add a sensor measurement vector to the streaming queue.
     * The vector should contain one value per picked sensor channel
     * (matching the mapping matrix column count).
     *
     * @param[in] data       Sensor measurement vector (nChannels x 1).
     */
    void addData(const Eigen::VectorXf &data);

    //=========================================================================================================
    /**
     * Set the dense mapping matrix (sensor → surface vertices).
     * Size: (nVertices × nChannels).
     *
     * @param[in] mat        Dense mapping matrix.
     */
    void setMappingMatrix(QSharedPointer<Eigen::MatrixXf> mat);

    //=========================================================================================================
    /**
     * Start or stop the streaming.
     *
     * @param[in] state      True to start, false to stop.
     */
    void setStreamingState(bool state);

    //=========================================================================================================
    /**
     * Check if streaming is active.
     *
     * @return True if streaming is running.
     */
    bool isStreaming() const;

    //=========================================================================================================
    /**
     * Set the streaming interval (time between frames).
     *
     * @param[in] msec       Interval in milliseconds (default: 17ms ≈ 60fps).
     */
    void setTimeInterval(int msec);

    //=========================================================================================================
    /**
     * Set the number of samples to average before emitting.
     *
     * @param[in] numAvr     Number of averages (1 = no averaging).
     */
    void setNumberAverages(int numAvr);

    //=========================================================================================================
    /**
     * Set the colormap type.
     *
     * @param[in] name       Colormap name ("MNE", "Hot", "Jet", "Viridis", "Cool", "RedBlue").
     */
    void setColormapType(const QString &name);

    //=========================================================================================================
    /**
     * Enable or disable looping (replay data when queue is exhausted).
     *
     * @param[in] enabled    True to enable looping.
     */
    void setLoopState(bool enabled);

    //=========================================================================================================
    /**
     * Set the sampling frequency of the incoming data.
     *
     * @param[in] sFreq     Sampling frequency in Hz.
     */
    void setSFreq(double sFreq);

    //=========================================================================================================
    /**
     * Clear all queued data and reset the worker state.
     */
    void clearData();

signals:
    //=========================================================================================================
    /**
     * Emitted when new per-vertex color data for a sensor surface is available.
     *
     * @param[in] surfaceKey  Key identifying the target surface.
     * @param[in] colors      Per-vertex ABGR color array.
     */
    void newSensorColorsAvailable(const QString &surfaceKey,
                                  const QVector<uint32_t> &colors);

private:
    QThread *m_pWorkerThread = nullptr;                         /**< Background thread for the data worker. */
    BRAINVIEWLIB::RtSensorDataWorker *m_pWorker = nullptr;      /**< Data streaming worker. */
    QTimer *m_pTimer = nullptr;                                 /**< Timer driving the streaming cadence. */
    bool m_bIsStreaming = false;                                 /**< Whether streaming is active. */
    int m_iTimeInterval = 17;                                   /**< Streaming interval in ms (~60fps). */
};

#endif // BRAINVIEW_RTSENSORDATACONTROLLER_H
