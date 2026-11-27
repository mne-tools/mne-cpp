//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsourcestream.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Real-time source-estimate streaming controller that owns the data worker and the per-hemisphere interpolation workers.
 *
 * RtSourceStream bundles a @ref mne::viz::RtSourceDataWorker (queue,
 * averaging, sparse mat-vec per hemisphere, colormap packing) with
 * two @ref mne::viz::RtSourceInterpolationMatWorker instances (one per
 * hemisphere) that recompute the sparse vertex-to-source weight
 * matrix when surfaces, source vertices or the interpolation
 * function change.
 *
 * The GUI sees a single object: push source vectors with @c addData,
 * scrub / play / loop via the streaming setters, and receive per-vertex colour buffers
 * through setNewSmoothedDataAvailableCallback().
 */

#ifndef VIZ_RTSOURCESTREAM_H
#define VIZ_RTSOURCESTREAM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

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
class FsLabel;
}

namespace mne::viz
{
class PeriodicTask;
class RtSourceDataWorker;
class RtSourceInterpolationMatWorker;
} // namespace mne::viz

namespace mne::viz
{
//=============================================================================================================
/**
 * RtSourceStream orchestrates real-time source estimate streaming.
 * It manages a background data worker thread, an optional background
 * interpolation matrix worker thread, and a timer that drives the data flow.
 *
 * The stream provides a simple public API for:
 * - Pushing new source estimate data
 * - Setting interpolation matrices (LH/RH) directly or triggering
 *   on-the-fly recomputation when parameters change
 * - Configuring streaming parameters (speed, looping, averaging)
 * - Configuring visualization parameters (colormap, thresholds)
 * - Starting/stopping the streaming
 *
 * On-the-fly recomputation:
 *   - Set surface/source geometry via setInterpolationInfoLeft/Right()
 *   - Configure interpolation function and cancel distance via
 *     setInterpolationFunction() / setCancelDistance()
 *   - Call recomputeInterpolation() to trigger background computation
 *   - The new matrices are automatically forwarded to the data worker
 *
 * Usage:
 *   1. Create the stream
 *   2. Set interpolation matrices via setInterpolationMatrixLeft/Right()
 *   3. Set the result callbacks (set...Callback()) before streaming starts;
 *      they run on the streaming or interpolation thread
 *   4. Call addData() to push source estimate vectors
 *   5. Call setStreamingState(true) to start streaming
 *
 * @brief Controller for real-time source estimate streaming.
 */
class VIZSHARED_EXPORT RtSourceStream
{
public:
    //=========================================================================================================
    /**
     * Constructor. Creates the worker and background thread.
     *
     */
    RtSourceStream();

    //=========================================================================================================
    /**
     * Destructor. Stops the worker thread and cleans up.
     */
    virtual ~RtSourceStream();

    //=========================================================================================================
    /**
     * Add a source estimate data vector to the streaming queue.
     * The vector should contain source values for all sources (LH + RH concatenated).
     *
     * @param[in] data       Source activity vector (nSourcesLH + nSourcesRH).
     */
    void addData(const Eigen::VectorXd& data);

    //=========================================================================================================
    /**
     * Set the interpolation matrix for the left hemisphere.
     *
     * @param[in] mat        Sparse interpolation matrix (nVerticesLH x nSourcesLH).
     */
    void setInterpolationMatrixLeft(std::shared_ptr<Eigen::SparseMatrix<float>> mat);

    //=========================================================================================================
    /**
     * Set the interpolation matrix for the right hemisphere.
     *
     * @param[in] mat        Sparse interpolation matrix (nVerticesRH x nSourcesRH).
     */
    void setInterpolationMatrixRight(std::shared_ptr<Eigen::SparseMatrix<float>> mat);

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
    void setColormapType(const std::string& name);

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

    //=========================================================================================================
    /**
     * Set the base surface colors for both hemispheres.
     * Sub-threshold vertices will display these colors (e.g., curvature or
     * annotation coloring) instead of being transparent.
     *
     * @param[in] baseColorsLh   Per-vertex ABGR colors for the left hemisphere.
     * @param[in] baseColorsRh   Per-vertex ABGR colors for the right hemisphere.
     */
    void setSurfaceColor(const std::vector<uint32_t>& baseColorsLh,
                         const std::vector<uint32_t>& baseColorsRh);

    //=========================================================================================================
    /**
     * Toggle between emitting interpolated color data (smoothed) and raw
     * source values split by hemisphere.
     *
     * @param[in] bStreamSmoothedData    True for smoothed colors (default), false for raw.
     */
    void setStreamSmoothedData(bool bStreamSmoothedData);

    //=========================================================================================================
    // ── On-the-fly interpolation matrix computation ─────────────────────
    //=========================================================================================================

    /**
     * Set the interpolation function for on-the-fly matrix computation.
     *
     * @param[in] sInterpolationFunction    Function name ("linear", "gaussian",
     *                                      "square", "cubic").
     */
    void setInterpolationFunction(const std::string& sInterpolationFunction);

    //=========================================================================================================
    /**
     * Set the cancel distance for on-the-fly matrix computation.
     * Distances larger than this are ignored in the interpolation.
     *
     * @param[in] dCancelDist    Cancel distance in meters.
     */
    void setCancelDistance(double dCancelDist);

    //=========================================================================================================
    /**
     * Set the surface and source geometry for the left hemisphere.
     * Used for on-the-fly interpolation matrix computation.
     *
     * @param[in] matVertices          Vertex positions (nVerts x 3).
     * @param[in] vecNeighborVertices   Per-vertex neighbor index lists.
     * @param[in] vecSourceVertices     Source vertex indices into the surface.
     */
    void setInterpolationInfoLeft(const Eigen::MatrixX3f& matVertices,
                                  const std::vector<Eigen::VectorXi>& vecNeighborVertices,
                                  const Eigen::VectorXi& vecSourceVertices);

    //=========================================================================================================
    /**
     * Set the surface and source geometry for the right hemisphere.
     * Used for on-the-fly interpolation matrix computation.
     *
     * @param[in] matVertices          Vertex positions (nVerts x 3).
     * @param[in] vecNeighborVertices   Per-vertex neighbor index lists.
     * @param[in] vecSourceVertices     Source vertex indices into the surface.
     */
    void setInterpolationInfoRight(const Eigen::MatrixX3f& matVertices,
                                   const std::vector<Eigen::VectorXi>& vecNeighborVertices,
                                   const Eigen::VectorXi& vecSourceVertices);

    //=========================================================================================================
    /**
     * Trigger an asynchronous recomputation of the interpolation matrices.
     * The new matrices will be automatically forwarded to the data worker.
     */
    void recomputeInterpolation();

    //=========================================================================================================
    /**
     * Set the visualization type (interpolation-based or annotation-based).
     *
     * @param[in] iVisType    0 = InterpolationBased, 1 = AnnotationBased.
     */
    void setVisualizationType(int iVisType);

    //=========================================================================================================
    /**
     * Set annotation info for the left hemisphere.
     *
     * @param[in] vecLabelIds   Per-vertex label IDs.
     * @param[in] lLabels       FreeSurfer Labels.
     * @param[in] vecVertNo     Source vertex numbers.
     */
    void setAnnotationInfoLeft(const Eigen::VectorXi& vecLabelIds,
                               const std::vector<mne::FsLabel>& lLabels,
                               const Eigen::VectorXi& vecVertNo);

    //=========================================================================================================
    /**
     * Set annotation info for the right hemisphere.
     *
     * @param[in] vecLabelIds   Per-vertex label IDs.
     * @param[in] lLabels       FreeSurfer Labels.
     * @param[in] vecVertNo     Source vertex numbers.
     */
    void setAnnotationInfoRight(const Eigen::VectorXi& vecLabelIds,
                                const std::vector<mne::FsLabel>& lLabels,
                                const Eigen::VectorXi& vecVertNo);

public:
    //=========================================================================================================
    /**
     * Set the callback for newSmoothedDataAvailable. Called when new interpolated per-vertex color data is available for rendering. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (colorsLh, colorsRh); empty to drop the results.
     */
    void setNewSmoothedDataAvailableCallback(std::function<void(const std::vector<uint32_t>&, const std::vector<uint32_t>&)> callback);

    //=========================================================================================================
    /**
     * Set the callback for newRawDataAvailable. Called when raw (non-interpolated) data is available. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (dataLh, dataRh); empty to drop the results.
     */
    void setNewRawDataAvailableCallback(std::function<void(const Eigen::VectorXd&, const Eigen::VectorXd&)> callback);

    //=========================================================================================================
    /**
     * Set the callback for newInterpolationMatrixLeftAvailable. Called when a new left hemisphere interpolation matrix has been computed. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (interpMat); empty to drop the results.
     */
    void setNewInterpolationMatrixLeftAvailableCallback(std::function<void(std::shared_ptr<Eigen::SparseMatrix<float>>)> callback);

    //=========================================================================================================
    /**
     * Set the callback for newInterpolationMatrixRightAvailable. Called when a new right hemisphere interpolation matrix has been computed. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (interpMat); empty to drop the results.
     */
    void setNewInterpolationMatrixRightAvailableCallback(std::function<void(std::shared_ptr<Eigen::SparseMatrix<float>>)> callback);

private:
    void onNewInterpolationMatrixLeft(const std::shared_ptr<Eigen::SparseMatrix<float>>& interpMat);
    void onNewInterpolationMatrixRight(const std::shared_ptr<Eigen::SparseMatrix<float>>& interpMat);

private:
    std::unique_ptr<RtSourceDataWorker> m_pWorker; /**< Data streaming worker. */
    std::unique_ptr<PeriodicTask> m_pStreamTask;   /**< Streaming thread driving the cadence. */
    bool m_bIsStreaming = false;                   /**< Whether streaming is active. */
    int m_iTimeInterval = 17;                      /**< Streaming interval in ms (~60fps). */

    std::unique_ptr<mne::ThreadPool> m_pInterpPool;                  /**< Single thread running interpolation recomputations in order. */
    std::atomic<bool> m_bShuttingDown{false};                        /**< Set in the destructor; queued recomputations are skipped. */
    std::unique_ptr<RtSourceInterpolationMatWorker> m_pInterpWorker; /**< Interpolation matrix worker. */

    std::function<void(const std::vector<uint32_t>&, const std::vector<uint32_t>&)> m_newSmoothedDataAvailableCallback; /**< Receives newSmoothedDataAvailable. */
    std::function<void(const Eigen::VectorXd&, const Eigen::VectorXd&)> m_newRawDataAvailableCallback;                  /**< Receives newRawDataAvailable. */
    std::function<void(std::shared_ptr<Eigen::SparseMatrix<float>>)> m_newInterpolationMatrixLeftAvailableCallback;     /**< Receives newInterpolationMatrixLeftAvailable. */
    std::function<void(std::shared_ptr<Eigen::SparseMatrix<float>>)> m_newInterpolationMatrixRightAvailableCallback;    /**< Receives newInterpolationMatrixRightAvailable. */
};

} // namespace mne::viz

#endif // VIZ_RTSOURCESTREAM_H
