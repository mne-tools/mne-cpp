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

#include <dsp/rt/rt_averaging.h>
#include <dsp/rt/rt_connectivity.h>
#include <dsp/rt/rt_cov.h>
#include <dsp/rt/rt_hpis.h>
#include <dsp/rt/rt_inv_op.h>
#include <dsp/rt/rt_noise.h>

#include <connectivity/connectivity.h>
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
#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QTimer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>
#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace CONNECTIVITYLIB;
using namespace FIFFLIB;
using namespace INVLIB;
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr double kPi = 3.14159265358979323846;

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
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
FiffInfo::SPtr makeInfo(double sfreq)
{
    FiffInfo::SPtr info = FiffInfo::SPtr::create();
    info->sfreq = sfreq;
    for (const QString& name : {QStringLiteral("MEG0111"), QStringLiteral("MEG0121"), QStringLiteral("STI014")}) {
        FiffChInfo ch;
        ch.ch_name = name;
        ch.kind = name.startsWith("STI") ? FIFFV_STIM_CH : FIFFV_MEG_CH;
        ch.unit = name.startsWith("STI") ? FIFF_UNIT_NONE : FIFF_UNIT_T;
        ch.cal = 1.0f;
        ch.range = 1.0f;
        info->chs.append(ch);
        info->ch_names.append(name);
    }
    info->nchan = info->chs.size();
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
    const FiffInfo::SPtr info = makeInfo(1000.0);
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
        const FiffEvoked& evoked = evokedSet.evoked.first();
        avgOk = evoked.nave == nBlocks && evoked.data.cols() == 100 && evoked.comment == "1" && std::fabs(evoked.data(0, 30) - 2.5) < 1e-12 && std::fabs(evoked.data(0, 10)) < 1e-12 && std::fabs(evoked.data(1, 60) + 2.0) < 1e-12 && std::fabs(evoked.times(20)) < 1e-6;
    }
    ok &= expect(avgOk, "RtAveragingWorker: 4 epochs average to 2.5 after the trigger, 0 before it");

    //! [rt_averaging_usage]
    RtAveraging threaded(10, 20, 80, 0, 0, 2, info); // owns the worker thread
    bool averaged = false;
    QObject::connect(&threaded, &RtAveraging::evokedStim,
                     [&](const FiffEvokedSet& set, const QStringList&) { averaged = set.evoked.first().nave == nBlocks; });
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
    RtCov rtCov(info);
    FiffCov cov;
    for (int start = 0; start < covData.cols(); start += 1000) {
        cov = rtCov.estimateCovariance(covData.middleCols(start, 1000), 4000); // empty until 4000 samples arrived
    }
    //! [rt_cov_usage]
    // regularize() adds 0.1 x mean MEG variance to the diagonal; the off-diagonal term is untouched
    ok &= expect(cov.dim == 2 && cov.names == QStringList({"MEG0111", "MEG0121"}) && cov.nfree == 4000 && std::fabs(cov.data(0, 1) - numpyCov(0, 1)) < 1e-6 * std::fabs(numpyCov(0, 1)),
                 QString("RtCov: covariance of 4000 samples matches numpy.cov off the diagonal (%1)").arg(cov.data(0, 1), 0, 'g', 6));

    // ---------------------------------------------------------------------------------------------------------
    // Noise spectrum: sin(25 Hz) + 0.1 cos(50 Hz) at 200 Hz in ten 64-sample blocks
    const FiffInfo::SPtr info200 = makeInfo(200.0);
    MatrixXd tones(3, 640);
    for (int t = 0; t < tones.cols(); ++t) {
        tones(0, t) = std::sin(2.0 * kPi * 25.0 * t / 200.0) + 0.1 * std::cos(2.0 * kPi * 50.0 * t / 200.0);
        tones(1, t) = tones(0, t);
        tones(2, t) = 0.0;
    }

    //! [rt_noise_usage]
    RtNoiseWorker noiseWorker(64, info200, 10); // 64-point FFT over 10 accumulated blocks
    MatrixXd spectrum;
    QObject::connect(&noiseWorker, &RtNoiseWorker::resultReady, [&](const MatrixXd& psd) { spectrum = psd; });
    for (int b = 0; b < 10; ++b) {
        noiseWorker.doWork(tones.middleCols(64 * b, 64)); // dB re unit^2/Hz, channels x (fft/2 + 1)
    }
    //! [rt_noise_usage]
    // scipy.signal.welch(x, 200, window=hann(66)[1:-1], nperseg=64, noverlap=0, detrend=False): 25 Hz -9.6521 dB, 50 Hz -29.6587 dB
    ok &= expect(spectrum.cols() == 33 && std::fabs(spectrum(0, 8) + 9.652129129685868) < 1e-6 && std::fabs(spectrum(0, 16) + 29.658719433796943) < 1e-6,
                 QString("RtNoiseWorker: 25 Hz at %1 dB, 50 Hz at %2 dB match scipy.signal.welch").arg(spectrum(0, 8), 0, 'f', 4).arg(spectrum(0, 16), 0, 'f', 4));

    RtNoise rtNoise(64, info200, 10);
    bool noiseDone = false;
    QObject::connect(&rtNoise, &RtNoise::SpecCalculated, [&](const MatrixXd& psd) { noiseDone = psd.topRows(2).isApprox(spectrum.topRows(2)); }); // the empty stim row is -inf dB
    rtNoise.start();
    for (int b = 0; b < 10; ++b) {
        rtNoise.append(tones.middleCols(64 * b, 64));
    }
    ok &= expect(waitFor(noiseDone, 5000), "RtNoise emits the same spectrum from its worker thread");
    rtNoise.stop();
    rtNoise.wait(2000);

    // ---------------------------------------------------------------------------------------------------------
    // Connectivity: the same network as Connectivity::calculate, computed on a worker thread
    ConnectivitySettings settings;
    for (int trial = 0; trial < 5; ++trial) {
        MatrixXd data(2, 256);
        for (int t = 0; t < 256; ++t) {
            const double alpha = std::sin(2.0 * kPi * 10.0 * t / 256.0 + trial);
            data(0, t) = alpha + 0.3 * normal(gen) * 1e12;
            data(1, t) = alpha + 0.3 * normal(gen) * 1e12;
        }
        settings.append(data);
    }
    settings.setSamplingFrequency(256);
    settings.setFFTSize(256);
    settings.setWindowType("hanning");
    settings.setNodePositions(MatrixX3f::Identity(2, 3));
    settings.setConnectivityMethods({"COH"});
    const MatrixXd offline = Connectivity::calculate(settings).first().getFullConnectivityMatrix();

    //! [rt_connectivity_usage]
    RtConnectivity rtConnectivity;
    MatrixXd online;
    bool connectivityDone = false;
    QObject::connect(&rtConnectivity, &RtConnectivity::newConnectivityResultAvailable,
                     [&](const QList<Network>& networks, const ConnectivitySettings&) {
                         online = networks.first().getFullConnectivityMatrix();
                         connectivityDone = true;
                     });
    rtConnectivity.append(settings); // computed on the worker thread
    //! [rt_connectivity_usage]
    const bool connectivityArrived = waitFor(connectivityDone, 10000);
    ok &= expect(connectivityArrived && online.isApprox(offline),
                 QString("RtConnectivity equals Connectivity::calculate (coherence %1)").arg(online.size() ? online(0, 1) : 0.0, 0, 'f', 3));
    rtConnectivity.stop();

    // ---------------------------------------------------------------------------------------------------------
    // HPI: fit the four coils of the test recording on the worker and directly
    QFile hpiFile(dataDir + "/MEG/sample/test_hpiFit_raw.fif");
    FiffRawData hpiRaw(hpiFile);
    const FiffInfo::SPtr hpiInfo = FiffInfo::SPtr::create(hpiRaw.info);
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
    const InvHpiModelParameters model(QVector<int>({166, 154, 161, 158}), hpiInfo->sfreq, hpiInfo->linefreq, false);

    RtHpiWorker hpiWorker(updater.getSensors());
    HpiFitResult workerFit;
    QObject::connect(&hpiWorker, &RtHpiWorker::resultReady, [&](const HpiFitResult& fit) { workerFit = fit; });
    hpiWorker.doWork(updater.getProjectedData(), updater.getProjectors(), model, updater.getHpiDigitizer());
    //! [rt_hpi_usage]
    HpiFitResult direct;
    InvHpiFit(updater.getSensors()).fit(updater.getProjectedData(), updater.getProjectors(), model, updater.getHpiDigitizer(), direct);
    ok &= expect(workerFit.GoF.size() == 4 && workerFit.GoF.minCoeff() > 0.98 && workerFit.devHeadTrans.trans.isApprox(direct.devHeadTrans.trans, 1e-6f),
                 QString("RtHpiWorker fits 4 coils (GoF >= %1) like InvHpiFit").arg(workerFit.GoF.size() ? workerFit.GoF.minCoeff() : 0.0, 0, 'f', 4));

    // ---------------------------------------------------------------------------------------------------------
    // Inverse operator from the sample forward solution and the covariance of the sample raw data
    QFile fwdFile(dataDir + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif");
    QFile rawFile(dataDir + "/MEG/sample/sample_audvis_trunc_raw.fif");
    FiffRawData raw(rawFile);
    FiffInfo::SPtr rawInfo = FiffInfo::SPtr::create(raw.info);
    QSharedPointer<MNEForwardSolution> fwd = QSharedPointer<MNEForwardSolution>::create(fwdFile);
    MatrixXd rawData;
    MatrixXd rawTimes;
    ok &= raw.read_raw_segment(rawData, rawTimes, raw.first_samp, raw.first_samp + 5999);
    RtCov rawCov(rawInfo);
    const FiffCov noiseCov = rawCov.estimateCovariance(rawData, 6000);

    //! [rt_inv_op_usage]
    RtInvOp rtInvOp(rawInfo, fwd); // MEG-only operator, loose 0.2, depth 0.8
    MNEInverseOperator inverse;
    bool inverseDone = false;
    QObject::connect(&rtInvOp, &RtInvOp::invOperatorCalculated, [&](const MNEInverseOperator& op) {
        inverse = op;
        inverseDone = true;
    });
    rtInvOp.append(noiseCov); // e.g. each new RtCov estimate
    //! [rt_inv_op_usage]
    const bool inverseArrived = waitFor(inverseDone, 60000);
    // 306 MEG channels in the forward solution, minus the bad MEG2443
    ok &= expect(inverseArrived && inverse.nsource == fwd->nsource && inverse.nchan == 305 && rawInfo->bads.contains("MEG2443"),
                 QString("RtInvOp builds a %1-channel operator for %2 sources").arg(inverse.nchan).arg(inverse.nsource));
    rtInvOp.stop();

    qInfo().noquote() << (ok ? "All dsp real-time checks passed." : "dsp real-time checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
