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
    const AnnotateAmplitudeParams& params,
    QStringList* bads)
{
    FiffAnnotations annot;
    if (bads)
        bads->clear();
    const Eigen::Index nTimes = data.cols();
    if (data.rows() == 0 || nTimes < 2 || (!params.peak && !params.flat))
        return annot;

    // mne's "data_or_ica" picks without the bad channels
    static const QStringList dataTypes{"mag", "grad", "eeg", "csd", "seeg", "ecog", "dbs", "hbo", "hbr",
                                       "fnirs_cw_amplitude", "fnirs_fd_ac_amplitude", "fnirs_fd_phase", "fnirs_od"};
    const int minSamples = static_cast<int>(std::round(params.dMinDuration * sfreq));
    VectorXi anyFlat = VectorXi::Zero(nTimes - 1);
    VectorXi anyPeak = VectorXi::Zero(nTimes - 1);

    for (int ch = 0; ch < static_cast<int>(data.rows()) && ch < info.chs.size(); ++ch) {
        if (!dataTypes.contains(info.channel_type(ch)) || info.bads.contains(info.ch_names[ch]))
            continue;
        const ArrayXd diff = (data.row(ch).tail(nTimes - 1) - data.row(ch).head(nTimes - 1)).array().abs().transpose();
        bool bad = false;
        for (const auto& [threshold, isPeak] : {std::pair{params.flat, false}, std::pair{params.peak, true}}) {
            if (!threshold)
                continue;
            const VectorXi mask = isPeak ? (diff >= *threshold).cast<int>().matrix().eval() : (diff <= *threshold).cast<int>().matrix().eval();
            // Runs shorter than the minimum duration do not count
            QVector<QPair<int, int>> segs = findContiguousSegments(mask);
            removeShortSegments(segs, minSamples);
            int count = 0;
            for (const auto& seg : segs)
                count += seg.second - seg.first + 1;
            // A run of n differences spans n + 1 samples
            const double percent = (count > 0 ? count + 1 : 0) * 100.0 / static_cast<double>(nTimes);
            if (percent >= params.dBadPercent) {
                bad = true;
            } else if (percent > 0.0) {
                VectorXi& any = isPeak ? anyPeak : anyFlat;
                for (const auto& seg : segs)
                    any.segment(seg.first, seg.second - seg.first + 1).setOnes();
            }
        }
        if (bad && bads)
            bads->append(info.ch_names[ch]);
    }

    for (const auto& [any, description] : {std::pair{&anyFlat, "BAD_flat"}, std::pair{&anyPeak, "BAD_peak"}}) {
        for (const auto& seg : findContiguousSegments(*any))
            annot.append(seg.first / sfreq, (seg.second - seg.first + 1) / sfreq, QString::fromLatin1(description));
    }
    return annot;
}
