//=============================================================================================================
/**
 * @file     rtsourcedataworker.h
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
 * @brief    RtSourceDataWorker class declaration.
 *
 */

#ifndef BRAINVIEW_RTSOURCEDATAWORKER_H
#define BRAINVIEW_RTSOURCEDATAWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

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

namespace BRAINVIEWLIB {

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
class RtSourceDataWorker : public QObject
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
                                        const QSharedPointer<Eigen::SparseMatrix<float>> &interpMat) const;

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
    double m_dSFreq = 1000.0;                                       /**< Sampling frequency in Hz. */

    QString m_sColormapType = QStringLiteral("Hot");                /**< Active colormap name. */
    double m_dThreshMin = 0.0;                                      /**< Lower normalization threshold. */
    double m_dThreshMid = 0.5;                                      /**< Mid normalization threshold. */
    double m_dThreshMax = 1.0;                                      /**< Upper normalization threshold. */
};

} // namespace BRAINVIEWLIB

#endif // BRAINVIEW_RTSOURCEDATAWORKER_H
