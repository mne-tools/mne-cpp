//=============================================================================================================
/**
 * @file     bridged_electrodes.h
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
 * @brief    Bridged electrode detection functions.
 */

#ifndef BRIDGED_ELECTRODES_H
#define BRIDGED_ELECTRODES_H

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

#include <QPair>
#include <QList>

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
 * Parameters for bridged electrode detection.
 */
struct DSPSHARED_EXPORT BridgedElectrodeParams
{
    double dElectricalDistanceThreshold = 0.3;  /**< Max electrical distance to flag as bridged. */
};

//=============================================================================================================
/**
 * @brief Compute electrical distance matrix between EEG channels.
 *
 * Electrical distance is defined as the standard deviation of the difference
 * signal between two channels, normalised by the geometric mean of their
 * individual standard deviations. Bridged electrodes will have near-zero
 * electrical distance.
 *
 * @param[in] data      Data matrix (n_channels x n_times).
 * @param[in] info      FiffInfo with channel types.
 *
 * @return Electrical distance matrix (n_eeg x n_eeg), with EEG channel
 *         indices stored in the order they appear in info.
 */
DSPSHARED_EXPORT Eigen::MatrixXd computeElectricalDistance(const Eigen::MatrixXd& data,
                                                            const FIFFLIB::FiffInfo& info);

//=============================================================================================================
/**
 * @brief Detect bridged EEG electrodes.
 *
 * Finds pairs of EEG channels whose electrical distance falls below
 * the threshold, indicating a physical or gel bridge.
 *
 * @param[in] data      Data matrix (n_channels x n_times).
 * @param[in] info      FiffInfo with channel types.
 * @param[in] params    Detection parameters.
 *
 * @return List of bridged electrode pairs as (channel_index_1, channel_index_2)
 *         using indices into info.chs.
 */
DSPSHARED_EXPORT QList<QPair<int,int>> computeBridgedElectrodes(
    const Eigen::MatrixXd& data,
    const FIFFLIB::FiffInfo& info,
    const BridgedElectrodeParams& params = BridgedElectrodeParams());

} // namespace UTILSLIB

#endif // BRIDGED_ELECTRODES_H
