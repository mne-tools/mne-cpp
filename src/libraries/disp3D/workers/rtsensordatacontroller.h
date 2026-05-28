//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rtsensordatacontroller.h
 * @since March 2026
 * @brief Real-time MEG / EEG sensor streaming controller that orchestrates the data and field-mapping workers.
 *
 * RtSensorDataController spins up a background @ref
 * RtSensorDataWorker (queue, averaging, dense mat-vec mapping,
 * ABGR packing) and an optional @ref RtSensorInterpolationMatWorker
 * (asynchronous recomputation of the dense mapping matrix when the
 * sensor configuration, projectors or target surface change) and
 * ties them together with a timer that paces the streaming.
 *
 * Once started, the controller emits ready-to-upload per-vertex
 * colour buffers; the GUI just connects
 * @c newSensorColorsAvailable to its surface-colour slot.
 */

#ifndef BRAINVIEW_RTSENSORDATACONTROLLER_H
#define BRAINVIEW_RTSENSORDATACONTROLLER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <fiff/fiff_evoked.h>
#include <fiff/fiff_coord_trans.h>

#include <QObject>
#include <QVector>
#include <QString>
#include <Eigen/Core>
#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QThread;
class QTimer;

namespace DISP3DLIB {
    class RtSensorDataWorker;
    class RtSensorInterpolationMatWorker;
}

//=============================================================================================================
/**
 * RtSensorDataController orchestrates real-time sensor data streaming.
 * It manages a background data worker thread, a background interpolation
 * matrix worker thread, and a timer that drives the data flow.
 *
 * Unlike the source-estimate controller (which uses sparse interpolation matrices
 * split by hemisphere), this controller uses a single dense mapping matrix
 * produced by FieldMap::computeMeg/EegMapping() that maps sensor measurements
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
 *   1. Create controller
 *   2. Either set mapping matrix via setMappingMatrix(), or configure
 *      interpolation parameters and call recomputeMapping()
 *   3. Connect newSensorColorsAvailable() to your rendering update slot
 *   4. Call addData() to push sensor measurement vectors
 *   5. Call setStreamingState(true) to start streaming
 *
 * @brief Controller for real-time sensor data streaming.
 */
class DISP3DSHARED_EXPORT RtSensorDataController : public QObject
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
    void setMappingMatrix(std::shared_ptr<Eigen::MatrixXf> mat);

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
    void setEvoked(const FIFFLIB::FiffEvoked &evoked);

    //=========================================================================================================
    /**
     * Set the head-to-MRI coordinate transform for the interpolation worker.
     *
     * @param[in] trans              The transform.
     * @param[in] applySensorTrans   Whether to apply the transform.
     */
    void setTransform(const FIFFLIB::FiffCoordTrans &trans, bool applySensorTrans);

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
    void setMegSurface(const QString &surfaceKey,
                       const Eigen::MatrixX3f &vertices,
                       const Eigen::MatrixX3f &normals,
                       const Eigen::MatrixX3i &triangles);

    //=========================================================================================================
    /**
     * Set the EEG target surface geometry for on-the-fly mapping.
     *
     * @param[in] surfaceKey    The key identifying the surface.
     * @param[in] vertices      Vertex positions (nVerts x 3).
     */
    void setEegSurface(const QString &surfaceKey,
                       const Eigen::MatrixX3f &vertices);

    //=========================================================================================================
    /**
     * Set bad channels for the interpolation worker.
     *
     * @param[in] bads    List of bad channel names.
     */
    void setBadChannels(const QStringList &bads);

    //=========================================================================================================
    /**
     * Trigger an asynchronous recomputation of the mapping matrix.
     * The new matrix will be automatically forwarded to the data worker
     * when ready.
     */
    void recomputeMapping();

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

    //=========================================================================================================
    /**
     * Emitted when raw (non-mapped) sensor data is available.
     *
     * @param[in] data        Raw sensor measurement vector.
     */
    void newRawSensorDataAvailable(const Eigen::VectorXf &data);

    //=========================================================================================================
    /**
     * Emitted when a new MEG mapping matrix has been computed by the background worker.
     *
     * @param[in] surfaceKey    Target surface key.
     * @param[in] mappingMat    Dense mapping matrix (nVerts x nChannels).
     * @param[in] pick          Channel indices picked for this mapping.
     */
    void newMegMappingAvailable(const QString &surfaceKey,
                                std::shared_ptr<Eigen::MatrixXf> mappingMat,
                                const QVector<int> &pick);

    //=========================================================================================================
    /**
     * Emitted when a new EEG mapping matrix has been computed by the background worker.
     *
     * @param[in] surfaceKey    Target surface key.
     * @param[in] mappingMat    Dense mapping matrix (nVerts x nChannels).
     * @param[in] pick          Channel indices picked for this mapping.
     */
    void newEegMappingAvailable(const QString &surfaceKey,
                                std::shared_ptr<Eigen::MatrixXf> mappingMat,
                                const QVector<int> &pick);

private slots:
    void onNewMegMapping(const QString &surfaceKey,
                         std::shared_ptr<Eigen::MatrixXf> mappingMat,
                         const QVector<int> &pick);
    void onNewEegMapping(const QString &surfaceKey,
                         std::shared_ptr<Eigen::MatrixXf> mappingMat,
                         const QVector<int> &pick);

private:
    QThread *m_pWorkerThread = nullptr;                         /**< Background thread for the data worker. */
    DISP3DLIB::RtSensorDataWorker *m_pWorker = nullptr;      /**< Data streaming worker. */
    QTimer *m_pTimer = nullptr;                                 /**< Timer driving the streaming cadence. */
    bool m_bIsStreaming = false;                                 /**< Whether streaming is active. */
    int m_iTimeInterval = 17;                                   /**< Streaming interval in ms (~60fps). */

    QThread *m_pInterpThread = nullptr;                          /**< Background thread for interpolation matrix worker. */
    DISP3DLIB::RtSensorInterpolationMatWorker *m_pInterpWorker = nullptr; /**< Interpolation matrix worker. */
};

#endif // BRAINVIEW_RTSENSORDATACONTROLLER_H
