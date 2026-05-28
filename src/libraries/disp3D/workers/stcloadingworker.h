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
 * INVLIB::InvSourceEstimate per hemisphere and builds the sparse
 * vertex-to-source interpolation matrix using
 * @ref GeometryInfo::scdc / @ref Interpolation::createInterpolationMat
 * with a configurable geodesic cancel distance.
 *
 * On completion the controller signals the GUI to attach the data
 * to the cortical surfaces, after which scrubbing and playback are
 * served by @ref SourceEstimateOverlay without any further I/O.
 */

#ifndef STCLOADINGWORKER_H
#define STCLOADINGWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <inv/inv_source_estimate.h>
#include <QObject>
#include <QString>
#include <QSharedPointer>
#include <Eigen/Sparse>
#include <atomic>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainSurface;

//=============================================================================================================
/**
 * StcLoadingWorker performs source estimate loading and interpolation matrix computation
 * in a background thread to avoid blocking the UI.
 *
 * @brief Background worker that loads source estimate (STC) files and emits loaded data for visualization.
 */
class DISP3DSHARED_EXPORT StcLoadingWorker : public QObject
{
    Q_OBJECT

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
     * @param[in] parent         Parent object.
     */
    StcLoadingWorker(const QString &lhPath,
                     const QString &rhPath,
                     BrainSurface *lhSurface,
                     BrainSurface *rhSurface,
                     double cancelDist = 0.05,
                     QObject *parent = nullptr);

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
    const INVLIB::InvSourceEstimate& stcLh() const { return m_stcLh; }

    //=========================================================================================================
    /**
     * Get the loaded right hemisphere source estimate.
     *
     * @return The right hemisphere source estimate.
     */
    const INVLIB::InvSourceEstimate& stcRh() const { return m_stcRh; }

    //=========================================================================================================
    /**
     * Get the computed left hemisphere interpolation matrix.
     *
     * @return Shared pointer to the interpolation matrix.
     */
    QSharedPointer<Eigen::SparseMatrix<float>> interpolationMatLh() const { return m_interpMatLh; }

    //=========================================================================================================
    /**
     * Get the computed right hemisphere interpolation matrix.
     *
     * @return Shared pointer to the interpolation matrix.
     */
    QSharedPointer<Eigen::SparseMatrix<float>> interpolationMatRh() const { return m_interpMatRh; }

    //=========================================================================================================
    /**
     * Check if left hemisphere data was loaded.
     *
     * @return True if LH data is loaded.
     */
    bool hasLh() const { return m_hasLh; }

    //=========================================================================================================
    /**
     * Check if right hemisphere data was loaded.
     *
     * @return True if RH data is loaded.
     */
    bool hasRh() const { return m_hasRh; }

    //=========================================================================================================
    /**
     * Request cancellation of the current loading/interpolation.
     * Thread-safe; takes effect at the next progress check.
     */
    void requestCancel() { m_cancelled.store(true, std::memory_order_relaxed); }

    /** @return true if cancellation has been requested. */
    bool isCancelled() const { return m_cancelled.load(std::memory_order_relaxed); }

public slots:
    //=========================================================================================================
    /**
     * Perform the loading and computation work.
     * This slot should be called after moving the worker to a thread.
     */
    void process();

signals:
    //=========================================================================================================
    /**
     * Emitted to report progress.
     *
     * @param[in] percent    Progress percentage (0-100).
     * @param[in] message    Status message describing current operation.
     */
    void progress(int percent, const QString &message);

    //=========================================================================================================
    /**
     * Emitted when loading is complete.
     *
     * @param[in] success    True if loading succeeded.
     */
    void finished(bool success);

    //=========================================================================================================
    /**
     * Emitted when an error occurs.
     *
     * @param[in] message    Error description.
     */
    void error(const QString &message);

private:
    QSharedPointer<Eigen::SparseMatrix<float>> computeInterpolationMatrix(
        const Eigen::MatrixX3f &matVertices,
        Eigen::VectorXi &vecSourceVertices,
        double cancelDist,
        const QString &hemiLabel,
        int progressStart,
        int progressEnd);

    QString m_lhPath;                                          /**< Path to LH STC file. */
    QString m_rhPath;                                          /**< Path to RH STC file. */
    BrainSurface *m_lhSurface;                                 /**< Pointer to LH surface. */
    BrainSurface *m_rhSurface;                                 /**< Pointer to RH surface. */
    double m_cancelDist;                                       /**< Cancel distance for interpolation. */

    INVLIB::InvSourceEstimate m_stcLh;                         /**< Loaded LH source estimate. */
    INVLIB::InvSourceEstimate m_stcRh;                         /**< Loaded RH source estimate. */
    bool m_hasLh = false;                                      /**< Flag indicating LH data loaded. */
    bool m_hasRh = false;                                      /**< Flag indicating RH data loaded. */

    QSharedPointer<Eigen::SparseMatrix<float>> m_interpMatLh;  /**< LH interpolation matrix. */
    QSharedPointer<Eigen::SparseMatrix<float>> m_interpMatRh;  /**< RH interpolation matrix. */
    std::atomic<bool> m_cancelled{false};                       /**< Cancellation flag. */
};

#endif // STCLOADINGWORKER_H
