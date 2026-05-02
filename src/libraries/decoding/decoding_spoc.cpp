//=============================================================================================================
/**
 * @file     decoding_spoc.cpp
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
 * @brief    DecodingSpoc class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "decoding_spoc.h"

#include <Skigen/Decomposition>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DECODINGLIB;
using namespace Eigen;

//=============================================================================================================

DecodingSpoc::DecodingSpoc(int nComponents,
                           TransformMode transformInto,
                           bool useLog)
    : m_nComponents(nComponents)
    , m_transformInto(transformInto)
    , m_useLog(useLog)
{
}

//=============================================================================================================

void DecodingSpoc::fit(const std::vector<MatrixXd>& epochs,
                       const VectorXd& y)
{
    if (epochs.empty()) {
        throw std::invalid_argument("DecodingSpoc::fit: epochs must be non-empty");
    }
    if (static_cast<int>(epochs.size()) != y.size()) {
        throw std::invalid_argument(
            "DecodingSpoc::fit: epochs and y must have the same length");
    }

    // Delegate core GED to Skigen
    Skigen::SPoC<double> spoc(m_nComponents);
    spoc.fit(epochs, y);

    m_filters = spoc.filters();
    m_patterns = spoc.patterns().transpose();  // Skigen: (n_comp × n_ch) → stored as (n_ch × n_comp)

    // Compute mean band power for z-score normalisation
    MatrixXd powerFeatures = computePowerFeatures(epochs);
    m_mean = powerFeatures.colwise().mean();

    m_std = VectorXd(powerFeatures.cols());
    for (int c = 0; c < powerFeatures.cols(); ++c) {
        VectorXd centered = powerFeatures.col(c).array() - m_mean(c);
        m_std(c) = std::sqrt(centered.squaredNorm()
                             / static_cast<double>(centered.size()));
    }

    m_fitted = true;
}

//=============================================================================================================

MatrixXd DecodingSpoc::transform(const std::vector<MatrixXd>& epochs) const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingSpoc::transform: not fitted");
    }

    if (m_transformInto == TransformMode::CspSpace) {
        const int nEpochs = static_cast<int>(epochs.size());
        const int nComp = static_cast<int>(m_filters.rows());
        const int nTimes = static_cast<int>(epochs[0].cols());

        MatrixXd result(nEpochs * nComp, nTimes);
        for (int e = 0; e < nEpochs; ++e) {
            result.middleRows(e * nComp, nComp) = m_filters * epochs[static_cast<size_t>(e)];
        }
        return result;
    }

    // AveragePower mode
    MatrixXd X = computePowerFeatures(epochs);

    if (m_useLog) {
        X = X.array().max(1e-30).log().matrix();
    } else {
        for (int c = 0; c < X.cols(); ++c) {
            double s = m_std(c);
            if (s < 1e-15) s = 1.0;
            X.col(c) = (X.col(c).array() - m_mean(c)) / s;
        }
    }

    return X;
}

//=============================================================================================================

MatrixXd DecodingSpoc::fitTransform(const std::vector<MatrixXd>& epochs,
                                    const VectorXd& y)
{
    fit(epochs, y);
    return transform(epochs);
}

//=============================================================================================================

const MatrixXd& DecodingSpoc::filters() const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingSpoc::filters: not fitted");
    }
    return m_filters;
}

//=============================================================================================================

const MatrixXd& DecodingSpoc::patterns() const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingSpoc::patterns: not fitted");
    }
    return m_patterns;
}

//=============================================================================================================

const VectorXd& DecodingSpoc::mean() const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingSpoc::mean: not fitted");
    }
    return m_mean;
}

//=============================================================================================================

const VectorXd& DecodingSpoc::stddev() const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingSpoc::stddev: not fitted");
    }
    return m_std;
}

//=============================================================================================================

bool DecodingSpoc::isFitted() const
{
    return m_fitted;
}

//=============================================================================================================

MatrixXd DecodingSpoc::computePowerFeatures(
    const std::vector<MatrixXd>& epochs) const
{
    const int nEpochs = static_cast<int>(epochs.size());
    const int nComp = static_cast<int>(m_filters.rows());

    MatrixXd features(nEpochs, nComp);
    for (int e = 0; e < nEpochs; ++e) {
        MatrixXd filtered = m_filters * epochs[static_cast<size_t>(e)];
        for (int c = 0; c < nComp; ++c) {
            features(e, c) = filtered.row(c).squaredNorm()
                            / static_cast<double>(filtered.cols());
        }
    }

    return features;
}
