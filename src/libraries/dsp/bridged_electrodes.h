//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bridged_electrodes.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Detection of electrically bridged EEG electrodes via the Electrical Distance metric.
 *
 * Two EEG electrodes are said to be bridged when conductive paste, sweat or
 * skin abrasion creates a low-impedance shortcut between them; the recorded
 * signals then become near-identical and any subsequent source localisation
 * is biased. Following Tenke & Kayser (2001, Clin. Neurophysiol. 112) the
 * detector computes the Electrical Distance — the time-domain variance of
 * the difference signal — for every channel pair, and flags pairs whose
 * normalised distance falls below a user-defined threshold (default 0.3).
 *
 * The implementation operates on the raw broadband signal; no rereferencing
 * or filtering is required up-front. Output is a list of @c (i, j) channel-
 * index pairs that the caller can mark bad, interpolate, or pass into the
 * SSS bad-channel set.
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
