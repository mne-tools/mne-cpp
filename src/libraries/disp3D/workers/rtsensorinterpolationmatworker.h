//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rtsensorinterpolationmatworker.h
 * @since 2026
 * @date  March 2026
 * @brief Background worker that recomputes the dense MEG / EEG sensor-to-surface mapping matrix off the GUI thread.
 *
 * Owns the inputs to the field-map kernel &mdash; evoked info, head
 * and helmet surface geometry, MRI / head coordinate transforms,
 * channel picks and SSP projectors. Whenever any of these change
 * via the (mutex-protected) setters, @ref computeMapping rebuilds
 * the dense (n_vertices x n_channels) matrix and emits it back to
 * the data worker, so the streaming pipeline never stalls.
 */

#ifndef BRAINVIEW_RTSENSORINTERPOLATIONMATWORKER_H
#define BRAINVIEW_RTSENSORINTERPOLATIONMATWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <fiff/fiff_evoked.h>
#include <fiff/fiff_coord_trans.h>

#include <QObject>
#include <QMutex>
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

namespace DISP3DLIB {

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
class DISP3DSHARED_EXPORT RtSensorInterpolationMatWorker : public QObject
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
                                std::shared_ptr<Eigen::MatrixXf> mappingMat,
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
                                std::shared_ptr<Eigen::MatrixXf> mappingMat,
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

} // namespace DISP3DLIB

#endif // BRAINVIEW_RTSENSORINTERPOLATIONMATWORKER_H
