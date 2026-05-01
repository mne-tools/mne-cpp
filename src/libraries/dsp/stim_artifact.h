//=============================================================================================================
/**
 * @file     stim_artifact.h
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
