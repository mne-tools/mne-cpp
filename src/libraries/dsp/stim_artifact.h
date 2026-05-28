//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     stim_artifact.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Declaration of fixStimArtifact — stimulus artifact repair by interpolation or zeroing.
 *
 * Repairs stimulus artifacts in continuous data by replacing samples in a window around each
 * stimulus event with either linearly interpolated values (from the window boundaries) or zeros.
 * This mirrors the functionality of MNE-Python's mne.preprocessing.fix_stim_artifact().
 */

#ifndef STIM_ARTIFACT_DSP_H
#define STIM_ARTIFACT_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB {

//=============================================================================================================
/**
 * @brief Mode for stimulus artifact repair.
 */
enum class StimArtifactMode
{
    Linear,     /**< Linear interpolation across the artifact window. */
    Window      /**< Zero-pad (set to zero) the artifact window. */
};

//=============================================================================================================
/**
 * @brief Repair stimulus artifacts by interpolating or zeroing data around events.
 *
 * For each event sample, the data in the window [event + tmin_samples, event + tmax_samples]
 * is either linearly interpolated from the boundary values or set to zero.
 *
 * @param[in,out] data         Data matrix (n_channels x n_times). Modified in-place.
 * @param[in]     events       Event matrix (Nx3: [sample, before, after]). Only the sample
 *                             column (col 0) is used to locate artifact windows.
 * @param[in]     sfreq        Sampling frequency in Hz.
 * @param[in]     eventId      If >= 0, only process events where column 2 (after) matches
 *                             this value. If < 0, process all events. Default: -1.
 * @param[in]     tmin         Start of artifact window in seconds relative to event
 *                             (negative = before event). Default: 0.0 (event onset).
 * @param[in]     tmax         End of artifact window in seconds relative to event.
 *                             Default: 0.01 (10 ms after event).
 * @param[in]     mode         Repair mode (Linear interpolation or zero-padding).
 *                             Default: StimArtifactMode::Linear.
 */
DSPSHARED_EXPORT void fixStimArtifact(Eigen::MatrixXd& data,
                                       const Eigen::MatrixXi& events,
                                       double sfreq,
                                       int eventId = -1,
                                       double tmin = 0.0,
                                       double tmax = 0.01,
                                       StimArtifactMode mode = StimArtifactMode::Linear);

} // namespace UTILSLIB

#endif // STIM_ARTIFACT_DSP_H
