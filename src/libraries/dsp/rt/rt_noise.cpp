//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_noise.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Definition of the RtNoise class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_noise.h"

#include <base/log.h>

#include <unsupported/Eigen/FFT>

#include <algorithm>
#include <cmath>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtNoise::RtNoise(int fftLength, double sfreq, int blocks)
: m_fftLength(std::max(fftLength, 1))
, m_sfreq(sfreq)
, m_blocks(std::max(blocks, 1))
, m_window(static_cast<std::size_t>(m_fftLength))
{
    for (int i = 0; i < m_fftLength; ++i) {
        m_window[static_cast<std::size_t>(i)] = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * (i + 1) / (m_fftLength + 1)));
    }
}

//=============================================================================================================

std::optional<MatrixXd> RtNoise::estimate_spectrum(const MatrixXd& data)
{
    if (m_received > 0 && data.rows() != m_data.rows()) {
        Log::warning("[RtNoise::estimate_spectrum] The number of channels changed; starting a new estimate.");
        m_received = 0;
    }
    if (m_received == 0) {
        m_data = data;
    } else {
        m_data.conservativeResize(NoChange, m_data.cols() + data.cols());
        m_data.rightCols(data.cols()) = data;
    }
    if (++m_received < m_blocks) {
        return std::nullopt;
    }
    m_received = 0;

    const Index halfSpec = m_fftLength / 2 + 1;
    const Index segments = std::max<Index>(1, m_data.cols() / m_fftLength); // complete segments only
    double windowPower = 0.0;
    for (const double w : m_window) {
        windowPower += w * w;
    }
    MatrixXd density = MatrixXd::Zero(m_data.rows(), halfSpec);
    RowVectorXd segment(m_fftLength);
    RowVectorXcd spectrum(halfSpec);
    Eigen::FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);
    for (Index n = 0; n < segments; ++n) {
        const Index offset = n * m_fftLength;
        const Index length = std::min<Index>(m_fftLength, m_data.cols() - offset);
        for (Index ch = 0; ch < m_data.rows(); ++ch) {
            segment.setZero();
            for (Index k = 0; k < length; ++k) {
                segment(k) = m_data(ch, offset + k) * m_window[static_cast<std::size_t>(k)];
            }
            fft.fwd(spectrum, segment);
            // One-sided density |X|^2 / (fs * sum w^2), as scipy.signal.welch(scaling="density")
            for (Index j = 0; j < halfSpec; ++j) {
                const bool unpaired = j == 0 || (m_fftLength % 2 == 0 && j == m_fftLength / 2);
                density(ch, j) += std::norm(spectrum(j)) / (m_sfreq * windowPower) * (unpaired ? 1.0 : 2.0);
            }
        }
    }
    return (10.0 * (density / static_cast<double>(segments)).array().log10()).matrix();
}
