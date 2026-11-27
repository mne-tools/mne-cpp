//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     stcloadingworker.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Background worker that loads source-time-course (.stc) files and prepares per-hemisphere interpolation matrices.
 *
 * StcLoadingWorker pulls the LH and RH @c .stc files from disk in a
 * background thread, parses them into an @ref
 * mne::InvSourceEstimate per hemisphere and builds the sparse
 * vertex-to-source interpolation matrix using
 * @ref mne::viz::GeometryInfo::scdc "GeometryInfo::scdc" / @ref mne::viz::Interpolation::createInterpolationMat "Interpolation::createInterpolationMat"
 * with a configurable geodesic cancel distance.
 *
 * On completion the controller signals the GUI to attach the data
 * to the cortical surfaces, after which scrubbing and playback are
 * served by @ref mne::viz::SourceEstimateOverlay without any further I/O.
 */

#ifndef VIZ_STCLOADINGWORKER_H
#define VIZ_STCLOADINGWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_global.h"
#include <mne/inv/inv_source_estimate.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Sparse>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne::viz
{
class BrainSurface;
} // namespace mne::viz

namespace mne::viz
{
//=============================================================================================================
/**
 * StcLoadingWorker performs source estimate loading and interpolation matrix computation
 * in a background thread to avoid blocking the UI.
 *
 * @brief Background worker that loads source estimate (STC) files and emits loaded data for visualization.
 */
class VIZSHARED_EXPORT StcLoadingWorker
{
public:
    //=========================================================================================================
    /**
     * Constructor
     *
     * @param[in] lhPath         Path to left hemisphere STC file.
     * @param[in] rhPath         Path to right hemisphere STC file.
     * @param[in] lhSurface      Pointer to left hemisphere surface.
     * @param[in] rhSurface      Pointer to right hemisphere surface.
     * @param[in] cancelDist     Cancel distance for interpolation.
     */
    StcLoadingWorker(const std::filesystem::path& lhPath,
                     const std::filesystem::path& rhPath,
                     BrainSurface* lhSurface,
                     BrainSurface* rhSurface,
                     double cancelDist = 0.05);

    //=========================================================================================================
    /**
     * Destructor
     */
    ~StcLoadingWorker();

    //=========================================================================================================
    /**
     * Get the loaded left hemisphere source estimate.
     *
     * @return The left hemisphere source estimate.
     */
    const mne::InvSourceEstimate& stcLh() const
    {
        return m_stcLh;
    }

    //=========================================================================================================
    /**
     * Get the loaded right hemisphere source estimate.
     *
     * @return The right hemisphere source estimate.
     */
    const mne::InvSourceEstimate& stcRh() const
    {
        return m_stcRh;
    }

    //=========================================================================================================
    /**
     * Get the computed left hemisphere interpolation matrix.
     *
     * @return Shared pointer to the interpolation matrix.
     */
    std::shared_ptr<Eigen::SparseMatrix<float>> interpolationMatLh() const
    {
        return m_interpMatLh;
    }

    //=========================================================================================================
    /**
     * Get the computed right hemisphere interpolation matrix.
     *
     * @return Shared pointer to the interpolation matrix.
     */
    std::shared_ptr<Eigen::SparseMatrix<float>> interpolationMatRh() const
    {
        return m_interpMatRh;
    }

    //=========================================================================================================
    /**
     * Check if left hemisphere data was loaded.
     *
     * @return True if LH data is loaded.
     */
    bool hasLh() const
    {
        return m_hasLh;
    }

    //=========================================================================================================
    /**
     * Check if right hemisphere data was loaded.
     *
     * @return True if RH data is loaded.
     */
    bool hasRh() const
    {
        return m_hasRh;
    }

    //=========================================================================================================
    /**
     * Request cancellation of the current loading/interpolation.
     * Thread-safe; takes effect at the next progress check.
     */
    void requestCancel()
    {
        m_cancelled.store(true, std::memory_order_relaxed);
    }

    /** @return true if cancellation has been requested. */
    bool isCancelled() const
    {
        return m_cancelled.load(std::memory_order_relaxed);
    }

public:
    //=========================================================================================================
    /**
     * Perform the loading and computation work.
     * This slot should be called after moving the worker to a thread.
     */
    void process();

public:
    //=========================================================================================================
    /**
     * Set the callback for progress. Called to report progress. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (percent, message); empty to drop the results.
     */
    void setProgressCallback(std::function<void(int, const std::string&)> callback);

    //=========================================================================================================
    /**
     * Set the callback for finished. Called when loading is complete. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (success); empty to drop the results.
     */
    void setFinishedCallback(std::function<void(bool)> callback);

    //=========================================================================================================
    /**
     * Set the callback for error. Called when an error occurs. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (message); empty to drop the results.
     */
    void setErrorCallback(std::function<void(const std::string&)> callback);

private:
    std::shared_ptr<Eigen::SparseMatrix<float>> computeInterpolationMatrix(
        const Eigen::MatrixX3f& matVertices,
        Eigen::VectorXi& vecSourceVertices,
        double cancelDist,
        const std::string& hemiLabel,
        int progressStart,
        int progressEnd);

    std::filesystem::path m_lhPath; /**< Path to LH STC file. */
    std::filesystem::path m_rhPath; /**< Path to RH STC file. */
    BrainSurface* m_lhSurface;      /**< Pointer to LH surface. */
    BrainSurface* m_rhSurface;      /**< Pointer to RH surface. */
    double m_cancelDist;            /**< Cancel distance for interpolation. */

    mne::InvSourceEstimate m_stcLh; /**< Loaded LH source estimate. */
    mne::InvSourceEstimate m_stcRh; /**< Loaded RH source estimate. */
    bool m_hasLh = false;           /**< Flag indicating LH data loaded. */
    bool m_hasRh = false;           /**< Flag indicating RH data loaded. */

    std::shared_ptr<Eigen::SparseMatrix<float>> m_interpMatLh; /**< LH interpolation matrix. */
    std::shared_ptr<Eigen::SparseMatrix<float>> m_interpMatRh; /**< RH interpolation matrix. */
    std::atomic<bool> m_cancelled{false};                      /**< Cancellation flag. */

    std::function<void(int, const std::string&)> m_progressCallback; /**< Receives progress. */
    std::function<void(bool)> m_finishedCallback;                    /**< Receives finished. */
    std::function<void(const std::string&)> m_errorCallback;         /**< Receives error. */
};

} // namespace mne::viz

#endif // VIZ_STCLOADINGWORKER_H
