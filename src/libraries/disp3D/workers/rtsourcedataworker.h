//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rtsourcedataworker.h
 * @since March 2026
 * @brief Background worker that converts streaming source vectors into smoothed per-vertex ABGR colour buffers.
 *
 * Holds the sparse interpolation matrices for the left and right
 * hemispheres, the active colormap and the (fmin, fmid, fmax)
 * normalisation thresholds. On every timer tick the queued source
 * vector is split per hemisphere, multiplied by its sparse
 * matrix, mapped through the colormap and packed as ABGR ready for
 * direct upload &mdash; the only per-frame work is one sparse
 * mat-vec plus a tight colour-packing loop.
 */

#ifndef BRAINVIEW_RTSOURCEDATAWORKER_H
#define BRAINVIEW_RTSOURCEDATAWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <QObject>
#include <QList>
#include <QMutex>
#include <QVector>
#include <QSharedPointer>
#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace DISP3DLIB {

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
class DISP3DSHARED_EXPORT RtSourceDataWorker : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] parent     Parent QObject.
     */
    explicit RtSourceDataWorker(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Add a new source estimate data vector to the streaming queue.
     * The vector contains source values for all sources (LH + RH concatenated).
     *
     * @param[in] data       Source activity vector (nSources x 1).
     */
    void addData(const Eigen::VectorXd &data);

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
    void setInterpolationMatrixLeft(QSharedPointer<Eigen::SparseMatrix<float>> mat);

    //=========================================================================================================
    /**
     * Set the interpolation matrix for the right hemisphere.
     *
     * @param[in] mat        Sparse interpolation matrix (nVertices x nSourcesRH).
     */
    void setInterpolationMatrixRight(QSharedPointer<Eigen::SparseMatrix<float>> mat);

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
    void setColormapType(const QString &name);

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
    void setSurfaceColor(const QVector<uint32_t> &baseColorsLh,
                         const QVector<uint32_t> &baseColorsRh);

public slots:
    //=========================================================================================================
    /**
     * Stream one frame of data. Called by the controller's timer.
     * Pops data from the queue, averages, interpolates, and emits per-vertex colors.
     */
    void streamData();

signals:
    //=========================================================================================================
    /**
     * Emitted when new interpolated color data is ready for rendering.
     *
     * @param[in] colorsLh   Per-vertex ABGR color array for the left hemisphere.
     * @param[in] colorsRh   Per-vertex ABGR color array for the right hemisphere.
     */
    void newRtSmoothedData(const QVector<uint32_t> &colorsLh,
                           const QVector<uint32_t> &colorsRh);

    //=========================================================================================================
    /**
     * Emitted when new raw (non-interpolated) data is available.
     *
     * @param[in] dataLh     Source values for left hemisphere.
     * @param[in] dataRh     Source values for right hemisphere.
     */
    void newRtRawData(const Eigen::VectorXd &dataLh,
                      const Eigen::VectorXd &dataRh);

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
     * @return Per-vertex ABGR color array.
     */
    QVector<uint32_t> computeHemiColors(const Eigen::VectorXf &sourceData,
                                        const QSharedPointer<Eigen::SparseMatrix<float>> &interpMat,
                                        const QVector<uint32_t> &baseColors) const;

    mutable QMutex m_mutex;                                         /**< Protects data members. */

    QList<Eigen::VectorXd> m_lDataQ;                                /**< Incoming data queue. */
    QList<Eigen::VectorXd> m_lDataLoopQ;                            /**< Copy for looping. */
    Eigen::VectorXd m_vecAverage;                                   /**< Running average accumulator. */
    int m_iSampleCtr = 0;                                           /**< Sample counter for averaging. */
    int m_iCurrentSample = 0;                                       /**< Current sample index in queue. */

    QSharedPointer<Eigen::SparseMatrix<float>> m_interpMatLh;       /**< LH interpolation matrix. */
    QSharedPointer<Eigen::SparseMatrix<float>> m_interpMatRh;       /**< RH interpolation matrix. */

    int m_iNumAverages = 1;                                         /**< Number of samples to average. */
    bool m_bIsLooping = true;                                       /**< Whether to loop data. */
    bool m_bStreamSmoothedData = true;                               /**< Whether to stream smoothed colors (true) or raw data (false). */
    double m_dSFreq = 1000.0;                                       /**< Sampling frequency in Hz. */

    QString m_sColormapType = QStringLiteral("Hot");                /**< Active colormap name. */
    double m_dThreshMin = 0.0;                                      /**< Lower normalization threshold. */
    double m_dThreshMid = 0.5;                                      /**< Mid normalization threshold. */
    double m_dThreshMax = 1.0;                                      /**< Upper normalization threshold. */

    QVector<uint32_t> m_baseColorsLh;                                /**< LH base surface colors (curvature/annotation). */
    QVector<uint32_t> m_baseColorsRh;                                /**< RH base surface colors (curvature/annotation). */
};

} // namespace DISP3DLIB

#endif // BRAINVIEW_RTSOURCEDATAWORKER_H
