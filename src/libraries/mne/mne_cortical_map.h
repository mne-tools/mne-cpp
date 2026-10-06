//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_cortical_map.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Resolution matrix K G of an inverse operator and its forward solution.
 *
 * @ref MNELIB::MNECorticalMap::makeCorticalMap multiplies the prepared inverse
 * kernel K by the forward gain G of the kernel's channels, giving the map from
 * true to estimated source activity (mne-python make_inverse_resolution_matrix).
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

namespace FIFFLIB
{
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
 * Builds the resolution matrix K G that maps source activity to its estimate.
 *
 * @brief Resolution matrix of an inverse operator
 *
 * @since 2.2.0
 *
 * @snippet ex_inv_api/main.cpp mne_cortical_map_usage
 */
class MNESHARED_EXPORT MNECorticalMap
{
public:
    //=========================================================================================================
    /**
     * Create the resolution matrix M = K G that maps source activity to its estimate.
     *
     * K is the inverse operator kernel; G holds the rows of the forward gain for the
     * kernel's channels, matched by name (mne-python make_inverse_resolution_matrix).
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
