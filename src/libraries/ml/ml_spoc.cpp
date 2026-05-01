//=============================================================================================================
/**
 * @file     ml_spoc.cpp
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
 * @brief    Implementation of MlSpoc.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_spoc.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>

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

MlSpoc::MlSpoc(int nComponents)
    : m_nComponents(nComponents)
    , m_bFitted(false)
{
}

//=============================================================================================================

void MlSpoc::fit(const QList<MatrixXd>& epochs,
                 const VectorXd& target)
{
    m_bFitted = false;

    if (epochs.isEmpty() || target.size() == 0) {
        qWarning() << "[MlSpoc::fit] Empty epoch list or target.";
        return;
    }

    const int nEpochs = epochs.size();
    if (target.size() != nEpochs) {
        qWarning() << "[MlSpoc::fit] Epoch count" << nEpochs
                    << "!= target size" << target.size();
        return;
    }

    const int nCh = static_cast<int>(epochs[0].rows());

    // 1. Normalise target to zero mean, unit variance
    double zMean = target.mean();
    double zStd = std::sqrt((target.array() - zMean).square().sum()
                            / static_cast<double>(nEpochs - 1));
    VectorXd z = (target.array() - zMean);
    if (zStd > 1e-15)
        z /= zStd;

    // 2. Compute per-epoch covariance and accumulate C and Cz
    MatrixXd C = MatrixXd::Zero(nCh, nCh);
    MatrixXd Cz = MatrixXd::Zero(nCh, nCh);

    for (int e = 0; e < nEpochs; ++e) {
        const MatrixXd& X = epochs[e];
        if (X.rows() != nCh) {
            qWarning() << "[MlSpoc::fit] Epoch" << e << "has" << X.rows()
                        << "channels, expected" << nCh;
            return;
        }

        // Demean across time
        MatrixXd Xc = X.colwise() - X.rowwise().mean().col(0);

        // Sample covariance (scaled by number of time points)
        MatrixXd covE = (Xc * Xc.transpose()) / static_cast<double>(Xc.cols());

        C += covE;
        Cz += z[e] * covE;
    }

    C /= static_cast<double>(nEpochs);
    Cz /= static_cast<double>(nEpochs);

    // 3. Solve generalised eigenvalue problem: Cz · w = λ · C · w
    GeneralizedSelfAdjointEigenSolver<MatrixXd> solver(Cz, C);

    if (solver.info() != Success) {
        qWarning() << "[MlSpoc::fit] Eigenvalue decomposition failed.";
        return;
    }

    // Eigenvalues are sorted ascending; we want the largest magnitude
    const VectorXd& allEvals = solver.eigenvalues();
    const MatrixXd& allEvecs = solver.eigenvectors();

    // Sort by absolute eigenvalue (descending)
    std::vector<int> sortIdx(nCh);
    std::iota(sortIdx.begin(), sortIdx.end(), 0);
    std::sort(sortIdx.begin(), sortIdx.end(), [&](int a, int b) {
        return std::abs(allEvals[a]) > std::abs(allEvals[b]);
    });

    int nComp = std::min(m_nComponents, nCh);
    m_matFilters.resize(nComp, nCh);
    m_vecEigenvalues.resize(nComp);

    for (int i = 0; i < nComp; ++i) {
        int idx = sortIdx[static_cast<size_t>(i)];
        m_vecEigenvalues[i] = allEvals[idx];

        VectorXd w = allEvecs.col(idx);
        // Normalise filter to unit norm
        double norm = w.norm();
        if (norm > 0.0)
            w /= norm;
        m_matFilters.row(i) = w.transpose();
    }

    // 4. Compute patterns: A = C · W · inv(W^T · C · W)
    MatrixXd W = m_matFilters.transpose();  // nCh × nComp
    MatrixXd CW = C * W;
    MatrixXd WtCW = W.transpose() * CW;
    MatrixXd WtCW_inv = WtCW.inverse();
    m_matPatterns = (CW * WtCW_inv).transpose();  // nComp × nCh

    m_bFitted = true;
}

//=============================================================================================================

MatrixXd MlSpoc::transform(const QList<MatrixXd>& epochs) const
{
    if (!m_bFitted) {
        qWarning() << "[MlSpoc::transform] SPoC not fitted.";
        return MatrixXd();
    }

    const int nEpochs = epochs.size();
    const int nComp = static_cast<int>(m_matFilters.rows());
    MatrixXd features(nEpochs, nComp);

    for (int e = 0; e < nEpochs; ++e) {
        // Apply spatial filter: s = W * X  (nComp × nTimes)
        MatrixXd s = m_matFilters * epochs[e];

        // Log-variance feature
        for (int c = 0; c < nComp; ++c) {
            double var = s.row(c).squaredNorm() / static_cast<double>(s.cols());
            features(e, c) = std::log(std::max(var, 1e-30));
        }
    }

    return features;
}

//=============================================================================================================

MatrixXd MlSpoc::fitTransform(const QList<MatrixXd>& epochs,
                               const VectorXd& target)
{
    fit(epochs, target);
    return transform(epochs);
}
