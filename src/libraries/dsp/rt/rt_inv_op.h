//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_inv_op.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Streaming minimum-norm inverse that follows each new noise covariance at little cost.
 *
 * RtInvOp maps streaming MEG / EEG data to source estimates with the MNE, dSPM or sLORETA inverse of
 * @ref mne::MNEInverseOperator and @ref mne::InvMinimumNorm, and follows each new noise covariance, e.g.
 * from @ref mne::RtCov. Everything that depends only on the forward solution (channel selection, depth
 * and orientation priors, the source-weighted gain covariance G·R·Gᵀ) is computed once per channel set.
 * A new covariance then costs the whitener and one eigendecomposition of a channels × channels matrix
 * instead of the SVD of the whitened gain, plus, for dSPM and sLORETA, one channels × channels by
 * channels × sources product for the noise normalization. The kernel is applied in factored form, so it
 * is never assembled. It is synchronous: call it from whichever thread suits the application.
 */

#ifndef RT_INV_OP_RTPROCESSING_H
#define RT_INV_OP_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"

#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <inv/inv_types.h>
#include <mne/mne_forward_solution.h>

#include <Eigen/Core>

#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Minimum-norm source estimates of streaming data with an operator that follows the noise covariance.
 *
 * The estimates equal @ref mne::InvMinimumNorm::calculateInverse of an operator built by
 * @ref mne::MNEInverseOperator with the same covariance: one row per source, for a free-orientation forward
 * solution either the component normal to the cortex or the magnitude of the current.
 *
 * @snippet ex_dsp_rt/main.cpp rt_inv_op_usage
 */
class DSPSHARED_EXPORT RtInvOp
{
public:
    /** @brief Inverse method and regularization. */
    struct Settings
    {
        InvEstimateMethod method = InvEstimateMethod::dSPM; /**< MNE, dSPM or sLORETA. */
        float lambda2 = 1.0f / 9.0f;                        /**< Regularization, 1 / SNR². */
        float loose = 0.2f;                                 /**< Weight of the tangential source components. */
        float depth = 0.8f;                                 /**< Depth weighting exponent; 0 disables it. */
        bool pick_normal = true;                            /**< Normal component instead of the magnitude. */
    };

    //=========================================================================================================
    /**
     * Uses dSPM with the default regularization.
     *
     * @param[in] info       Measurement info of the data; its bad channels are left out.
     * @param[in] forward    Forward solution, e.g. MEG only via pick_types.
     */
    RtInvOp(const FiffInfo& info, MNEForwardSolution forward);

    //=========================================================================================================
    /**
     * @param[in] info       Measurement info of the data; its bad channels are left out.
     * @param[in] forward    Forward solution, e.g. MEG only via pick_types.
     * @param[in] settings   Inverse method and regularization.
     */
    RtInvOp(const FiffInfo& info, MNEForwardSolution forward, Settings settings);

    //=========================================================================================================
    /**
     * Rebuilds the operator for a new noise covariance.
     *
     * @param[in] noiseCov   Noise covariance of the data.
     *
     * @return False, with the previous operator kept, if the method is not supported or no channel is in
     *         the measurement info, the covariance and the forward solution.
     */
    [[nodiscard]] bool update(const FiffCov& noiseCov);

    //=========================================================================================================
    /**
     * @return Whether an operator was built, so that apply() gives estimates.
     */
    [[nodiscard]] bool ready() const;

    //=========================================================================================================
    /**
     * @param[in] data   All channels of the measurement info by samples.
     * @param[in] nave   Number of averages in the data; scales the dSPM and sLORETA noise normalization.
     *
     * @return The source estimates, sources by samples; empty before an operator was built or if the
     *         data has another channel count.
     */
    [[nodiscard]] Eigen::MatrixXd apply(const Eigen::MatrixXd& data, int nave = 1) const;

    //=========================================================================================================
    /**
     * @return The noise covariance of the operator's channels, projected and decomposed by
     *         @ref mne::FiffCov::prepare_noise_cov; empty before an operator was built.
     */
    [[nodiscard]] const FiffCov& noise_covariance() const;

    //=========================================================================================================
    /**
     * @return The gain of the operator's channels, channels by source components, in the orientation of the
     *         source covariance.
     */
    [[nodiscard]] const Eigen::MatrixXd& gain() const;

    //=========================================================================================================
    /**
     * @return The diagonal of the source covariance, depth and orientation priors scaled like
     *         @ref mne::MNEInverseOperator::source_cov; empty before an operator was built.
     */
    [[nodiscard]] Eigen::VectorXd source_covariance() const;

private:
    void prepare_channels(const StringList& names);

    FiffInfo m_info;              /**< Measurement info of the data. */
    MNEForwardSolution m_forward; /**< Forward solution, surface-oriented for a loose constraint. */
    Settings m_settings;          /**< Inverse method and regularization. */
    bool m_fixed = false;         /**< Whether the forward solution has fixed orientations. */
    Eigen::VectorXd m_orient;     /**< Orientation prior of each source component. */

    // Depend on the channel set only
    StringList m_names;            /**< Channels of the operator. */
    std::vector<int> m_picks;      /**< Their rows in the data. */
    Eigen::MatrixXd m_gain;        /**< Gain G of these channels, channels by source components. */
    Eigen::VectorXd m_sourcePrior; /**< Depth times orientation prior R₀ of each source component. */
    Eigen::MatrixXd m_gainCov;     /**< G·R₀·Gᵀ. */
    Eigen::MatrixXd m_outputLeads; /**< R₀·Gᵀ of the estimated components, transposed: channels by components. */

    // Depend on the noise covariance too
    FiffCov m_noiseCov;          /**< Prepared noise covariance. */
    double m_scale = 0.0;        /**< Scale s of the source prior. */
    Eigen::MatrixXd m_kernel;    /**< Channels by channels part of the factored kernel. */
    Eigen::VectorXd m_noiseNorm; /**< Inverse noise normalization of each source; empty for MNE. */
};

} // namespace mne

#endif // RT_INV_OP_RTPROCESSING_H
