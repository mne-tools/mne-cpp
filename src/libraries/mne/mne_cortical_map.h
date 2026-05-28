//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_cortical_map.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Vertex-to-vertex map between two cortical surfaces (e.g. subject -> fsaverage).
 *
 * @ref MNELIB::MNECorticalMap stores the resampling weights used by
 * FreeSurfer's surface-based morphing: for each target-surface vertex,
 * the small set of source-surface vertices and barycentric coefficients
 * that produce its value. Read from @c .map files and assembled into a
 * sparse linear morph by @ref MNEMorphMap.
 */

#ifndef MNE_CORTICAL_MAP_H
#define MNE_CORTICAL_MAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MNEForwardSolution;
class MNEInverseOperator;

//=============================================================================================================
/**
 * Cortical map interpolation: creates a mapping matrix that transforms
 * sensor-level data to source space using forward and inverse operators.
 *
 * @brief Cortical map interpolation utilities
 *
 * @since 2.2.0
 */
class MNESHARED_EXPORT MNECorticalMap
{
public:
    //=========================================================================================================
    /**
     * Create a cortical mapping matrix that transforms sensor-level data
     * to source space.
     *
     * The mapping is computed as: M = inv_kernel * fwd_gain^T
     *
     * where inv_kernel is the inverse operator kernel and fwd_gain is
     * the forward solution gain matrix.
     *
     * @param[in] fwd    The forward solution containing the gain matrix.
     * @param[in] inv    The inverse operator (must have been prepared / have a kernel).
     * @param[in] info   The measurement info for channel selection.
     *
     * @return The cortical mapping matrix (nSources x nSources).
     */
    static Eigen::MatrixXd makeCorticalMap(
        const MNEForwardSolution& fwd,
        const MNEInverseOperator& inv,
        const FIFFLIB::FiffInfo& info);

    MNECorticalMap() = delete;
};

} // namespace MNELIB

#endif // MNE_CORTICAL_MAP_H
