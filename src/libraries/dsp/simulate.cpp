//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     simulate.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Simulation utilities implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "simulate.h"

#include <inv/inv_source_estimate.h>
#include <inv/inv_convenience.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_cov.h>
#include <mne/mne_forward_solution.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace INVLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE FUNCTIONS
//=============================================================================================================

InvSourceEstimate UTILSLIB::simulateStc(
    const VectorXi& activeVertices,
    const VectorXi& allVertices,
    const SimulateStcParams& params)
{
    const int nAll = static_cast<int>(allVertices.size());
    const int nActive = static_cast<int>(activeVertices.size());
    const float tstep = 1.0f / params.sfreq;
    const int nTimes = static_cast<int>(params.duration * params.sfreq);

    if (nTimes <= 0 || nAll <= 0) {
        qWarning() << "[simulateStc] Invalid parameters.";
        return InvSourceEstimate();
    }

    // Build mapping: active vertex -> row in allVertices
    QList<int> activeRows;
    for (int a = 0; a < nActive; ++a) {
        bool found = false;
        for (int v = 0; v < nAll; ++v) {
            if (allVertices(v) == activeVertices(a)) {
                activeRows.append(v);
                found = true;
                break;
            }
        }
        if (!found) {
            qWarning() << "[simulateStc] Active vertex" << activeVertices(a) << "not in allVertices.";
            return InvSourceEstimate();
        }
    }

    // Generate Gaussian-envelope waveforms for each active source
    MatrixXd data = MatrixXd::Zero(nAll, nTimes);

    std::mt19937 gen(params.seed);
    std::uniform_real_distribution<double> timeDist(0.2, 0.8);

    for (int a = 0; a < nActive; ++a) {
        // Center the Gaussian at a random fraction of the duration
        double centerFrac = timeDist(gen);
        double centerSample = centerFrac * nTimes;
        double sigma = nTimes * 0.1; // 10% of duration

        for (int t = 0; t < nTimes; ++t) {
            double exponent = -0.5 * std::pow((t - centerSample) / sigma, 2.0);
            data(activeRows[a], t) = std::exp(exponent);
        }
    }

    return InvSourceEstimate(data, allVertices, params.tmin, tstep);
}

//=============================================================================================================

InvSourceEstimate UTILSLIB::simulateStcFromWaveforms(
    const MatrixXd& waveforms,
    const VectorXi& activeVertices,
    const VectorXi& allVertices,
    float tmin,
    float tstep)
{
    const int nAll = static_cast<int>(allVertices.size());
    const int nActive = static_cast<int>(activeVertices.size());
    const int nTimes = static_cast<int>(waveforms.cols());

    if (waveforms.rows() != nActive) {
        qWarning() << "[simulateStcFromWaveforms] waveforms.rows() != activeVertices.size()";
        return InvSourceEstimate();
    }

    MatrixXd data = MatrixXd::Zero(nAll, nTimes);

    for (int a = 0; a < nActive; ++a) {
        for (int v = 0; v < nAll; ++v) {
            if (allVertices(v) == activeVertices(a)) {
                data.row(v) = waveforms.row(a);
                break;
            }
        }
    }

    return InvSourceEstimate(data, allVertices, tmin, tstep);
}

//=============================================================================================================

FiffEvoked UTILSLIB::simulateEvoked(
    const MNEForwardSolution& fwd,
    const InvSourceEstimate& stc,
    const FiffInfo& info,
    const FiffCov& noiseCov,
    int nave,
    int seed)
{
    // First generate noiseless data
    FiffEvoked evoked = simulateEvokedNoiseless(fwd, stc, info);

    if (evoked.data.size() == 0)
        return evoked;

    const int nChan = static_cast<int>(evoked.data.rows());
    const int nTimes = static_cast<int>(evoked.data.cols());

    // Add noise from covariance
    if (noiseCov.data.size() > 0) {
        // Coloured like mne.simulation.simulate_evoked: the colourer of the prepared covariance (SSP applied,
        // MEG and EEG decomposed separately), so the noise has no MEG-EEG covariance; divided by sqrt(nave).
        const FiffCov prepared = noiseCov.prepare_noise_cov(evoked.info, evoked.info.ch_names);
        if (prepared.eig.size() != nChan) {
            qWarning() << "[simulateEvoked] Noise covariance does not cover the forward channels.";
            return evoked;
        }
        const MatrixXd sqrtCov = prepared.eigvec.transpose() * prepared.eig.cwiseMax(0.0).cwiseSqrt().asDiagonal();

        std::mt19937 gen(seed);
        std::normal_distribution<double> dist(0.0, 1.0);

        MatrixXd noise(nChan, nTimes);
        for (int i = 0; i < nChan; ++i) {
            for (int j = 0; j < nTimes; ++j) {
                noise(i, j) = dist(gen);
            }
        }

        double scaleFactor = 1.0 / std::sqrt(static_cast<double>(nave));
        evoked.data += sqrtCov * noise * scaleFactor;
    }

    evoked.nave = nave;
    return evoked;
}

//=============================================================================================================

FiffEvoked UTILSLIB::simulateEvokedNoiseless(
    const MNEForwardSolution& fwd,
    const InvSourceEstimate& stc,
    const FiffInfo& info)
{
    return INVLIB::applyForward(fwd, stc, info);
}
