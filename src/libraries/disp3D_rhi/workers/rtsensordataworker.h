//=============================================================================================================
/**
 * @file     rtsensordataworker.h
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
 * @brief    RtSensorDataWorker class declaration.
 *
 */

#ifndef BRAINVIEW_RTSENSORDATAWORKER_H
#define BRAINVIEW_RTSENSORDATAWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include <QObject>
#include <QList>
#include <QMutex>
#include <QVector>
#include <QSharedPointer>
#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace DISP3DRHILIB {

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
class DISP3DRHISHARED_EXPORT RtSensorDataWorker : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] parent     Parent QObject.
     */
    explicit RtSensorDataWorker(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Add a new sensor measurement vector to the streaming queue.
     * The vector contains one measurement per picked sensor channel.
     *
     * @param[in] data       Sensor measurement vector (nChannels x 1).
     */
    void addData(const Eigen::VectorXf &data);

    //=========================================================================================================
    /**
     * Clear all queued data and reset averaging state.
     */
    void clear();

    //=========================================================================================================
    /**
     * Set the dense mapping matrix (sensor → surface vertices).
     * This matrix is typically produced by FieldMap::computeMegMapping() or
     * computeEegMapping(). Size: (nVertices × nChannels).
     *
     * @param[in] mat        Dense mapping matrix.
     */
    void setMappingMatrix(QSharedPointer<Eigen::MatrixXf> mat);

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
    void setColormapType(const QString &name);

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
     * Pops data from the queue, averages, maps to surface, normalises
     * symmetrically, and emits per-vertex colors.
     */
    void streamData();

signals:
    //=========================================================================================================
    /**
     * Emitted when new per-vertex color data is ready for rendering.
     *
     * @param[in] surfaceKey  Surface key identifying the target surface.
     * @param[in] colors      Per-vertex ABGR color array.
     */
    void newRtSensorColors(const QString &surfaceKey,
                           const QVector<uint32_t> &colors);

private:
    //=========================================================================================================
    /**
     * Map sensor data to surface, normalise symmetrically, and convert to
     * per-vertex ABGR colors.
     *
     * @param[in] sensorData    Measurement vector (nChannels x 1).
     * @return Per-vertex ABGR color array (nVertices).
     */
    QVector<uint32_t> computeSurfaceColors(const Eigen::VectorXf &sensorData) const;

    mutable QMutex m_mutex;                                         /**< Protects data members. */

    QList<Eigen::VectorXf> m_lDataQ;                                /**< Incoming data queue. */
    QList<Eigen::VectorXf> m_lDataLoopQ;                            /**< Copy for looping. */
    Eigen::VectorXf m_vecAverage;                                   /**< Running average accumulator. */
    int m_iSampleCtr = 0;                                           /**< Sample counter for averaging. */
    int m_iCurrentSample = 0;                                       /**< Current sample index for loop replay. */

    QSharedPointer<Eigen::MatrixXf> m_mappingMat;                   /**< Dense mapping matrix (nVerts x nChans). */
    QString m_sSurfaceKey;                                          /**< Surface key for the target surface. */

    int m_iNumAverages = 1;                                         /**< Number of samples to average. */
    bool m_bIsLooping = true;                                       /**< Whether to loop data. */
    double m_dSFreq = 1000.0;                                       /**< Sampling frequency in Hz. */

    QString m_sColormapType = QStringLiteral("MNE");                /**< Active colormap name. */
};

} // namespace DISP3DRHILIB

#endif // BRAINVIEW_RTSENSORDATAWORKER_H
