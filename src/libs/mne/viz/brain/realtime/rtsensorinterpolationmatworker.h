//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsensorinterpolationmatworker.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Background worker that recomputes the dense MEG / EEG sensor-to-surface mapping matrix off the GUI thread.
 *
 * Owns the inputs to the field-map kernel &mdash; evoked info, head
 * and helmet surface geometry, MRI / head coordinate transforms,
 * channel picks and SSP projectors. Whenever any of these change
 * via the (mutex-protected) setters, @ref mne::viz::RtSensorInterpolationMatWorker::computeMapping "computeMapping" rebuilds
 * the dense (n_vertices x n_channels) matrix and emits it back to
 * the data worker, so the streaming pipeline never stalls.
 */

#ifndef VIZ_RTSENSORINTERPOLATIONMATWORKER_H
#define VIZ_RTSENSORINTERPOLATIONMATWORKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_global.h"
#include <mne/fiff/fiff_evoked.h>
#include <mne/fiff/fiff_coord_trans.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
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
 * RtSensorInterpolationMatWorker computes dense sensor-to-surface field-mapping
 * matrices in a background thread.
 *
 * This worker encapsulates the expensive mne::FwdFieldMap::computeMegMapping() and
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
class VIZSHARED_EXPORT RtSensorInterpolationMatWorker
{
public:
    //=========================================================================================================
    /**
     * Constructor.
     *
     */
    RtSensorInterpolationMatWorker();

    //=========================================================================================================
    /**
     * Set the evoked data that contains channel info and sensor definitions.
     *
     * @param[in] evoked    The evoked dataset.
     */
    void setEvoked(const mne::FiffEvoked& evoked);

    //=========================================================================================================
    /**
     * Set the head-to-MRI coordinate transform.
     *
     * @param[in] trans              The transform.
     * @param[in] applySensorTrans   Whether to apply the transform.
     */
    void setTransform(const mne::FiffCoordTrans& trans, bool applySensorTrans);

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
    void setMegSurface(const std::string& surfaceKey,
                       const Eigen::MatrixX3f& vertices,
                       const Eigen::MatrixX3f& normals,
                       const Eigen::MatrixX3i& triangles);

    //=========================================================================================================
    /**
     * Set the surface geometry used for EEG field mapping.
     *
     * @param[in] surfaceKey    The key identifying the surface.
     * @param[in] vertices      Vertex positions (nVerts x 3).
     */
    void setEegSurface(const std::string& surfaceKey,
                       const Eigen::MatrixX3f& vertices);

    //=========================================================================================================
    /**
     * Set the bad channels. Triggers recomputation if changed.
     *
     * @param[in] bads           List of bad channel names.
     */
    void setBadChannels(const std::vector<std::string>& bads);

public:
    //=========================================================================================================
    /**
     * Compute the sensor-to-surface mapping matrices.
     * This slot can be called directly or connected to a signal.
     * The computation reads the stored parameters and builds dense mapping
     * matrices via mne::FwdFieldMap::computeMegMapping() / computeEegMapping().
     */
    void computeMapping();

public:
    //=========================================================================================================
    /**
     * Set the callback for newMegMappingAvailable. Called when new MEG mapping data is available. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (surfaceKey, mappingMat, pick); empty to drop the results.
     */
    void setNewMegMappingAvailableCallback(std::function<void(const std::string&, std::shared_ptr<Eigen::MatrixXf>, const std::vector<int>&)> callback);

    //=========================================================================================================
    /**
     * Set the callback for newEegMappingAvailable. Called when new EEG mapping data is available. It runs on the thread that produced the result.
     *
     * @param[in] callback   Receives (surfaceKey, mappingMat, pick); empty to drop the results.
     */
    void setNewEegMappingAvailableCallback(std::function<void(const std::string&, std::shared_ptr<Eigen::MatrixXf>, const std::vector<int>&)> callback);

private:
    mutable std::mutex m_mutex; /**< Protects all data members. */

    mne::FiffEvoked m_evoked; /**< Evoked data with channel info. */
    bool m_hasEvoked = false; /**< Whether evoked data has been set. */

    mne::FiffCoordTrans m_headToMriTrans; /**< Head-to-MRI transform. */
    bool m_applySensorTrans = true;       /**< Whether to apply sensor transform. */

    bool m_megOnHead = false; /**< Map MEG onto head vs helmet. */

    // MEG target surface
    std::string m_megSurfaceKey;     /**< Key of the MEG target surface. */
    Eigen::MatrixX3f m_megVertices;  /**< MEG surface vertices. */
    Eigen::MatrixX3f m_megNormals;   /**< MEG surface normals. */
    Eigen::MatrixX3i m_megTriangles; /**< MEG surface triangles. */
    bool m_hasMegSurface = false;    /**< Whether MEG surface has been set. */

    // EEG target surface
    std::string m_eegSurfaceKey;    /**< Key of the EEG target surface. */
    Eigen::MatrixX3f m_eegVertices; /**< EEG surface vertices. */
    bool m_hasEegSurface = false;   /**< Whether EEG surface has been set. */

    std::vector<std::string> m_bads; /**< Current bad channel names. */

    std::function<void(const std::string&, std::shared_ptr<Eigen::MatrixXf>, const std::vector<int>&)> m_newMegMappingAvailableCallback; /**< Receives newMegMappingAvailable. */
    std::function<void(const std::string&, std::shared_ptr<Eigen::MatrixXf>, const std::vector<int>&)> m_newEegMappingAvailableCallback; /**< Receives newEegMappingAvailable. */
};

} // namespace mne::viz

#endif // VIZ_RTSENSORINTERPOLATIONMATWORKER_H
