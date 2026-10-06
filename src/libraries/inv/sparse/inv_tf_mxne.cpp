//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_tf_mxne.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of the TF-MxNE solver including the Gabor-dictionary builder.
 *
 * Implements the Gabor tight-frame dictionary @c buildGaborDictionary
 * (complex Morlet atoms tiled across the requested frequency band) and
 * the FISTA-style accelerated proximal-gradient loop that alternates
 * the L21 spatial prox and the L1 temporal prox until convergence.
 * The final solution is optionally debiased by re-fitting only the
 * selected support without regularisation before being projected back
 * to the time domain into the output @ref InvSourceEstimate.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_tf_mxne.h"

#include <Eigen/SVD>

#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtMath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXd InvTfMxne::buildGaborDictionary(int iNSamples, int iNFreqs,
                                         double dFMin, double dFMax,
                                         double dSFreq)
{
    // Build a set of Gabor atoms: windowed complex exponentials at different frequencies
    // Returns real-valued matrix: for each frequency, we store cos and sin rows
    const int nAtoms = 2 * iNFreqs; // cos + sin per frequency
    MatrixXd dict = MatrixXd::Zero(nAtoms, iNSamples);

    VectorXd timeVec(iNSamples);
    for (int t = 0; t < iNSamples; ++t) {
        timeVec(t) = static_cast<double>(t) / dSFreq;
    }

    // Log-spaced frequencies between fMin and fMax
    for (int f = 0; f < iNFreqs; ++f) {
        double freq;
        if (iNFreqs > 1) {
            double logMin = std::log(dFMin);
            double logMax = std::log(dFMax);
            freq = std::exp(logMin + (logMax - logMin) * f / (iNFreqs - 1));
        } else {
            freq = (dFMin + dFMax) / 2.0;
        }

        // Gaussian window width: ~3 cycles at this frequency
        double sigma = 3.0 / (2.0 * M_PI * freq);

        double tCenter = timeVec(iNSamples / 2);

        for (int t = 0; t < iNSamples; ++t) {
            double dt = timeVec(t) - tCenter;
            double envelope = std::exp(-0.5 * dt * dt / (sigma * sigma));
            dict(2 * f, t) = envelope * std::cos(2.0 * M_PI * freq * timeVec(t));
            dict(2 * f + 1, t) = envelope * std::sin(2.0 * M_PI * freq * timeVec(t));
        }

        // Normalize each atom to unit norm
        double normCos = dict.row(2 * f).norm();
        if (normCos > 1e-12)
            dict.row(2 * f) /= normCos;

        double normSin = dict.row(2 * f + 1).norm();
        if (normSin > 1e-12)
            dict.row(2 * f + 1) /= normSin;
    }

    return dict;
}

//=============================================================================================================

InvTfMxneResult InvTfMxne::compute(const MatrixXd& matGain,
                                   const MatrixXd& matData,
                                   const InvTfMxneParams& params)
{
    InvTfMxneResult result;

    const int nChannels = matGain.rows();
    const int nSources = matGain.cols();
    const int nTimes = matData.cols();

    if (nChannels == 0 || nSources == 0 || nTimes == 0) {
        return result;
    }
    const int nAtoms = 2 * params.iNFreqs;

    if (matData.rows() != nChannels) {
        qWarning() << "[InvTfMxne::compute] Dimension mismatch: data rows" << matData.rows()
                   << "!= gain rows" << nChannels;
        return result;
    }

    // Build Gabor dictionary: (nAtoms × nTimes)
    MatrixXd Phi = buildGaborDictionary(nTimes, params.iNFreqs,
                                        params.dFMin, params.dFMax,
                                        params.dSFreq);

    // TF coefficients: Z (nSources × nAtoms)
    // The model is: M = G * X, where X = Z * Phi (each source has TF representation)
    // Equivalently in expanded form: M = G_expanded * z_vec
    // where G_expanded = G ⊗ Phi^T, z_vec = vec(Z)
    // But we solve iteratively using Block Coordinate Descent.

    // FISTA on 0.5 ||M - G Z Phi||^2 + alpha_space ||Z||_21 + alpha_time ||Z||_1. The step is 1/L with the global
    // Lipschitz constant L = ||G||_2^2 ||Phi||_2^2; per-source steps on a joint gradient overshoot and stall.
    const double gNorm = JacobiSVD<MatrixXd>(matGain).singularValues()(0);
    const double phiNorm = JacobiSVD<MatrixXd>(Phi).singularValues()(0);
    const double lipschitz = gNorm * gNorm * phiNorm * phiNorm;

    // Sparse-group prox: soft-threshold each coefficient, then shrink the source's group norm.
    const auto prox = [&](MatrixXd& V) {
        const double threshL1 = params.dAlphaTime / lipschitz;
        const double threshL21 = params.dAlphaSpace / lipschitz;
        V = V.array().sign() * (V.array().abs() - threshL1).max(0.0);
        for (int j = 0; j < nSources; ++j) {
            const double groupNorm = V.row(j).norm();
            if (groupNorm > threshL21)
                V.row(j) *= 1.0 - threshL21 / groupNorm;
            else
                V.row(j).setZero();
        }
    };
    const auto objectiveOf = [&](const MatrixXd& V) {
        return 0.5 * (matData - matGain * V * Phi).squaredNorm() + params.dAlphaSpace * V.rowwise().norm().sum() + params.dAlphaTime * V.cwiseAbs().sum();
    };

    MatrixXd Z = MatrixXd::Zero(nSources, nAtoms);
    MatrixXd Y = Z;
    double tk = 1.0;
    double prevObj = objectiveOf(Z);
    for (int iter = 0; iter < params.iMaxIterations; ++iter) {
        MatrixXd zNew = Y + matGain.transpose() * (matData - matGain * Y * Phi) * Phi.transpose() / lipschitz;
        prox(zNew);
        const double tNew = 0.5 * (1.0 + std::sqrt(1.0 + 4.0 * tk * tk));
        Y = zNew + ((tk - 1.0) / tNew) * (zNew - Z);
        Z = zNew;
        tk = tNew;
        result.nIterations = iter + 1;
        const double objective = objectiveOf(Z);
        if (std::abs(prevObj - objective) <= params.dTolerance * std::abs(objective))
            break;
        prevObj = objective;
    }
    MatrixXd residual = matData - matGain * Z * Phi;

    // Reconstruct time-domain source estimate from TF coefficients
    MatrixXd X = Z * Phi; // (nSources × nTimes)

    // Find active sources
    QVector<int> activeVertices;
    for (int j = 0; j < nSources; ++j) {
        if (Z.row(j).norm() > 1e-12) {
            activeVertices.append(j);
        }
    }

    // Optional debiasing: re-estimate amplitudes on active set
    MatrixXd finalX;
    if (params.bDebias && !activeVertices.isEmpty()) {
        MatrixXd Gactive(nChannels, activeVertices.size());
        for (int i = 0; i < activeVertices.size(); ++i) {
            Gactive.col(i) = matGain.col(activeVertices[i]);
        }
        // Least-squares on active set: X_active = pinv(G_active) * M
        finalX = Gactive.bdcSvd<ComputeThinU | ComputeThinV>().solve(matData);
    } else if (!activeVertices.isEmpty()) {
        finalX = MatrixXd(activeVertices.size(), nTimes);
        for (int i = 0; i < activeVertices.size(); ++i) {
            finalX.row(i) = X.row(activeVertices[i]);
        }
    } else {
        finalX = MatrixXd::Zero(0, nTimes);
    }

    // Build result
    VectorXi vertices(activeVertices.size());
    for (int i = 0; i < activeVertices.size(); ++i) {
        vertices(i) = activeVertices[i];
    }

    result.stc = InvSourceEstimate(finalX, vertices, 0.0f,
                                   static_cast<float>(1.0 / params.dSFreq));
    result.stc.method = InvEstimateMethod::MixedNorm;
    result.activeVertices = activeVertices;
    result.residualNorm = residual.norm();

    // Store TF coefficients for active sources
    if (!activeVertices.isEmpty()) {
        result.tfCoefficients = MatrixXd(activeVertices.size(), nAtoms);
        for (int i = 0; i < activeVertices.size(); ++i) {
            result.tfCoefficients.row(i) = Z.row(activeVertices[i]);
        }
    }

    return result;
}
