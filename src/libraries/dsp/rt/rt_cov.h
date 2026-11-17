//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_cov.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Real-time noise covariance estimation from streaming MEG / EEG data blocks.
 *
 * RtCov estimates the channel–channel noise covariance used by linear inverse operators (MNE, dSPM,
 * sLORETA, beamformers) from data that arrives block by block. It accumulates the channel sums and the
 * outer product X⋅Xᵀ of every block, so its memory does not grow with the number of samples; once
 * enough samples have arrived it returns the unbiased covariance, regularized like
 * @ref mne::FiffCov::regularize, and starts over. It is synchronous: call it from whichever thread
 * receives the data.
 */

#ifndef RT_COV_RTPROCESSING_H
#define RT_COV_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"

#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

#include <Eigen/Core>

#include <optional>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Estimates the noise covariance of the MEG and EEG channels from streaming data.
 *
 * @snippet ex_dsp_rt/main.cpp rt_cov_usage
 */
class DSPSHARED_EXPORT RtCov
{
public:
    //=========================================================================================================
    /**
     * @param[in] info   Measurement info of the data; its MEG and EEG channels are used.
     */
    explicit RtCov(const FiffInfo& info);

    //=========================================================================================================
    /**
     * Adds a block of data and returns the covariance once enough samples have arrived.
     *
     * @param[in] data         A block of data, all channels of the measurement info by samples.
     * @param[in] maxSamples   Number of samples to accumulate before a covariance is estimated.
     *
     * @return The regularized noise covariance of the MEG and EEG channels once at least maxSamples
     *         samples have arrived, after which the next estimate starts from nothing; no value
     *         before that, or if the measurement info has no channels.
     */
    std::optional<FiffCov> estimate_covariance(const Eigen::MatrixXd& data, int maxSamples);

private:
    FiffInfo m_info;          /**< Measurement info of the data. */
    std::vector<int> m_picks; /**< MEG and EEG channels. */
    Eigen::VectorXd m_sum;    /**< Sum of the samples of each picked channel. */
    Eigen::MatrixXd m_outer;  /**< Sum of the outer products of the samples. */
    int m_samples = 0;        /**< Number of accumulated samples. */
};

} // namespace mne

#endif // RT_COV_RTPROCESSING_H
