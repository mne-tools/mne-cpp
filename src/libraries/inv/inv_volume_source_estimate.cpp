//=============================================================================================================
/**
 * @file     inv_volume_source_estimate.cpp
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
 * @brief    InvVolumeSourceEstimate class implementation.
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
