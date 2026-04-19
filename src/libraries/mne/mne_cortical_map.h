//=============================================================================================================
/**
 * @file     mne_cortical_map.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief     MNECorticalMap class declaration.
 *
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
