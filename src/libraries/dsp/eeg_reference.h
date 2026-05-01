//=============================================================================================================
/**
 * @file     eeg_reference.h
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
 * @brief    EEG re-referencing functions declaration.
 *
 */

#ifndef EEG_REFERENCE_H
#define EEG_REFERENCE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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
 * @brief Re-reference EEG channels to average, specific channel(s), or REST.
 *
 * Only modifies rows corresponding to EEG channels (kind == FIFFV_EEG_CH).
 * Non-EEG channels are left untouched.
 *
 * @param[in,out] data          Data matrix (n_channels x n_times). Modified in-place.
 * @param[in]     info          Measurement info (used to identify EEG channels).
 * @param[in]     refChannels   Reference channel name(s).
 *                              - Empty or "average" -> average reference (mean of all good EEG channels).
 *                              - Single channel name -> re-reference to that channel.
 *                              - Multiple channel names -> re-reference to mean of those channels.
 * @param[in]     projection    If true, add an SSP projector for average reference instead of modifying data.
 *                              (default: false — modify data directly)
 */
DSPSHARED_EXPORT void setEegReference(Eigen::MatrixXd& data,
                                       const FIFFLIB::FiffInfo& info,
                                       const QStringList& refChannels = QStringList(),
                                       bool projection = false);

//=============================================================================================================
/**
 * @brief Add reference channel(s) back as zero-filled rows.
 *
 * When EEG data was recorded with a physical reference electrode, that channel
 * is implicit (all zeros after re-referencing). This function adds it back explicitly
 * so it can participate in average re-referencing or source localization.
 *
 * @param[in,out] data      Data matrix. Rows are added at the end.
 * @param[in,out] info      Measurement info. New EEG channels are appended.
 * @param[in]     chNames   Names of the reference channel(s) to add.
 */
DSPSHARED_EXPORT void addReferenceChannels(Eigen::MatrixXd& data,
                                            FIFFLIB::FiffInfo& info,
                                            const QStringList& chNames);

//=============================================================================================================
/**
 * @brief Create bipolar derivations from EEG channels.
 *
 * Creates new bipolar channels by subtracting cathode from anode for each pair.
 * The original channels are replaced with the bipolar derivations.
 *
 * @param[in,out] data      Data matrix. Replaced with bipolar derivation rows.
 * @param[in,out] info      Measurement info. Updated with new bipolar channel info.
 * @param[in]     anodes    List of anode channel names.
 * @param[in]     cathodes  List of cathode channel names (same length as anodes).
 * @param[in]     dropOriginals  If true, remove the original anode/cathode channels
 *                               from the output (default: true).
 */
DSPSHARED_EXPORT void setBipolarReference(Eigen::MatrixXd& data,
                                           FIFFLIB::FiffInfo& info,
                                           const QStringList& anodes,
                                           const QStringList& cathodes,
                                           bool dropOriginals = true);

} // namespace UTILSLIB

#endif // EEG_REFERENCE_H
