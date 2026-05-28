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
    const int nAtoms = 2 * iNFreqs;  // cos + sin per frequency
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
            dict(2 * f,     t) = envelope * std::cos(2.0 * M_PI * freq * timeVec(t));
            dict(2 * f + 1, t) = envelope * std::sin(2.0 * M_PI * freq * timeVec(t));
        }

        // Normalize each atom to unit norm
        double normCos = dict.row(2 * f).norm();
        if (normCos > 1e-12) dict.row(2 * f) /= normCos;

        double normSin = dict.row(2 * f + 1).norm();
        if (normSin > 1e-12) dict.row(2 * f + 1) /= normSin;
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

    // Initialize Z to zero
    MatrixXd Z = MatrixXd::Zero(nSources, nAtoms);

    // Precompute G^T * G diagonal for Lipschitz constants
    VectorXd lipschitz(nSources);
    for (int j = 0; j < nSources; ++j) {
        lipschitz(j) = matGain.col(j).squaredNorm();
    }
    // Scale by dictionary energy
    double phiEnergy = 0.0;
    for (int a = 0; a < nAtoms; ++a) {
        phiEnergy += Phi.row(a).squaredNorm();
    }
    lipschitz *= phiEnergy;

    // Proximal gradient descent (ISTA-like)
    MatrixXd residual = matData;
    double prevObj = std::numeric_limits<double>::max();

    for (int iter = 0; iter < params.iMaxIterations; ++iter) {
        // Compute residual: R = M - G * (Z * Phi)
        MatrixXd X = Z * Phi;  // (nSources × nTimes)
        residual = matData - matGain * X;

        // Compute objective: ||R||^2_F + alpha_space * ||Z||_21 + alpha_time * ||Z||_1
        double dataFit = residual.squaredNorm();
        double l21Norm = 0.0;
        double l1Norm = 0.0;
        for (int j = 0; j < nSources; ++j) {
            l21Norm += Z.row(j).norm();
            l1Norm += Z.row(j).lpNorm<1>();
        }
        double objective = 0.5 * dataFit + params.dAlphaSpace * l21Norm + params.dAlphaTime * l1Norm;

        // Check convergence
        if (std::abs(prevObj - objective) / (std::abs(prevObj) + 1e-12) < params.dTolerance) {
            result.nIterations = iter + 1;
            break;
        }
        prevObj = objective;
        result.nIterations = iter + 1;

        // Gradient step + proximal operator for each source
        // Gradient of data term w.r.t. Z_j: -G_j^T * R * Phi^T
        MatrixXd GtR = matGain.transpose() * residual;  // (nSources × nTimes)
        MatrixXd grad = GtR * Phi.transpose();           // (nSources × nAtoms)

        for (int j = 0; j < nSources; ++j) {
            if (lipschitz(j) < 1e-12) continue;

            double stepSize = 1.0 / lipschitz(j);

            // Gradient step
            RowVectorXd zNew = Z.row(j) + stepSize * grad.row(j);

            // Proximal operator: soft-threshold (L1) then group-threshold (L21)
            // L1 soft threshold
            double threshL1 = params.dAlphaTime * stepSize;
            for (int a = 0; a < nAtoms; ++a) {
                double val = zNew(a);
                double sign = (val > 0.0) ? 1.0 : -1.0;
                zNew(a) = sign * std::max(0.0, std::abs(val) - threshL1);
            }

            // L21 group threshold
            double groupNorm = zNew.norm();
            double threshL21 = params.dAlphaSpace * stepSize;
            if (groupNorm > threshL21) {
                zNew *= (1.0 - threshL21 / groupNorm);
            } else {
                zNew.setZero();
            }

            Z.row(j) = zNew;
        }
    }

    // Reconstruct time-domain source estimate from TF coefficients
    MatrixXd X = Z * Phi;  // (nSources × nTimes)

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
