//=============================================================================================================
/**
 * @file     inv_vector_source_estimate.cpp
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
 * @brief    InvVectorSourceEstimate class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_vector_source_estimate.h"

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvVectorSourceEstimate::InvVectorSourceEstimate()
: InvSourceEstimate()
{
    orientationType = InvOrientationType::Free;
}

//=============================================================================================================

InvVectorSourceEstimate::InvVectorSourceEstimate(const MatrixXd& p_sol,
                                                   const VectorXi& p_vertices,
                                                   float p_tmin, float p_tstep)
: InvSourceEstimate(p_sol, p_vertices, p_tmin, p_tstep)
{
    orientationType = InvOrientationType::Free;
}

//=============================================================================================================

int InvVectorSourceEstimate::nVertices() const
{
    return static_cast<int>(data.rows()) / 3;
}

//=============================================================================================================

InvSourceEstimate InvVectorSourceEstimate::magnitude() const
{
    const int nVert = nVertices();
    const int nTimes = static_cast<int>(data.cols());

    MatrixXd mag(nVert, nTimes);
    for (int v = 0; v < nVert; ++v) {
        for (int t = 0; t < nTimes; ++t) {
            double x = data(v * 3, t);
            double y = data(v * 3 + 1, t);
            double z = data(v * 3 + 2, t);
            mag(v, t) = std::sqrt(x * x + y * y + z * z);
        }
    }

    return InvSourceEstimate(mag, vertices, tmin, tstep);
}

//=============================================================================================================

MatrixXd InvVectorSourceEstimate::vertexData(int vertexIdx) const
{
    if (vertexIdx < 0 || vertexIdx >= nVertices())
        return MatrixXd();

    return data.middleRows(vertexIdx * 3, 3);
}

//=============================================================================================================

InvSourceEstimate InvVectorSourceEstimate::projectToNormals(const MatrixX3f& normals) const
{
    const int nVert = nVertices();
    const int nTimes = static_cast<int>(data.cols());

    if (normals.rows() != nVert)
        return InvSourceEstimate();

    MatrixXd proj(nVert, nTimes);
    for (int v = 0; v < nVert; ++v) {
        Vector3d n(normals(v, 0), normals(v, 1), normals(v, 2));
        n.normalize();
        for (int t = 0; t < nTimes; ++t) {
            proj(v, t) = n[0] * data(v * 3, t) + n[1] * data(v * 3 + 1, t) + n[2] * data(v * 3 + 2, t);
        }
    }

    return InvSourceEstimate(proj, vertices, tmin, tstep);
}
