//=============================================================================================================
/**
 * @file     inv_vector_source_estimate.h
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
 * @brief    InvVectorSourceEstimate — free-orientation surface source estimate (3 values per vertex).
 */

#ifndef INV_VECTOR_SOURCE_ESTIMATE_H
#define INV_VECTOR_SOURCE_ESTIMATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_source_estimate.h"

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * @brief Vector source estimate: each vertex carries a 3D orientation vector across time.
 *
 * data has shape (n_vertices*3 x n_times). Rows are interleaved: [x0,y0,z0, x1,y1,z1, ...].
 * Use magnitude() to collapse to scalar (n_vertices x n_times) for visualisation.
 */
class INVSHARED_EXPORT InvVectorSourceEstimate : public InvSourceEstimate
{
public:
    typedef QSharedPointer<InvVectorSourceEstimate> SPtr;
    typedef QSharedPointer<const InvVectorSourceEstimate> ConstSPtr;

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

} // namespace INVLIB

#endif // INV_VECTOR_SOURCE_ESTIMATE_H
