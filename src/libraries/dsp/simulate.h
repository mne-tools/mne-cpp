//=============================================================================================================
/**
 * @file     simulate.h
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
 * @brief    Simulation utilities for source estimates and evoked data.
 *
 * Provides functions equivalent to MNE-Python's mne.simulation module:
 *   - simulateStc(): create synthetic source time courses
 *   - simulateEvoked(): generate synthetic evoked data from forward model
 */

#ifndef SIMULATE_DSP_H
#define SIMULATE_DSP_H

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

#include <QList>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace INVLIB {
    class InvSourceEstimate;
}
namespace FIFFLIB {
    class FiffEvoked;
    class FiffInfo;
    class FiffCov;
}
namespace MNELIB {
    class MNEForwardSolution;
}

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Parameters for source time course simulation.
 */
struct DSPSHARED_EXPORT SimulateStcParams
{
    float sfreq    = 1000.0f;  /**< Sampling frequency (Hz). */
    float tmin     = 0.0f;     /**< Start time (seconds). */
    float duration = 0.5f;     /**< Duration (seconds). */
    int   seed     = 42;       /**< Random seed for reproducibility. */
};

//=============================================================================================================
/**
 * @brief Create a synthetic source time course.
 *
 * Generates an InvSourceEstimate with Gaussian-envelope waveforms at
 * specified source vertices. Each active source has a Gaussian
 * temporal profile centered at different times.
 *
 * @param[in] activeVertices  Vertex indices for active sources.
 * @param[in] allVertices     Full vertex index vector for the source space.
 * @param[in] params          Simulation parameters (sfreq, tmin, duration).
 *
 * @return Simulated source estimate.
 */
DSPSHARED_EXPORT INVLIB::InvSourceEstimate simulateStc(
    const Eigen::VectorXi& activeVertices,
    const Eigen::VectorXi& allVertices,
    const SimulateStcParams& params = SimulateStcParams());

//=============================================================================================================
/**
 * @brief Create a synthetic source time course from custom waveforms.
 *
 * @param[in] waveforms       Waveform data (n_active_sources x n_times).
 * @param[in] activeVertices  Vertex indices for the active sources.
 * @param[in] allVertices     Full vertex index vector.
 * @param[in] tmin            Start time (s).
 * @param[in] tstep           Time step (s).
 *
 * @return Simulated source estimate with zeros at inactive vertices.
 */
DSPSHARED_EXPORT INVLIB::InvSourceEstimate simulateStcFromWaveforms(
    const Eigen::MatrixXd& waveforms,
    const Eigen::VectorXi& activeVertices,
    const Eigen::VectorXi& allVertices,
    float tmin = 0.0f,
    float tstep = 0.001f);

//=============================================================================================================
/**
 * @brief Simulate evoked data from a source estimate and forward model.
 *
 * Projects the source estimate through the forward model to generate
 * sensor-space data, optionally adding noise from a noise covariance.
 *
 * Equivalent to MNE-Python's mne.simulation.simulate_evoked().
 *
 * @param[in] fwd         Forward solution (gain matrix).
 * @param[in] stc         Source time course.
 * @param[in] info        Measurement info (for output channel info).
 * @param[in] noiseCov    Noise covariance for additive noise (optional).
 * @param[in] nave        Number of averages (noise scaled by 1/sqrt(nave)).
 * @param[in] seed        Random seed for noise generation.
 *
 * @return Simulated evoked data.
 */
DSPSHARED_EXPORT FIFFLIB::FiffEvoked simulateEvoked(
    const MNELIB::MNEForwardSolution& fwd,
    const INVLIB::InvSourceEstimate& stc,
    const FIFFLIB::FiffInfo& info,
    const FIFFLIB::FiffCov& noiseCov,
    int nave = 1,
    int seed = 42);

//=============================================================================================================
/**
 * @brief Simulate evoked data without noise.
 *
 * Projects source estimate through forward model (no noise added).
 *
 * @param[in] fwd         Forward solution.
 * @param[in] stc         Source time course.
 * @param[in] info        Measurement info.
 *
 * @return Simulated noiseless evoked data.
 */
DSPSHARED_EXPORT FIFFLIB::FiffEvoked simulateEvokedNoiseless(
    const MNELIB::MNEForwardSolution& fwd,
    const INVLIB::InvSourceEstimate& stc,
    const FIFFLIB::FiffInfo& info);

} // namespace UTILSLIB

#endif // SIMULATE_DSP_H
