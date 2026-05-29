//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     decoding_spoc.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of the Source Power Comodulation regression decoder.
 *
 * Builds the unweighted trial-mean covariance @f$\Sigma@f$ and the
 * target-weighted covariance @f$\Sigma_z@f$ from centred labels, solves
 * the generalised eigenproblem @f$\Sigma_z w = \lambda \Sigma w@f$ with
 * Eigen's @c GeneralizedSelfAdjointEigenSolver, and keeps the
 * @c n_components eigenvectors with the largest absolute eigenvalues as
 * spatial filters. Activation patterns are computed as the columns of
 * the pseudoinverse of the filter matrix and stored alongside the per-
 * component mean and standard deviation of the band-power features so
 * that subsequent calls to @ref DecodingSpoc::transform can z-score on
 * demand without re-fitting.
 *
 * The feature path is identical in shape to @ref DecodingCsp: epochs
 * are filtered with the stored spatial filters, then either reduced to
 * one (optionally log- or z-scored) band-power value per component
 * (@c AveragePower) or returned in time-resolved form (@c CspSpace).
 * The continuous-regression nature of SPoC is entirely captured by the
 * use of the target-weighted covariance during fit; downstream the
 * output is just a deterministic feature matrix consumable by any
 * linear regressor.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "decoding_spoc.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Eigenvalues>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <numeric>
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

    // Delegate core GED (SPoC algorithm)
    const auto n_epochs = static_cast<Index>(epochs.size());
    const auto n_ch = epochs[0].rows();

    // 1. Normalize target
    double z_mean = y.mean();
    double z_std = std::sqrt(
        (y.array() - z_mean).square().sum()
        / static_cast<double>(n_epochs - 1));
    VectorXd z = (y.array() - z_mean).matrix();
    if (z_std > 1e-15) z /= z_std;

    // 2. Per-epoch covariance → C and Cz
    MatrixXd C  = MatrixXd::Zero(n_ch, n_ch);
    MatrixXd Cz = MatrixXd::Zero(n_ch, n_ch);

    for (Index e = 0; e < n_epochs; ++e) {
        const auto& X = epochs[static_cast<size_t>(e)];
        MatrixXd Xc = X.colwise() - X.rowwise().mean();
        MatrixXd cov_e = (Xc * Xc.transpose())
                         / static_cast<double>(Xc.cols());
        C  += cov_e;
        Cz += z(e) * cov_e;
    }
    C  /= static_cast<double>(n_epochs);
    Cz /= static_cast<double>(n_epochs);

    // 3. Generalized eigenvalue problem: Cz w = λ C w
    GeneralizedSelfAdjointEigenSolver<MatrixXd> solver(Cz, C);
    if (solver.info() != Eigen::Success) {
        throw std::runtime_error(
            "DecodingSpoc::fit: eigenvalue decomposition failed");
    }

    const VectorXd& all_evals = solver.eigenvalues();
    const MatrixXd& all_evecs = solver.eigenvectors();

    // 4. Sort by |eigenvalue| descending
    std::vector<int> idx(static_cast<size_t>(n_ch));
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](int a, int b) {
        return std::abs(all_evals(a)) > std::abs(all_evals(b));
    });

    int n_comp = std::min(m_nComponents, static_cast<int>(n_ch));
    m_filters.resize(n_comp, n_ch);

    for (int i = 0; i < n_comp; ++i) {
        int j = idx[static_cast<size_t>(i)];
        VectorXd w = all_evecs.col(j);
        double norm = w.norm();
        if (norm > 0.0) w /= norm;
        m_filters.row(i) = w.transpose();
    }

    // 5. Patterns: A = C W inv(W^T C W)
    MatrixXd Wt = m_filters.transpose();
    MatrixXd CW = C * Wt;
    MatrixXd WtCW = Wt.transpose() * CW;
    m_patterns = (CW * WtCW.inverse());  // (n_ch × n_comp) — already correct shape

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
            result.middleRows(static_cast<Eigen::Index>(e) * nComp, nComp) = m_filters * epochs[static_cast<size_t>(e)];
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
