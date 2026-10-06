//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     annotate_artifact.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of annotateMusclZscore and annotateAmplitude.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotate_artifact.h"
#include "connectivity_aec.h"
#include "firfilter.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

//=============================================================================================================
/**
 * @brief Collect contiguous runs of true values into (start, end) pairs (inclusive).
 */
QVector<QPair<int, int>> findContiguousSegments(const VectorXi& mask)
{
    QVector<QPair<int, int>> segs;
    const int n = static_cast<int>(mask.size());
    int i = 0;
    while (i < n) {
        if (mask(i)) {
            int start = i;
            while (i < n && mask(i))
                ++i;
            segs.append({start, i - 1});
        } else {
            ++i;
        }
    }
    return segs;
}

//=============================================================================================================
/**
 * @brief Remove segments shorter than minSamples.
 */
void removeShortSegments(QVector<QPair<int, int>>& segs, int minSamples)
{
    if (minSamples <= 1)
        return;
    QVector<QPair<int, int>> filtered;
    for (const auto& seg : segs) {
        if (seg.second - seg.first + 1 >= minSamples)
            filtered.append(seg);
    }
    segs = filtered;
}

//=============================================================================================================
/**
 * @brief Smallest 2^a 3^b 5^c >= n (mne.filter.next_fast_len, the FFT length of apply_hilbert's n_fft="auto").
 */
int nextFastLen(int n)
{
    int best = std::numeric_limits<int>::max();
    for (long long p5 = 1; p5 < 2LL * n + 1; p5 *= 5) {
        for (long long p35 = p5; p35 < 2LL * n + 1; p35 *= 3) {
            long long v = p35;
            while (v < n)
                v *= 2;
            best = static_cast<int>(std::min<long long>(best, v));
        }
    }
    return best;
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffAnnotations UTILSLIB::annotateMusclZscore(
    const MatrixXd& data,
    const FiffInfo& info,
    double sfreq,
    const AnnotateMusclParams& params,
    RowVectorXd* scores)
{
    // Adapted from mne.preprocessing.annotate_muscle_zscore (MNE-Python, BSD-3-Clause).
    FiffAnnotations annot;
    const Index nTimes = data.cols();
    if (data.rows() == 0 || nTimes == 0)
        return annot;

    RowVectorXi picks = info.pick_types(QStringLiteral("mag"), false, false);
    if (picks.size() == 0)
        picks = info.pick_types(QStringLiteral("grad"), false, false);
    if (picks.size() == 0)
        picks = info.pick_types(false, true, false);
    if (picks.size() == 0) {
        qWarning("annotateMusclZscore: no MEG or EEG channels found");
        return annot;
    }
    MatrixXd sub(picks.size(), nTimes);
    for (Index i = 0; i < picks.size(); ++i)
        sub.row(i) = data.row(picks(i));

    const MatrixXd band = FirFilter::filterData(sub, sfreq, params.dFilterLow, params.dFilterHigh);
    const int nFft = nextFastLen(static_cast<int>(nTimes));
    RowVectorXd score = RowVectorXd::Zero(nTimes);
    for (Index i = 0; i < band.rows(); ++i) {
        const RowVectorXd env = ConnectivityAec::hilbertEnvelope(band.row(i).transpose(), nFft).transpose();
        const double mean = env.mean();
        const double sd = std::sqrt((env.array() - mean).square().mean());
        if (sd > 0.0)
            score += (env.array() - mean).matrix() / sd;
    }
    score /= std::sqrt(static_cast<double>(band.rows()));
    score = FirFilter::filterData(score, sfreq, -1.0, 4.0);
    if (scores)
        *scores = score;

    VectorXi mask(nTimes);
    for (Index i = 0; i < nTimes; ++i)
        mask(i) = score(i) > params.dThreshold ? 1 : 0;
    // Good stretches shorter than min_length_good, including those at the edges, become bad.
    const double minGood = params.dMinLengthGood * sfreq;
    for (const auto& seg : findContiguousSegments((1 - mask.array()).matrix())) {
        if (seg.second - seg.first + 1 < minGood)
            mask.segment(seg.first, seg.second - seg.first + 1).setOnes();
    }
    for (const auto& seg : findContiguousSegments(mask)) {
        const int last = std::min(seg.second + 1, static_cast<int>(nTimes) - 1);
        annot.append(seg.first / sfreq, (last - seg.first) / sfreq, QStringLiteral("BAD_muscle"));
    }
    return annot;
}

//=============================================================================================================

FiffAnnotations UTILSLIB::annotateAmplitude(
    const MatrixXd& data,
    const FiffInfo& info,
    double sfreq,
    const AnnotateAmplitudeParams& params)
{
    FiffAnnotations annot;
    const Eigen::Index nCh = data.rows();
    const Eigen::Index nTimes = data.cols();

    if (nCh == 0 || nTimes == 0)
        return annot;

    const bool checkPeakMax = std::isfinite(params.dPeakMax);
    const bool checkPeakMin = std::isfinite(params.dPeakMin);
    const bool checkFlat = params.dFlatMin > 0.0;
    const int minSamples = static_cast<int>(std::round(params.dMinDuration * sfreq));

    //--- Peak amplitude check per channel ---
    if (checkPeakMax || checkPeakMin) {
        for (Eigen::Index ch = 0; ch < nCh; ++ch) {
            const QString chName = (ch < info.ch_names.size()) ? info.ch_names[static_cast<int>(ch)] : QString("CH%1").arg(ch);

            VectorXi mask(nTimes);
            for (Eigen::Index s = 0; s < nTimes; ++s) {
                const double val = data(ch, s);
                mask(static_cast<int>(s)) = ((checkPeakMax && val > params.dPeakMax) ||
                                             (checkPeakMin && val < params.dPeakMin))
                    ? 1
                    : 0;
            }

            auto segs = findContiguousSegments(mask);
            removeShortSegments(segs, minSamples);

            for (const auto& seg : segs) {
                const double onset = static_cast<double>(seg.first) / sfreq;
                const double duration = static_cast<double>(seg.second - seg.first + 1) / sfreq;
                annot.append(onset, duration, params.badDescription, QStringList{chName});
            }
        }
    }

    //--- Flatness check per channel ---
    if (checkFlat) {
        const int winSamples = std::max(1, static_cast<int>(std::round(params.dWindowSec * sfreq)));

        for (Eigen::Index ch = 0; ch < nCh; ++ch) {
            const QString chName = (ch < info.ch_names.size()) ? info.ch_names[static_cast<int>(ch)] : QString("CH%1").arg(ch);

            VectorXi mask = VectorXi::Zero(static_cast<int>(nTimes));

            for (Eigen::Index s = 0; s <= nTimes - winSamples; ++s) {
                const auto seg = data.block(ch, s, 1, winSamples);
                const double p2p = seg.maxCoeff() - seg.minCoeff();
                if (p2p < params.dFlatMin) {
                    for (int j = static_cast<int>(s); j < static_cast<int>(s) + winSamples; ++j)
                        mask(j) = 1;
                }
            }

            auto segs = findContiguousSegments(mask);
            removeShortSegments(segs, minSamples);

            for (const auto& seg : segs) {
                const double onset = static_cast<double>(seg.first) / sfreq;
                const double duration = static_cast<double>(seg.second - seg.first + 1) / sfreq;
                annot.append(onset, duration, QStringLiteral("BAD_flat"), QStringList{chName});
            }
        }
    }

    return annot;
}
