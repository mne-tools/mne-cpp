//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     eog_regression.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Declaration of EogRegression — EOG artifact removal via linear regression.
 *
 * Fits a least-squares model from EOG channels to all other channels and subtracts the
 * predicted EOG contribution. This mirrors MNE-Python's mne.preprocessing.EOGRegression.
 */

#ifndef EOG_REGRESSION_DSP_H
#define EOG_REGRESSION_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>
#include <QVector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Remove EOG artifacts via linear regression.
 *
 * As mne.preprocessing.EOGRegression: fits data_ch = beta * EOG + residual on time-demeaned
 * signals for every good data channel and subtracts beta times the demeaned EOG.
 *
 * @snippet ex_dsp_artifacts/main.cpp eog_regression_usage
 */
class DSPSHARED_EXPORT EogRegression
{
public:
    //=========================================================================================================
    /**
     * Constructs a default EogRegression object.
     */
    EogRegression() = default;

    //=========================================================================================================
    /**
     * @brief Fit regression coefficients from EOG to all other channels.
     *
     * Ordinary least squares on demeaned signals: beta = T * E^T * (E * E^T)^{-1}, T being the
     * good data channels (FiffInfoBase::pick_data_channels without the bads).
     *
     * @param[in] data         Data matrix (n_channels x n_times).
     * @param[in] info         Measurement info (identifies EOG channels by kind == FIFFV_EOG_CH).
     * @param[in] eogChannels  Optional explicit EOG channel names. If empty, the good channels of kind FIFFV_EOG_CH.
     */
    void fit(const Eigen::MatrixXd& data,
             const FIFFLIB::FiffInfo& info,
             const QStringList& eogChannels = QStringList());

    //=========================================================================================================
    /**
     * @brief Apply the fitted regression to remove EOG artifacts in-place.
     *
     * Subtracts beta times the demeaned EOG from the channels fit() regressed.
     * fit() must be called first.
     *
     * @param[in,out] data   Data matrix (n_channels x n_times). Modified in-place.
     * @param[in]     info   Measurement info.
     */
    void apply(Eigen::MatrixXd& data,
               const FIFFLIB::FiffInfo& info) const;

    //=========================================================================================================
    /**
     * @brief Convenience: fit and apply in one step.
     *
     * @param[in,out] data         Data matrix (n_channels x n_times). Modified in-place.
     * @param[in]     info         Measurement info.
     * @param[in]     eogChannels  Optional explicit EOG channel names.
     */
    static void fitApply(Eigen::MatrixXd& data,
                         const FIFFLIB::FiffInfo& info,
                         const QStringList& eogChannels = QStringList());

    //=========================================================================================================
    /**
     * @brief Returns the fitted regression coefficients.
     * @return Matrix (n_non_eog_channels x n_eog_channels).
     */
    const Eigen::MatrixXd& coefficients() const
    {
        return m_matBeta;
    }

    //=========================================================================================================
    /**
     * @brief Returns true if fit() has been called successfully.
     *
     * @return True if regression coefficients are available, false otherwise.
     */
    bool isFitted() const
    {
        return m_bFitted;
    }

private:
    Eigen::MatrixXd m_matBeta;       /**< Regression coefficients (n_targets x n_eog). */
    QVector<int> m_vecEogIndices;    /**< Indices of EOG channels in the data matrix. */
    QVector<int> m_vecTargetIndices; /**< Indices of the regressed data channels. */
    bool m_bFitted = false;          /**< Whether fit() has been called. */
};

} // namespace UTILSLIB

#endif // EOG_REGRESSION_DSP_H
