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
 * Implements the epoch / raw loops by constructing a single
 * @ref InvMinimumNorm kernel and re-applying it, MNE-Python's whitened-data
 * SNR estimate, and the Welch PSD that re-uses
 * the @c utils/spectral primitives to keep the spectral pipeline
 * identical to the one used elsewhere in mne-cpp.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_convenience.h"
#include <fiff/fiff_ch_name.h>
#include <base/log.h>
#include <base/string_utils.h>
#include "minimum_norm/inv_minimum_norm.h"

#include <mne/mne_inverse_operator.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_source_spaces.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_proj.h>
#include <math/numerics.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// LOCAL HELPERS
//=============================================================================================================

namespace
{

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
        double w = 0.5 * (1.0 - std::cos(2.0 * std::numbers::pi * i / (copyLen - 1)));
        segment(i) *= w;
    }

    // Compute FFT via correlation (real FFT using DFT)
    int nFreqs = nFft / 2 + 1;
    VectorXd psd(nFreqs);

    for (int f = 0; f < nFreqs; ++f) {
        double re = 0.0, im = 0.0;
        for (int t = 0; t < nFft; ++t) {
            double angle = -2.0 * std::numbers::pi * f * t / nFft;
            re += segment(t) * std::cos(angle);
            im += segment(t) * std::sin(angle);
        }
        psd(f) = (re * re + im * im) / (sfreq * nFft);
        // Double for non-DC/Nyquist bins (one-sided spectrum)
        if (f > 0 && f < nFreqs - 1)
            psd(f) *= 2.0;
    }

    return psd;
}

} // anonymous namespace

//=============================================================================================================
// DEFINE FUNCTIONS
//=============================================================================================================

std::vector<InvSourceEstimate> mne::apply_inverse_epochs(
    const std::vector<MatrixXd>& epochs,
    const MNEInverseOperator& inverse,
    float lambda2,
    std::string_view method,
    float tmin,
    float tstep,
    bool pickNormal)
{
    std::vector<InvSourceEstimate> results;

    if (epochs.empty()) {
        mne::Log::warning("[apply_inverse_epochs] No epochs provided.");
        return results;
    }

    // Create the minimum norm estimator
    InvMinimumNorm mn(inverse, lambda2, method);

    // Setup once with nave=1 (per-epoch)
    mn.doInverseSetup(1, pickNormal);

    results.reserve(epochs.size());
    for (const MatrixXd& epoch : epochs) {
        results.push_back(mn.calculateInverse(epoch, tmin, tstep, pickNormal));
        if (results.back().empty()) {
            mne::Log::warning("[apply_inverse_epochs] Epoch {} produced empty source estimate.", results.size() - 1);
        }
    }

    return results;
}

//=============================================================================================================

InvSourceEstimate mne::apply_inverse_raw(
    const FiffRawData& raw,
    const MNEInverseOperator& inverse,
    float lambda2,
    std::string_view method,
    int from,
    int to,
    bool pickNormal)
{
    // Default: use full range
    if (from < 0)
        from = raw.first_samp;
    if (to < 0)
        to = raw.last_samp;

    // Pick channels matching the inverse operator
    RowVectorXi picks = FiffInfo::pick_channels(raw.info.ch_names, inverse.noise_cov.names);
    if (picks.size() == 0) {
        mne::Log::warning("[apply_inverse_raw] No channels match the inverse operator.");
        return InvSourceEstimate();
    }

    // Read raw data
    MatrixXd data, times;
    raw.read_raw_segment(data, times, from, to, picks);

    if (data.cols() == 0) {
        mne::Log::warning("[apply_inverse_raw] No data read from raw file.");
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

FiffEvoked mne::apply_forward(const MNEForwardSolution& fwd, const InvSourceEstimate& stc, const FiffInfo& info)
{
    // Adapted from mne.forward.forward.apply_forward / _apply_forward (MNE-Python, BSD-3-Clause).
    if (fwd.sol.data.size() == 0 || stc.empty()) {
        mne::Log::warning("[apply_forward] Empty forward solution or source estimate.");
        return FiffEvoked();
    }
    MNEForwardSolution fixed = fwd;
    const int nHemi = fixed.src.size();
    if (nHemi > 0 && !fixed.isFixedOrient()) {
        fixed.convert_to_surf_ori();
        fixed.to_fixed_ori();
    }
    const MatrixXd& gain = fixed.sol.data;

    std::vector<int> columns(static_cast<std::size_t>(stc.data.rows()));
    if (nHemi == 0) {
        if (gain.cols() != stc.data.rows()) {
            mne::Log::warning("[apply_forward] Forward columns {} != source estimate rows {}", gain.cols(), stc.data.rows());
            return FiffEvoked();
        }
        std::iota(columns.begin(), columns.end(), 0);
    } else {
        const int nLh = stc.nVerticesLh >= 0 ? stc.nVerticesLh : static_cast<int>(stc.vertices.size());
        for (int r = 0; r < std::ssize(columns); ++r) {
            const int hemi = (r < nLh || nHemi == 1) ? 0 : 1;
            const VectorXi& vertno = fixed.src.hemisphereAt(hemi)->vertno;
            int offset = 0;
            for (int h = 0; h < hemi; ++h)
                offset += static_cast<int>(fixed.src.hemisphereAt(h)->vertno.size());
            const auto* hit = std::find(vertno.data(), vertno.data() + vertno.size(), stc.vertices(r));
            if (hit == vertno.data() + vertno.size()) {
                mne::Log::warning("[apply_forward] Vertex {} is not in the forward solution.", stc.vertices(r));
                return FiffEvoked();
            }
            columns[r] = offset + static_cast<int>(hit - vertno.data());
        }
    }

    // A gain matrix without channel names is taken to have the info's channels in order
    RowVectorXi sel(fixed.sol.row_names.empty() ? gain.rows() : fixed.sol.row_names.size());
    if (fixed.sol.row_names.empty()) {
        if (info.nchan != gain.rows()) {
            mne::Log::warning("[apply_forward] Unnamed forward rows {} != info channels {}", gain.rows(), info.nchan);
            return FiffEvoked();
        }
        std::iota(sel.begin(), sel.end(), 0);
    }
    for (int c = 0; c < static_cast<int>(fixed.sol.row_names.size()); ++c) {
        sel(c) = static_cast<int>(mne::channel_index(info.ch_names, fixed.sol.row_names[c]));
        if (sel(c) < 0) {
            mne::Log::warning("[apply_forward] Channel {} of the forward is not in the info.", fixed.sol.row_names[c]);
            return FiffEvoked();
        }
    }

    FiffEvoked evoked;
    MatrixXd selected(gain.rows(), std::ssize(columns));
    for (int r = 0; r < std::ssize(columns); ++r)
        selected.col(r) = gain.col(columns[r]);
    evoked.data = selected * stc.data;
    evoked.info = info.pick_info(sel);
    evoked.times = RowVectorXf::LinSpaced(stc.data.cols(), stc.tmin, stc.tmin + (stc.data.cols() - 1) * stc.tstep);
    evoked.first = static_cast<int>(std::lround(stc.tmin * info.sfreq));
    evoked.last = evoked.first + static_cast<int>(stc.data.cols()) - 1;
    evoked.nave = 1;
    evoked.comment = "Simulated";
    return evoked;
}

//=============================================================================================================

std::pair<VectorXd, VectorXd> mne::estimate_snr(
    const FiffEvoked& evoked,
    const MNEInverseOperator& inverse)
{
    // Adapted from MNE-Python mne.minimum_norm.estimate_snr (BSD-3-Clause), itself after
    // compute_regularization in MNE-C's mne_analyze/regularization.c.
    const MNEInverseOperator inv = inverse.prepare_inverse_operator(evoked.nave, 1.0f / 9.0f, false);
    if (inv.eigen_fields.empty()) {
        mne::Log::warning("[estimate_snr] Could not prepare the inverse operator.");
        return std::pair<VectorXd, VectorXd>();
    }
    const MatrixXd white = inv.whitener * inv.proj * evoked.pick_channels(inv.noise_cov.names).data;
    const MatrixXd whiteEf = inv.eigen_fields.data * white;

    int rank = 0;
    if (!inv.noise_cov.diag) {
        rank = static_cast<int>((inv.noise_cov.eig.array() > 0).count());
    } else {
        MatrixXd proj;
        rank = inv.noise_cov.dim - FiffProj::make_projector(inv.projs, inv.noise_cov.names, proj);
    }

    const int nTimes = static_cast<int>(white.cols());
    VectorXd snr = white.colwise().squaredNorm().transpose() / rank;
    VectorXd lambda2 = VectorXd::Constant(nTimes, 10.0);
    std::vector<bool> remaining(nTimes, true);
    for (int t = 0; t < nTimes; ++t) {
        if (snr(t) <= 1.0) {
            lambda2(t) = std::numeric_limits<double>::infinity();
            remaining[t] = false;
        }
    }

    const ArrayXd sing2 = inv.sing.array().square();
    const double limit = mne::Numerics::chi2Isf(1e-3, rank);
    bool converged = false;
    for (int iter = 0; iter < 1000 && !converged; ++iter) {
        converged = true;
        for (int t = 0; t < nTimes; ++t) {
            if (!remaining[t])
                continue;
            const ArrayXd keep = (inv.sing.array() == 0).select(1.0, lambda2(t) / (sing2 + lambda2(t)));
            if ((whiteEf.col(t).array() * keep).matrix().squaredNorm() < limit) {
                remaining[t] = false;
            } else {
                lambda2(t) *= 0.99;
                converged = false;
            }
        }
    }
    if (!converged)
        mne::Log::warning("[estimate_snr] SNR estimation did not converge.");

    return std::pair<VectorXd, VectorXd>(snr.cwiseSqrt(), lambda2.cwiseSqrt().cwiseInverse());
}

//=============================================================================================================

std::pair<MatrixXd, int> mne::compute_whitener(
    const FiffCov& noiseCov,
    int rank)
{
    const int dim = noiseCov.dim;

    if (dim <= 0) {
        mne::Log::warning("[compute_whitener] Empty noise covariance.");
        return std::pair<MatrixXd, int>(MatrixXd(), 0);
    }

    // Use pre-computed eigendecomposition if available
    VectorXd eig;
    MatrixXd eigvec;

    // eigvec holds one eigenvector per row, as FiffCov does.
    if (noiseCov.eig.size() > 0 && noiseCov.eigvec.size() > 0) {
        eig = noiseCov.eig;
        eigvec = noiseCov.eigvec;
    } else {
        SelfAdjointEigenSolver<MatrixXd> solver(noiseCov.data);
        eig = solver.eigenvalues();
        eigvec = solver.eigenvectors().transpose();
    }

    // Auto-detect rank from eigenvalue spectrum. FiffCov::prepare_noise_cov already zeroed the null space per
    // channel type; a threshold relative to the largest (EEG) eigenvalue would drop every MEG component.
    const bool prepared = noiseCov.eig.size() > 0 && noiseCov.eigvec.size() > 0;
    if (rank <= 0) {
        double maxEig = eig.maxCoeff();
        double threshold = prepared ? 0.0 : maxEig * 1e-10;
        rank = 0;
        for (int i = 0; i < eig.size(); ++i) {
            if (eig(i) > threshold)
                ++rank;
        }
        if (rank == 0)
            rank = 1;
    }

    // Build whitening matrix: W = diag(1/sqrt(eig)) @ eigvec
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

    MatrixXd whitener = invSqrtEig.asDiagonal() * eigvec;

    return std::pair<MatrixXd, int>(whitener, effectiveRank);
}

//=============================================================================================================

std::pair<MatrixXd, VectorXd> mne::compute_source_psd(
    const InvSourceEstimate& stc,
    float sfreq,
    float fmin,
    float fmax,
    int nFft)
{
    if (stc.empty()) {
        mne::Log::warning("[compute_source_psd] Empty source estimate.");
        return std::pair<MatrixXd, VectorXd>();
    }

    const int nSources = static_cast<int>(stc.data.rows());
    const int nTimes = static_cast<int>(stc.data.cols());

    if (nFft <= 0)
        nFft = nTimes;
    if (fmax < 0)
        fmax = sfreq / 2.0f;

    const int nFreqs = nFft / 2 + 1;

    // Build frequency vector
    VectorXd freqs(nFreqs);
    for (int f = 0; f < nFreqs; ++f) {
        freqs(f) = static_cast<double>(f) * sfreq / nFft;
    }

    // Find frequency range indices
    int fminIdx = 0, fmaxIdx = nFreqs - 1;
    for (int f = 0; f < nFreqs; ++f) {
        if (freqs(f) >= fmin) {
            fminIdx = f;
            break;
        }
    }
    for (int f = nFreqs - 1; f >= 0; --f) {
        if (freqs(f) <= fmax) {
            fmaxIdx = f;
            break;
        }
    }

    int nBandFreqs = fmaxIdx - fminIdx + 1;
    if (nBandFreqs <= 0) {
        mne::Log::warning("[compute_source_psd] No frequencies in range.");
        return std::pair<MatrixXd, VectorXd>();
    }

    // Compute PSD for each source
    MatrixXd psd(nSources, nBandFreqs);

    for (int s = 0; s < nSources; ++s) {
        VectorXd fullPsd = computeRowPsd(stc.data.row(s).transpose(), nFft, sfreq);
        psd.row(s) = fullPsd.segment(fminIdx, nBandFreqs).transpose();
    }

    VectorXd bandFreqs = freqs.segment(fminIdx, nBandFreqs);

    return std::pair<MatrixXd, VectorXd>(psd, bandFreqs);
}

//=============================================================================================================

std::map<std::string, VectorXd> mne::compute_source_band_power(
    const InvSourceEstimate& stc,
    float sfreq,
    const std::map<std::string, std::pair<float, float>>& bands)
{
    std::map<std::string, VectorXd> result;

    if (stc.empty() || bands.empty()) {
        return result;
    }

    // Compute full PSD
    auto [psd, freqs] = compute_source_psd(stc, sfreq);
    if (psd.size() == 0)
        return result;

    const int nSources = static_cast<int>(psd.rows());
    const int nFreqs = static_cast<int>(freqs.size());
    double df = (nFreqs > 1) ? (freqs(1) - freqs(0)) : 1.0;

    for (const auto& [name, range] : bands) {
        const float bfmin = range.first;
        const float bfmax = range.second;

        VectorXd bandPower = VectorXd::Zero(nSources);

        for (int f = 0; f < nFreqs; ++f) {
            if (freqs(f) >= bfmin && freqs(f) <= bfmax) {
                bandPower += psd.col(f) * df;
            }
        }

        result[name] = bandPower;
    }

    return result;
}
