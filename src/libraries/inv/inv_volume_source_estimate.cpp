//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_volume_source_estimate.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of @ref INVLIB::InvVolumeSourceEstimate.
 *
 * Implements the shape setter, the inactive-voxel-zero-fill volume
 * reconstruction at a given time index, and the activity-weighted
 * centre-of-mass computation that drives the MRI viewer's
 * @em focus-on-activity behaviour.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_volume_source_estimate.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvVolumeSourceEstimate::InvVolumeSourceEstimate()
: InvSourceEstimate()
{
    sourceSpaceType = InvSourceSpaceType::Volume;
}

//=============================================================================================================

InvVolumeSourceEstimate::InvVolumeSourceEstimate(const MatrixXd& p_sol,
                                                   const VectorXi& p_vertices,
                                                   float p_tmin, float p_tstep)
: InvSourceEstimate(p_sol, p_vertices, p_tmin, p_tstep)
{
    sourceSpaceType = InvSourceSpaceType::Volume;
}

//=============================================================================================================

void InvVolumeSourceEstimate::setShape(const QVector<int>& shape)
{
    if (shape.size() == 3)
        m_shape = shape;
}

//=============================================================================================================

VectorXd InvVolumeSourceEstimate::toVolume(int timeIdx) const
{
    if (!hasShape() || timeIdx < 0 || timeIdx >= static_cast<int>(data.cols()))
        return VectorXd();

    int nTotal = m_shape[0] * m_shape[1] * m_shape[2];
    VectorXd vol = VectorXd::Zero(nTotal);

    for (int i = 0; i < vertices.size(); ++i) {
        int vIdx = vertices[i];
        if (vIdx >= 0 && vIdx < nTotal)
            vol[vIdx] = data(i, timeIdx);
    }

    return vol;
}

//=============================================================================================================

Vector3d InvVolumeSourceEstimate::centreOfMass(int timeIdx) const
{
    if (!hasShape() || timeIdx < 0 || timeIdx >= static_cast<int>(data.cols()))
        return Vector3d::Zero();

    int ny = m_shape[1];
    int nz = m_shape[2];

    double totalWeight = 0.0;
    Vector3d com = Vector3d::Zero();

    for (int i = 0; i < vertices.size(); ++i) {
        double w = std::abs(data(i, timeIdx));
        if (w < 1e-15)
            continue;

        int vIdx = vertices[i];
        int ix = vIdx / (ny * nz);
        int iy = (vIdx % (ny * nz)) / nz;
        int iz = vIdx % nz;

        com += w * Vector3d(ix, iy, iz);
        totalWeight += w;
    }

    if (totalWeight > 1e-15)
        com /= totalWeight;

    return com;
}
