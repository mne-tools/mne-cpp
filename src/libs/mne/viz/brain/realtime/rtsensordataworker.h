//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsensordataworker.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Background worker that turns queued sensor packets into per-vertex ABGR colour buffers via a dense mapping matrix.
 *
 * On every timer tick the worker pops the next packet (or averages
 * the queue), multiplies it by the dense (n_vertices x n_channels)
 * mapping matrix produced by @ref mne::viz::SensorFieldMapper, applies
 * symmetric normalisation around zero, maps through the active
 * MEG / EEG colormap and packs the result as ABGR uint32 ready for
 * direct upload into the target @ref mne::viz::BrainSurface colour slot.
 *
 * All public setters are mutex-protected so the controller can
 * swap the mapping matrix or change colormap mid-stream.
 */

#ifndef VIZ_RTSENSORDATAWORKER_H
#define VIZ_RTSENSORDATAWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * RtSensorDataWorker processes incoming sensor measurement data in a background thread.
 * It manages a data queue, performs averaging, applies a dense mapping matrix
 * (MEG or EEG sensor → surface vertex), normalises values symmetrically, and
 * converts them to per-vertex ABGR colours for rendering.
 *
 * This worker is driven by a timer in the RtSensorDataController and follows
 * the same architecture as RtSourceDataWorker, adapted for dense sensor-to-surface
 * mapping instead of sparse source interpolation.
 *
 * @brief Background worker for real-time sensor data streaming.
 */
class VIZSHARED_EXPORT RtSensorDataWorker
{
public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     */
    RtSensorDataWorker();

    //=========================================================================================================
    /**
     * Add a new sensor measurement vector to the streaming queue.
     * The vector contains one measurement per picked sensor channel.
     *
     * @param[in] data       Sensor measurement vector (nChannels x 1).
     */
    void addData(const Eigen::VectorXf& data);

    //=========================================================================================================
    /**
     * Clear all queued data and reset averaging state.
     */
    void clear();

    //=========================================================================================================
    /**
     * Set the dense mapping matrix (sensor → surface vertices).
     * This matrix is typically produced by mne::FwdFieldMap::computeMegMapping() or
     * computeEegMapping(). Size: (nVertices × nChannels).
     *
     * @param[in] surfaceKey Key of the surface the matrix maps onto; colours are emitted for it.
     * @param[in] mat        Dense mapping matrix.
     */
    void setMappingMatrix(const std::string& surfaceKey, std::shared_ptr<Eigen::MatrixXf> mat);

    //=========================================================================================================
    /**
     * Set the number of samples to average before emitting.
     *
     * @param[in] numAvr     Number of averages (1 = no averaging).
     */
    void setNumberAverages(int numAvr);

    //=========================================================================================================
    /**
     * Set the colormap type used for color conversion.
     *
     * @param[in] name       Colormap name ("MNE", "Hot", "Jet", "Viridis", "Cool", "RedBlue").
     */
    void setColormapType(const std::string& name);

    //=========================================================================================================
    /**
     * Set explicit normalization thresholds instead of auto-normalization.
     * When set, the symmetric auto-normalization is replaced by fixed thresholds.
     * Values below min are mapped to 0, above max to 1.
     *
     * Pass min == 0 and max == 0 to re-enable auto-normalization.
     *
     * @param[in] min        Lower threshold (values below are at colormap start).
     * @param[in] max        Upper threshold (values above are at colormap end).
     */
    void setThresholds(double min, double max);

    //=========================================================================================================
    /**
     * Enable or disable looping (replay from beginning when queue is exhausted).
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
     * Toggle between emitting per-vertex color data (smoothed) and raw
     * sensor values.
     *
     * When enabled (default), streamData() performs mapping + normalization +
     * color conversion and emits newRtSensorColors(). When disabled, it
     * emits newRtRawSensorData() with the raw measurement vector, which
     * can be used for GPU-side mapping.
     *
     * @param[in] bStreamSmoothedData    True for smoothed colors (default), false for raw.
     */
    void setStreamSmoothedData(bool bStreamSmoothedData);

public:
    //=========================================================================================================
    /**
     * Stream one frame of data. Called by the controller's timer.
     * Pops data from the queue, averages, maps to surface, normalises
     * symmetrically, and emits per-vertex colors.
     */
    void streamData();

public:
    //=========================================================================================================
    /**
     * Set the callback for newRtSensorColors. Called when new per-vertex color data is ready for rendering. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (surfaceKey, colors); empty to drop the results.
     */
    void setNewRtSensorColorsCallback(std::function<void(const std::string&, const std::vector<uint32_t>&)> callback);

    //=========================================================================================================
    /**
     * Set the callback for newRtRawSensorData. Called when raw (non-mapped) sensor data is available. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (data); empty to drop the results.
     */
    void setNewRtRawSensorDataCallback(std::function<void(const Eigen::VectorXf&)> callback);

private:
    //=========================================================================================================
    /**
     * Map sensor data to surface, normalise symmetrically, and convert to
     * per-vertex ABGR colors.
     *
     * @param[in] sensorData    Measurement vector (nChannels x 1).
     * @return Per-vertex ABGR color array (nVertices).
     */
    std::vector<uint32_t> computeSurfaceColors(const Eigen::VectorXf& sensorData) const;

    mutable std::mutex m_mutex; /**< Protects data members. */

    std::deque<Eigen::VectorXf> m_lDataQ;     /**< Incoming data queue. */
    std::deque<Eigen::VectorXf> m_lDataLoopQ; /**< Copy for looping. */
    Eigen::VectorXf m_vecAverage;             /**< Running average accumulator. */
    int m_iSampleCtr = 0;                     /**< Sample counter for averaging. */
    int m_iCurrentSample = 0;                 /**< Current sample index for loop replay. */

    std::shared_ptr<Eigen::MatrixXf> m_mappingMat; /**< Dense mapping matrix (nVerts x nChans). */
    std::string m_sSurfaceKey;                     /**< FsSurface key for the target surface. */

    int m_iNumAverages = 1;            /**< Number of samples to average. */
    bool m_bIsLooping = true;          /**< Whether to loop data. */
    bool m_bStreamSmoothedData = true; /**< Whether to stream smoothed colors (true) or raw data (false). */
    double m_dSFreq = 1000.0;          /**< Sampling frequency in Hz. */

    std::string m_sColormapType = "MNE"; /**< Active colormap name. */
    double m_dThreshMin = 0.0;           /**< Explicit min threshold (0 = auto). */
    double m_dThreshMax = 0.0;           /**< Explicit max threshold (0 = auto). */
    bool m_bUseAutoNorm = true;          /**< True = symmetric auto-norm (default). */

    std::function<void(const std::string&, const std::vector<uint32_t>&)> m_newRtSensorColorsCallback; /**< Receives newRtSensorColors. */
    std::function<void(const Eigen::VectorXf&)> m_newRtRawSensorDataCallback;                          /**< Receives newRtRawSensorData. */
};

} // namespace mne::viz

#endif // VIZ_RTSENSORDATAWORKER_H
