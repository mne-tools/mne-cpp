//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_dics.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Dynamic Imaging of Coherent Sources (DICS) beamformer — frequency-domain source-power and source-time-course estimation.
 *
 * @ref INVLIB::InvDICS implements the DICS beamformer of Gross et al.,
 * PNAS 98(2), 694-699 (2001). Where LCMV operates on a time-domain data
 * covariance, DICS designs one spatial filter per frequency bin from
 * the corresponding cross-spectral density (CSD) matrix, yielding
 * narrow-band source-power maps and source time-courses suitable for
 * oscillatory and induced-response analyses. The class exposes
 * @c makeDICS (per-frequency filter design from a CSD stack),
 * @c applyDICSCsd (source-power map per frequency) and the time-domain
 * application helpers. All conventions — including the
 * @c realFilter / complex-CSD handling and the unit-noise-gain
 * normalisation — match mne-python's @c mne.beamformer.make_dics.
 */

#ifndef INV_DICS_H
#define INV_DICS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "../inv_source_estimate.h"
#include "inv_beamformer.h"
#include "inv_beamformer_settings.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

#include <fiff/fiff_cov.h>

#include <QList>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB { class MNEForwardSolution; }
namespace FIFFLIB { class FiffInfo; }

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Dynamic Imaging of Coherent Sources (DICS) beamformer for frequency-domain
 * source localization using cross-spectral density (CSD) matrices.
 *
 * Typical workflow:
 * @code
 *   // 1. Compute CSD matrices for frequency bands of interest (externally)
 *   //    csdMatrices[i] = CSD at frequency[i], shape (n_channels, n_channels)
 *
 *   // 2. Build the DICS spatial filter
 *   InvBeamformer filters = InvDICS::makeDICS(info, forward, csdMatrices, frequencies,
 *                                              reg, realFilter, pickOri, weightNorm);
 *
 *   // 3. Apply to CSD to get source power per frequency
 *   InvSourceEstimate power = InvDICS::applyDICSCsd(csdMatrices, frequencies, filters);
 * @endcode
 *
 * @brief DICS beamformer (frequency-domain).
 */
class INVSHARED_EXPORT InvDICS
{
public:

    //=========================================================================================================
    /**
     * Compute DICS beamformer spatial filters for one or more frequencies.
     *
     * @param[in] info          Measurement info.
     * @param[in] forward       Forward solution containing the leadfield.
     * @param[in] csdMatrices   Cross-spectral density matrices, one per frequency bin.
     *                          Each is (n_channels, n_channels), may be complex.
     * @param[in] frequencies   Center frequency (Hz) for each CSD matrix.
     * @param[in] reg           Regularization parameter (default 0.05).
     * @param[in] realFilter    If true, take real part of CSD before computing filter (default true).
     * @param[in] noiseCov      Noise covariance for whitening. If empty, identity whitening.
     * @param[in] pickOri       Orientation selection mode.
     * @param[in] weightNorm    Weight normalization strategy.
     * @param[in] reduceRank    Reduce leadfield rank by 1.
     * @param[in] invMethod     Denominator inversion method.
     *
     * @return Beamformer with one filter weight matrix per frequency.
     */
    static InvBeamformer makeDICS(const FIFFLIB::FiffInfo &info,
                                  const MNELIB::MNEForwardSolution &forward,
                                  const std::vector<Eigen::MatrixXd> &csdMatrices,
                                  const Eigen::VectorXd &frequencies,
                                  double reg = 0.05,
                                  bool realFilter = true,
                                  const FIFFLIB::FiffCov &noiseCov = FIFFLIB::FiffCov(),
                                  BeamformerPickOri pickOri = BeamformerPickOri::None,
                                  BeamformerWeightNorm weightNorm = BeamformerWeightNorm::UnitNoiseGain,
                                  bool reduceRank = false,
                                  BeamformerInversion invMethod = BeamformerInversion::Matrix);

    //=========================================================================================================
    /**
     * Apply DICS beamformer to CSD matrices to estimate source power per frequency.
     *
     *   power_i(f) = trace(W_f_i @ CSD_f @ W_f_i^T)
     *
     * @param[in] csdMatrices   CSD matrices (one per frequency).
     * @param[in] frequencies   Center frequencies (Hz).
     * @param[in] filters       Pre-computed DICS beamformer from makeDICS().
     *
     * @return Source estimate where data has shape (n_sources, n_freqs) with power values.
     */
    static InvSourceEstimate applyDICSCsd(const std::vector<Eigen::MatrixXd> &csdMatrices,
                                          const Eigen::VectorXd &frequencies,
                                          const InvBeamformer &filters);

    //=========================================================================================================
    /**
     * Apply DICS beamformer to time-domain data at a single frequency.
     *
     * This is an approximate application — for accurate frequency-domain source
     * estimation, use applyDICSCsd() with CSD matrices.
     *
     * @param[in] data      Data matrix (n_channels, n_times).
     * @param[in] tmin      Start time (s).
     * @param[in] tstep     Time step (s).
     * @param[in] filters   Pre-computed DICS beamformer (must have exactly 1 frequency).
     * @param[in] freqIdx   Index of the frequency filter to use (default 0).
     *
     * @return Source time-course estimate.
     */
    static InvSourceEstimate applyDICS(const Eigen::MatrixXd &data,
                                       float tmin,
                                       float tstep,
                                       const InvBeamformer &filters,
                                       int freqIdx = 0);

    //=========================================================================================================
    /**
     * Apply DICS beamformer to each epoch in a list.
     *
     * @param[in] epochs    List of epoch data matrices (n_channels x n_times each).
     * @param[in] tmin      Start time of each epoch (seconds).
     * @param[in] tstep     Time step (1/sfreq).
     * @param[in] filters   Pre-computed DICS beamformer from makeDICS().
     * @param[in] freqIdx   Index of the frequency filter to use (default 0).
     *
     * @return List of source estimates, one per epoch.
     */
    static QList<InvSourceEstimate> applyDICSEpochs(const QList<Eigen::MatrixXd> &epochs,
                                                     float tmin,
                                                     float tstep,
                                                     const InvBeamformer &filters,
                                                     int freqIdx = 0);
};

} // NAMESPACE INVLIB

#endif // INV_DICS_H
