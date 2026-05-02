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

#include <Skigen/Decomposition>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

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

    Skigen::SSD<double> ssd(m_nComponents);
    ssd.fit(data, sfreq, signalLow, signalHigh, noiseLow, noiseHigh, m_regParam);

    m_filters = ssd.filters();
    m_patterns = ssd.patterns().transpose();  // Skigen: (n_comp × n_ch) → stored as (n_ch × n_comp)
    m_eigenvalues = ssd.eigenvalues();
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
