//=============================================================================================================
/**
 * @file     bad_channels_lof.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    LOF-based bad channel detection functions.
 */

#ifndef BAD_CHANNELS_LOF_H
#define BAD_CHANNELS_LOF_H

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

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB {

//=============================================================================================================
/**
 * Parameters for LOF bad channel detection.
 */
struct DSPSHARED_EXPORT LofBadChannelParams
{
    int iNNeighbors = 20;       /**< Number of neighbours for LOF (k parameter). */
    double dThreshold = 2.0;    /**< LOF score threshold for marking a channel as bad. */
    bool bMegOnly = false;      /**< If true, only check MEG channels. */
    bool bEegOnly = false;      /**< If true, only check EEG channels. */
};

//=============================================================================================================
/**
 * @brief Detect bad channels using Local Outlier Factor (LOF).
 *
 * Computes features for each channel (standard deviation, kurtosis,
 * max absolute amplitude) and applies the LOF algorithm to identify
 * outlier channels. LOF compares the local density of a point's
 * neighbourhood to the densities of its neighbours.
 *
 * @param[in] data      Data matrix (n_channels x n_times).
 * @param[in] info      FiffInfo with channel types.
 * @param[in] params    LOF parameters.
 *
 * @return List of bad channel names.
 */
DSPSHARED_EXPORT QStringList findBadChannelsLof(const Eigen::MatrixXd& data,
                                                 const FIFFLIB::FiffInfo& info,
                                                 const LofBadChannelParams& params = LofBadChannelParams());

//=============================================================================================================
/**
 * @brief Compute LOF scores for a feature matrix.
 *
 * Pure algorithmic LOF implementation operating on feature vectors.
 *
 * @param[in] features   Feature matrix (n_points x n_features).
 * @param[in] k          Number of neighbours.
 *
 * @return LOF score for each point (n_points x 1). Values > 1 indicate outliers.
 */
DSPSHARED_EXPORT Eigen::VectorXd computeLofScores(const Eigen::MatrixXd& features,
                                                    int k);

} // namespace UTILSLIB

#endif // BAD_CHANNELS_LOF_H
