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
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Eigenvalues>

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

    const int nChannels = epochsClass1[0].rows();

    // Compute average covariance for each class
    MatrixXd cov1 = MatrixXd::Zero(nChannels, nChannels);
    for (const auto& epoch : epochsClass1) {
        MatrixXd centered = epoch.colwise() - epoch.rowwise().mean();
        cov1 += centered * centered.transpose() / static_cast<double>(epoch.cols() - 1);
    }
    cov1 /= epochsClass1.size();

    MatrixXd cov2 = MatrixXd::Zero(nChannels, nChannels);
    for (const auto& epoch : epochsClass2) {
        MatrixXd centered = epoch.colwise() - epoch.rowwise().mean();
        cov2 += centered * centered.transpose() / static_cast<double>(epoch.cols() - 1);
    }
    cov2 /= epochsClass2.size();

    // Composite covariance
    MatrixXd covComposite = cov1 + cov2;

    // Whitening transform: W = D^(-1/2) * U^T from eigendecomp of composite
    SelfAdjointEigenSolver<MatrixXd> eigComposite(covComposite);
    VectorXd d = eigComposite.eigenvalues();
    MatrixXd U = eigComposite.eigenvectors();

    // Regularize: clamp small eigenvalues
    double dMin = d.maxCoeff() * 1e-10;
    for (int i = 0; i < d.size(); ++i) {
        if (d(i) < dMin) d(i) = dMin;
    }

    VectorXd dInvSqrt = d.array().sqrt().inverse();
    MatrixXd W = dInvSqrt.asDiagonal() * U.transpose();

    // Whiten class covariances
    MatrixXd S1 = W * cov1 * W.transpose();

    // Eigendecompose whitened class-1 covariance
    SelfAdjointEigenSolver<MatrixXd> eigS1(S1);
    VectorXd lambdas = eigS1.eigenvalues();
    MatrixXd B = eigS1.eigenvectors();

    // CSP filters = selected columns of B^T * W
    // Eigenvalues are sorted ascending — first columns maximize class 2,
    // last columns maximize class 1
    MatrixXd allFilters = B.transpose() * W;  // (nChannels × nChannels)

    // Select top and bottom components
    int nPerClass = m_nComponents / 2;
    int nTotal = std::min(m_nComponents, nChannels);
    nPerClass = std::min(nPerClass, nChannels / 2);

    m_filters = MatrixXd(nTotal, nChannels);
    m_eigenvalues = VectorXd(nTotal);

    // Bottom nPerClass (maximize class 2 variance)
    for (int i = 0; i < nPerClass; ++i) {
        m_filters.row(i) = allFilters.row(i);
        m_eigenvalues(i) = lambdas(i);
    }
    // Top nPerClass (maximize class 1 variance)
    for (int i = 0; i < nTotal - nPerClass; ++i) {
        m_filters.row(nPerClass + i) = allFilters.row(nChannels - 1 - i);
        m_eigenvalues(nPerClass + i) = lambdas(nChannels - 1 - i);
    }

    // Compute patterns: A = Cov_composite * W^T * (W * Cov_composite * W^T)^{-1}
    // Simplified: patterns = pinv(filters)^T
    m_patterns = m_filters.bdcSvd(ComputeThinU | ComputeThinV).solve(
        MatrixXd::Identity(nTotal, nTotal)).transpose();

    m_bFitted = true;
}

//=============================================================================================================

MatrixXd MlCsp::transform(const QList<MatrixXd>& epochs) const
{
    if (!m_bFitted) {
        qWarning() << "[MlCsp::transform] CSP not fitted.";
        return MatrixXd();
    }

    MatrixXd features(epochs.size(), m_nComponents);

    for (int e = 0; e < epochs.size(); ++e) {
        // Apply spatial filters
        MatrixXd filtered = m_filters * epochs[e];  // (nComponents × nTimes)

        // Compute log-variance for each component
        for (int c = 0; c < m_nComponents; ++c) {
            double variance = filtered.row(c).squaredNorm() / static_cast<double>(filtered.cols());
            features(e, c) = std::log(variance);
        }
    }

    // Normalize features (subtract mean log-variance per epoch for scale invariance)
    for (int e = 0; e < epochs.size(); ++e) {
        double mean = features.row(e).mean();
        features.row(e).array() -= mean;
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
