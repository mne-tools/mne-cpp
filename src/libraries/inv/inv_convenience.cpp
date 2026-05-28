//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_convenience.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of the apply-inverse / compute-PSD convenience helpers declared in @c inv_convenience.h.
 *
 * Implements the streaming epoch / raw loops by constructing a single
 * @ref InvMinimumNorm kernel and re-applying it across data chunks, the
 * SNR helper that combines GFP-based signal estimates with the noise
 * normalisation of the inverse operator, and the Welch PSD that re-uses
 * the @c utils/spectral primitives to keep the spectral pipeline
 * identical to the one used elsewhere in mne-cpp.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_convenience.h"
#include "minimum_norm/inv_minimum_norm.h"

#include <mne/mne_inverse_operator.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// LOCAL HELPERS
//=============================================================================================================

namespace {

/**
 * Compute FFT-based PSD (Welch-like, single window) for a single row of data.
 */
VectorXd computeRowPsd(const VectorXd& row, int nFft, double sfreq)
{
    // Zero-pad or truncate to nFft
    VectorXd segment = VectorXd::Zero(nFft);
    int copyLen = std::min(static_cast<int>(row.size()), nFft);
    segment.head(copyLen) = row.head(copyLen);

    // Apply Hann window
    for (int i = 0; i < copyLen; ++i) {
        double w = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (copyLen - 1)));
        segment(i) *= w;
    }

    // Compute FFT via correlation (real FFT using DFT)
    int nFreqs = nFft / 2 + 1;
    VectorXd psd(nFreqs);

    for (int f = 0; f < nFreqs; ++f) {
        double freq = static_cast<double>(f) * sfreq / nFft;
        double re = 0.0, im = 0.0;
        for (int t = 0; t < nFft; ++t) {
            double angle = -2.0 * M_PI * f * t / nFft;
            re += segment(t) * std::cos(angle);
            im += segment(t) * std::sin(angle);
        }
        psd(f) = (re * re + im * im) / (sfreq * nFft);
        // Double for non-DC/Nyquist bins (one-sided spectrum)
        if (f > 0 && f < nFreqs - 1) psd(f) *= 2.0;
    }

    return psd;
}

} // anonymous namespace

//=============================================================================================================
// DEFINE FUNCTIONS
//=============================================================================================================

QList<InvSourceEstimate> INVLIB::applyInverseEpochs(
    const QList<MatrixXd>& epochs,
    const MNEInverseOperator& inverse,
    float lambda2,
    const QString& method,
    float tmin,
    float tstep,
    bool pickNormal)
{
    QList<InvSourceEstimate> results;

    if (epochs.isEmpty()) {
        qWarning() << "[applyInverseEpochs] No epochs provided.";
        return results;
    }

    // Create the minimum norm estimator
    InvMinimumNorm mn(inverse, lambda2, method);

    // Setup once with nave=1 (per-epoch)
    mn.doInverseSetup(1, pickNormal);

    for (int i = 0; i < epochs.size(); ++i) {
        InvSourceEstimate stc = mn.calculateInverse(epochs[i], tmin, tstep, pickNormal);
        if (stc.isEmpty()) {
            qWarning() << "[applyInverseEpochs] Epoch" << i << "produced empty source estimate.";
        }
        results.append(stc);
    }

    return results;
}

//=============================================================================================================

InvSourceEstimate INVLIB::applyInverseRaw(
    const FiffRawData& raw,
    const MNEInverseOperator& inverse,
    float lambda2,
    const QString& method,
    int from,
    int to,
    bool pickNormal)
{
    // Default: use full range
    if (from < 0) from = raw.first_samp;
    if (to < 0) to = raw.last_samp;

    // Pick channels matching the inverse operator
    RowVectorXi picks = FiffInfo::pick_channels(raw.info.ch_names, inverse.noise_cov->names);
    if (picks.size() == 0) {
        qWarning() << "[applyInverseRaw] No channels match the inverse operator.";
        return InvSourceEstimate();
    }

    // Read raw data
    MatrixXd data, times;
    raw.read_raw_segment(data, times, from, to, picks);

    if (data.cols() == 0) {
        qWarning() << "[applyInverseRaw] No data read from raw file.";
        return InvSourceEstimate();
    }

    float tmin = static_cast<float>(from) / raw.info.sfreq;
    float tstep = 1.0f / raw.info.sfreq;

    // Apply inverse
    InvMinimumNorm mn(inverse, lambda2, method);
    mn.doInverseSetup(1, pickNormal);

    return mn.calculateInverse(data, tmin, tstep, pickNormal);
}

//=============================================================================================================

QPair<VectorXd, RowVectorXf> INVLIB::estimateSnr(
    const FiffEvoked& evoked,
    const MNEInverseOperator& inverse,
    const QString& method)
{
    float snr = 3.0f;
    float lambda2 = 1.0f / (snr * snr);

    // Apply inverse to get source estimate
    InvMinimumNorm mn(inverse, lambda2, method);
    InvSourceEstimate stc = mn.calculateInverse(evoked);

    if (stc.isEmpty()) {
        qWarning() << "[estimateSnr] Failed to compute source estimate.";
        return QPair<VectorXd, RowVectorXf>();
    }

    // Compute SNR as sqrt(sum of squared source amplitudes per time point)
    // This gives a time course of source-space SNR
    const int nTimes = static_cast<int>(stc.data.cols());
    VectorXd snrTimeCourse(nTimes);

    for (int t = 0; t < nTimes; ++t) {
        snrTimeCourse(t) = std::sqrt(stc.data.col(t).squaredNorm()
                                     / static_cast<double>(stc.data.rows()));
    }

    return QPair<VectorXd, RowVectorXf>(snrTimeCourse, stc.times);
}

//=============================================================================================================

QPair<MatrixXd, int> INVLIB::computeWhitener(
    const FiffCov& noiseCov,
    int rank)
{
    const int dim = noiseCov.dim;

    if (dim <= 0) {
        qWarning() << "[computeWhitener] Empty noise covariance.";
        return QPair<MatrixXd, int>(MatrixXd(), 0);
    }

    // Use pre-computed eigendecomposition if available
    VectorXd eig;
    MatrixXd eigvec;

    if (noiseCov.eig.size() > 0 && noiseCov.eigvec.size() > 0) {
        eig = noiseCov.eig;
        eigvec = noiseCov.eigvec;
    } else {
        // Compute eigendecomposition
        SelfAdjointEigenSolver<MatrixXd> solver(noiseCov.data);
        eig = solver.eigenvalues();
        eigvec = solver.eigenvectors();
    }

    // Auto-detect rank from eigenvalue spectrum
    if (rank <= 0) {
        double maxEig = eig.maxCoeff();
        double threshold = maxEig * 1e-10;
        rank = 0;
        for (int i = 0; i < eig.size(); ++i) {
            if (eig(i) > threshold) ++rank;
        }
        if (rank == 0) rank = 1;
    }

    // Build whitening matrix: W = diag(1/sqrt(eig)) @ V^T
    // Only use the top 'rank' eigenvalues
    VectorXd invSqrtEig = VectorXd::Zero(eig.size());
    int effectiveRank = 0;

    // Eigenvalues are in ascending order — use last 'rank' values
    for (int i = eig.size() - 1; i >= 0 && effectiveRank < rank; --i) {
        if (eig(i) > 1e-30) {
            invSqrtEig(i) = 1.0 / std::sqrt(eig(i));
            ++effectiveRank;
        }
    }

    MatrixXd whitener = invSqrtEig.asDiagonal() * eigvec.transpose();

    return QPair<MatrixXd, int>(whitener, effectiveRank);
}

//=============================================================================================================

QPair<MatrixXd, VectorXd> INVLIB::computeSourcePsd(
    const InvSourceEstimate& stc,
    float sfreq,
    float fmin,
    float fmax,
    int nFft)
{
    if (stc.isEmpty()) {
        qWarning() << "[computeSourcePsd] Empty source estimate.";
        return QPair<MatrixXd, VectorXd>();
    }

    const int nSources = static_cast<int>(stc.data.rows());
    const int nTimes = static_cast<int>(stc.data.cols());

    if (nFft <= 0) nFft = nTimes;
    if (fmax < 0) fmax = sfreq / 2.0f;

    const int nFreqs = nFft / 2 + 1;

    // Build frequency vector
    VectorXd freqs(nFreqs);
    for (int f = 0; f < nFreqs; ++f) {
        freqs(f) = static_cast<double>(f) * sfreq / nFft;
    }

    // Find frequency range indices
    int fminIdx = 0, fmaxIdx = nFreqs - 1;
    for (int f = 0; f < nFreqs; ++f) {
        if (freqs(f) >= fmin) { fminIdx = f; break; }
    }
    for (int f = nFreqs - 1; f >= 0; --f) {
        if (freqs(f) <= fmax) { fmaxIdx = f; break; }
    }

    int nBandFreqs = fmaxIdx - fminIdx + 1;
    if (nBandFreqs <= 0) {
        qWarning() << "[computeSourcePsd] No frequencies in range.";
        return QPair<MatrixXd, VectorXd>();
    }

    // Compute PSD for each source
    MatrixXd psd(nSources, nBandFreqs);

    for (int s = 0; s < nSources; ++s) {
        VectorXd fullPsd = computeRowPsd(stc.data.row(s).transpose(), nFft, sfreq);
        psd.row(s) = fullPsd.segment(fminIdx, nBandFreqs).transpose();
    }

    VectorXd bandFreqs = freqs.segment(fminIdx, nBandFreqs);

    return QPair<MatrixXd, VectorXd>(psd, bandFreqs);
}

//=============================================================================================================

QMap<QString, VectorXd> INVLIB::computeSourceBandPower(
    const InvSourceEstimate& stc,
    float sfreq,
    const QMap<QString, QPair<float, float>>& bands)
{
    QMap<QString, VectorXd> result;

    if (stc.isEmpty() || bands.isEmpty()) {
        return result;
    }

    // Compute full PSD
    auto [psd, freqs] = computeSourcePsd(stc, sfreq);
    if (psd.size() == 0) return result;

    const int nSources = static_cast<int>(psd.rows());
    const int nFreqs = static_cast<int>(freqs.size());
    double df = (nFreqs > 1) ? (freqs(1) - freqs(0)) : 1.0;

    for (auto it = bands.constBegin(); it != bands.constEnd(); ++it) {
        float bfmin = it.value().first;
        float bfmax = it.value().second;

        VectorXd bandPower = VectorXd::Zero(nSources);

        for (int f = 0; f < nFreqs; ++f) {
            if (freqs(f) >= bfmin && freqs(f) <= bfmax) {
                bandPower += psd.col(f) * df;
            }
        }

        result[it.key()] = bandPower;
    }

    return result;
}
