//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     rt_cov.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    Real-time noise covariance estimation from streaming MEG / EEG data blocks.
 *
 * RtCov estimates the channel–channel noise covariance used by linear inverse operators (MNE, dSPM,
 * sLORETA, beamformers) from data that arrives block by block. With a half-life, older samples count
 * exponentially less, so the estimate follows the current noise; without one, every sample counts the
 * same. Each block costs one rank update of the running statistics, and the memory does not grow with
 * the number of samples. It is synchronous: call it from whichever thread receives the data.
 */

#ifndef RT_COV_RTPROCESSING_H
#define RT_COV_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <limits>
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
     * @param[in] info       Measurement info of the data; its MEG and EEG channels are used, or all channels
     *                       if it has neither.
     * @param[in] halfLife   Age in seconds at which a sample counts half as much as the newest one; infinite
     *                       weighs every sample the same.
     */
    explicit RtCov(const FiffInfo& info, double halfLife = std::numeric_limits<double>::infinity());

    //=========================================================================================================
    /**
     * Adds a block of data. A block with a channel count other than the measurement info's or the used
     * channels' is ignored with a warning.
     *
     * @param[in] data   All channels of the measurement info, or only the used ones, by samples.
     */
    void update(const Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
     * @return The noise covariance of the used channels, the weighted equivalent of numpy.cov with
     *         aweights, regularized like @ref mne::FiffCov::regularize; nfree is the effective number of
     *         samples. No value before two samples have arrived.
     */
    [[nodiscard]] std::optional<FiffCov> covariance() const;

    //=========================================================================================================
    /**
     * Forgets every sample.
     */
    void reset();

private:
    FiffInfo m_info;           /**< Measurement info of the data. */
    std::vector<int> m_picks;  /**< Used channels. */
    double m_decay = 1.0;      /**< Weight factor per sample of age. */
    double m_weight = 0.0;     /**< Sum of the sample weights. */
    double m_weightSq = 0.0;   /**< Sum of the squared sample weights. */
    Eigen::VectorXd m_mean;    /**< Weighted mean of the used channels. */
    Eigen::MatrixXd m_scatter; /**< Weighted sum of the centred outer products, lower triangle. */
};

} // namespace mne

#endif // RT_COV_RTPROCESSING_H
