//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Preprocessing tools of the dsp library on synthetic data with a known ground truth.
 *
 * Three ICA implementations must unmix a square wave, a sine and a sawtooth;
 * epochs are cut around known events; bipolar and average references follow
 * their definitions; xDAWN must find the channel pattern of a target response;
 * and the surface Laplacian must match MNE-Python 1.11
 * (compute_current_source_density). The example exits non-zero otherwise.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/channel_derivation.h>
#include <dsp/epoch_extractor.h>
#include <dsp/extended_infomax.h>
#include <dsp/ica.h>
#include <dsp/picard_ica.h>
#include <dsp/surface_laplacian.h>
#include <dsp/xdawn.h>

#include <mne/mne_epoch_data.h>

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
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kSFreq = 250.0;

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

//=============================================================================================================

double correlation(const RowVectorXd& x, const RowVectorXd& y)
{
    const RowVectorXd xc = x.array() - x.mean();
    const RowVectorXd yc = y.array() - y.mean();
    return xc.dot(yc) / (xc.norm() * yc.norm());
}

//=============================================================================================================
/**
 * Smallest over true sources of the best |correlation| with any estimated source: 1 means perfect unmixing.
 */
double worstRecovery(const MatrixXd& estimated, const MatrixXd& truth)
{
    double worst = 1.0;
    for (int i = 0; i < truth.rows(); ++i) {
        double best = 0.0;
        for (int j = 0; j < estimated.rows(); ++j) {
            best = std::max(best, std::fabs(correlation(estimated.row(j), truth.row(i))));
        }
        worst = std::min(worst, best);
    }
    return worst;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    // Three non-Gaussian sources mixed into three channels
    const int n = 2000;
    MatrixXd sources(3, n);
    for (int t = 0; t < n; ++t) {
        sources(0, t) = std::sin(2.0 * kPi * 7.0 * t / kSFreq) > 0.0 ? 1.0 : -1.0; // square
        sources(1, t) = std::sin(2.0 * kPi * 3.0 * t / kSFreq);                    // sine
        sources(2, t) = std::fmod(5.0 * t / kSFreq, 1.0) * 2.0 - 1.0;              // sawtooth
    }
    Matrix3d mixing;
    mixing << 1.0, 0.5, 0.3, 0.4, 1.0, 0.6, 0.2, 0.3, 1.0;
    const MatrixXd mixed = mixing * sources;

    //! [ica_run]
    const IcaResult ica = ICA::run(mixed, 3);                         // FastICA, logcosh, fixed seed
    const MatrixXd cleaned = ICA::excludeComponents(mixed, ica, {0}); // drop component 0, back-project the rest
    //! [ica_run]
    ok &= expect(worstRecovery(ica.matSources, sources) > 0.99, QString("FastICA recovers all sources (worst |r| %1)").arg(worstRecovery(ica.matSources, sources)));
    ok &= expect((mixed - cleaned - ica.matMixing.col(0) * ica.matSources.row(0)).cwiseAbs().maxCoeff() < 1e-9,
                 "excluding a component removes exactly its back-projection");

    //! [picard_ica_run]
    const IcaResult picard = PicardIca::run(mixed, 3);
    //! [picard_ica_run]
    ok &= expect(worstRecovery(picard.matSources, sources) > 0.99, QString("Picard recovers all sources (worst |r| %1)").arg(worstRecovery(picard.matSources, sources)));

    //! [extended_infomax_compute]
    const MatrixXd centered = mixed.colwise() - mixed.rowwise().mean();                            // infomax expects zero-mean data
    const InfomaxResult infomax = ExtendedInfomax::compute(centered, 3, 500, 0.1, 1e-7, true, 42); // the 0.001 default is too slow here
    //! [extended_infomax_compute]
    ok &= expect(infomax.converged && worstRecovery(infomax.matSources, sources) > 0.99,
                 QString("extended Infomax recovers the sub-Gaussian sources (worst |r| %1)").arg(worstRecovery(infomax.matSources, sources)));

    // Continuous data with an evoked response on channels 0-1 after each event of code 1
    std::seed_seq seed{3}; // fixed on purpose: the checks below need reproducible data
    std::mt19937 gen(seed);
    std::normal_distribution<double> noise(0.0, 0.2);
    MatrixXd raw(4, 5000);
    for (int c = 0; c < raw.rows(); ++c) {
        for (int t = 0; t < raw.cols(); ++t) {
            raw(c, t) = noise(gen) + 0.5; // DC offset that baseline correction must remove
        }
    }
    QVector<int> events;
    QVector<int> codes;
    for (int k = 0; k < 40; ++k) {
        const int onset = 200 + k * 115;
        events.append(onset);
        codes.append(k % 2 == 0 ? 1 : 2);
        for (int t = 0; t < 50 && k % 2 == 0; ++t) {
            const double bump = std::sin(kPi * t / 50.0);
            raw(0, onset + 25 + t) += 2.0 * bump;
            raw(1, onset + 25 + t) += 1.0 * bump;
        }
    }

    //! [epoch_extractor_usage]
    EpochExtractor::Params params;
    params.dTmin = -0.1;
    params.dTmax = 0.4;
    params.dBaseMin = -0.1;
    params.dBaseMax = 0.0;
    const QVector<MNEEpochData> epochs = EpochExtractor::extract(raw, events, kSFreq, params, codes);
    QVector<MNEEpochData> targets;
    for (const MNEEpochData& epoch : epochs) {
        if (epoch.event == 1) {
            targets.append(epoch);
        }
    }
    const MatrixXd evoked = EpochExtractor::average(EpochExtractor::rejectMarked(targets));
    //! [epoch_extractor_usage]
    // Samples 25 + 25 after the event, i.e. 50 samples after the epoch start at -0.1 s (25 samples)
    ok &= expect(epochs.size() == 40 && evoked.cols() == 126 && std::fabs(evoked(0, 25 + 50) - 2.0) < 0.15 && std::fabs(evoked(3, 25 + 50)) < 0.15,
                 QString("40 epochs of 126 samples; evoked peak %1 on channel 0 (2.0), %2 on channel 3 (0)")
                     .arg(evoked(0, 75))
                     .arg(evoked(3, 75)));

    //! [xdawn_fit]
    const XdawnResult xdawn = Xdawn::fit(epochs, 1, 1); // enhance the response to event code 1
    const MatrixXd component = Xdawn::apply(epochs[0].epoch, xdawn);
    //! [xdawn_fit]
    const VectorXd pattern = xdawn.matPatterns.col(0) / xdawn.matPatterns.col(0).cwiseAbs().maxCoeff();
    ok &= expect(xdawn.bValid && component.rows() == 1 && std::fabs(std::fabs(pattern(1) / pattern(0)) - 0.5) < 0.1 && std::fabs(pattern(2)) < 0.15,
                 QString("xDAWN pattern ratio ch1/ch0 %1 (0.5), ch2 %2 (0)").arg(pattern(1) / pattern(0)).arg(pattern(2)));

    //! [channel_derivation_usage]
    const QStringList names = {"LH1", "LH2", "LH3", "RA1"};
    const QVector<DerivationRule> bipolar = ChannelDerivation::buildBipolar(names); // LH1-LH2, LH2-LH3
    const QVector<DerivationRule> average = ChannelDerivation::buildCommonAverage(names);
    const auto [bipolarData, bipolarNames] = ChannelDerivation::apply(raw, names, bipolar);
    const auto [averageData, averageNames] = ChannelDerivation::apply(raw, names, average);
    //! [channel_derivation_usage]
    ok &= expect(bipolarNames == QStringList({"LH1-LH2", "LH2-LH3"}) && (bipolarData.row(0) - (raw.row(0) - raw.row(1))).cwiseAbs().maxCoeff() < 1e-12 && averageData.colwise().sum().cwiseAbs().maxCoeff() < 1e-12,
                 "bipolar = LH1 - LH2, LH2 - LH3; average reference sums to zero");

    //! [surface_laplacian_compute]
    MatrixX3d electrodes(8, 3); // head coordinates in metres, on a 9 cm sphere
    electrodes << 0.0, 0.0, 0.09,
        0.05785088487178853, 0.0, 0.06894399988070801,
        0.0, 0.05785088487178853, 0.06894399988070801,
        -0.05785088487178853, 0.0, 0.06894399988070801,
        0.0, -0.05785088487178853, 0.06894399988070801,
        0.0626727816288017, 0.0626727816288017, 0.015628335990023737,
        -0.0626727816288017, 0.0626727816288017, 0.015628335990023737,
        -0.06267278162880172, -0.0626727816288017, 0.015628335990023737;
    MatrixXd eeg(8, 10);
    for (int k = 0; k < 8; ++k) {
        for (int t = 0; t < 10; ++t) {
            eeg(k, t) = std::sin(t * (k + 1) * 0.3);
        }
    }
    const SurfaceLaplacianResult csd = SurfaceLaplacian::compute(eeg, electrodes, 1e-5, 4, 50, 0.09);
    //! [surface_laplacian_compute]
    // mne.preprocessing.compute_current_source_density(sphere=(0, 0, 0, 0.09), lambda2=1e-5, stiffness=4), sample 3
    VectorXd mneCsd(8);
    mneCsd << 984.4089021671608, 1280.7679646395302, 457.7070056428769, -775.4476983026417,
        -977.028371298518, -378.06791643722704, -311.9232346310464, 211.576727857144;
    const double csdErr = (csd.matData.col(3) - mneCsd).cwiseAbs().maxCoeff() / mneCsd.cwiseAbs().maxCoeff();
    ok &= expect(csdErr < 1e-6, QString("surface Laplacian = mne compute_current_source_density (relative error %1)").arg(csdErr));

    qInfo() << (ok ? "All dsp preprocessing checks passed." : "dsp preprocessing checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
