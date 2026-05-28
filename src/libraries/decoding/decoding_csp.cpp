//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file decoding_csp.cpp
 * @since 2026
 * @date  May 2026
 * @brief Implementation of the Common Spatial Patterns decoder.
 *
 * Estimates the per-class covariance matrices from a list of labelled,
 * band-passed epochs, solves the generalised eigenproblem
 * @f$\Sigma_1 w = \lambda (\Sigma_1 + \Sigma_2) w@f$ with Eigen's
 * @c GeneralizedSelfAdjointEigenSolver, and keeps the @c n_components
 * eigenvectors with the largest and smallest eigenvalues as the bank of
 * spatial filters; the corresponding activation patterns are recovered
 * from the pseudoinverse computed via a thin SVD so that ill-conditioned
 * filter matrices degrade gracefully instead of throwing.
 *
 * @ref DecodingCsp::transform reduces an epoch tensor either to one
 * log-transformed (or z-scored) band-power value per component or to
 * the time-resolved CSP-space projection, depending on the
 * @c TransformMode selected at construction. @ref DecodingCsp::inverseTransform
 * maps a power-feature matrix back to sensor space by weighting the
 * patterns with the per-feature scores, which is what the application
 * layer uses to render CSP topographies side-by-side with the
 * classification output.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "decoding_csp.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Eigenvalues>
#include <Eigen/SVD>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DECODINGLIB;
using namespace Eigen;

//=============================================================================================================

DecodingCsp::DecodingCsp(int nComponents,
                         TransformMode transformInto,
                         bool useLog)
    : m_nComponents(nComponents)
    , m_transformInto(transformInto)
    , m_useLog(useLog)
{
}

//=============================================================================================================

void DecodingCsp::fit(const std::vector<MatrixXd>& epochs,
                      const VectorXi& y)
{
    if (epochs.empty()) {
        throw std::invalid_argument("DecodingCsp::fit: epochs must be non-empty");
    }
    if (static_cast<int>(epochs.size()) != y.size()) {
        throw std::invalid_argument(
            "DecodingCsp::fit: epochs and y must have the same length");
    }

    // Identify unique classes
    std::set<int> classSet(y.data(), y.data() + y.size());
    if (classSet.size() != 2) {
        throw std::invalid_argument(
            "DecodingCsp::fit: y must contain exactly 2 unique class labels");
    }

    auto it = classSet.begin();
    int label1 = *it++;
    int label2 = *it;

    // Split epochs by class
    std::vector<MatrixXd> epochs1, epochs2;
    for (int i = 0; i < y.size(); ++i) {
        if (y(i) == label1) {
            epochs1.push_back(epochs[static_cast<size_t>(i)]);
        } else {
            epochs2.push_back(epochs[static_cast<size_t>(i)]);
        }
    }

    const auto n_ch = epochs1[0].rows();

    // Average covariance for each class
    MatrixXd cov1 = MatrixXd::Zero(n_ch, n_ch);
    for (const auto& epoch : epochs1) {
        MatrixXd centered = epoch.colwise() - epoch.rowwise().mean();
        cov1 += centered * centered.transpose()
                / static_cast<double>(epoch.cols() - 1);
    }
    cov1 /= static_cast<double>(epochs1.size());

    MatrixXd cov2 = MatrixXd::Zero(n_ch, n_ch);
    for (const auto& epoch : epochs2) {
        MatrixXd centered = epoch.colwise() - epoch.rowwise().mean();
        cov2 += centered * centered.transpose()
                / static_cast<double>(epoch.cols() - 1);
    }
    cov2 /= static_cast<double>(epochs2.size());

    // Composite covariance
    MatrixXd cov_comp = cov1 + cov2;

    // Whitening: W = D^{-1/2} U^T
    SelfAdjointEigenSolver<MatrixXd> eig_comp(cov_comp);
    VectorXd d = eig_comp.eigenvalues();
    MatrixXd U = eig_comp.eigenvectors();

    const double d_min = d.maxCoeff() * 1e-10;
    for (Index i = 0; i < d.size(); ++i) {
        if (d(i) < d_min) d(i) = d_min;
    }

    MatrixXd W = d.array().sqrt().inverse().matrix().asDiagonal()
                 * U.transpose();

    // Whiten class-1 covariance and eigendecompose
    MatrixXd S1 = W * cov1 * W.transpose();
    SelfAdjointEigenSolver<MatrixXd> eig_s1(S1);
    VectorXd lambdas = eig_s1.eigenvalues();
    MatrixXd B = eig_s1.eigenvectors();

    // All filters in eigenvalue order (ascending)
    MatrixXd all_filters = B.transpose() * W;

    // Select first and last components
    int n_per_class = m_nComponents / 2;
    int n_total = std::min(m_nComponents, static_cast<int>(n_ch));
    n_per_class = std::min(n_per_class, static_cast<int>(n_ch) / 2);

    m_filters = MatrixXd(n_total, n_ch);
    VectorXd eigenvalues(n_total);

    // Bottom n_per_class (maximize class-2 variance)
    for (int i = 0; i < n_per_class; ++i) {
        m_filters.row(i) = all_filters.row(i);
        eigenvalues(i) = lambdas(i);
    }
    // Top (n_total - n_per_class) (maximize class-1 variance)
    for (int i = 0; i < n_total - n_per_class; ++i) {
        m_filters.row(n_per_class + i) =
            all_filters.row(static_cast<int>(n_ch) - 1 - i);
        eigenvalues(n_per_class + i) =
            lambdas(static_cast<int>(n_ch) - 1 - i);
    }

    // Patterns = pinv(filters) — shape (n_ch × n_comp)
    auto svd = m_filters.bdcSvd<ComputeThinU | ComputeThinV>();
    m_patterns = svd.solve(MatrixXd::Identity(n_total, n_total));

    // Compute mean band power for z-score normalisation
    MatrixXd powerFeatures = computePowerFeatures(epochs);
    m_mean = powerFeatures.colwise().mean();

    VectorXd centered = VectorXd(powerFeatures.rows());
    m_std = VectorXd(powerFeatures.cols());
    for (int c = 0; c < powerFeatures.cols(); ++c) {
        centered = powerFeatures.col(c).array() - m_mean(c);
        m_std(c) = std::sqrt(centered.squaredNorm()
                             / static_cast<double>(centered.size()));
    }

    m_fitted = true;
}

//=============================================================================================================

MatrixXd DecodingCsp::transform(const std::vector<MatrixXd>& epochs) const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingCsp::transform: not fitted");
    }

    if (m_transformInto == TransformMode::CspSpace) {
        // Return data in CSP space: (n_epochs * n_components, n_times)
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
        // z-score
        for (int c = 0; c < X.cols(); ++c) {
            double s = m_std(c);
            if (s < 1e-15) s = 1.0;
            X.col(c) = (X.col(c).array() - m_mean(c)) / s;
        }
    }

    return X;
}

//=============================================================================================================

MatrixXd DecodingCsp::fitTransform(const std::vector<MatrixXd>& epochs,
                                   const VectorXi& y)
{
    fit(epochs, y);
    return transform(epochs);
}

//=============================================================================================================

MatrixXd DecodingCsp::inverseTransform(const MatrixXd& X) const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingCsp::inverseTransform: not fitted");
    }
    if (m_transformInto != TransformMode::AveragePower) {
        throw std::runtime_error(
            "DecodingCsp::inverseTransform: only valid for AveragePower mode");
    }

    const int nComp = static_cast<int>(m_filters.rows());
    if (X.cols() != nComp) {
        throw std::invalid_argument(
            "DecodingCsp::inverseTransform: X must have n_components columns");
    }

    // patterns_ is (n_channels × n_components), X is (n_epochs × n_components)
    // Result: (n_epochs × n_channels)
    return X * m_patterns.transpose();
}

//=============================================================================================================

const MatrixXd& DecodingCsp::filters() const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingCsp::filters: not fitted");
    }
    return m_filters;
}

//=============================================================================================================

const MatrixXd& DecodingCsp::patterns() const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingCsp::patterns: not fitted");
    }
    return m_patterns;
}

//=============================================================================================================

const VectorXd& DecodingCsp::mean() const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingCsp::mean: not fitted");
    }
    return m_mean;
}

//=============================================================================================================

const VectorXd& DecodingCsp::stddev() const
{
    if (!m_fitted) {
        throw std::runtime_error("DecodingCsp::stddev: not fitted");
    }
    return m_std;
}

//=============================================================================================================

bool DecodingCsp::isFitted() const
{
    return m_fitted;
}

//=============================================================================================================

MatrixXd DecodingCsp::computePowerFeatures(
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
