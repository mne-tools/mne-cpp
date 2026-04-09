//=============================================================================================================
/**
 * @file     xdawn.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
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
 * @brief    Implementation of the Xdawn class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xdawn.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QHash>

//=============================================================================================================
// C++ INCLUDES
//=============================================================================================================

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// PRIVATE HELPERS
//=============================================================================================================

namespace {

MatrixXd computePatterns(const MatrixXd& filters, const MatrixXd& dataCov)
{
    if (filters.size() == 0 || dataCov.size() == 0) {
        return {};
    }

    MatrixXd gram = filters.transpose() * dataCov * filters;
    CompleteOrthogonalDecomposition<MatrixXd> cod(gram);
    return dataCov * filters * cod.pseudoInverse();
}

} // anonymous namespace

//=============================================================================================================
// MEMBER DEFINITIONS
//=============================================================================================================

XdawnResult Xdawn::fit(const QVector<MNEEpochData>& epochs,
                       int                          iTargetEvent,
                       int                          nComponents,
                       double                       dReg)
{
    XdawnResult result;
    result.iTargetEvent = iTargetEvent;

    if (epochs.isEmpty()) {
        qWarning() << "Xdawn::fit: empty epoch list.";
        return result;
    }

    QVector<int> goodIdx;
    goodIdx.reserve(epochs.size());
    for (int i = 0; i < epochs.size(); ++i) {
        if (!epochs[i].bReject) {
            goodIdx.append(i);
        }
    }

    if (goodIdx.isEmpty()) {
        qWarning() << "Xdawn::fit: no non-rejected epochs available.";
        return result;
    }

    const int nCh = static_cast<int>(epochs[goodIdx[0]].epoch.rows());
    const int nSamp = static_cast<int>(epochs[goodIdx[0]].epoch.cols());
    if (nCh == 0 || nSamp == 0) {
        qWarning() << "Xdawn::fit: epoch matrices are empty.";
        return result;
    }

    for (int idx : goodIdx) {
        if (epochs[idx].epoch.rows() != nCh || epochs[idx].epoch.cols() != nSamp) {
            qWarning() << "Xdawn::fit: epoch dimension mismatch.";
            return result;
        }
    }

    nComponents = std::max(1, std::min(nComponents, nCh));

    QHash<int, MatrixXd> classSums;
    QHash<int, int> classCounts;
    MatrixXd targetSum = MatrixXd::Zero(nCh, nSamp);
    int nTarget = 0;

    for (int idx : goodIdx) {
        const MNEEpochData& ep = epochs[idx];
        if (!classSums.contains(ep.event)) {
            classSums.insert(ep.event, MatrixXd::Zero(nCh, nSamp));
            classCounts.insert(ep.event, 0);
        }

        classSums[ep.event] += ep.epoch;
        classCounts[ep.event] += 1;

        if (ep.event == iTargetEvent) {
            targetSum += ep.epoch;
            ++nTarget;
        }
    }

    if (nTarget == 0) {
        qWarning() << "Xdawn::fit: no target epochs found for event" << iTargetEvent;
        return result;
    }

    result.matTargetEvoked = targetSum / static_cast<double>(nTarget);

    MatrixXd noiseCov = MatrixXd::Zero(nCh, nCh);
    MatrixXd dataCov  = MatrixXd::Zero(nCh, nCh);
    long long nNoiseSamples = 0;
    long long nDataSamples  = 0;

    QHash<int, MatrixXd> classMeans;
    for (auto it = classSums.constBegin(); it != classSums.constEnd(); ++it) {
        classMeans.insert(it.key(), it.value() / static_cast<double>(classCounts.value(it.key())));
    }

    for (int idx : goodIdx) {
        const MNEEpochData& ep = epochs[idx];
        const MatrixXd residual = ep.epoch - classMeans.value(ep.event);

        dataCov  += ep.epoch * ep.epoch.transpose();
        noiseCov += residual * residual.transpose();
        nDataSamples  += nSamp;
        nNoiseSamples += nSamp;
    }

    if (nNoiseSamples <= 0 || nDataSamples <= 0) {
        qWarning() << "Xdawn::fit: failed to accumulate covariance samples.";
        return result;
    }

    result.matSignalCov = result.matTargetEvoked * result.matTargetEvoked.transpose() / static_cast<double>(nSamp);
    result.matNoiseCov  = noiseCov / static_cast<double>(nNoiseSamples);
    dataCov             = dataCov  / static_cast<double>(nDataSamples);

    const double traceNoise = result.matNoiseCov.trace();
    const double regValue = std::max(dReg, 0.0) * ((traceNoise > 0.0) ? traceNoise / static_cast<double>(nCh) : 1.0);
    MatrixXd regNoiseCov = result.matNoiseCov;
    regNoiseCov.diagonal().array() += regValue;

    SelfAdjointEigenSolver<MatrixXd> noiseEig(regNoiseCov);
    if (noiseEig.info() != Success) {
        qWarning() << "Xdawn::fit: noise covariance eigendecomposition failed.";
        return result;
    }

    VectorXd noiseVals = noiseEig.eigenvalues().cwiseMax(1e-12);
    MatrixXd noiseVecs = noiseEig.eigenvectors();
    MatrixXd invSqrtNoise = noiseVecs * noiseVals.cwiseInverse().cwiseSqrt().asDiagonal() * noiseVecs.transpose();

    MatrixXd whitenedSignal = invSqrtNoise * result.matSignalCov * invSqrtNoise;
    SelfAdjointEigenSolver<MatrixXd> signalEig(whitenedSignal);
    if (signalEig.info() != Success) {
        qWarning() << "Xdawn::fit: signal covariance eigendecomposition failed.";
        return result;
    }

    result.matFilters.resize(nCh, nComponents);
    const MatrixXd signalVecsAsc = signalEig.eigenvectors().rightCols(nComponents);
    MatrixXd signalVecs(signalVecsAsc.rows(), signalVecsAsc.cols());
    for (int i = 0; i < nComponents; ++i) {
        signalVecs.col(i) = signalVecsAsc.col(nComponents - 1 - i);
    }

    result.matFilters = invSqrtNoise * signalVecs;

    for (int col = 0; col < result.matFilters.cols(); ++col) {
        const double noiseNorm = std::sqrt(result.matFilters.col(col).transpose()
                                           * regNoiseCov
                                           * result.matFilters.col(col));
        if (noiseNorm > 1e-12) {
            result.matFilters.col(col) /= noiseNorm;
        }
    }

    result.matPatterns = computePatterns(result.matFilters, dataCov);
    result.bValid = true;
    return result;
}

//=============================================================================================================

MatrixXd Xdawn::apply(const MatrixXd& matEpoch, const XdawnResult& result)
{
    if (!result.bValid || result.matFilters.size() == 0) {
        return {};
    }

    if (matEpoch.rows() != result.matFilters.rows()) {
        qWarning() << "Xdawn::apply: channel count mismatch.";
        return {};
    }

    return result.matFilters.transpose() * matEpoch;
}

//=============================================================================================================

MatrixXd Xdawn::denoise(const MatrixXd& matEpoch, const XdawnResult& result, int nComponents)
{
    if (!result.bValid || result.matFilters.size() == 0 || result.matPatterns.size() == 0) {
        return matEpoch;
    }

    if (matEpoch.rows() != result.matFilters.rows()) {
        qWarning() << "Xdawn::denoise: channel count mismatch.";
        return {};
    }

    if (nComponents <= 0 || nComponents > result.matFilters.cols()) {
        nComponents = result.matFilters.cols();
    }

    MatrixXd filters  = result.matFilters.leftCols(nComponents);
    MatrixXd patterns = result.matPatterns.leftCols(nComponents);

    return patterns * (filters.transpose() * matEpoch);
}

//=============================================================================================================

QVector<MNEEpochData> Xdawn::denoiseEpochs(const QVector<MNEEpochData>& epochs,
                                          const XdawnResult&           result,
                                          int                          nComponents)
{
    QVector<MNEEpochData> out = epochs;
    for (int i = 0; i < out.size(); ++i) {
        out[i].epoch = denoise(out[i].epoch, result, nComponents);
    }
    return out;
}
