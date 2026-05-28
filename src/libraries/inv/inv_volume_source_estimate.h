//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_volume_source_estimate.h
 * @since 2026
 * @date  May 2026
 * @brief Volume (voxel-grid) source estimate that augments @ref InvSourceEstimate with 3-D grid shape information.
 *
 * @ref INVLIB::InvVolumeSourceEstimate represents an inverse solution
 * on a regular voxel grid — used by volume MNE, beamformers in volume
 * mode and Gamma-MAP on volumetric source spaces. In addition to the
 * inherited vertex/data/time fields it stores an @c (nx, ny, nz) grid
 * shape and exposes @ref toVolume to materialise a dense 3-D volume at
 * one time index (filling inactive voxels with zero) and
 * @ref centreOfMass for a quick activity centroid — both feeding directly
 * into the @c MRILIB volume viewers.
 */

#ifndef INV_VOLUME_SOURCE_ESTIMATE_H
#define INV_VOLUME_SOURCE_ESTIMATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_source_estimate.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * @brief Volume source estimate: scalar values at voxel locations.
 *
 * Extends InvSourceEstimate with grid shape information to allow reshaping
 * the flat data into a 3D volume.
 */
class INVSHARED_EXPORT InvVolumeSourceEstimate : public InvSourceEstimate
{
public:
    typedef QSharedPointer<InvVolumeSourceEstimate> SPtr;
    typedef QSharedPointer<const InvVolumeSourceEstimate> ConstSPtr;

    //=========================================================================================================
    InvVolumeSourceEstimate();

    //=========================================================================================================
    /**
     * Construct from data and vertices.
     *
     * @param[in] p_sol       Data (n_voxels x n_times).
     * @param[in] p_vertices  Voxel indices.
     * @param[in] p_tmin      Start time.
     * @param[in] p_tstep     Time step.
     */
    InvVolumeSourceEstimate(const Eigen::MatrixXd& p_sol,
                             const Eigen::VectorXi& p_vertices,
                             float p_tmin, float p_tstep);

    //=========================================================================================================
    /**
     * Set the 3D grid shape (nx, ny, nz) for volume reshaping.
     *
     * @param[in] shape  Grid dimensions {nx, ny, nz}.
     */
    void setShape(const QVector<int>& shape);

    //=========================================================================================================
    /**
     * Get the 3D grid shape.
     */
    const QVector<int>& shape() const { return m_shape; }

    //=========================================================================================================
    /**
     * Check whether the grid shape has been set.
     */
    bool hasShape() const { return m_shape.size() == 3; }

    //=========================================================================================================
    /**
     * Extract a 3D volume for a given time point, filling the grid with zeros
     * where no source is active.
     *
     * @param[in] timeIdx  Time sample index.
     *
     * @return Flattened volume (nx*ny*nz) with values at active voxels.
     */
    Eigen::VectorXd toVolume(int timeIdx) const;

    //=========================================================================================================
    /**
     * Compute the centre of mass of the activity at a given time point.
     *
     * @param[in] timeIdx  Time sample index.
     *
     * @return 3D position (x, y, z) in voxel coordinates, or (0,0,0) if no activity.
     */
    Eigen::Vector3d centreOfMass(int timeIdx) const;

private:
    QVector<int> m_shape;  // {nx, ny, nz}
};

} // namespace INVLIB

#endif // INV_VOLUME_SOURCE_ESTIMATE_H
