//=============================================================================================================
/**
 * @file     rtsensorinterpolationmatworker.h
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
 * @brief    RtSensorInterpolationMatWorker class declaration.
 *
 */

#ifndef BRAINVIEW_RTSENSORINTERPOLATIONMATWORKER_H
#define BRAINVIEW_RTSENSORINTERPOLATIONMATWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include <fiff/fiff_evoked.h>
#include <fiff/fiff_coord_trans.h>

#include <QObject>
#include <QMutex>
#include <QSharedPointer>
#include <QVector>
#include <QString>
#include <QMap>
#include <Eigen/Core>

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainSurface;

//=============================================================================================================
// DEFINE NAMESPACE
//=============================================================================================================

namespace DISP3DRHILIB {

//=============================================================================================================
/**
 * RtSensorInterpolationMatWorker computes dense sensor-to-surface field-mapping
 * matrices in a background thread.
 *
 * This worker encapsulates the expensive FieldMap::computeMegMapping() and
 * computeEegMapping() calls that were previously performed synchronously on the
 * main thread inside BrainView::buildSensorFieldMapping().
 *
 * The worker stores all relevant parameters (evoked info, surface geometry,
 * transforms, etc.) and recomputes the mapping matrices when any parameter
 * changes. It emits newMappingMatrixAvailable() when a new matrix is ready.
 *
 * Thread-safety: all public setters are mutex-protected and schedule a
 * recomputation via the computeMapping() slot.
 *
 * @brief Background worker for computing sensor field mapping matrices.
 */
class DISP3DRHISHARED_EXPORT RtSensorInterpolationMatWorker : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     * @param[in] parent     Parent QObject.
     */
    explicit RtSensorInterpolationMatWorker(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Set the evoked data that contains channel info and sensor definitions.
     *
     * @param[in] evoked    The evoked dataset.
     */
    void setEvoked(const FIFFLIB::FiffEvoked &evoked);

    //=========================================================================================================
    /**
     * Set the head-to-MRI coordinate transform.
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
     * Set the surface geometry used for MEG field mapping.
     * This must include vertex positions and normals (for MEG).
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
     * Set the surface geometry used for EEG field mapping.
     *
     * @param[in] surfaceKey    The key identifying the surface.
     * @param[in] vertices      Vertex positions (nVerts x 3).
     */
    void setEegSurface(const QString &surfaceKey,
                       const Eigen::MatrixX3f &vertices);

    //=========================================================================================================
    /**
     * Set the bad channels. Triggers recomputation if changed.
     *
     * @param[in] bads           List of bad channel names.
     */
    void setBadChannels(const QStringList &bads);

public slots:
    //=========================================================================================================
    /**
     * Compute the sensor-to-surface mapping matrices.
     * This slot can be called directly or connected to a signal.
     * The computation reads the stored parameters and builds dense mapping
     * matrices via FieldMap::computeMegMapping() / computeEegMapping().
     */
    void computeMapping();

signals:
    //=========================================================================================================
    /**
     * Emitted when new MEG mapping data is available.
     *
     * @param[in] surfaceKey    The surface the mapping targets.
     * @param[in] mappingMat    Dense mapping matrix (nVerts x nChannels).
     * @param[in] pick          Channel indices picked for this mapping.
     */
    void newMegMappingAvailable(const QString &surfaceKey,
                                QSharedPointer<Eigen::MatrixXf> mappingMat,
                                const QVector<int> &pick);

    //=========================================================================================================
    /**
     * Emitted when new EEG mapping data is available.
     *
     * @param[in] surfaceKey    The surface the mapping targets.
     * @param[in] mappingMat    Dense mapping matrix (nVerts x nChannels).
     * @param[in] pick          Channel indices picked for this mapping.
     */
    void newEegMappingAvailable(const QString &surfaceKey,
                                QSharedPointer<Eigen::MatrixXf> mappingMat,
                                const QVector<int> &pick);

private:
    mutable QMutex m_mutex;                              /**< Protects all data members. */

    FIFFLIB::FiffEvoked m_evoked;                        /**< Evoked data with channel info. */
    bool m_hasEvoked = false;                            /**< Whether evoked data has been set. */

    FIFFLIB::FiffCoordTrans m_headToMriTrans;            /**< Head-to-MRI transform. */
    bool m_applySensorTrans = true;                      /**< Whether to apply sensor transform. */

    bool m_megOnHead = false;                            /**< Map MEG onto head vs helmet. */

    // MEG target surface
    QString m_megSurfaceKey;                             /**< Key of the MEG target surface. */
    Eigen::MatrixX3f m_megVertices;                      /**< MEG surface vertices. */
    Eigen::MatrixX3f m_megNormals;                       /**< MEG surface normals. */
    Eigen::MatrixX3i m_megTriangles;                     /**< MEG surface triangles. */
    bool m_hasMegSurface = false;                        /**< Whether MEG surface has been set. */

    // EEG target surface
    QString m_eegSurfaceKey;                             /**< Key of the EEG target surface. */
    Eigen::MatrixX3f m_eegVertices;                      /**< EEG surface vertices. */
    bool m_hasEegSurface = false;                        /**< Whether EEG surface has been set. */

    QStringList m_bads;                                  /**< Current bad channel names. */
};

} // namespace DISP3DRHILIB

#endif // BRAINVIEW_RTSENSORINTERPOLATIONMATWORKER_H
