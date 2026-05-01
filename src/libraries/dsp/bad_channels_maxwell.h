//=============================================================================================================
/**
 * @file     bad_channels_maxwell.h
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
 * @brief    Bad channel detection via SSS reconstruction residuals.
 *
 * Channels whose SSS reconstruction error is anomalously high are flagged
 * as bad. This mirrors MNE-Python's mne.preprocessing.find_bad_channels_maxwell().
 *
 * Reference:
 *   Taulu, S., Kajola, M. (2005). J. Appl. Phys. 97, 124905.
 */

#ifndef BAD_CHANNELS_MAXWELL_H
#define BAD_CHANNELS_MAXWELL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Parameters for SSS-based bad channel detection.
 */
struct DSPSHARED_EXPORT BadChannelsMaxwellParams
{
    int    iOrderIn      = 8;           /**< Internal SSS order. */
    int    iOrderOut     = 3;           /**< External SSS order. */
    Eigen::Vector3d origin{0.0, 0.0, 0.04}; /**< Expansion origin (metres, head frame). */
    double dZThreshold   = 4.0;        /**< Z-score threshold for flagging bad channels. */
    int    iBlockSize    = 0;          /**< Block size for averaging (0 = use all data). */
    double dMinRatio     = 1e-4;       /**< Minimum relative residual to consider (skip near-zero). */
};

//=============================================================================================================
/**
 * @brief Result of SSS-based bad channel detection.
 */
struct DSPSHARED_EXPORT BadChannelsMaxwellResult
{
    QStringList badChannels;            /**< Names of detected bad channels. */
    QList<int>  badIndices;             /**< Indices in the MEG channel list. */
    Eigen::VectorXd residuals;          /**< Per-channel reconstruction residuals. */
    Eigen::VectorXd zScores;            /**< Per-channel z-scores of residuals. */
};

//=============================================================================================================
/**
 * @brief Detect bad MEG channels using SSS reconstruction residuals.
 *
 * Computes the SSS reconstruction of the data, then flags channels whose
 * residual (difference between original and reconstructed) is anomalously
 * large relative to the population of channels.
 *
 * @param[in] matData   MEG data (n_channels x n_times). Should include all
 *                      channels (MEG and non-MEG).
 * @param[in] info      Measurement info with channel positions.
 * @param[in] params    Detection parameters.
 *
 * @return Detection result with bad channel names, indices, residuals, z-scores.
 */
DSPSHARED_EXPORT BadChannelsMaxwellResult findBadChannelsMaxwell(
    const Eigen::MatrixXd& matData,
    const FIFFLIB::FiffInfo& info,
    const BadChannelsMaxwellParams& params = BadChannelsMaxwellParams());

} // namespace UTILSLIB

#endif // BAD_CHANNELS_MAXWELL_H
