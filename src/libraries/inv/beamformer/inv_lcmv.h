//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_lcmv.h
 * @since 2026
 * @date  May 2026
 * @brief Linearly Constrained Minimum Variance (LCMV) beamformer — time-domain source-power and source-time-course estimation.
 *
 * @ref INVLIB::InvLCMV implements the LCMV spatial filter of Van Veen
 * et al., IEEE TBME 44(9), 867-880 (1997): it inverts a regularised
 * data-covariance matrix and constrains the resulting filter so that the
 * forward field of every grid point passes through with unit gain and
 * all other sources are suppressed in the minimum-variance sense. The
 * class exposes @c makeLCMV (filter design from forward solution + data
 * covariance), @c applyLCMV / @c applyLCMVRaw / @c applyLCMVEpochs
 * (time-course projection) and @c applyLCMVCov (source-power map from a
 * covariance). A @c makeLCMVResolutionMatrix helper feeds the resolution
 * analysis pipeline. All normalisation conventions follow Sekihara &amp;
 * Nagarajan (Springer, 2008) so output matches mne-python's
 * @c mne.beamformer.make_lcmv.
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

#include <QList>

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

    //=========================================================================================================
    /**
     * Apply LCMV beamformer to a list of epochs.
     *
     * @param[in] epochs    List of epoch data matrices (n_channels x n_times each).
     * @param[in] tmin      Start time of each epoch (seconds).
     * @param[in] tstep     Time step (1/sfreq).
     * @param[in] filters   Pre-computed LCMV beamformer from makeLCMV().
     *
     * @return List of source estimates, one per epoch.
     */
    static QList<InvSourceEstimate> applyLCMVEpochs(const QList<Eigen::MatrixXd> &epochs,
                                                     float tmin,
                                                     float tstep,
                                                     const InvBeamformer &filters);

    //=========================================================================================================
    /**
     * Compute the LCMV resolution matrix: R = W @ G.
     *
     * The resolution matrix describes the point-spread function of the beamformer.
     * Each row shows how a unit source at one location maps through the forward
     * model and back through the beamformer. Ideally R = I (perfect resolution).
     *
     * @param[in] forward   Forward solution with leadfield.
     * @param[in] info      Measurement info.
     * @param[in] dataCov   Data covariance.
     * @param[in] reg       Regularization (default 0.05).
     * @param[in] noiseCov  Noise covariance for whitening (optional).
     *
     * @return Resolution matrix (n_sources x n_sources).
     */
    static Eigen::MatrixXd makeLCMVResolutionMatrix(
        const MNELIB::MNEForwardSolution &forward,
        const FIFFLIB::FiffInfo &info,
        const FIFFLIB::FiffCov &dataCov,
        double reg = 0.05,
        const FIFFLIB::FiffCov &noiseCov = FIFFLIB::FiffCov());

private:
    /**
     * Apply whitening and projection to data, then project through spatial filter.
     */
    static Eigen::MatrixXd applyFilter(const Eigen::MatrixXd &data,
                                       const InvBeamformer &filters);
};

} // NAMESPACE INVLIB

#endif // INV_LCMV_H
