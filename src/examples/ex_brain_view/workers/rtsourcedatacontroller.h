//=============================================================================================================
/**
 * @file     rtsourcedatacontroller.h
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
 * @brief    RtSourceDataController class declaration.
 *
 */

#ifndef BRAINVIEW_RTSOURCEDATACONTROLLER_H
#define BRAINVIEW_RTSOURCEDATACONTROLLER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QThread;
class QTimer;

namespace BRAINVIEWLIB {
    class RtSourceDataWorker;
}

//=============================================================================================================
/**
 * RtSourceDataController orchestrates real-time source estimate streaming.
 * It manages a background worker thread and a timer that drives the data flow.
 *
 * The controller provides a simple public API for:
 * - Pushing new source estimate data
 * - Setting interpolation matrices (LH/RH)
 * - Configuring streaming parameters (speed, looping, averaging)
 * - Configuring visualization parameters (colormap, thresholds)
 * - Starting/stopping the streaming
 *
 * Usage:
 *   1. Create controller
 *   2. Set interpolation matrices via setInterpolationMatrixLeft/Right()
 *   3. Connect newSmoothedDataAvailable() to your rendering update slot
 *   4. Call addData() to push source estimate vectors
 *   5. Call setStreamingState(true) to start streaming
 *
 * @brief Controller for real-time source estimate streaming.
 */
class RtSourceDataController : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor. Creates the worker and background thread.
     *
     * @param[in] parent     Parent QObject.
     */
    explicit RtSourceDataController(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor. Stops the worker thread and cleans up.
     */
    ~RtSourceDataController() override;

    //=========================================================================================================
    /**
     * Add a source estimate data vector to the streaming queue.
     * The vector should contain source values for all sources (LH + RH concatenated).
     *
     * @param[in] data       Source activity vector (nSourcesLH + nSourcesRH).
     */
    void addData(const Eigen::VectorXd &data);

    //=========================================================================================================
    /**
     * Set the interpolation matrix for the left hemisphere.
     *
     * @param[in] mat        Sparse interpolation matrix (nVerticesLH x nSourcesLH).
     */
    void setInterpolationMatrixLeft(QSharedPointer<Eigen::SparseMatrix<float>> mat);

    //=========================================================================================================
    /**
     * Set the interpolation matrix for the right hemisphere.
     *
     * @param[in] mat        Sparse interpolation matrix (nVerticesRH x nSourcesRH).
     */
    void setInterpolationMatrixRight(QSharedPointer<Eigen::SparseMatrix<float>> mat);

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
     * @param[in] name       Colormap name ("Hot", "Jet", "Viridis", "Cool", "RedBlue").
     */
    void setColormapType(const QString &name);

    //=========================================================================================================
    /**
     * Set the normalization thresholds.
     *
     * @param[in] min        Lower threshold.
     * @param[in] mid        Mid-point threshold.
     * @param[in] max        Upper threshold.
     */
    void setThresholds(double min, double mid, double max);

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
     * Emitted when new interpolated per-vertex color data is available for rendering.
     *
     * @param[in] colorsLh   Per-vertex ABGR color array for the left hemisphere.
     * @param[in] colorsRh   Per-vertex ABGR color array for the right hemisphere.
     */
    void newSmoothedDataAvailable(const QVector<uint32_t> &colorsLh,
                                  const QVector<uint32_t> &colorsRh);

    //=========================================================================================================
    /**
     * Emitted when raw (non-interpolated) data is available.
     *
     * @param[in] dataLh     Source values for left hemisphere.
     * @param[in] dataRh     Source values for right hemisphere.
     */
    void newRawDataAvailable(const Eigen::VectorXd &dataLh,
                             const Eigen::VectorXd &dataRh);

private:
    QThread *m_pWorkerThread = nullptr;                     /**< Background thread for the data worker. */
    BRAINVIEWLIB::RtSourceDataWorker *m_pWorker = nullptr;  /**< Data streaming worker. */
    QTimer *m_pTimer = nullptr;                             /**< Timer driving the streaming cadence. */
    bool m_bIsStreaming = false;                             /**< Whether streaming is active. */
    int m_iTimeInterval = 17;                               /**< Streaming interval in ms (~60fps). */
};

#endif // BRAINVIEW_RTSOURCEDATACONTROLLER_H
