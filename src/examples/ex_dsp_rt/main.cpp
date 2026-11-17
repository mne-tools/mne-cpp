//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    DSP real-time workers: averaging, covariance, noise spectrum, connectivity, HPI fitting and inverse operators.
 *
 * Each worker is fed blocks the way MNE Scan feeds it and its result is
 * compared with a closed form, NumPy/SciPy 1.15 (scipy.signal.welch) or the
 * corresponding offline call. The threaded front ends are driven through
 * their signals with a bounded event loop. Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <algorithm>
#include <dsp/rt/rt_averaging.h>
#include <memory>
#include <base/string_utils.h>
#include <dsp/rt/rt_connectivity.h>
#include <dsp/rt/rt_cov.h>
#include <dsp/rt/rt_inv_op.h>
#include <dsp/rt/rt_noise.h>

#include <connectivity/compute_connectivity.h>
#include <connectivity/connectivitysettings.h>
#include <connectivity/network/network.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_raw_data.h>
#include <inv/hpi/inv_hpi_data_updater.h>
#include <inv/hpi/inv_hpi_fit.h>
#include <inv/hpi/inv_hpi_model_parameters.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QEventLoop>
#include <QFile>
#include <QTimer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <format>
#include <print>
#include <random>
#include <span>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr double kPi = 3.14159265358979323846;

bool expect(bool condition, std::string_view what)
{
    std::println("{} {}", condition ? "  ok  " : "  FAIL", what);
    return condition;
}

/** Runs the event loop until @p done is true or @p msec elapse; returns @p done. */
bool waitFor(const bool& done, int msec)
{
    QEventLoop loop;
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        if (done) {
            loop.quit();
        }
    });
    QTimer::singleShot(msec, &loop, &QEventLoop::quit);
    poll.start(5);
    loop.exec();
    return done;
}

/** Two MEG magnetometers and one stimulus channel at @p sfreq. */
std::shared_ptr<FiffInfo> makeInfo(double sfreq)
{
    std::shared_ptr<FiffInfo> info = std::make_shared<FiffInfo>();
    info->sfreq = sfreq;
    for (const QString& name : {QStringLiteral("MEG 0111"), QStringLiteral("MEG 0121"), QStringLiteral("STI 014")}) {
        FiffChInfo ch;
        ch.ch_name = name.toStdString();
        ch.kind = name.startsWith("STI") ? FIFFV_STIM_CH : FIFFV_MEG_CH;
        ch.unit = name.startsWith("STI") ? FIFF_UNIT_NONE : FIFF_UNIT_T;
        ch.cal = 1.0f;
        ch.range = 1.0f;
        info->chs.push_back(ch);
        info->ch_names.push_back(name.toStdString());
    }
    info->nchan = static_cast<int>(info->chs.size());
    return info;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption dataOption("data", "MNE-CPP test data <dir>.", "dir",
                                  QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data");
    parser.addOption(dataOption);
    parser.process(app);
    const QString dataDir = parser.value(dataOption);
    std::seed_seq seed{42}; // fixed on purpose: the checks below need reproducible data
    std::mt19937 gen(seed);
    bool ok = true;

    // ---------------------------------------------------------------------------------------------------------
    // Averaging: every block holds one trigger at sample 50; epoch k carries amplitude k on MEG0111
    const std::shared_ptr<FiffInfo> info = makeInfo(1000.0);
    const int nBlocks = 4;
    QList<MatrixXd> blocks;
    for (int k = 1; k <= nBlocks; ++k) {
        MatrixXd block = MatrixXd::Zero(3, 200);
        block.row(0).segment(50, 100).setConstant(k);
        block.row(1).setConstant(-2.0);
        block(2, 50) = 1.0; // trigger code 1
        blocks.append(block);
    }

    //! [rt_averaging_worker_usage]
    RtAveragingWorker averager(10, 20, 80, 0, 0, 2, info); // up to 10 epochs, 20 pre / 80 post samples, trigger row 2
    FiffEvokedSet evokedSet;
    QObject::connect(&averager, &RtAveragingWorker::resultReady,
                     [&](const FiffEvokedSet& set, const QStringList&) { evokedSet = set; });
    for (const MatrixXd& block : blocks) {
        averager.doWork(block); // runs synchronously; RtAveraging moves this worker to a thread
    }
    //! [rt_averaging_worker_usage]
    // Average of k = 1..4 is 2.5 after the trigger and 0 before it; MEG0121 stays at -2
    bool avgOk = evokedSet.evoked.size() == 1;
    if (avgOk) {
        const FiffEvoked& evoked = evokedSet.evoked.front();
        avgOk = evoked.nave == nBlocks && evoked.data.cols() == 100 && evoked.comment == "1" && std::fabs(evoked.data(0, 30) - 2.5) < 1e-12 && std::fabs(evoked.data(0, 10)) < 1e-12 && std::fabs(evoked.data(1, 60) + 2.0) < 1e-12 && std::fabs(evoked.times(20)) < 1e-6;
    }
    ok &= expect(avgOk, "RtAveragingWorker: 4 epochs average to 2.5 after the trigger, 0 before it");

    //! [rt_averaging_usage]
    RtAveraging threaded(10, 20, 80, 0, 0, 2, info); // owns the worker thread
    bool averaged = false;
    QObject::connect(&threaded, &RtAveraging::evokedStim,
                     [&](const FiffEvokedSet& set, const QStringList&) { averaged = set.evoked.front().nave == nBlocks; });
    for (const MatrixXd& block : blocks) {
        threaded.append(block);
    }
    //! [rt_averaging_usage]
    ok &= expect(waitFor(averaged, 5000), "RtAveraging delivers the 4-epoch average on the caller's thread");
    threaded.stop();

    // ---------------------------------------------------------------------------------------------------------
    // Covariance: 4000 correlated samples of the two MEG channels
    MatrixXd covData = MatrixXd::Zero(3, 4000);
    std::normal_distribution<double> normal(0.0, 1e-12);
    for (int t = 0; t < covData.cols(); ++t) {
        const double a = normal(gen);
        covData(0, t) = a + 3e-12;
        covData(1, t) = 0.5 * a + normal(gen);
    }
    const MatrixXd centred = covData.topRows(2).colwise() - covData.topRows(2).rowwise().mean();
    const MatrixXd numpyCov = centred * centred.transpose() / static_cast<double>(covData.cols() - 1); // numpy.cov

    //! [rt_cov_usage]
    RtCov rtCov(*info); // every sample counts the same; RtCov(*info, 2.0) forgets with a 2 s half-life
    for (int start = 0; start < covData.cols(); start += 1000) {
        rtCov.update(covData.middleCols(start, 1000));
    }
    const FiffCov cov = rtCov.covariance().value_or(FiffCov());
    //! [rt_cov_usage]
    // regularize() adds 0.1 x mean MEG variance to the diagonal; the off-diagonal term is untouched
    ok &= expect(cov.dim == 2 && cov.names == StringList{"MEG 0111", "MEG 0121"} && cov.nfree == 4000 && std::fabs(cov.data(0, 1) - numpyCov(0, 1)) < 1e-6 * std::fabs(numpyCov(0, 1)),
                 std::format("RtCov: covariance of 4000 samples matches numpy.cov off the diagonal ({:.6g})", cov.data(0, 1)));

    // ---------------------------------------------------------------------------------------------------------
    // Noise spectrum: sin(25 Hz) + 0.1 cos(50 Hz) at 200 Hz in ten 64-sample blocks
    const std::shared_ptr<FiffInfo> info200 = makeInfo(200.0);
    MatrixXd tones(3, 640);
    for (int t = 0; t < tones.cols(); ++t) {
        tones(0, t) = std::sin(2.0 * kPi * 25.0 * t / 200.0) + 0.1 * std::cos(2.0 * kPi * 50.0 * t / 200.0);
        tones(1, t) = tones(0, t);
        tones(2, t) = 0.0;
    }

    //! [rt_noise_usage]
    RtNoise noise(64, info200->sfreq, 10); // 64-point FFT over 10 accumulated blocks
    MatrixXd spectrum;
    for (int b = 0; b < 10; ++b) {
        if (auto psd = noise.estimate_spectrum(tones.middleCols(64 * b, 64))) { // dB re unit^2/Hz, channels x (fft/2 + 1)
            spectrum = std::move(*psd);
        }
    }
    //! [rt_noise_usage]
    // scipy.signal.welch(x, 200, window=hann(66)[1:-1], nperseg=64, noverlap=0, detrend=False): 25 Hz -9.6521 dB, 50 Hz -29.6587 dB
    ok &= expect(spectrum.cols() == 33 && std::fabs(spectrum(0, 8) + 9.652129129685868) < 1e-6 && std::fabs(spectrum(0, 16) + 29.658719433796943) < 1e-6,
                 std::format("RtNoise: 25 Hz at {:.4f} dB, 50 Hz at {:.4f} dB match scipy.signal.welch", spectrum(0, 8), spectrum(0, 16)));

    // ---------------------------------------------------------------------------------------------------------
    // Connectivity: coherence of the newest trials, updated trial by trial
    ConnectivitySettings settings;
    settings.setSamplingFrequency(256);
    settings.setFFTSize(256);
    settings.setWindowType("hanning");
    settings.setNodePositions(MatrixX3f::Identity(2, 3));
    settings.setConnectivityMethods({"COH"});
    std::vector<MatrixXd> trials;
    for (int trial = 0; trial < 8; ++trial) {
        MatrixXd data(2, 256);
        for (int t = 0; t < 256; ++t) {
            const double alpha = std::sin(2.0 * kPi * 10.0 * t / 256.0 + trial);
            data(0, t) = alpha + 0.3 * normal(gen) * 1e12;
            data(1, t) = alpha + 0.3 * normal(gen) * 1e12;
        }
        trials.push_back(std::move(data));
    }

    //! [rt_connectivity_usage]
    RtConnectivity rtConnectivity(settings, 5); // networks of the five newest trials
    std::vector<Network> networks;
    for (const MatrixXd& trial : trials) {
        networks = rtConnectivity.update(trial); // spectra of this trial only; the oldest one drops out
    }
    //! [rt_connectivity_usage]
    ConnectivitySettings newest = settings;
    newest.append(std::span(trials).last(5));
    const MatrixXd offline = compute_connectivity(newest).front().connectivity_matrix();
    const MatrixXd online = networks.front().connectivity_matrix();
    ok &= expect(online.isApprox(offline),
                 std::format("RtConnectivity equals compute_connectivity of the newest trials (coherence {:.3f})", online(0, 1)));

    // ---------------------------------------------------------------------------------------------------------
    // HPI: fit the four coils of the test recording
    FiffRawData hpiRaw((dataDir + "/MEG/sample/test_hpiFit_raw.fif").toStdString());
    const std::shared_ptr<FiffInfo> hpiInfo = std::make_shared<FiffInfo>(hpiRaw.info);
    MatrixXd hpiData;
    MatrixXd hpiTimes;
    ok &= hpiRaw.read_raw_segment(hpiData, hpiTimes, hpiRaw.first_samp, hpiRaw.first_samp + 399);
    MatrixXd projectors = MatrixXd::Identity(hpiInfo->nchan, hpiInfo->nchan);
    FiffInfo projInfo = *hpiInfo;
    for (FiffProj& proj : projInfo.projs) {
        proj.active = true;
    }
    projInfo.make_projector(projectors);

    //! [rt_hpi_usage]
    InvHpiDataUpdater updater(hpiInfo); // MEG picks, sensor geometry and digitised coil positions
    updater.prepareDataAndProjectors(hpiData, projectors);
    const InvHpiModelParameters model(std::vector<int>({166, 154, 161, 158}), hpiInfo->sfreq, hpiInfo->linefreq, false);

    InvHpiFit hpiFit(updater.getSensors()); // fit one block; run it on any thread for continuous tracking
    HpiFitResult fit;
    hpiFit.fit(updater.getProjectedData(), updater.getProjectors(), model, updater.getHpiDigitizer(), fit);
    //! [rt_hpi_usage]
    ok &= expect(fit.GoF.size() == 4 && fit.GoF.minCoeff() > 0.98 && fit.devHeadTrans.from == FIFFV_COORD_DEVICE && fit.devHeadTrans.to == FIFFV_COORD_HEAD,
                 std::format("InvHpiFit fits 4 coils (GoF >= {:.4f}) device to head", fit.GoF.size() ? fit.GoF.minCoeff() : 0.0));

    // ---------------------------------------------------------------------------------------------------------
    // Inverse operator from the sample forward solution and the covariance of the sample raw data
    FiffRawData raw((dataDir + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());
    std::shared_ptr<FiffInfo> rawInfo = std::make_shared<FiffInfo>(raw.info);
    std::shared_ptr<MNEForwardSolution> fwd = std::make_shared<MNEForwardSolution>((dataDir + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif").toStdU16String());
    MatrixXd rawData;
    MatrixXd rawTimes;
    ok &= raw.read_raw_segment(rawData, rawTimes, raw.first_samp, raw.first_samp + 5999);
    const MNEForwardSolution megFwd = fwd->pick_types(true, false);

    //! [rt_inv_op_usage]
    RtCov rtNoise(*rawInfo, 2.0);                                           // noise covariance with a 2 s half-life
    RtInvOp rtInvOp(*rawInfo, megFwd, {.method = InvEstimateMethod::dSPM}); // forward-only work happens on the first update
    MatrixXd sources;
    for (Index start = 0; start + 600 <= rawData.cols(); start += 600) { // 1 s blocks
        const MatrixXd block = rawData.middleCols(start, 600);
        rtNoise.update(block);
        if (const auto cov = rtNoise.covariance(); cov && rtInvOp.update(*cov)) {
            sources = rtInvOp.apply(block); // dSPM of the normal component, one row per source
        }
    }
    //! [rt_inv_op_usage]
    ok &= expect(sources.rows() == fwd->nsource && sources.cols() == 600 && sources.allFinite(),
                 std::format("RtInvOp follows each covariance and estimates {} sources", sources.rows()));

    // A new covariance costs a fraction of rebuilding the operator
    const FiffCov noiseCov = rtNoise.covariance().value_or(FiffCov());
    const auto t0 = std::chrono::steady_clock::now();
    ok &= rtInvOp.update(noiseCov);
    const auto t1 = std::chrono::steady_clock::now();
    const MNEInverseOperator inverse(*rawInfo, megFwd, noiseCov, 0.2f, 0.8f);
    const auto t2 = std::chrono::steady_clock::now();
    const auto ms = [](auto d) {
        return std::chrono::duration<double, std::milli>(d).count();
    };
    ok &= expect(inverse.nchan == 305 && std::ranges::contains(rawInfo->bads, "MEG 2443"),
                 std::format("Updating takes {:.0f} ms, rebuilding the {}-channel operator {:.0f} ms", ms(t1 - t0), inverse.nchan, ms(t2 - t1)));

    std::println("{}", ok ? "All dsp real-time checks passed." : "dsp real-time checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
