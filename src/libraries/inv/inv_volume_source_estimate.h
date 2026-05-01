//=============================================================================================================
/**
 * @file     inv_volume_source_estimate.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    InvVolumeSourceEstimate — volume (voxel-based) source estimate.
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
