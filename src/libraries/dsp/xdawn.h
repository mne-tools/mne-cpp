//=============================================================================================================
/**
 * @file     xdawn.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
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
 * @brief    Declaration of the Xdawn class for event-related response enhancement.
 *
 * xDAWN estimates spatial filters that maximise the signal-to-noise ratio of a target
 * event-related response. The implementation here operates on epoched data and follows the
 * standard generalised-eigenvalue formulation using a target evoked covariance and a
 * residual-noise covariance estimated from class-wise residual epochs.
 */

#ifndef XDAWN_DSP_H
#define XDAWN_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

#include <mne/mne_epoch_data.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Result of an xDAWN decomposition.
 */
struct DSPSHARED_EXPORT XdawnResult
{
    Eigen::MatrixXd matFilters;       /**< Spatial filters W (n_channels x n_components). */
    Eigen::MatrixXd matPatterns;      /**< Spatial patterns A (n_channels x n_components). */
    Eigen::MatrixXd matSignalCov;     /**< Target-evoked covariance (n_channels x n_channels). */
    Eigen::MatrixXd matNoiseCov;      /**< Residual-noise covariance (n_channels x n_channels). */
    Eigen::MatrixXd matTargetEvoked;  /**< Average target evoked response (n_channels x n_samples). */
    int             iTargetEvent = 1; /**< Event code used as the target class. */
    bool            bValid       = false; /**< True if fit() produced a usable decomposition. */
};

//=============================================================================================================
/**
 * @brief Event-related response enhancement with xDAWN spatial filtering.
 *
 * Typical usage:
 * @code
 *   auto xd = Xdawn::fit(epochs, 1, 4);
 *   Eigen::MatrixXd comp = Xdawn::apply(epoch.epoch, xd);      // 4 x n_samples
 *   Eigen::MatrixXd den  = Xdawn::denoise(epoch.epoch, xd, 2); // n_channels x n_samples
 * @endcode
 */
class DSPSHARED_EXPORT Xdawn
{
public:
    //=========================================================================================================
    /**
     * Fit xDAWN spatial filters for a target event code.
     *
     * Uses non-rejected epochs only. The target signal covariance is estimated from the
     * average of epochs whose MNEEpochData::event equals @p iTargetEvent. The noise covariance
     * is estimated from residuals around the class-wise average for each event type.
     *
     * @param[in] epochs         Input epochs.
     * @param[in] iTargetEvent   Event code whose ERP/ERF should be enhanced.
     * @param[in] nComponents    Number of xDAWN components to retain.
     * @param[in] dReg           Relative diagonal regularisation added to the noise covariance.
     *
     * @return XdawnResult containing filters, patterns, and covariances.
     */
    static XdawnResult fit(const QVector<MNELIB::MNEEpochData>& epochs,
                           int                                  iTargetEvent = 1,
                           int                                  nComponents  = 2,
                           double                               dReg         = 1e-6);

    //=========================================================================================================
    /**
     * Project one epoch into xDAWN component space.
     *
     * @param[in] matEpoch   Epoch data (n_channels x n_samples).
     * @param[in] result     Result from fit().
     *
     * @return Component activations (n_components x n_samples).
     */
    static Eigen::MatrixXd apply(const Eigen::MatrixXd& matEpoch,
                                 const XdawnResult&     result);

    //=========================================================================================================
    /**
     * Reconstruct one epoch from the first @p nComponents xDAWN components.
     *
     * If @p nComponents <= 0 or exceeds the fitted dimensionality, all fitted components are used.
     *
     * @param[in] matEpoch      Epoch data (n_channels x n_samples).
     * @param[in] result        Result from fit().
     * @param[in] nComponents   Number of leading xDAWN components to keep.
     *
     * @return Denoised epoch (n_channels x n_samples).
     */
    static Eigen::MatrixXd denoise(const Eigen::MatrixXd& matEpoch,
                                   const XdawnResult&     result,
                                   int                    nComponents = -1);

    //=========================================================================================================
    /**
     * Apply denoise() to all epochs in the vector, preserving metadata.
     *
     * @param[in] epochs         Input epochs.
     * @param[in] result         Result from fit().
     * @param[in] nComponents    Number of leading xDAWN components to keep.
     *
     * @return Copy of the input epochs with denoised epoch matrices.
     */
    static QVector<MNELIB::MNEEpochData> denoiseEpochs(const QVector<MNELIB::MNEEpochData>& epochs,
                                                       const XdawnResult&                   result,
                                                       int                                  nComponents = -1);
};

} // namespace UTILSLIB

#endif // XDAWN_DSP_H
