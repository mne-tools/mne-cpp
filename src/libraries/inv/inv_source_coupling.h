//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_source_coupling.h
 * @since 2026
 * @date  April 2026
 * @brief Container for an N-tuple of correlated grid sources discovered by RAP-/TRAP-MUSIC.
 *
 * Sub-space scanning algorithms (RAP-MUSIC, TRAP-MUSIC, PWL-RAP-MUSIC)
 * return groups of N coupled dipoles together with their mutual
 * correlation. @ref INVLIB::InvSourceCoupling stores the grid indices,
 * the per-source dipole moments and the @c N×N correlation matrix for one
 * such tuple, plus the time window over which the coupling was evaluated.
 * An @ref InvSourceEstimate may carry many of these (one per iteration of
 * the scan) so multi-dipole solutions remain interpretable downstream.
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
