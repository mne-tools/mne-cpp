//=============================================================================================================
/**
 * @file     inv_lcmv.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.1.0
 * @date     March, 2026
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
 * @brief    InvLCMV class declaration — Linearly Constrained Minimum Variance beamformer.
 *
 * References:
 *   Van Veen et al., IEEE Trans. Biomed. Eng. 44(9), 867-880, 1997.
 *   Sekihara & Nagarajan, Adaptive Spatial Filters, Springer, 2008.
 */

#ifndef INV_LCMV_H
#define INV_LCMV_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "../inv_source_estimate.h"
#include "inv_beamformer.h"
#include "inv_beamformer_settings.h"

#include <fiff/fiff_cov.h>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB { class MNEForwardSolution; }
namespace FIFFLIB { class FiffEvoked; class FiffInfo; }

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Linearly Constrained Minimum Variance (LCMV) beamformer for MEG/EEG source localization.
 *
 * Typical workflow:
 * @code
 *   // 1. Compute the spatial filter from forward model + data covariance
 *   InvBeamformer filters = InvLCMV::makeLCMV(info, forward, dataCov, noiseCov, reg,
 *                                              pickOri, weightNorm, reduceRank, inversion);
 *
 *   // 2. Apply to evoked data -> source time courses
 *   InvSourceEstimate stc = InvLCMV::applyLCMV(evoked, filters);
 *
 *   // 3. Or apply to covariance -> source power map
 *   InvSourceEstimate power = InvLCMV::applyLCMVCov(dataCov, filters);
 * @endcode
 *
 * @brief LCMV beamformer (time-domain).
 */
class INVSHARED_EXPORT InvLCMV
{
public:

    //=========================================================================================================
    /**
     * Compute LCMV beamformer spatial filters.
     *
     * @param[in] info          Measurement info (for channel matching).
     * @param[in] forward       Forward solution containing the leadfield.
     * @param[in] dataCov       Data covariance matrix (from recording epoch).
     * @param[in] reg           Regularization parameter (default 0.05 = 5% of trace).
     * @param[in] noiseCov      Noise covariance for whitening. If empty, identity whitening.
     * @param[in] pickOri       Orientation selection mode (default: None = keep all).
     * @param[in] weightNorm    Weight normalization (default: UnitNoiseGain).
     * @param[in] reduceRank    Reduce leadfield rank by 1 (default: false).
     * @param[in] invMethod     Inversion method for denominator (default: Matrix).
     *
     * @return The computed beamformer containing spatial filter weights.
     */
    static InvBeamformer makeLCMV(const FIFFLIB::FiffInfo &info,
                                  const MNELIB::MNEForwardSolution &forward,
                                  const FIFFLIB::FiffCov &dataCov,
                                  double reg = 0.05,
                                  const FIFFLIB::FiffCov &noiseCov = FIFFLIB::FiffCov(),
                                  BeamformerPickOri pickOri = BeamformerPickOri::None,
                                  BeamformerWeightNorm weightNorm = BeamformerWeightNorm::UnitNoiseGain,
                                  bool reduceRank = false,
                                  BeamformerInversion invMethod = BeamformerInversion::Matrix);

    //=========================================================================================================
    /**
     * Apply LCMV beamformer to evoked data to produce source time courses.
     *
     *   stc = W @ (whitener @ proj @ evoked.data)
     *
     * @param[in] evoked    Evoked data (averaged, channel x time).
     * @param[in] filters   Pre-computed LCMV beamformer from makeLCMV().
     *
     * @return Source estimate with time courses at each source point.
     */
    static InvSourceEstimate applyLCMV(const FIFFLIB::FiffEvoked &evoked,
                                       const InvBeamformer &filters);

    //=========================================================================================================
    /**
     * Apply LCMV beamformer to raw data matrix.
     *
     * @param[in] data      Data matrix (n_channels, n_times).
     * @param[in] tmin      Start time in seconds.
     * @param[in] tstep     Time step in seconds.
     * @param[in] filters   Pre-computed LCMV beamformer from makeLCMV().
     *
     * @return Source estimate with time courses.
     */
    static InvSourceEstimate applyLCMVRaw(const Eigen::MatrixXd &data,
                                          float tmin,
                                          float tstep,
                                          const InvBeamformer &filters);

    //=========================================================================================================
    /**
     * Apply LCMV beamformer to a data covariance to produce a source power map.
     *
     *   power_i = trace(W_i @ Cm @ W_i^T)
     *
     * @param[in] dataCov   Data covariance matrix.
     * @param[in] filters   Pre-computed LCMV beamformer from makeLCMV().
     *
     * @return Source estimate where data column 0 contains power at each source.
     */
    static InvSourceEstimate applyLCMVCov(const FIFFLIB::FiffCov &dataCov,
                                          const InvBeamformer &filters);

private:
    /**
     * Apply whitening and projection to data, then project through spatial filter.
     */
    static Eigen::MatrixXd applyFilter(const Eigen::MatrixXd &data,
                                       const InvBeamformer &filters);
};

} // NAMESPACE INVLIB

#endif // INV_LCMV_H
