//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rtsourcedatacontroller.h
 * @since March 2026
 * @brief Real-time source-estimate streaming controller that owns the data worker and the per-hemisphere interpolation workers.
 *
 * RtSourceDataController bundles a @ref RtSourceDataWorker (queue,
 * averaging, sparse mat-vec per hemisphere, colormap packing) with
 * two @ref RtSourceInterpolationMatWorker instances (one per
 * hemisphere) that recompute the sparse vertex-to-source weight
 * matrix when surfaces, source vertices or the interpolation
 * function change.
 *
 * The GUI sees a single object: push source vectors with @c addData,
 * scrub / play / loop via the streaming setters, and listen on
 * @c newSmoothedDataAvailable for per-vertex colour buffers.
 */

#ifndef BRAINVIEW_RTSOURCEDATACONTROLLER_H
#define BRAINVIEW_RTSOURCEDATACONTROLLER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <QList>
#include <QString>
#include <vector>
#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QThread;
class QTimer;

namespace FSLIB {
    class FsLabel;
}

namespace DISP3DLIB {
    class RtSourceDataWorker;
    class RtSourceInterpolationMatWorker;
}

//=============================================================================================================
/**
 * RtSourceDataController orchestrates real-time source estimate streaming.
 * It manages a background data worker thread, an optional background
 * interpolation matrix worker thread, and a timer that drives the data flow.
 *
 * The controller provides a simple public API for:
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
 *   1. Create controller
 *   2. Set interpolation matrices via setInterpolationMatrixLeft/Right()
 *   3. Connect newSmoothedDataAvailable() to your rendering update slot
 *   4. Call addData() to push source estimate vectors
 *   5. Call setStreamingState(true) to start streaming
 *
 * @brief Controller for real-time source estimate streaming.
 */
class DISP3DSHARED_EXPORT RtSourceDataController : public QObject
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

    //=========================================================================================================
    /**
     * Set the base surface colors for both hemispheres.
     * Sub-threshold vertices will display these colors (e.g., curvature or
     * annotation coloring) instead of being transparent.
     *
     * @param[in] baseColorsLh   Per-vertex ABGR colors for the left hemisphere.
     * @param[in] baseColorsRh   Per-vertex ABGR colors for the right hemisphere.
     */
    void setSurfaceColor(const QVector<uint32_t> &baseColorsLh,
                         const QVector<uint32_t> &baseColorsRh);

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
    void setInterpolationFunction(const QString &sInterpolationFunction);

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
    void setInterpolationInfoLeft(const Eigen::MatrixX3f &matVertices,
                                  const std::vector<Eigen::VectorXi> &vecNeighborVertices,
                                  const Eigen::VectorXi &vecSourceVertices);

    //=========================================================================================================
    /**
     * Set the surface and source geometry for the right hemisphere.
     * Used for on-the-fly interpolation matrix computation.
     *
     * @param[in] matVertices          Vertex positions (nVerts x 3).
     * @param[in] vecNeighborVertices   Per-vertex neighbor index lists.
     * @param[in] vecSourceVertices     Source vertex indices into the surface.
     */
    void setInterpolationInfoRight(const Eigen::MatrixX3f &matVertices,
                                   const std::vector<Eigen::VectorXi> &vecNeighborVertices,
                                   const Eigen::VectorXi &vecSourceVertices);

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
    void setAnnotationInfoLeft(const Eigen::VectorXi &vecLabelIds,
                               const QList<FSLIB::FsLabel> &lLabels,
                               const Eigen::VectorXi &vecVertNo);

    //=========================================================================================================
    /**
     * Set annotation info for the right hemisphere.
     *
     * @param[in] vecLabelIds   Per-vertex label IDs.
     * @param[in] lLabels       FreeSurfer Labels.
     * @param[in] vecVertNo     Source vertex numbers.
     */
    void setAnnotationInfoRight(const Eigen::VectorXi &vecLabelIds,
                                const QList<FSLIB::FsLabel> &lLabels,
                                const Eigen::VectorXi &vecVertNo);

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

    //=========================================================================================================
    /**
     * Emitted when a new left hemisphere interpolation matrix has been computed.
     *
     * @param[in] interpMat   Sparse interpolation matrix (nVertices x nSources).
     */
    void newInterpolationMatrixLeftAvailable(QSharedPointer<Eigen::SparseMatrix<float>> interpMat);

    //=========================================================================================================
    /**
     * Emitted when a new right hemisphere interpolation matrix has been computed.
     *
     * @param[in] interpMat   Sparse interpolation matrix (nVertices x nSources).
     */
    void newInterpolationMatrixRightAvailable(QSharedPointer<Eigen::SparseMatrix<float>> interpMat);

private slots:
    void onNewInterpolationMatrixLeft(QSharedPointer<Eigen::SparseMatrix<float>> interpMat);
    void onNewInterpolationMatrixRight(QSharedPointer<Eigen::SparseMatrix<float>> interpMat);

private:
    QThread *m_pWorkerThread = nullptr;                     /**< Background thread for the data worker. */
    DISP3DLIB::RtSourceDataWorker *m_pWorker = nullptr;  /**< Data streaming worker. */
    QTimer *m_pTimer = nullptr;                             /**< Timer driving the streaming cadence. */
    bool m_bIsStreaming = false;                             /**< Whether streaming is active. */
    int m_iTimeInterval = 17;                               /**< Streaming interval in ms (~60fps). */

    QThread *m_pInterpThread = nullptr;                     /**< Background thread for interpolation matrix worker. */
    DISP3DLIB::RtSourceInterpolationMatWorker *m_pInterpWorker = nullptr; /**< Interpolation matrix worker. */
};

#endif // BRAINVIEW_RTSOURCEDATACONTROLLER_H
