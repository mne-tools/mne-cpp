//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsensorstream.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Real-time MEG / EEG sensor streaming controller that orchestrates the data and field-mapping workers.
 *
 * RtSensorStream spins up a background @ref mne::viz::RtSensorDataWorker (queue, averaging, dense mat-vec mapping,
 * ABGR packing) and an optional @ref mne::viz::RtSensorInterpolationMatWorker
 * (asynchronous recomputation of the dense mapping matrix when the
 * sensor configuration, projectors or target surface change) and
 * ties them together with a timer that paces the streaming.
 *
 * Once started, the stream hands ready-to-upload per-vertex colour
 * buffers to the callback set with setNewSensorColorsAvailableCallback().
 */

#ifndef VIZ_RTSENSORSTREAM_H
#define VIZ_RTSENSORSTREAM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_global.h"
#include <mne/fiff/fiff_evoked.h>
#include <mne/fiff/fiff_coord_trans.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class ThreadPool;
}

namespace mne::viz
{
class PeriodicTask;
class RtSensorDataWorker;
class RtSensorInterpolationMatWorker;
} // namespace mne::viz

namespace mne::viz
{
//=============================================================================================================
/**
 * RtSensorStream orchestrates real-time sensor data streaming.
 * It manages a background data worker thread, a background interpolation
 * matrix worker thread, and a timer that drives the data flow.
 *
 * Unlike the source-estimate controller (which uses sparse interpolation matrices
 * split by hemisphere), this controller uses a single dense mapping matrix
 * produced by mne::FwdFieldMap::computeMeg/EegMapping() that maps sensor measurements
 * directly to surface vertex values.
 *
 * The controller can either accept a precomputed mapping matrix via
 * setMappingMatrix(), or compute one on-the-fly in a background thread
 * by providing evoked data, surface geometry, and transforms via the
 * setInterpolationInfo() / setEvoked() / setTransform() methods.
 * When parameters change, recomputeMapping() triggers an asynchronous
 * recomputation; the new matrix is automatically forwarded to the data worker.
 *
 * Usage:
 *   1. Create the stream
 *   2. Either set mapping matrix via setMappingMatrix(), or configure
 *      interpolation parameters and call recomputeMapping()
 *   3. Set the result callbacks (set...Callback()) before streaming starts;
 *      they run on the streaming or interpolation thread
 *   4. Call addData() to push sensor measurement vectors
 *   5. Call setStreamingState(true) to start streaming
 *
 * @brief Controller for real-time sensor data streaming.
 */
class VIZSHARED_EXPORT RtSensorStream
{
public:
    //=========================================================================================================
    /**
     * Constructor. Creates the worker and background thread.
     *
     */
    RtSensorStream();

    //=========================================================================================================
    /**
     * Destructor. Stops the worker thread and cleans up.
     */
    virtual ~RtSensorStream();

    //=========================================================================================================
    /**
     * Add a sensor measurement vector to the streaming queue.
     * The vector should contain one value per picked sensor channel
     * (matching the mapping matrix column count).
     *
     * @param[in] data       Sensor measurement vector (nChannels x 1).
     */
    void addData(const Eigen::VectorXf& data);

    //=========================================================================================================
    /**
     * Set the dense mapping matrix (sensor → surface vertices).
     * Size: (nVertices × nChannels).
     *
     * @param[in] surfaceKey Key of the surface the matrix maps onto; colours are emitted for it.
     * @param[in] mat        Dense mapping matrix.
     */
    void setMappingMatrix(const std::string& surfaceKey, std::shared_ptr<Eigen::MatrixXf> mat);

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
    void setColormapType(const std::string& name);

    //=========================================================================================================
    /**
     * Set explicit normalization thresholds for sensor data.
     * Pass min == 0 and max == 0 to re-enable symmetric auto-normalization.
     *
     * @param[in] min        Lower threshold.
     * @param[in] max        Upper threshold.
     */
    void setThresholds(double min, double max);

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

    //=========================================================================================================
    /**
     * Toggle between emitting per-vertex color data (smoothed) and raw
     * sensor values.
     *
     * @param[in] bStreamSmoothedData    True for smoothed colors (default), false for raw.
     */
    void setStreamSmoothedData(bool bStreamSmoothedData);

    //=========================================================================================================
    // ── On-the-fly interpolation matrix computation ─────────────────────
    //=========================================================================================================

    /**
     * Set the evoked data that contains channel info and sensor definitions.
     * This configures the interpolation worker for on-the-fly recomputation.
     *
     * @param[in] evoked    The evoked dataset.
     */
    void setEvoked(const mne::FiffEvoked& evoked);

    //=========================================================================================================
    /**
     * Set the head-to-MRI coordinate transform for the interpolation worker.
     *
     * @param[in] trans              The transform.
     * @param[in] applySensorTrans   Whether to apply the transform.
     */
    void setTransform(const mne::FiffCoordTrans& trans, bool applySensorTrans);

    //=========================================================================================================
    /**
     * Set whether the MEG field should be mapped onto the head (BEM) surface
     * rather than the helmet surface.
     *
     * @param[in] onHead    True to map onto head.
     */
    void setMegFieldMapOnHead(bool onHead);

    //=========================================================================================================
    /**
     * Set the MEG target surface geometry for on-the-fly mapping.
     *
     * @param[in] surfaceKey    The key identifying the surface.
     * @param[in] vertices      Vertex positions (nVerts x 3).
     * @param[in] normals       Vertex normals (nVerts x 3).
     * @param[in] triangles     Triangle indices (nTris x 3).
     */
    void setMegSurface(const std::string& surfaceKey,
                       const Eigen::MatrixX3f& vertices,
                       const Eigen::MatrixX3f& normals,
                       const Eigen::MatrixX3i& triangles);

    //=========================================================================================================
    /**
     * Set the EEG target surface geometry for on-the-fly mapping.
     *
     * @param[in] surfaceKey    The key identifying the surface.
     * @param[in] vertices      Vertex positions (nVerts x 3).
     */
    void setEegSurface(const std::string& surfaceKey,
                       const Eigen::MatrixX3f& vertices);

    //=========================================================================================================
    /**
     * Set bad channels for the interpolation worker.
     *
     * @param[in] bads    List of bad channel names.
     */
    void setBadChannels(const std::vector<std::string>& bads);

    //=========================================================================================================
    /**
     * Trigger an asynchronous recomputation of the mapping matrix.
     * The new matrix will be automatically forwarded to the data worker
     * when ready.
     */
    void recomputeMapping();

public:
    //=========================================================================================================
    /**
     * Set the callback for newSensorColorsAvailable. Called when new per-vertex color data for a sensor surface is available. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (surfaceKey, colors); empty to drop the results.
     */
    void setNewSensorColorsAvailableCallback(std::function<void(const std::string&, const std::vector<uint32_t>&)> callback);

    //=========================================================================================================
    /**
     * Set the callback for newRawSensorDataAvailable. Called when raw (non-mapped) sensor data is available. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (data); empty to drop the results.
     */
    void setNewRawSensorDataAvailableCallback(std::function<void(const Eigen::VectorXf&)> callback);

    //=========================================================================================================
    /**
     * Set the callback for newMegMappingAvailable. Called when a new MEG mapping matrix has been computed by the background worker. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (surfaceKey, mappingMat, pick); empty to drop the results.
     */
    void setNewMegMappingAvailableCallback(std::function<void(const std::string&, std::shared_ptr<Eigen::MatrixXf>, const std::vector<int>&)> callback);

    //=========================================================================================================
    /**
     * Set the callback for newEegMappingAvailable. Called when a new EEG mapping matrix has been computed by the background worker. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (surfaceKey, mappingMat, pick); empty to drop the results.
     */
    void setNewEegMappingAvailableCallback(std::function<void(const std::string&, std::shared_ptr<Eigen::MatrixXf>, const std::vector<int>&)> callback);

private:
    void onNewMegMapping(const std::string& surfaceKey,
                         std::shared_ptr<Eigen::MatrixXf> mappingMat,
                         const std::vector<int>& pick);
    void onNewEegMapping(const std::string& surfaceKey,
                         std::shared_ptr<Eigen::MatrixXf> mappingMat,
                         const std::vector<int>& pick);

private:
    std::unique_ptr<RtSensorDataWorker> m_pWorker; /**< Data streaming worker. */
    std::unique_ptr<PeriodicTask> m_pStreamTask;   /**< Streaming thread driving the cadence. */
    bool m_bIsStreaming = false;                   /**< Whether streaming is active. */
    int m_iTimeInterval = 17;                      /**< Streaming interval in ms (~60fps). */

    std::unique_ptr<mne::ThreadPool> m_pInterpPool;                  /**< Single thread running interpolation recomputations in order. */
    std::atomic<bool> m_bShuttingDown{false};                        /**< Set in the destructor; queued recomputations are skipped. */
    std::unique_ptr<RtSensorInterpolationMatWorker> m_pInterpWorker; /**< Interpolation matrix worker. */

    std::function<void(const std::string&, const std::vector<uint32_t>&)> m_newSensorColorsAvailableCallback;                            /**< Receives newSensorColorsAvailable. */
    std::function<void(const Eigen::VectorXf&)> m_newRawSensorDataAvailableCallback;                                                     /**< Receives newRawSensorDataAvailable. */
    std::function<void(const std::string&, std::shared_ptr<Eigen::MatrixXf>, const std::vector<int>&)> m_newMegMappingAvailableCallback; /**< Receives newMegMappingAvailable. */
    std::function<void(const std::string&, std::shared_ptr<Eigen::MatrixXf>, const std::vector<int>&)> m_newEegMappingAvailableCallback; /**< Receives newEegMappingAvailable. */
};

} // namespace mne::viz

#endif // VIZ_RTSENSORSTREAM_H
