//=============================================================================================================
/**
 * @file     decoding_ssd.cpp
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
 * @brief    DecodingSsd class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "decoding_ssd.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Eigenvalues>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DECODINGLIB;
using namespace Eigen;

//=============================================================================================================

DecodingSsd::DecodingSsd(int nComponents, double regParam)
    : m_nComponents(nComponents)
    , m_regParam(regParam)
{
}

//=============================================================================================================

void DecodingSsd::fit(const Ref<const MatrixXd>& data,
                      double sfreq,
                      double signalLow, double signalHigh,
                      double noiseLow, double noiseHigh)
{
    if (noiseLow > signalLow || signalHigh > noiseHigh) {
        throw std::invalid_argument(
            "DecodingSsd::fit: signal band must be within noise band");
    }

    const auto n_ch = data.rows();
    const auto n_times = data.cols();

    if (n_ch < 2 || n_times < 2) {
        throw std::invalid_argument(
            "DecodingSsd::fit: data must have >= 2 channels and >= 2 time points");
    }

    int n_comp = std::min(m_nComponents, static_cast<int>(n_ch));

    // Mean-center
    MatrixXd centered = data.colwise() - data.rowwise().mean();

    // Bandpass filter for signal and noise bands
    MatrixXd data_signal = bandpassFilter(centered, sfreq, signalLow, signalHigh);
    MatrixXd data_noise  = bandpassFilter(centered, sfreq, noiseLow, noiseHigh);

    // Covariance matrices
    MatrixXd cov_signal = (data_signal * data_signal.transpose())
                          / static_cast<double>(n_times - 1);
    MatrixXd cov_noise  = (data_noise * data_noise.transpose())
                          / static_cast<double>(n_times - 1);

    // Regularize noise covariance
    cov_noise += m_regParam * cov_noise.trace()
                 / static_cast<double>(n_ch)
                 * MatrixXd::Identity(n_ch, n_ch);

    // Generalized eigenvalue problem: C_signal w = λ C_noise w
    GeneralizedSelfAdjointEigenSolver<MatrixXd> ges(cov_signal, cov_noise);
    if (ges.info() != Eigen::Success) {
        throw std::runtime_error(
            "DecodingSsd::fit: eigenvalue decomposition failed");
    }

    // Eigenvalues ascending → reverse for descending
    VectorXd evals = ges.eigenvalues().reverse();
    MatrixXd evecs = ges.eigenvectors().rowwise().reverse();

    m_eigenvalues = evals.head(n_comp);
    m_filters = evecs.leftCols(n_comp).transpose();  // (n_comp × n_ch)

    // Patterns: A = C W inv(W^T C W)
    MatrixXd cov_full = (centered * centered.transpose())
                        / static_cast<double>(n_times - 1);
    MatrixXd WtCW = m_filters * cov_full * m_filters.transpose();
    m_patterns = (cov_full * m_filters.transpose()
                  * WtCW.inverse());  // (n_ch × n_comp)

    m_fitted = true;
}

//=============================================================================================================

MatrixXd DecodingSsd::transform(const Ref<const MatrixXd>& data) const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingSsd::transform: not fitted");
    }
    return m_filters * data;
}

//=============================================================================================================

MatrixXd DecodingSsd::fitTransform(const Ref<const MatrixXd>& data,
                                   double sfreq,
                                   double signalLow, double signalHigh,
                                   double noiseLow, double noiseHigh)
{
    fit(data, sfreq, signalLow, signalHigh, noiseLow, noiseHigh);
    return transform(data);
}

//=============================================================================================================

MatrixXd DecodingSsd::apply(const Ref<const MatrixXd>& data) const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingSsd::apply: not fitted");
    }

    // Denoise: project to SSD space, then reconstruct using patterns
    // patterns_ is (n_ch × n_comp), ssdSources is (n_comp × n_times)
    MatrixXd ssdSources = transform(data);
    return m_patterns * ssdSources;
}

//=============================================================================================================

const MatrixXd& DecodingSsd::filters() const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingSsd::filters: not fitted");
    }
    return m_filters;
}

//=============================================================================================================

const MatrixXd& DecodingSsd::patterns() const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingSsd::patterns: not fitted");
    }
    return m_patterns;
}

//=============================================================================================================

const VectorXd& DecodingSsd::eigenvalues() const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingSsd::eigenvalues: not fitted");
    }
    return m_eigenvalues;
}

//=============================================================================================================

bool DecodingSsd::isFitted() const
{
    return m_fitted;
}

//=============================================================================================================

MatrixXd DecodingSsd::bandpassFilter(const MatrixXd& data,
                                     double sfreq,
                                     double lowFreq,
                                     double highFreq)
{
    const auto n_ch = data.rows();
    const auto n_times = data.cols();

    int filter_order = std::min(
        static_cast<int>(std::round(3.0 * sfreq / lowFreq)),
        static_cast<int>(n_times) - 1);
    if (filter_order % 2 != 0) ++filter_order;
    filter_order = std::min(filter_order, 128);

    const int half = filter_order / 2;
    const double pi = M_PI;

    // Windowed-sinc coefficients
    VectorXd h(filter_order + 1);
    double w_low  = 2.0 * pi * lowFreq  / sfreq;
    double w_high = 2.0 * pi * highFreq / sfreq;

    for (int i = 0; i <= filter_order; ++i) {
        int n = i - half;
        if (n == 0) {
            h(i) = (w_high - w_low) / pi;
        } else {
            h(i) = (std::sin(w_high * static_cast<double>(n))
                   - std::sin(w_low * static_cast<double>(n)))
                  / (pi * static_cast<double>(n));
        }
        // Hamming window
        h(i) *= 0.54
               - 0.46 * std::cos(
                   2.0 * pi * static_cast<double>(i)
                   / static_cast<double>(filter_order));
    }

    // Normalize to unit gain at center frequency
    double center = (lowFreq + highFreq) / 2.0;
    double w_center = 2.0 * pi * center / sfreq;
    double gain = 0.0;
    for (int i = 0; i <= filter_order; ++i) {
        gain += h(i) * std::cos(
            w_center * static_cast<double>(i - half));
    }
    if (std::abs(gain) > 1e-12) h /= std::abs(gain);

    // Forward + reverse convolution (zero-phase)
    MatrixXd result(n_ch, n_times);

    for (Index ch = 0; ch < n_ch; ++ch) {
        VectorXd forward(n_times);
        for (Index t = 0; t < n_times; ++t) {
            double sum = 0.0;
            for (int k = 0; k <= filter_order; ++k) {
                auto idx = t - k;
                if (idx >= 0 && idx < n_times)
                    sum += h(k) * data(ch, idx);
            }
            forward(t) = sum;
        }

        VectorXd backward(n_times);
        for (Index t = n_times - 1; t >= 0; --t) {
            double sum = 0.0;
            for (int k = 0; k <= filter_order; ++k) {
                auto idx = t + k;
                if (idx >= 0 && idx < n_times)
                    sum += h(k) * forward(idx);
            }
            backward(t) = sum;
        }

        result.row(ch) = backward.transpose();
    }

    return result;
}
