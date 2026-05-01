//=============================================================================================================
/**
 * @file     ml_ssd.cpp
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
 * @brief    MlSsd class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_ssd.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Eigenvalues>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MlSsd::MlSsd(int nComponents)
: m_nComponents(nComponents)
{
}

//=============================================================================================================

void MlSsd::fit(const MatrixXd& matData,
                double dSFreq,
                const QPair<double, double>& signalBand,
                const QPair<double, double>& noiseBand,
                double dRegParam)
{
    const int nCh = static_cast<int>(matData.rows());
    const int nTimes = static_cast<int>(matData.cols());

    if (nCh < 2 || nTimes < 2) {
        qWarning() << "[MlSsd::fit] Insufficient data dimensions.";
        return;
    }

    if (m_nComponents > nCh)
        m_nComponents = nCh;

    // Mean-center the data
    MatrixXd dataCentered = matData.colwise() - matData.rowwise().mean();

    // Bandpass filter for signal and noise bands
    MatrixXd dataSignal = bandpassFilter(dataCentered, dSFreq, signalBand.first, signalBand.second);
    MatrixXd dataNoise = bandpassFilter(dataCentered, dSFreq, noiseBand.first, noiseBand.second);

    // Compute covariance matrices
    MatrixXd covSignal = (dataSignal * dataSignal.transpose()) / static_cast<double>(nTimes - 1);
    MatrixXd covNoise = (dataNoise * dataNoise.transpose()) / static_cast<double>(nTimes - 1);

    // Regularise the noise covariance
    covNoise += dRegParam * covNoise.trace() / static_cast<double>(nCh) * MatrixXd::Identity(nCh, nCh);

    // Solve the generalised eigenvalue problem: C_signal * w = lambda * C_noise * w
    // Equivalent to: inv(C_noise) * C_signal * w = lambda * w
    GeneralizedSelfAdjointEigenSolver<MatrixXd> ges(covSignal, covNoise);

    if (ges.info() != Success) {
        qWarning() << "[MlSsd::fit] Eigenvalue decomposition failed.";
        return;
    }

    // Eigenvalues are in ascending order; we want the largest first
    VectorXd eigenvalues = ges.eigenvalues().reverse();
    MatrixXd eigenvectors = ges.eigenvectors().rowwise().reverse();

    // Select top n_components
    m_eigenvalues = eigenvalues.head(m_nComponents);
    m_filters = eigenvectors.leftCols(m_nComponents).transpose();  // (n_components × n_channels)

    // Compute patterns: A = C * W * inv(W' * C * W)
    MatrixXd covFull = (dataCentered * dataCentered.transpose()) / static_cast<double>(nTimes - 1);
    MatrixXd WtCW = m_filters * covFull * m_filters.transpose();
    m_patterns = (covFull * m_filters.transpose() * WtCW.inverse()).transpose();  // (n_components × n_channels)

    m_bFitted = true;
}

//=============================================================================================================

MatrixXd MlSsd::transform(const MatrixXd& matData) const
{
    if (!m_bFitted) {
        qWarning() << "[MlSsd::transform] SSD not fitted.";
        return MatrixXd();
    }

    return m_filters * matData;
}

//=============================================================================================================

MatrixXd MlSsd::fitTransform(const MatrixXd& matData,
                               double dSFreq,
                               const QPair<double, double>& signalBand,
                               const QPair<double, double>& noiseBand,
                               double dRegParam)
{
    fit(matData, dSFreq, signalBand, noiseBand, dRegParam);
    return transform(matData);
}

//=============================================================================================================

MatrixXd MlSsd::bandpassFilter(const MatrixXd& matData,
                                 double dSFreq,
                                 double dLowFreq,
                                 double dHighFreq)
{
    // Simple windowed-sinc FIR bandpass filter
    const int nCh = static_cast<int>(matData.rows());
    const int nTimes = static_cast<int>(matData.cols());

    // Determine filter order (use a modest order for computational efficiency)
    int filterOrder = std::min(static_cast<int>(std::round(3.0 * dSFreq / dLowFreq)), nTimes - 1);
    if (filterOrder % 2 != 0)
        filterOrder += 1;
    filterOrder = std::min(filterOrder, 128);

    const int halfOrder = filterOrder / 2;

    // Build windowed-sinc coefficients for bandpass
    VectorXd h(filterOrder + 1);
    double wLow = 2.0 * M_PI * dLowFreq / dSFreq;
    double wHigh = 2.0 * M_PI * dHighFreq / dSFreq;

    for (int i = 0; i <= filterOrder; ++i) {
        int n = i - halfOrder;
        if (n == 0) {
            h[i] = (wHigh - wLow) / M_PI;
        } else {
            h[i] = (std::sin(wHigh * n) - std::sin(wLow * n)) / (M_PI * n);
        }
        // Apply Hamming window
        h[i] *= 0.54 - 0.46 * std::cos(2.0 * M_PI * i / filterOrder);
    }

    // Normalize to unit gain at center frequency
    double centerFreq = (dLowFreq + dHighFreq) / 2.0;
    double wCenter = 2.0 * M_PI * centerFreq / dSFreq;
    double gainReal = 0.0;
    for (int i = 0; i <= filterOrder; ++i) {
        gainReal += h[i] * std::cos(wCenter * (i - halfOrder));
    }
    if (std::abs(gainReal) > 1e-12)
        h /= std::abs(gainReal);

    // Apply filter via convolution (zero-phase: forward + reverse)
    MatrixXd result(nCh, nTimes);

    for (int ch = 0; ch < nCh; ++ch) {
        // Forward pass
        VectorXd forward(nTimes);
        for (int t = 0; t < nTimes; ++t) {
            double sum = 0.0;
            for (int k = 0; k <= filterOrder; ++k) {
                int idx = t - k;
                if (idx >= 0 && idx < nTimes)
                    sum += h[k] * matData(ch, idx);
            }
            forward[t] = sum;
        }

        // Reverse pass (for zero-phase filtering)
        VectorXd backward(nTimes);
        for (int t = nTimes - 1; t >= 0; --t) {
            double sum = 0.0;
            for (int k = 0; k <= filterOrder; ++k) {
                int idx = t + k;
                if (idx >= 0 && idx < nTimes)
                    sum += h[k] * forward[idx];
            }
            backward[t] = sum;
        }

        result.row(ch) = backward.transpose();
    }

    return result;
}
