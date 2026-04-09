//=============================================================================================================
/**
 * @file     inv_beamformer_compute.h
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
 * @brief    Shared beamformer computation routines for LCMV and DICS.
 *
 * Core mathematical formulations:
 *   Unit-gain filter:   W_ug = [G^T Cm^{-1} G]^{-1} G^T Cm^{-1}
 *   Unit-noise-gain:    W = W_ug / sqrt(diag(W_ug W_ug^T))
 *   NAI:                W = W_ung / sqrt(noise_level)
 *
 * References:
 *   Van Veen et al., IEEE Trans. Biomed. Eng. 44(9), 867-880, 1997.
 *   Sekihara & Nagarajan, Adaptive Spatial Filters, Springer, 2008.
 *   Gross & Ioannides, Phys. Med. Biol. 44, 2081-2097, 1999.
 */

#ifndef INV_BEAMFORMER_COMPUTE_H
#define INV_BEAMFORMER_COMPUTE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "inv_beamformer_settings.h"

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
 * Shared computation routines used by both LCMV and DICS beamformers.
 *
 * All methods are static — no state is kept between calls.
 *
 * @brief Core beamformer math.
 */
class INVSHARED_EXPORT InvBeamformerCompute
{
public:

    //=========================================================================================================
    /**
     * Compute beamformer spatial filter weights from a leadfield and data covariance (or CSD).
     *
     * This implements the core LCMV/DICS algorithm:
     *   1. Regularize and invert covariance
     *   2. Reshape leadfield per source (n_channels x n_orient)
     *   3. Optionally reduce leadfield rank
     *   4. Select orientation (max-power, normal, or keep all)
     *   5. Invert denominator G^T Cm^{-1} G
     *   6. Apply weight normalization
     *
     * @param[in] G             Leadfield matrix (n_channels, n_sources * n_orient).
     * @param[in] Cm            Data covariance or CSD matrix (n_channels, n_channels). Real-valued.
     * @param[in] reg           Regularization parameter (fraction of trace to add, e.g. 0.05).
     * @param[in] nOrient       Orientations per source: 1 (fixed) or 3 (free).
     * @param[in] weightNorm    Weight normalization strategy.
     * @param[in] pickOri       Orientation selection mode.
     * @param[in] reduceRank    If true, reduce leadfield rank by 1 (for MEG sphere models).
     * @param[in] invMethod     Denominator inversion strategy (matrix or scalar).
     * @param[in] nn            Source normals (n_sources, 3). Used for max-power sign alignment.
     * @param[out] W            Output spatial filter weights (n_sources * n_orient_out, n_channels).
     * @param[out] maxPowerOri  Output max-power orientations (n_sources, 3). Empty if not max-power.
     *
     * @return True on success.
     */
    static bool computeBeamformer(const Eigen::MatrixXd &G,
                                  const Eigen::MatrixXd &Cm,
                                  double reg,
                                  int nOrient,
                                  BeamformerWeightNorm weightNorm,
                                  BeamformerPickOri pickOri,
                                  bool reduceRank,
                                  BeamformerInversion invMethod,
                                  const Eigen::MatrixX3d &nn,
                                  Eigen::MatrixXd &W,
                                  Eigen::MatrixX3d &maxPowerOri);

    //=========================================================================================================
    /**
     * Compute source power from spatial filter weights and a data covariance / CSD matrix.
     *
     *   power_i = trace(W_i Cm W_i^T)
     *
     * @param[in] Cm        Data covariance or CSD (n_channels, n_channels).
     * @param[in] W         Spatial filter weights (n_sources * n_orient, n_channels).
     * @param[in] nOrient   Orientations per source.
     *
     * @return Source power vector (n_sources).
     */
    static Eigen::VectorXd computePower(const Eigen::MatrixXd &Cm,
                                        const Eigen::MatrixXd &W,
                                        int nOrient);

    //=========================================================================================================
    /**
     * Symmetric matrix power: X^p via eigendecomposition.
     *
     * @param[in] X         Symmetric matrix.
     * @param[in] p         Power exponent (e.g. -1, -0.5).
     * @param[in] reduceRank  Drop smallest eigenvalue before exponentiating.
     *
     * @return X^p.
     */
    static Eigen::MatrixXd symMatPow(const Eigen::MatrixXd &X, double p, bool reduceRank = false);

private:
    //=========================================================================================================
    /**
     * Regularized pseudo-inverse of a symmetric matrix via eigendecomposition.
     *
     *   C_reg = C + reg * trace(C) / rank(C) * I
     *   C_inv = V diag(1 / max(eig, threshold)) V^T
     *
     * @param[in] C             Symmetric positive semi-definite matrix.
     * @param[in] reg           Regularization fraction.
     * @param[out] CInv         Regularized inverse.
     * @param[out] loadingFactor  The noise floor added by regularization.
     * @param[out] rankOut      The detected rank.
     */
    static void regPinv(const Eigen::MatrixXd &C,
                        double reg,
                        Eigen::MatrixXd &CInv,
                        double &loadingFactor,
                        int &rankOut);

    //=========================================================================================================
    /**
     * Reduce the rank of a per-source leadfield block by removing the smallest singular component.
     *
     * Used for rank-deficient forward models (e.g., MEG spherical models where the radial
     * component carries no information).
     *
     * @param[in,out] Gk   Leadfield block (n_channels, n_orient). Modified in place.
     */
    static void reduceLeadfieldRank(Eigen::MatrixXd &Gk);
};

} // NAMESPACE INVLIB

#endif // INV_BEAMFORMER_COMPUTE_H
