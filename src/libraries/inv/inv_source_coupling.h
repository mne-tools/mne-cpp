//=============================================================================================================
/**
 * @file     inv_source_coupling.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2025
 *
 * @section  LICENSE
 *
 * Copyright (C) 2025, Christoph Dinh. All rights reserved.
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
 * @brief    InvSourceCoupling struct declaration for correlated source groups (e.g. RAP-MUSIC dipole tuples).
 *
 */

#ifndef INV_SOURCE_COUPLING_H
#define INV_SOURCE_COUPLING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_global.h"

#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Describes a group of N coupled (correlated) source grid points found by algorithms such as RAP-MUSIC.
 * The group can be a pair, triplet, or any N-tuple of grid indices together with their orientations and
 * an N x N correlation matrix.
 *
 * @brief N-tuple of correlated grid sources with orientations and correlation matrix.
 */
struct INVSHARED_EXPORT InvSourceCoupling
{
    std::vector<int>        gridIndices;    /**< Grid indices of the coupled sources (size N). */
    std::vector<Eigen::Vector3d> moments;   /**< Dipole moment / orientation for each coupled source (size N). */
    Eigen::MatrixXd         correlations;   /**< N x N correlation matrix between the coupled sources. */
    float                   tmin;           /**< Start of the time window (s) this coupling represents. */
    float                   tmax;           /**< End of the time window (s) this coupling represents. */

    InvSourceCoupling();
};

} // NAMESPACE INVLIB

#endif // INV_SOURCE_COUPLING_H
