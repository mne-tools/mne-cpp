//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsourcedataworker.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Background worker that converts streaming source vectors into smoothed per-vertex ABGR colour buffers.
 *
 * Holds the sparse interpolation matrices for the left and right
 * hemispheres, the active colormap and the (fmin, fmid, fmax)
 * normalisation thresholds. On every timer tick the queued source
 * vector is split per hemisphere, multiplied by its sparse
 * matrix, mapped through the colormap and packed as ABGR ready for
 * direct upload &mdash; the only per-frame work is one sparse
 * mat-vec plus a tight colour-packing loop.
 */

#ifndef VIZ_RTSOURCEDATAWORKER_H
#define VIZ_RTSOURCEDATAWORKER_H

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
 * RtSourceDataWorker processes incoming source estimate data in a background thread.
 * It manages a data queue, performs averaging, applies interpolation matrices, and
 * converts source values to per-vertex ABGR colors for rendering.
 *
 * This worker is driven by a timer in the RtSourceDataController.
 *
 * @brief Background worker for real-time source estimate streaming.
 */
class VIZSHARED_EXPORT RtSourceDataWorker
{
public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     */
    RtSourceDataWorker();

    //=========================================================================================================
    /**
     * Add a new source estimate data vector to the streaming queue.
     * The vector contains source values for all sources (LH + RH concatenated).
     *
     * @param[in] data       Source activity vector (nSources x 1).
     */
    void addData(const Eigen::VectorXd& data);

    //=========================================================================================================
    /**
     * Clear all queued data.
     */
    void clear();

    //=========================================================================================================
    /**
     * Set the interpolation matrix for the left hemisphere.
     *
     * @param[in] mat        Sparse interpolation matrix (nVertices x nSourcesLH).
     */
    void setInterpolationMatrixLeft(std::shared_ptr<Eigen::SparseMatrix<float>> mat);

    //=========================================================================================================
    /**
     * Set the interpolation matrix for the right hemisphere.
     *
     * @param[in] mat        Sparse interpolation matrix (nVertices x nSourcesRH).
     */
    void setInterpolationMatrixRight(std::shared_ptr<Eigen::SparseMatrix<float>> mat);

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
     * @param[in] name       Colormap name ("Hot", "Jet", "Viridis", "Cool", "RedBlue").
     */
    void setColormapType(const std::string& name);

    //=========================================================================================================
    /**
     * Set the threshold values for normalization.
     *
     * @param[in] min        Lower threshold (values below are transparent).
     * @param[in] mid        Mid-point threshold (fade-in range).
     * @param[in] max        Upper threshold (values above are clamped).
     */
    void setThresholds(double min, double mid, double max);

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
     * Toggle between emitting interpolated color data (smoothed) and raw
     * source values split by hemisphere.
     *
     * When enabled (default), streamData() performs interpolation + colormap
     * conversion and emits newRtSmoothedData(). When disabled, it emits
     * newRtRawData() with the raw source values per hemisphere, which can
     * be used for GPU-side interpolation.
     *
     * @param[in] bStreamSmoothedData    True for smoothed colors (default), false for raw.
     */
    void setStreamSmoothedData(bool bStreamSmoothedData);

    //=========================================================================================================
    /**
     * Set the base surface colors for both hemispheres.
     * Sub-threshold vertices will display these colors instead of being transparent.
     *
     * @param[in] baseColorsLh   Per-vertex ABGR colors for the left hemisphere.
     * @param[in] baseColorsRh   Per-vertex ABGR colors for the right hemisphere.
     */
    void setSurfaceColor(const std::vector<uint32_t>& baseColorsLh,
                         const std::vector<uint32_t>& baseColorsRh);

public:
    //=========================================================================================================
    /**
     * Stream one frame of data. Called by the controller's timer.
     * Pops data from the queue, averages, interpolates, and emits per-vertex colors.
     */
    void streamData();

public:
    //=========================================================================================================
    /**
     * Set the callback for newRtSmoothedData. Called when new interpolated color data is ready for rendering. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (colorsLh, colorsRh); empty to drop the results.
     */
    void setNewRtSmoothedDataCallback(std::function<void(const std::vector<uint32_t>&, const std::vector<uint32_t>&)> callback);

    //=========================================================================================================
    /**
     * Set the callback for newRtRawData. Called when new raw (non-interpolated) data is available. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (dataLh, dataRh); empty to drop the results.
     */
    void setNewRtRawDataCallback(std::function<void(const Eigen::VectorXd&, const Eigen::VectorXd&)> callback);

private:
    //=========================================================================================================
    /**
     * Convert a normalized value [0,1] to a packed ABGR color using the current colormap.
     *
     * @param[in] value      Normalized value in [0,1].
     * @param[in] alpha      Alpha value.
     * @return Packed ABGR color.
     */
    uint32_t valueToColor(double value, uint8_t alpha = 255) const;

    //=========================================================================================================
    /**
     * Interpolate source data and convert to per-vertex colors for one hemisphere.
     *
     * @param[in] sourceData     Raw source values for this hemisphere.
     * @param[in] interpMat      Interpolation matrix for this hemisphere.
     * @param[in] baseColors Per-vertex base surface colors (ABGR).
     * @return Per-vertex ABGR color array.
     */
    std::vector<uint32_t> computeHemiColors(const Eigen::VectorXf& sourceData,
                                            const std::shared_ptr<Eigen::SparseMatrix<float>>& interpMat,
                                            const std::vector<uint32_t>& baseColors) const;

    mutable std::mutex m_mutex; /**< Protects data members. */

    std::deque<Eigen::VectorXd> m_lDataQ;     /**< Incoming data queue. */
    std::deque<Eigen::VectorXd> m_lDataLoopQ; /**< Copy for looping. */
    Eigen::VectorXd m_vecAverage;             /**< Running average accumulator. */
    int m_iSampleCtr = 0;                     /**< Sample counter for averaging. */
    int m_iCurrentSample = 0;                 /**< Current sample index in queue. */

    std::shared_ptr<Eigen::SparseMatrix<float>> m_interpMatLh; /**< LH interpolation matrix. */
    std::shared_ptr<Eigen::SparseMatrix<float>> m_interpMatRh; /**< RH interpolation matrix. */

    int m_iNumAverages = 1;            /**< Number of samples to average. */
    bool m_bIsLooping = true;          /**< Whether to loop data. */
    bool m_bStreamSmoothedData = true; /**< Whether to stream smoothed colors (true) or raw data (false). */
    double m_dSFreq = 1000.0;          /**< Sampling frequency in Hz. */

    std::string m_sColormapType = "Hot"; /**< Active colormap name. */
    double m_dThreshMin = 0.0;           /**< Lower normalization threshold. */
    double m_dThreshMid = 0.5;           /**< Mid normalization threshold. */
    double m_dThreshMax = 1.0;           /**< Upper normalization threshold. */

    std::vector<uint32_t> m_baseColorsLh; /**< LH base surface colors (curvature/annotation). */
    std::vector<uint32_t> m_baseColorsRh; /**< RH base surface colors (curvature/annotation). */

    std::function<void(const std::vector<uint32_t>&, const std::vector<uint32_t>&)> m_newRtSmoothedDataCallback; /**< Receives newRtSmoothedData. */
    std::function<void(const Eigen::VectorXd&, const Eigen::VectorXd&)> m_newRtRawDataCallback;                  /**< Receives newRtRawData. */
};

} // namespace mne::viz

#endif // VIZ_RTSOURCEDATAWORKER_H
