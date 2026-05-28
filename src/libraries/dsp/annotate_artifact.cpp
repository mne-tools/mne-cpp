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
#include "iirfilter.h"

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

namespace {

//=============================================================================================================
/**
 * @brief Collect contiguous runs of true values into (start, end) pairs (inclusive).
 */
QVector<QPair<int,int>> findContiguousSegments(const VectorXi& mask)
{
    QVector<QPair<int,int>> segs;
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
 * @brief Merge segments that are closer than gapSamples apart.
 */
void mergeCloseSegments(QVector<QPair<int,int>>& segs, int gapSamples)
{
    if (segs.size() < 2)
        return;
    QVector<QPair<int,int>> merged;
    merged.append(segs.first());
    for (int i = 1; i < segs.size(); ++i) {
        if (segs[i].first - merged.last().second <= gapSamples) {
            merged.last().second = segs[i].second;
        } else {
            merged.append(segs[i]);
        }
    }
    segs = merged;
}

//=============================================================================================================
/**
 * @brief Remove segments shorter than minSamples.
 */
void removeShortSegments(QVector<QPair<int,int>>& segs, int minSamples)
{
    if (minSamples <= 1)
        return;
    QVector<QPair<int,int>> filtered;
    for (const auto& seg : segs) {
        if (seg.second - seg.first + 1 >= minSamples)
            filtered.append(seg);
    }
    segs = filtered;
}

//=============================================================================================================
/**
 * @brief Find channel indices matching a given kind.
 */
QVector<int> findChannelsByKind(const FiffInfo& info, int kind)
{
    QVector<int> indices;
    for (int i = 0; i < info.chs.size(); ++i) {
        if (info.chs[i].kind == kind)
            indices.append(i);
    }
    return indices;
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffAnnotations UTILSLIB::annotateMusclZscore(
    const MatrixXd& data,
    const FiffInfo& info,
    double sfreq,
    const AnnotateMusclParams& params)
{
    FiffAnnotations annot;
    const Eigen::Index nTimes = data.cols();

    if (data.rows() == 0 || nTimes == 0)
        return annot;

    //--- 1. Find MEG channels; fall back to EEG if none ---
    QVector<int> chIdx = findChannelsByKind(info, FIFFV_MEG_CH);
    if (chIdx.isEmpty())
        chIdx = findChannelsByKind(info, FIFFV_EEG_CH);
    if (chIdx.isEmpty()) {
        qWarning("annotateMusclZscore: no MEG or EEG channels found");
        return annot;
    }

    //--- 2. Extract submatrix ---
    const int nCh = chIdx.size();
    MatrixXd sub(nCh, nTimes);
    for (int i = 0; i < nCh; ++i)
        sub.row(i) = data.row(chIdx[i]);

    //--- 3. Bandpass filter ---
    auto sos = IirFilter::designButterworth(params.iFilterOrder,
                                            IirFilter::BandPass,
                                            params.dFilterLow,
                                            params.dFilterHigh,
                                            sfreq);
    MatrixXd filtered = IirFilter::applyZeroPhaseMatrix(sub, sos);

    //--- 4. Envelope approximation (absolute value) ---
    filtered = filtered.cwiseAbs();

    //--- 5. Z-score per channel ---
    MatrixXd zscores(nCh, nTimes);
    int validChannels = 0;
    for (int ch = 0; ch < nCh; ++ch) {
        const double mean = filtered.row(ch).mean();
        const double variance = (filtered.row(ch).array() - mean).square().mean();
        const double stddev = std::sqrt(variance);
        if (stddev < 1e-30) {
            zscores.row(ch).setZero();
            continue;
        }
        zscores.row(ch) = (filtered.row(ch).array() - mean) / stddev;
        ++validChannels;
    }

    if (validChannels == 0)
        return annot;

    //--- 6. Average z-score across channels ---
    RowVectorXd avgZ = zscores.colwise().mean();

    //--- 7. Threshold ---
    VectorXi mask(nTimes);
    for (Eigen::Index i = 0; i < nTimes; ++i)
        mask(static_cast<int>(i)) = (avgZ(i) > params.dThreshold) ? 1 : 0;

    //--- 8. Find contiguous segments ---
    auto segs = findContiguousSegments(mask);

    //--- 9. Merge close segments ---
    const int gapSamples = static_cast<int>(std::round(params.dMinGapSec * sfreq));
    mergeCloseSegments(segs, gapSamples);

    //--- 10. Remove short segments ---
    const int minSamples = static_cast<int>(std::round(params.dMinDuration * sfreq));
    removeShortSegments(segs, minSamples);

    //--- Convert to annotations ---
    for (const auto& seg : segs) {
        const double onset = static_cast<double>(seg.first) / sfreq;
        const double duration = static_cast<double>(seg.second - seg.first + 1) / sfreq;
        annot.append(onset, duration, QStringLiteral("BAD_muscle"));
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
    const bool checkFlat    = params.dFlatMin > 0.0;
    const int minSamples = static_cast<int>(std::round(params.dMinDuration * sfreq));

    //--- Peak amplitude check per channel ---
    if (checkPeakMax || checkPeakMin) {
        for (Eigen::Index ch = 0; ch < nCh; ++ch) {
            const QString chName = (ch < info.ch_names.size()) ? info.ch_names[static_cast<int>(ch)] : QString("CH%1").arg(ch);

            VectorXi mask(nTimes);
            for (Eigen::Index s = 0; s < nTimes; ++s) {
                const double val = data(ch, s);
                mask(static_cast<int>(s)) = ((checkPeakMax && val > params.dPeakMax) ||
                                              (checkPeakMin && val < params.dPeakMin)) ? 1 : 0;
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
