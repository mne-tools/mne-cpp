//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     artifact_detect.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of ArtifactDetect.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "artifact_detect.h"
#include "firfilter.h"
#include "peak_finder.h"

//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_constants.h>
#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// C++ INCLUDES
//=============================================================================================================

#include <cmath>
#include <algorithm>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// PRIVATE HELPERS
//=============================================================================================================

namespace
{

QVector<int> goodChannelsOfKind(const FiffInfo& info, int kind)
{
    QVector<int> idx;
    for (int i = 0; i < info.nchan; ++i) {
        if (info.chs[i].kind == kind && !info.bads.contains(info.ch_names.value(i, info.chs[i].ch_name)))
            idx.append(i);
    }
    return idx;
}

} // namespace

//=============================================================================================================

QVector<int> ArtifactDetect::qrsDetector(const RowVectorXd& filtered, double dSFreq, double dThreshold, double dTStart)
{
    // Adapted from mne.preprocessing.qrs_detector (MNE-Python, BSD-3-Clause), filtering done by the caller.
    const int winSize = static_cast<int>(std::nearbyint(60.0 * dSFreq / 120.0));
    const int init = static_cast<int>(dSFreq);
    const int nStart = static_cast<int>(dSFreq * dTStart);
    const RowVectorXd ecgAbs = filtered.tail(std::max<Index>(filtered.size() - nStart, 0)).cwiseAbs();
    const int nPoints = static_cast<int>(ecgAbs.size());
    if (nPoints < 3 * init || init < 1) {
        qWarning() << "ArtifactDetect::detectEcg: need at least 3 s of data after tstart.";
        return {};
    }
    const double initMax = (ecgAbs.segment(0, init).maxCoeff() + ecgAbs.segment(init, init).maxCoeff() + ecgAbs.segment(2 * init, init).maxCoeff()) / 3.0;

    std::vector<double> threshRuns;
    if (dThreshold < 0.0) {
        for (int k = 0; k < 16; ++k)
            threshRuns.push_back(0.3 + 0.05 * k);
    } else {
        threshRuns.push_back(dThreshold);
    }

    QVector<QVector<int>> cleanEvents;
    for (double threshValue : threshRuns) {
        const double thresh1 = initMax * threshValue;
        QVector<int> times;
        std::vector<double> rms;
        for (int ii = 0; ii < nPoints - winSize;) {
            const auto window = ecgAbs.segment(ii, winSize);
            if (window(0) > thresh1) {
                Index maxTime = 0;
                window.maxCoeff(&maxTime);
                times.append(ii + static_cast<int>(maxTime));
                rms.push_back(std::sqrt(window.squaredNorm() / winSize));
                ii += winSize;
            } else {
                ++ii;
            }
        }
        // mne's crossing count per window is always < n_thresh (the window starts above threshold), so it is omitted
        if (rms.empty())
            continue;
        double mean = 0.0;
        for (double r : rms)
            mean += r;
        mean /= static_cast<double>(rms.size());
        double var = 0.0;
        for (double r : rms)
            var += (r - mean) * (r - mean);
        const double rmsThresh = mean + std::sqrt(var / static_cast<double>(rms.size())) * 2.5;
        QVector<int> events;
        for (int k = 0; k < times.size(); ++k) {
            if (rms[static_cast<std::size_t>(k)] < rmsThresh)
                events.append(times[k] + nStart);
        }
        if (!events.isEmpty())
            cleanEvents.append(events);
    }
    if (cleanEvents.isEmpty())
        return {};

    // Pick the threshold whose heart rate is closest to the median plausible (40-160 bpm) one, else 80 bpm
    std::vector<double> rates;
    std::vector<double> plausible;
    for (const QVector<int>& events : std::as_const(cleanEvents)) {
        rates.push_back(60.0 * events.size() / (filtered.size() / dSFreq));
        if (rates.back() >= 40.0 && rates.back() <= 160.0)
            plausible.push_back(rates.back());
    }
    double ideal = 80.0;
    if (!plausible.empty()) {
        std::sort(plausible.begin(), plausible.end());
        const std::size_t m = plausible.size() / 2;
        ideal = plausible.size() % 2 ? plausible[m] : 0.5 * (plausible[m - 1] + plausible[m]);
    }
    std::size_t best = 0;
    for (std::size_t k = 1; k < rates.size(); ++k) {
        if (std::abs(rates[k] - ideal) < std::abs(rates[best] - ideal))
            best = k;
    }
    return cleanEvents[static_cast<int>(best)];
}

//=============================================================================================================
// PUBLIC DEFINITIONS
//=============================================================================================================

QVector<int> ArtifactDetect::detectEcg(const MatrixXd& matData,
                                       const FiffInfo& fiffInfo,
                                       double dSFreq,
                                       const EcgParams& params)
{
    // Adapted from mne.preprocessing.find_ecg_events / _make_ecg (MNE-Python, BSD-3-Clause).
    if (matData.cols() == 0)
        return {};

    RowVectorXd ecg;
    const QVector<int> ecgIdx = goodChannelsOfKind(fiffInfo, FIFFV_ECG_CH);
    if (!ecgIdx.isEmpty()) {
        ecg = matData.row(ecgIdx.first());
    } else {
        QVector<int> mags, grads;
        for (int i : goodChannelsOfKind(fiffInfo, FIFFV_MEG_CH))
            (fiffInfo.chs[i].unit == FIFF_UNIT_T ? mags : grads).append(i);
        const QVector<int>& meg = mags.isEmpty() ? grads : mags;
        if (meg.isEmpty()) {
            qWarning() << "ArtifactDetect::detectEcg: no ECG channel and no MEG channels to synthesise one from.";
            return {};
        }
        ecg = RowVectorXd::Zero(matData.cols());
        for (int i : meg)
            ecg += matData.row(i);
        ecg /= static_cast<double>(meg.size());
    }

    const RowVectorXd filtered = FirFilter::filterDataZeroDouble(ecg, dSFreq, params.dFilterLow, params.dFilterHigh);
    return qrsDetector(filtered, dSFreq, params.dQrsThreshold, params.dTStart);
}

//=============================================================================================================

QVector<int> ArtifactDetect::detectEog(const MatrixXd& matData,
                                       const FiffInfo& fiffInfo,
                                       double dSFreq,
                                       const EogParams& params)
{
    // Adapted from mne.preprocessing._find_eog_events (MNE-Python, BSD-3-Clause).
    const QVector<int> eogIdx = goodChannelsOfKind(fiffInfo, FIFFV_EOG_CH);
    if (eogIdx.isEmpty() || matData.cols() == 0) {
        qWarning() << "ArtifactDetect::detectEog: No EOG channel found.";
        return {};
    }

    // The channel with the most 2-45 Hz energy
    MatrixXd eog(eogIdx.size(), matData.cols());
    for (int k = 0; k < eogIdx.size(); ++k)
        eog.row(k) = matData.row(eogIdx[k]);
    const MatrixXd broad = FirFilter::filterDataZeroDouble(eog, dSFreq, 2.0, std::min(45.0, dSFreq / 2.0 - 0.75));
    Index best = 0;
    broad.rowwise().squaredNorm().maxCoeff(&best);

    const RowVectorXd filtered = FirFilter::filterDataZeroDouble(eog.row(best), dSFreq, params.dFilterLow, params.dFilterHigh);
    const double mean = filtered.mean();
    const int extrema = std::abs(filtered.maxCoeff() - mean) > std::abs(filtered.minCoeff() - mean) ? 1 : -1;
    const int nStart = static_cast<int>(dSFreq * params.dTStart);

    QVector<int> events;
    const VectorXd tail = filtered.tail(std::max<Index>(filtered.size() - nStart, 0)).transpose();
    for (const auto& peak : peakFinderMne(tail, params.dThreshold, extrema))
        events.append(peak.first + nStart);
    return events;
}
