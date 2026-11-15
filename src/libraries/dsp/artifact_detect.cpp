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
#include <base/log.h>
#include "firfilter.h"
#include "peak_finder.h"

//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_constants.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_evoked.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// C++ INCLUDES
//=============================================================================================================

#include <cmath>
#include <algorithm>
#include <iterator>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// PRIVATE HELPERS
//=============================================================================================================

namespace
{

std::vector<int> goodChannelsOfKind(const FiffInfo& info, int kind)
{
    std::vector<int> idx;
    for (int i = 0; i < info.nchan; ++i) {
        if (info.chs[i].kind == kind && !info.isBad(info.chs[i].ch_name))
            idx.push_back(i);
    }
    return idx;
}

// Adapted from mne.preprocessing.ssp._compute_exg_proj (MNE-Python, BSD-3-Clause).
std::vector<FiffProj> exgProjectors(const MatrixXd& data, const FiffInfo& info, double sf, const std::vector<int>& events,
                                    const ArtifactDetectProjParams& p, const QString& mode)
{
    std::vector<FiffProj> projs = info.projs;
    if (events.empty()) {
        Log::warning("[ArtifactDetect] No {} events found", mode.toStdString());
        return projs;
    }

    std::vector<int> picks;
    for (int kind : {FIFFV_MEG_CH, FIFFV_EEG_CH, FIFFV_EOG_CH, FIFFV_ECG_CH}) {
        std::ranges::copy(goodChannelsOfKind(info, kind), std::back_inserter(picks));
    }
    std::ranges::sort(picks);
    RowVectorXi sel(picks.size());
    MatrixXd picked(picks.size(), data.cols());
    for (int c = 0; c < std::ssize(picks); ++c) {
        sel(c) = picks[c];
        picked.row(c) = data.row(picks[c]);
    }
    const FiffInfo pickedInfo = info.pick_info(sel);
    const MatrixXd filtered = FirFilter::filterDataZeroDouble(picked, sf, p.dFilterLow, p.dFilterHigh, FirFilter::FirWindow::Hamming);

    // Epochs(proj=True) applies every info projector before rejection.
    std::vector<FiffProj> active = info.projs;
    for (FiffProj& proj : active) {
        proj.active = true;
    }
    MatrixXd projector;
    FiffProj::make_projector(active, pickedInfo.ch_names, projector, pickedInfo.bads);

    std::vector<double> limits(picks.size(), 0.0);
    for (int c = 0; c < std::ssize(picks); ++c) {
        const FiffChInfo& ch = pickedInfo.chs[c];
        if (ch.kind == FIFFV_MEG_CH) {
            limits[c] = ch.unit == FIFF_UNIT_T ? p.dMagReject : p.dGradReject;
        } else if (ch.kind == FIFFV_EEG_CH) {
            limits[c] = p.dEegReject;
        } else if (ch.kind == FIFFV_EOG_CH) {
            limits[c] = p.dEogReject;
        }
    }

    const int first = static_cast<int>(std::round(p.dTmin * sf));
    const int last = static_cast<int>(std::round(p.dTmax * sf));
    MatrixXd sum = MatrixXd::Zero(picks.size(), last - first + 1);
    int nave = 0;
    for (int ev : events) {
        if (ev + first < 0 || ev + last >= filtered.cols()) {
            continue;
        }
        const MatrixXd epoch = projector * filtered.middleCols(ev + first, last - first + 1);
        bool good = true;
        for (int c = 0; c < std::ssize(picks) && good; ++c) {
            good = limits[c] <= 0.0 || epoch.row(c).maxCoeff() - epoch.row(c).minCoeff() <= limits[c];
        }
        if (good) {
            sum += epoch;
            ++nave;
        }
    }
    if (nave == 0) {
        Log::warning("[ArtifactDetect] No good {} epochs found", mode.toStdString());
        return projs;
    }

    FiffEvoked evoked;
    evoked.info = pickedInfo;
    evoked.nave = nave;
    evoked.data = sum / nave;
    evoked.times = RowVectorXf::LinSpaced(last - first + 1, static_cast<float>(first / sf), static_cast<float>(last / sf));
    std::vector<FiffProj> exg = FiffProj::compute_from_evoked(evoked, p.nGrad, p.nMag, p.nEeg);
    for (FiffProj& proj : exg) {
        proj.desc = mode.toStdString() + "-" + proj.desc;
    }
    std::ranges::move(exg, std::back_inserter(projs));
    return projs;
}

} // namespace

//=============================================================================================================

std::vector<int> ArtifactDetect::qrsDetector(const RowVectorXd& filtered, double dSFreq, double dThreshold, double dTStart)
{
    // Adapted from mne.preprocessing.qrs_detector (MNE-Python, BSD-3-Clause), filtering done by the caller.
    const int winSize = static_cast<int>(std::nearbyint(60.0 * dSFreq / 120.0));
    const int init = static_cast<int>(dSFreq);
    const int nStart = static_cast<int>(dSFreq * dTStart);
    const RowVectorXd ecgAbs = filtered.tail(std::max<Index>(filtered.size() - nStart, 0)).cwiseAbs();
    const int nPoints = static_cast<int>(ecgAbs.size());
    if (nPoints < 3 * init || init < 1) {
        Log::warning("ArtifactDetect::detectEcg: need at least 3 s of data after tstart.");
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

    std::vector<std::vector<int>> cleanEvents;
    for (double threshValue : threshRuns) {
        const double thresh1 = initMax * threshValue;
        std::vector<int> times;
        std::vector<double> rms;
        for (int ii = 0; ii < nPoints - winSize;) {
            const auto window = ecgAbs.segment(ii, winSize);
            if (window(0) > thresh1) {
                Index maxTime = 0;
                window.maxCoeff(&maxTime);
                times.push_back(ii + static_cast<int>(maxTime));
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
        std::vector<int> events;
        for (int k = 0; k < std::ssize(times); ++k) {
            if (rms[static_cast<std::size_t>(k)] < rmsThresh)
                events.push_back(times[k] + nStart);
        }
        if (!events.empty())
            cleanEvents.push_back(events);
    }
    if (cleanEvents.empty())
        return {};

    // Pick the threshold whose heart rate is closest to the median plausible (40-160 bpm) one, else 80 bpm
    std::vector<double> rates;
    std::vector<double> plausible;
    for (const std::vector<int>& events : std::as_const(cleanEvents)) {
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

std::vector<int> ArtifactDetect::detectEcg(const MatrixXd& matData,
                                           const FiffInfo& fiffInfo,
                                           double dSFreq,
                                           const EcgParams& params)
{
    // Adapted from mne.preprocessing.find_ecg_events / _make_ecg (MNE-Python, BSD-3-Clause).
    if (matData.cols() == 0)
        return {};

    RowVectorXd ecg;
    const std::vector<int> ecgIdx = goodChannelsOfKind(fiffInfo, FIFFV_ECG_CH);
    if (!ecgIdx.empty()) {
        ecg = matData.row(ecgIdx.front());
    } else {
        std::vector<int> mags, grads;
        for (int i : goodChannelsOfKind(fiffInfo, FIFFV_MEG_CH))
            (fiffInfo.chs[i].unit == FIFF_UNIT_T ? mags : grads).push_back(i);
        const std::vector<int>& meg = mags.empty() ? grads : mags;
        if (meg.empty()) {
            Log::warning("ArtifactDetect::detectEcg: no ECG channel and no MEG channels to synthesise one from.");
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

std::vector<int> ArtifactDetect::detectEog(const MatrixXd& matData,
                                           const FiffInfo& fiffInfo,
                                           double dSFreq,
                                           const EogParams& params)
{
    // Adapted from mne.preprocessing._find_eog_events (MNE-Python, BSD-3-Clause).
    const std::vector<int> eogIdx = goodChannelsOfKind(fiffInfo, FIFFV_EOG_CH);
    if (eogIdx.empty() || matData.cols() == 0) {
        Log::warning("ArtifactDetect::detectEog: No EOG channel found.");
        return {};
    }

    // The channel with the most 2-45 Hz energy
    MatrixXd eog(eogIdx.size(), matData.cols());
    for (int k = 0; k < std::ssize(eogIdx); ++k)
        eog.row(k) = matData.row(eogIdx[k]);
    const MatrixXd broad = FirFilter::filterDataZeroDouble(eog, dSFreq, 2.0, std::min(45.0, dSFreq / 2.0 - 0.75));
    Index best = 0;
    broad.rowwise().squaredNorm().maxCoeff(&best);

    const RowVectorXd filtered = FirFilter::filterDataZeroDouble(eog.row(best), dSFreq, params.dFilterLow, params.dFilterHigh);
    const double mean = filtered.mean();
    const int extrema = std::abs(filtered.maxCoeff() - mean) > std::abs(filtered.minCoeff() - mean) ? 1 : -1;
    const int nStart = static_cast<int>(dSFreq * params.dTStart);

    std::vector<int> events;
    const VectorXd tail = filtered.tail(std::max<Index>(filtered.size() - nStart, 0)).transpose();
    for (const auto& peak : peakFinderMne(tail, params.dThreshold, extrema))
        events.push_back(peak.first + nStart);
    return events;
}

//=============================================================================================================

std::vector<FiffProj> ArtifactDetect::computeProjEcg(const MatrixXd& matData, const FiffInfo& fiffInfo, double dSFreq,
                                                     const ProjParams& proj, const EcgParams& detect)
{
    return exgProjectors(matData, fiffInfo, dSFreq, detectEcg(matData, fiffInfo, dSFreq, detect), proj, "ECG");
}

//=============================================================================================================

std::vector<FiffProj> ArtifactDetect::computeProjEog(const MatrixXd& matData, const FiffInfo& fiffInfo, double dSFreq,
                                                     const ProjParams& proj, const EogParams& detect)
{
    return exgProjectors(matData, fiffInfo, dSFreq, detectEog(matData, fiffInfo, dSFreq, detect), proj, "EOG");
}
