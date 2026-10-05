//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Artifact detection and repair with the dsp library on a recording with planted defects.
 *
 * Eight EEG channels, one EOG and one ECG channel are simulated. EEG 2 is
 * flat, EEG 5 is noisy, EEG 6 and EEG 7 are bridged, blinks leak into the
 * frontal channels, the heart beats every 0.8 s and two stimuli cause 5 ms
 * spikes. Every detector and repair step must find exactly what was planted;
 * the example exits non-zero otherwise.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/annotate_artifact.h>
#include <dsp/artifact_detect.h>
#include <dsp/bad_channel_detect.h>
#include <dsp/bad_channels_lof.h>
#include <dsp/bridged_electrodes.h>
#include <dsp/eeg_reference.h>
#include <dsp/eog_regression.h>
#include <dsp/stim_artifact.h>

#include <fiff/fiff_annotations.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>
#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kSFreq = 250.0;
constexpr int kEeg = 8;
constexpr int kEog = kEeg;
constexpr int kEcg = kEeg + 1;

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

//=============================================================================================================

FiffInfo makeInfo()
{
    FiffInfo info;
    info.sfreq = kSFreq;
    for (int i = 0; i < kEeg + 2; ++i) {
        FiffChInfo ch;
        ch.scanNo = ch.logNo = i + 1;
        ch.kind = i < kEeg ? FIFFV_EEG_CH : (i == kEog ? FIFFV_EOG_CH : FIFFV_ECG_CH);
        ch.ch_name = i < kEeg ? QString("EEG%1").arg(i) : (i == kEog ? QString("EOG") : QString("ECG"));
        ch.range = ch.cal = 1.0f;
        ch.unit = FIFF_UNIT_V;
        info.chs.append(ch);
        info.ch_names.append(ch.ch_name);
    }
    info.nchan = info.chs.size();
    return info;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    std::seed_seq seed{11}; // fixed on purpose: the checks below need reproducible data
    std::mt19937 gen(seed);
    std::normal_distribution<double> noise(0.0, 1.0);

    FiffInfo info = makeInfo();
    const int n = 2500; // 10 s
    MatrixXd data(kEeg + 2, n);
    QVector<int> blinkOnsets = {300, 1100, 1900};
    for (int t = 0; t < n; ++t) {
        const double alpha = 10e-6 * std::sin(2.0 * kPi * 10.0 * t / kSFreq);
        double blink = 0.0;
        for (int onset : blinkOnsets) {
            blink += 300e-6 * std::exp(-std::pow((t - onset - 25) / 10.0, 2));
        }
        const double beat = 1e-3 * std::exp(-std::pow(std::fmod(t, 200.0) - 100.0, 2) / 2.0); // every 0.8 s
        for (int c = 0; c < kEeg; ++c) {
            data(c, t) = alpha * (1.0 + 0.1 * c) + 8e-6 * noise(gen) + blink * (c < 2 ? 0.4 : 0.05);
        }
        data(kEog, t) = blink + 2e-6 * noise(gen);
        data(kEcg, t) = beat + 2e-6 * noise(gen);
    }
    data.row(2).setConstant(1e-6);                                                                  // flat channel
    data.row(5) += 40e-6 * MatrixXd::NullaryExpr(1, n, [&]() { return noise(gen); });               // noisy channel
    data.row(7) = data.row(6) + 0.1e-6 * MatrixXd::NullaryExpr(1, n, [&]() { return noise(gen); }); // bridged pair

    //! [artifact_detect_usage]
    const QVector<int> beats = ArtifactDetect::detectEcg(data, info, kSFreq);  // R-peak samples on the ECG channel
    const QVector<int> blinks = ArtifactDetect::detectEog(data, info, kSFreq); // onsets above +-150 uV on the EOG channel
    //! [artifact_detect_usage]
    ok &= expect(beats.size() == 12 && std::abs(beats[0] - 100) <= 2 && std::abs(beats[1] - beats[0] - 200) <= 2,
                 QString("ECG: %1 beats, first at sample %2 (12 beats every 200 samples from 100)").arg(beats.size()).arg(beats.value(0)));
    ok &= expect(blinks.size() == 3 && std::abs(blinks[1] - (1100 + 25)) <= 15,
                 QString("EOG: %1 blinks, second at sample %2 (3 blinks around 325, 1125, 1925)").arg(blinks.size()).arg(blinks.value(1)));

    //! [bad_channel_detect_usage]
    BadChannelDetect::Params badParams;
    badParams.dFlatThreshold = 1e-9; // EEG in volts
    const QVector<int> flat = BadChannelDetect::detectFlat(data.topRows(kEeg), badParams.dFlatThreshold);
    const QVector<int> bad = BadChannelDetect::detect(data.topRows(kEeg), badParams);
    //! [bad_channel_detect_usage]
    ok &= expect(flat == QVector<int>({2}) && bad.contains(2) && bad.contains(5),
                 QString("BadChannelDetect: flat %1, bad %2 (flat 2, noisy 5)").arg(flat.size() == 1 ? flat[0] : -1).arg(bad.size()));

    //! [find_bad_channels_lof_usage]
    LofBadChannelParams lofParams;
    lofParams.iNNeighbors = 4;
    lofParams.bEegOnly = true;
    const QStringList lofBad = findBadChannelsLof(data, info, lofParams); // features: std, kurtosis, max |x|

    MatrixXd features(6, 2); // the generic LOF score on a 2-D point cloud: one far point
    features << 0.0, 0.0, 0.1, 0.0, 0.0, 0.1, 0.1, 0.1, 0.05, 0.05, 3.0, 3.0;
    const VectorXd lofScores = computeLofScores(features, 3);
    //! [find_bad_channels_lof_usage]
    // sklearn.neighbors.LocalOutlierFactor(n_neighbors=3): 0.96746 x 4, 1.10819, 44.47793
    VectorXd sklearnLof(6);
    sklearnLof << 0.96745631, 0.96745631, 0.96745631, 0.96745631, 1.10819419, 44.47792863;
    ok &= expect((lofScores - sklearnLof).cwiseAbs().maxCoeff() < 1e-6,
                 QString("LOF scores = sklearn (far point %1); %2 EEG channel(s) flagged").arg(lofScores(5)).arg(lofBad.size()));

    //! [bridged_electrodes_usage]
    const QList<QPair<int, int>> bridged = computeBridgedElectrodes(data, info);
    //! [bridged_electrodes_usage]
    ok &= expect(bridged.size() == 1 && bridged[0] == qMakePair(6, 7),
                 QString("bridged electrodes: %1 pair(s), first %2-%3 (6-7)").arg(bridged.size()).arg(bridged.value(0).first).arg(bridged.value(0).second));

    //! [eog_regression_usage]
    MatrixXd cleaned = data;
    EogRegression regression;
    regression.fit(cleaned, info); // EOG channels are found by kind
    regression.apply(cleaned, info);
    //! [eog_regression_usage]
    const double beforeBlink = data(0, 1125);
    const double afterBlink = cleaned(0, 1125);
    ok &= expect(std::fabs(regression.coefficients()(0, 0) - 0.4) < 0.02 && std::fabs(afterBlink) < 0.1 * std::fabs(beforeBlink),
                 QString("EOG regression: beta %1 (0.4), blink on EEG0 %2 -> %3 V").arg(regression.coefficients()(0, 0)).arg(beforeBlink).arg(afterBlink));

    //! [set_eeg_reference_usage]
    MatrixXd averageRef = data;
    setEegReference(averageRef, info); // average of the EEG channels; EOG/ECG rows untouched
    //! [set_eeg_reference_usage]
    ok &= expect(averageRef.topRows(kEeg).colwise().sum().cwiseAbs().maxCoeff() < 1e-15 && averageRef.row(kEcg) == data.row(kEcg),
                 "average reference: EEG sums to zero, ECG unchanged");

    //! [fix_stim_artifact_usage]
    MatrixXd stim = data;
    MatrixXi events(2, 3);
    events << 500, 0, 1, 1500, 0, 1;
    for (int r = 0; r < events.rows(); ++r) {
        stim.block(0, events(r, 0), kEeg, 2).array() += 1e-3; // 8 ms stimulus artefact
    }
    fixStimArtifact(stim, events, kSFreq, 1, -0.004, 0.008); // interpolate between the clean samples at -4 and +8 ms
    //! [fix_stim_artifact_usage]
    // Inside the window the data must be the straight line between the clean samples at 499 and 502
    double stimErr = 0.0;
    for (int s = 0; s <= 3; ++s) {
        const VectorXd line = data.col(499) + (data.col(502) - data.col(499)) * (s / 3.0);
        stimErr = std::max(stimErr, (stim.col(499 + s) - line).topRows(kEeg).cwiseAbs().maxCoeff());
    }
    ok &= expect(stimErr < 1e-12, QString("stimulus artefact replaced by linear interpolation (max deviation %1 V)").arg(stimErr));

    //! [annotate_amplitude_usage]
    AnnotateAmplitudeParams amplitude;
    amplitude.dPeakMax = 200e-6;
    const FiffInfo eogInfo = info.pick_info(RowVectorXi::Constant(1, kEog));
    const FiffAnnotations exceeded = annotateAmplitude(data.row(kEog), eogInfo, kSFreq, amplitude);
    //! [annotate_amplitude_usage]
    ok &= expect(exceeded.size() == 3, QString("annotateAmplitude marks %1 EOG excursions above 200 uV (3)").arg(exceeded.size()));

    qInfo() << (ok ? "All dsp artifact checks passed." : "dsp artifact checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
