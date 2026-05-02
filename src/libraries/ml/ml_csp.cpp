//=============================================================================================================
/**
 * @file     ml_csp.cpp
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
 * @brief    CSP implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_csp.h"

//=============================================================================================================
// SKIGEN INCLUDES
//=============================================================================================================

#include <Skigen/Decomposition>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MlCsp::MlCsp(int nComponents)
    : m_nComponents(nComponents)
{
}

//=============================================================================================================

void MlCsp::fit(const QList<MatrixXd>& epochsClass1,
                 const QList<MatrixXd>& epochsClass2)
{
    if (epochsClass1.isEmpty() || epochsClass2.isEmpty()) {
        qWarning() << "[MlCsp::fit] Empty epoch lists.";
        return;
    }

    // Convert QList → std::vector for skigen
    std::vector<MatrixXd> e1(epochsClass1.cbegin(), epochsClass1.cend());
    std::vector<MatrixXd> e2(epochsClass2.cbegin(), epochsClass2.cend());

    Skigen::CSP<double> csp(m_nComponents);
    csp.fit(e1, e2);

    m_filters     = csp.filters();
    m_patterns    = csp.patterns();
    m_eigenvalues = csp.eigenvalues();
    m_bFitted     = true;
}

//=============================================================================================================

MatrixXd MlCsp::transform(const QList<MatrixXd>& epochs) const
{
    if (!m_bFitted) {
        qWarning() << "[MlCsp::transform] CSP not fitted.";
        return MatrixXd();
    }

    // Delegate to skigen CSP (reuse fitted filters stored in m_filters)
    // We reconstruct a fitted CSP instance by directly computing features
    // using the stored filters, matching the skigen transform logic.
    std::vector<MatrixXd> e(epochs.cbegin(), epochs.cend());

    Skigen::CSP<double> csp(m_nComponents);
    // Use the stored filters directly rather than re-fitting
    const int nEpochs = static_cast<int>(e.size());
    const int nComp = static_cast<int>(m_filters.rows());
    MatrixXd features(nEpochs, nComp);

    for (int ep = 0; ep < nEpochs; ++ep) {
        MatrixXd filtered = m_filters * e[static_cast<size_t>(ep)];
        for (int c = 0; c < nComp; ++c) {
            double var = filtered.row(c).squaredNorm()
                       / static_cast<double>(filtered.cols());
            features(ep, c) = std::log(var);
        }
    }

    // Normalize: subtract per-epoch mean for scale invariance
    for (int ep = 0; ep < nEpochs; ++ep) {
        features.row(ep).array() -= features.row(ep).mean();
    }

    return features;
}

//=============================================================================================================

MatrixXd MlCsp::fitTransform(const QList<MatrixXd>& epochsClass1,
                               const QList<MatrixXd>& epochsClass2)
{
    fit(epochsClass1, epochsClass2);

    QList<MatrixXd> allEpochs;
    allEpochs.append(epochsClass1);
    allEpochs.append(epochsClass2);

    return transform(allEpochs);
}
