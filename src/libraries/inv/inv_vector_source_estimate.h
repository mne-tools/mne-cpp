//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_vector_source_estimate.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Free-orientation surface source estimate carrying a 3-component vector per vertex.
 *
 * @ref mne::InvVectorSourceEstimate extends
 * @ref mne::InvSourceEstimate so that the data matrix holds an interleaved
 * @c (x,y,z) triplet per surface vertex — the output of free- or loose-
 * orientation MNE / dSPM solutions before any orientation pooling. The
 * class adds the @ref mne::InvVectorSourceEstimate::magnitude "magnitude" collapse to scalar magnitude, the
 * @ref mne::InvVectorSourceEstimate::projectToNormals "projectToNormals" signed-projection helper and per-vertex
 * @ref mne::InvVectorSourceEstimate::vertexData "vertexData" access, mirroring mne-python's
 * @c VectorSourceEstimate.
 */

#ifndef INV_VECTOR_SOURCE_ESTIMATE_H
#define INV_VECTOR_SOURCE_ESTIMATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_source_estimate.h"

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Vector source estimate: each vertex carries a 3D orientation vector across time.
 *
 * data has shape (n_vertices*3 x n_times). Rows are interleaved: [x0,y0,z0, x1,y1,z1, ...].
 * Use magnitude() to collapse to scalar (n_vertices x n_times) for visualisation.
 *
 * @snippet ex_inv_api/main.cpp inv_vector_source_estimate_usage
 */
class INVSHARED_EXPORT InvVectorSourceEstimate : public InvSourceEstimate
{
public:
    //=========================================================================================================
    InvVectorSourceEstimate();

    //=========================================================================================================
    /**
     * Construct from data and vertices.
     *
     * @param[in] p_sol       Data (n_vertices*3 x n_times).
     * @param[in] p_vertices  Vertex indices (n_vertices).
     * @param[in] p_tmin      Start time.
     * @param[in] p_tstep     Time step.
     */
    InvVectorSourceEstimate(const Eigen::MatrixXd& p_sol,
                            const Eigen::VectorXi& p_vertices,
                            float p_tmin, float p_tstep);

    //=========================================================================================================
    /**
     * Number of source vertices (data.rows() / 3).
     *
     * @return Number of vertices, each carrying an x/y/z component row.
     */
    int nVertices() const;

    //=========================================================================================================
    /**
     * Compute the magnitude (L2 norm) at each vertex across time.
     *
     * @return Scalar source estimate (n_vertices x n_times).
     */
    InvSourceEstimate magnitude() const;

    //=========================================================================================================
    /**
     * Extract the (x, y, z) data for a specific vertex.
     *
     * @param[in] vertexIdx  Local index into vertices array.
     *
     * @return Matrix (3 x n_times) for the requested vertex.
     */
    Eigen::MatrixXd vertexData(int vertexIdx) const;

    //=========================================================================================================
    /**
     * Project the 3D vectors onto surface normals to produce a signed scalar estimate.
     *
     * @param[in] normals  Surface normals (n_vertices x 3).
     *
     * @return Scalar source estimate (n_vertices x n_times).
     */
    InvSourceEstimate projectToNormals(const Eigen::MatrixX3f& normals) const;
};

} // namespace mne

#endif // INV_VECTOR_SOURCE_ESTIMATE_H
