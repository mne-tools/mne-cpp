//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     eeg_reference.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    EEG re-referencing operators — common-average, single-electrode and REST.
 *
 * Every EEG measurement is implicitly a potential difference between an
 * active electrode and a reference; changing that reference is therefore
 * a linear operation on the data matrix. The functions in this header
 * build and apply the most widely used reference operators: subtraction
 * of a single electrode (or the average of a few), the common-average
 * reference (CAR), and — where a forward model is supplied — the reference
 * electrode standardisation technique (REST) which projects the data to
 * the equivalent reference at infinity.
 *
 * Re-referencing is implemented as a single matrix multiplication so that
 * channel covariances and inverse operators remain valid after re-
 * referencing, provided the same operator is also applied to the noise
 * covariance.
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
