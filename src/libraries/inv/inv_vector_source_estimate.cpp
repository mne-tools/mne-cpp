//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_vector_source_estimate.cpp
 * @since 2026
 * @date  May 2026
 * @brief Implementation of @ref INVLIB::InvVectorSourceEstimate.
 *
 * Implements the constructors, the L2-norm magnitude collapse, the
 * signed projection onto surface normals and the per-vertex slice
 * accessor. All operations work directly on the interleaved
 * @c (3·n_vertices × n_times) data block held by the base
 * @ref InvSourceEstimate.
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
