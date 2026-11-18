// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

//=============================================================================================================
// test_inv_rt_library.cpp -- Tests for the Inverse and RT Processing libraries
//
// Covers: InvDipoleFitSettings, InvRapMusic, InvDipole, InvDipolePair,
//         RtConnectivity, RtHpi, RtInvOp, RtAveraging, RtNoise, filterFile
//=============================================================================================================

#include <QtTest/QtTest>
#include <memory>
#include <random>
#include <span>
#include <QFile>
#include <QTemporaryFile>
#include <QThread>
#include <QCoreApplication>
#include <Eigen/Dense>

#include <base/log.h>

#include <fiff/fiff.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>

#include <fiff/fiff_cov.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_source_spaces.h>

#include <inv/dipole_fit/inv_dipole_fit_settings.h>
#include <inv/dipole_fit/inv_ecd.h>
#include <inv/dipole_fit/inv_ecd_set.h>
#include <inv/rap_music/inv_rap_music.h>
#include <inv/rap_music/inv_dipole.h>
#include <inv/minimum_norm/inv_minimum_norm.h>
#include <inv/hpi/inv_hpi_model_parameters.h>
#include <inv/hpi/inv_sensor_set.h>
#include <inv/hpi/inv_hpi_data_updater.h>
#include <inv/hpi/inv_hpi_fit.h>
#include <inv/hpi/inv_hpi_fit_data.h>
#include <inv/hpi/inv_signal_model.h>

#include <dsp/rt/rt_connectivity.h>
#include <dsp/rt/rt_cov.h>
#include <dsp/rt/rt_hpi.h>
#include <dsp/rt/rt_inv_op.h>
#include <dsp/rt/rt_averaging.h>
#include <dsp/rt/rt_noise.h>
#include <dsp/rt/rt_filter.h>
#include <dsp/filterkernel.h>

#include <connectivity/compute_connectivity.h>
#include <connectivity/connectivitysettings.h>

using namespace mne;
using namespace Eigen;

//=============================================================================================================

class TestInvRtLibrary : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;
    bool hasData() const
    {
        return !m_sDataPath.isEmpty();
    }

    QString rawPath() const
    {
        return m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
    }
    QString fwdPath() const
    {
        return m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
    }

private slots:
    void initTestCase();
    void cleanupTestCase();

    // ---- Inverse: InvDipoleFitSettings ----
    void dipoleFitSettings_defaultCtor();
    void dipoleFitSettings_cliParsing();

    // ---- Inverse: InvRapMusic ----
    void rapMusic_defaultCtorAndBasics();
    void rapMusic_mathHelpers();

    // ---- Inverse: InvDipole ----
    void dipole_fullExercise();

    // ---- RT Processing: RtConnectivity ----
    void rtConnectivity_slidesOverTheNewestTrials();

    // ---- RT Processing: RtHpi ----
    void rtHpi_fitsTheNewestWindowFromTheLastPose();

    // ---- RT Processing: RtInvOp ----
    void rtInvOp_matchesTheBatchInverse();

    // ---- RT Processing: RtAveraging ----
    void rtAveraging_averagesTheNewestEpochs();

    // ---- RT Processing: RtNoise ----
    void rtNoise_hanningWindow();
    void rtNoise_lifecycle();

    // ---- RT Processing: filterFile ----
    void rtprocessing_filterFile();

    // ---- HPI Fit (from boost) ----
    void hpiFit_constructWithSensorSet();
    void hpiFit_storeHeadPosition();
    void hpiFit_fitWithSyntheticData();
    void hpiFitData_defaultConstruction();

    // ---- InvHpiModelParameters (from boost) ----
    void hpiModelParams_construction();
    void hpiModelParams_copyAndCompare();

    // ---- InvSignalModel (from boost) ----
    void signalModel_construction();

    // ---- FilterKernel (from boost) ----
    void filterKernel_design_lpf();
    void filterKernel_design_hpf();
    void filterKernel_design_bpf();

    // ---- Filter apply (from boost) ----
    void filter_applyToRawData();
    void filter_applyWithChannelSelection();

    // ---- RtNoise extras (from coverage_push) ----

    // ---- FilterOverlapAdd (from coverage_push) ----
    void filterOverlapAdd_calculate();
    void filterData_paramOverload();
};

//=============================================================================================================

void TestInvRtLibrary::initTestCase()
{
    mne::Log::install_qt_message_handler();
    QString base = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";
    if (QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif"))
        m_sDataPath = base;
}

void TestInvRtLibrary::cleanupTestCase()
{
}

//=============================================================================================================
// Inverse: InvDipoleFitSettings
//=============================================================================================================

void TestInvRtLibrary::dipoleFitSettings_defaultCtor()
{
    InvDipoleFitSettings settings;
    QVERIFY(settings.measname.empty());
    QVERIFY(!settings.include_meg);
    QVERIFY(!settings.include_eeg);
    QVERIFY(!settings.accurate);
    QVERIFY(settings.filter.filter_on);
    settings.checkIntegrity();
}

void TestInvRtLibrary::dipoleFitSettings_cliParsing()
{
    // Full comprehensive parsing
    {
        const char* args[] = {"dipole_fit",
                              "--meg", "--eeg", "--accurate",
                              "--meas", "test_meas.fif",
                              "--dip", "test_output.dip",
                              "--set", "2",
                              "--noise", "noise.fif",
                              "--bem", "bem.fif",
                              "--mri", "mri.fif",
                              "--tmin", "0.050",
                              "--tmax", "0.150",
                              "--tstep", "0.005",
                              "--bmin", "0.000",
                              "--bmax", "0.050",
                              "--integ", "0.005",
                              "--guess", "guess.fif",
                              "--guessrad", "80.0",
                              "--exclude", "20.0",
                              "--mindist", "10.0",
                              "--grid", "10.0",
                              "--gradnoise", "5.0",
                              "--magnoise", "20.0",
                              "--eegnoise", "0.2",
                              "--diagnoise",
                              "--reg", "0.1",
                              "--gradreg", "0.15",
                              "--magreg", "0.12",
                              "--eegreg", "0.08",
                              "--proj", "proj.fif",
                              "--noproj",
                              "--eegrad", "90.0",
                              "--eegmodel", "Default",
                              "--eegscalp",
                              "--magdip",
                              "--bdip", "bdip_output.bdip",
                              "--gui",
                              "--filtersize", "2048",
                              "--highpass", "1.0",
                              "--lowpass", "30.0",
                              "--lowpassw", "3.0",
                              "--filteroff",
                              "--verbose"};
        int argc = sizeof(args) / sizeof(args[0]);
        char** argv = const_cast<char**>(args);
        InvDipoleFitSettings settings(&argc, argv);
        QVERIFY(settings.include_meg);
        QVERIFY(settings.include_eeg);
        QVERIFY(settings.accurate);
        QCOMPARE(settings.measname, QString("test_meas.fif"));
    }

    // Fewer args (bad channels, raw mode)
    {
        const char* args[] = {"dipole_fit", "--meg",
                              "--meas", "raw.fif", "--dip", "out.dip",
                              "--bad", "bad.txt",
                              "--guesssurf", "inner_skull.fif"};
        int argc = sizeof(args) / sizeof(args[0]);
        char** argv = const_cast<char**>(args);
        InvDipoleFitSettings settings(&argc, argv);
        QVERIFY(settings.include_meg);
    }
}

//=============================================================================================================
// Inverse: InvRapMusic
//=============================================================================================================

void TestInvRtLibrary::rapMusic_defaultCtorAndBasics()
{
    InvRapMusic rap;
    const char* name = rap.getName();
    QVERIFY(QString(name).contains("RAP"));

    rap.setStcAttr(100, 0.5f);
    const MNESourceSpaces& src = rap.getSourceSpace();
    Q_UNUSED(src);
}

void TestInvRtLibrary::rapMusic_mathHelpers()
{
    if (!hasData())
        QSKIP("No test data");
    if (!QFile::exists(fwdPath()))
        QSKIP("Forward file not found");

    MNEForwardSolution fwd(fwdPath().toStdU16String());
    if (fwd.empty())
        QSKIP("Could not read forward");

    InvRapMusic rap;
    bool initOk = rap.init(fwd, false, 2, 0.5);
    QVERIFY(initOk);
    QVERIFY(QString(rap.getName()).contains("RAP"));
}

//=============================================================================================================
// Inverse: InvDipole
//=============================================================================================================

void TestInvRtLibrary::dipole_fullExercise()
{
    // Default constructor
    InvDipole<double> dip;
    QCOMPARE(dip.x(), 0.0);
    QCOMPARE(dip.y(), 0.0);
    QCOMPARE(dip.z(), 0.0);
    QCOMPARE(dip.phi_x(), 0.0);
    QCOMPARE(dip.phi_y(), 0.0);
    QCOMPARE(dip.phi_z(), 0.0);

    dip.x() = 1.0;
    dip.y() = 2.0;
    dip.z() = 3.0;
    dip.phi_x() = 0.5;
    dip.phi_y() = 0.6;
    dip.phi_z() = 0.7;
    QCOMPARE(dip.x(), 1.0);
    QCOMPARE(dip.y(), 2.0);
    QCOMPARE(dip.z(), 3.0);
    QCOMPARE(dip.phi_x(), 0.5);

    InvDipolePair<double> pair;
    pair.m_iIdx1 = 0;
    pair.m_iIdx2 = 5;
    pair.m_vCorrelation = 0.95;

    InvDipole<float> dipF;
    dipF.x() = 1.0f;
    QCOMPARE(dipF.x(), 1.0f);

    InvDipole<int> dipI;
    dipI.x() = 42;
    QCOMPARE(dipI.x(), 42);
}

//=============================================================================================================
// RT Processing: RtConnectivity
//=============================================================================================================

void TestInvRtLibrary::rtConnectivity_slidesOverTheNewestTrials()
{
    // Two channels share a 10 Hz rhythm with a random phase per trial, a third is noise
    std::seed_seq seed{5};
    std::mt19937 rng(seed);
    std::normal_distribution<double> noise(0.0, 0.3);
    std::uniform_real_distribution<double> phase(0.0, 2.0 * M_PI);
    std::vector<MatrixXd> trials;
    for (int t = 0; t < 12; ++t) {
        const double phi = phase(rng);
        MatrixXd trial(3, 128);
        for (int k = 0; k < trial.cols(); ++k) {
            const double w = 2.0 * M_PI * 10.0 * k / 128.0;
            trial(0, k) = std::sin(w + phi) + noise(rng);
            trial(1, k) = std::sin(w + phi - 0.7) + noise(rng);
            trial(2, k) = noise(rng);
        }
        trials.push_back(trial);
    }
    const std::vector<std::string> methods = {"COH", "IMAGCOH", "PLI", "WPLI", "USPLI", "DSWPLI", "PLV", "COR"};
    ConnectivitySettings settings;
    settings.setSamplingFrequency(128);
    settings.setFFTSize(128);
    settings.setWindowType("hanning");
    settings.set_frequency_bins(8, 5);
    settings.setNodePositions(MatrixX3f::Zero(3, 3));
    settings.setConnectivityMethods(methods);

    // Each update gives what compute_connectivity gives for the five newest trials
    const int window = 5;
    RtConnectivity rtConnectivity(settings, window);
    for (std::size_t t = 0; t < trials.size(); ++t) {
        const std::vector<Network> streamed = rtConnectivity.update(trials[t]);
        ConnectivitySettings batch = settings;
        const std::size_t first = t + 1 > window ? t + 1 - window : 0;
        batch.append(std::span(trials).subspan(first, t + 1 - first));
        const std::vector<Network> expected = compute_connectivity(batch);
        QCOMPARE(streamed.size(), methods.size());
        QCOMPARE(rtConnectivity.settings().size(), static_cast<int>(t + 1 - first));
        for (std::size_t m = 0; m < methods.size(); ++m) {
            // The unbiased estimators are undefined, NaN, for a single trial
            const MatrixXd a = streamed[m].connectivity_matrix();
            const MatrixXd b = expected[m].connectivity_matrix();
            QCOMPARE(streamed[m].method(), expected[m].method());
            QVERIFY((a.array().isNaN() == b.array().isNaN()).all());
            const double error = (a - b).array().isNaN().select(0.0, (a - b).cwiseAbs()).maxCoeff();
            QVERIFY2(error < 1e-9, qPrintable(QString("%1 after trial %2: %3").arg(QString::fromStdString(streamed[m].method())).arg(t).arg(error)));
        }
    }

    // A longer trial starts the window over
    MatrixXd longer = MatrixXd::Random(3, 256);
    QCOMPARE(rtConnectivity.update(longer).size(), methods.size());
    QCOMPARE(rtConnectivity.settings().size(), 1);
    rtConnectivity.reset();
    QVERIFY(rtConnectivity.networks().empty());
}

//=============================================================================================================
// RT Processing: RtHpi
//=============================================================================================================

void TestInvRtLibrary::rtHpi_fitsTheNewestWindowFromTheLastPose()
{
    if (!hasData())
        QSKIP("Sample test data not found");
    FiffRawData raw((m_sDataPath + "/MEG/sample/test_hpiFit_raw.fif").toStdString());
    MatrixXd data;
    MatrixXd times;
    QVERIFY(raw.read_raw_segment(data, times, raw.first_samp, raw.first_samp + 1999));
    auto info = std::make_shared<FiffInfo>(raw.info);
    const std::vector<int> freqs = {166, 154, 161, 158};
    const InvHpiModelParameters model(freqs, info->sfreq, info->linefreq, false);

    // The batch fit of each window, started from the last accepted fit
    const auto reference = [&](Index end, HpiFitResult start) {
        InvHpiDataUpdater updater(info);
        updater.prepareDataAndProjectors(data.middleCols(end - 400, 400), MatrixXd::Identity(info->nchan, info->nchan));
        InvHpiFit(updater.getSensors()).fit(updater.getProjectedData(), updater.getProjectors(), model, updater.getHpiDigitizer(), false, start);
        return start;
    };
    HpiFitResult initial;
    initial.devHeadTrans = info->dev_head_t;
    initial.devHeadTrans.from = FIFFV_COORD_DEVICE;
    initial.devHeadTrans.to = FIFFV_COORD_HEAD;
    initial.errorDistances.assign(freqs.size(), 0.0);

    for (const double maxError : {0.01, 0.0}) {
        // 100-sample blocks; a 400-sample window fitted every 200 samples
        RtHpi rtHpi(info, {.coil_frequencies = freqs, .window = 400, .hop = 200, .max_error = maxError});
        HpiFitResult last = initial;
        int fits = 0;
        for (Index start = 0; start < data.cols(); start += 100) {
            const std::optional<HpiFitResult> fit = rtHpi.update(data.middleCols(start, 100));
            const Index end = start + 100;
            QCOMPARE(fit.has_value(), end >= 400 && end % 200 == 0);
            if (!fit) {
                continue;
            }
            ++fits;
            const HpiFitResult expected = reference(end, last);
            QVERIFY(fit->devHeadTrans.trans.isApprox(expected.devHeadTrans.trans, 1e-5f));
            QVERIFY(fit->GoF.isApprox(expected.GoF, 1e-6));
            QCOMPARE(rtHpi.accepted(*fit), maxError > 0.0);
            if (rtHpi.accepted(*fit)) {
                last = *fit;
            }
            // A rejected fit leaves the head where it was
            QVERIFY(rtHpi.head_transform().trans.isApprox(last.devHeadTrans.trans));
        }
        QCOMPARE(fits, 9);
        QVERIFY(last.GoF.size() == 0 || last.GoF.minCoeff() > 0.98);
    }

    // A block longer than the window fits its newest samples; fewer than three coils fit nothing
    RtHpi longBlock(info, {.coil_frequencies = freqs, .window = 400, .hop = 200});
    const std::optional<HpiFitResult> fit = longBlock.update(data.leftCols(1000));
    QVERIFY(fit && fit->devHeadTrans.trans.isApprox(reference(1000, initial).devHeadTrans.trans, 1e-5f));
    RtHpi twoCoils(info, {.coil_frequencies = {166, 154}, .window = 400, .hop = 200});
    QVERIFY(!twoCoils.update(data.leftCols(400)));
    QVERIFY(!twoCoils.update(data.topRows(3)));
}

//=============================================================================================================
// RT Processing: RtInvOp
//=============================================================================================================

void TestInvRtLibrary::rtInvOp_matchesTheBatchInverse()
{
    if (!hasData())
        QSKIP("Sample test data not found");

    FiffRawData raw(rawPath().toStdString());
    MatrixXd data;
    MatrixXd times;
    QVERIFY(raw.read_raw_segment(data, times, raw.first_samp, raw.first_samp + 2999));
    const MNEForwardSolution fwd = MNEForwardSolution(fwdPath().toStdU16String()).pick_types(true, false);
    const float lambda2 = 1.0f / 9.0f;
    struct Case
    {
        InvEstimateMethod method;
        const char* name;
        int nave;
        bool pickNormal;
    };
    const Case cases[] = {{InvEstimateMethod::MNE, "MNE", 1, true}, {InvEstimateMethod::dSPM, "dSPM", 1, true}, {InvEstimateMethod::sLORETA, "sLORETA", 1, true}, {InvEstimateMethod::dSPM, "dSPM", 4, true}, {InvEstimateMethod::dSPM, "dSPM", 4, false}};
    std::vector<RtInvOp> streaming;
    for (const Case& c : cases) {
        streaming.emplace_back(raw.info, fwd, RtInvOp::Settings{.method = c.method, .lambda2 = lambda2, .pick_normal = c.pickNormal});
        QVERIFY(!streaming.back().ready());
    }

    // The stored covariance, then one estimated from the data: the operator follows each
    RtCov estimate(raw.info);
    estimate.update(data);
    const FiffCov covariances[] = {FiffCov((m_sDataPath + "/MEG/sample/sample_audvis-cov.fif").toStdU16String()), estimate.covariance().value()};
    for (const FiffCov& cov : covariances) {
        const MNEInverseOperator inverse(raw.info, fwd, cov, 0.2f, 0.8f);
        for (std::size_t i = 0; i < std::size(cases); ++i) {
            QVERIFY(streaming[i].update(cov));
            QVERIFY(streaming[i].ready());

            InvMinimumNorm batch(inverse, lambda2, cases[i].name);
            batch.doInverseSetup(cases[i].nave, cases[i].pickNormal);
            const StringList& names = batch.getPreparedInverseOperator().noise_cov.names;
            MatrixXd picked(std::ssize(names), data.cols());
            for (Index row = 0; row < picked.rows(); ++row) {
                picked.row(row) = data.row(channel_index(raw.info.ch_names, names[static_cast<std::size_t>(row)]));
            }
            const MatrixXd expected = batch.calculateInverse(picked, 0.0f, 1.0f / raw.info.sfreq, cases[i].pickNormal).data;

            const MatrixXd result = streaming[i].apply(data, cases[i].nave);
            QCOMPARE(result.rows(), expected.rows());
            QCOMPARE(result.cols(), expected.cols());
            const double error = (result - expected).cwiseAbs().maxCoeff() / expected.cwiseAbs().maxCoeff();
            QVERIFY2(error < 1e-6, qPrintable(QString("%1, nave %2: relative error %3").arg(cases[i].name).arg(cases[i].nave).arg(error)));
        }
    }
}

//=============================================================================================================
// RT Processing: RtAveraging
//=============================================================================================================

void TestInvRtLibrary::rtAveraging_averagesTheNewestEpochs()
{
    std::shared_ptr<FiffInfo> pInfo(new FiffInfo());
    pInfo->sfreq = 1000.0;
    FiffChInfo chData;
    chData.ch_name = "MEG 0111";
    chData.kind = FIFFV_MEG_CH;
    chData.unit = FIFF_UNIT_T;
    chData.range = 1.0;
    chData.cal = 1.0;
    FiffChInfo chTrig;
    chTrig.ch_name = "STI 014";
    chTrig.kind = FIFFV_STIM_CH;
    chTrig.range = 1.0;
    chTrig.cal = 1.0;
    pInfo->chs.push_back(chData);
    pInfo->chs.push_back(chTrig);
    pInfo->nchan = 2;
    pInfo->ch_names.push_back("MEG 0111");
    pInfo->ch_names.push_back("STI 014");
    RtAveraging averaging(pInfo, {.pre_stim = 50, .post_stim = 100, .trigger_channel = 1, .averages = 2});
    int updates = 0;
    for (int block = 0; block < 5; ++block) {
        MatrixXd data = MatrixXd::Zero(2, 200);
        for (int j = 0; j < 200; ++j)
            data(0, j) = std::sin(2.0 * M_PI * 10.0 * j / 1000.0);
        if (block % 2 == 0)
            data(1, 100) = 5.0;
        updates += static_cast<int>(averaging.update(data).size());
    }
    // Three epochs, two averaged: the epoch of the stationary sine is samples 50..199 of any block
    QCOMPARE(updates, 3);
    const FiffEvoked& evoked = averaging.evoked().evoked.front();
    QCOMPARE(evoked.comment, "5");
    QCOMPARE(evoked.nave, 2);
    for (int k = 0; k < 150; ++k)
        QVERIFY(std::fabs(evoked.data(0, k) - std::sin(2.0 * M_PI * 10.0 * (50 + k) / 1000.0)) < 1e-12);
}

//=============================================================================================================
// RT Processing: RtNoise
//=============================================================================================================

void TestInvRtLibrary::rtNoise_hanningWindow()
{
    // A unit tone at an exact bin: PSD density times bin width (fs / nfft) recovers its power 1/2 within the Hann leakage
    RtNoise noise(256, 1000.0, 4);
    std::optional<MatrixXd> psdDb;
    for (int b = 0; b < 4; ++b) {
        MatrixXd block(5, 256);
        for (int t = 0; t < 256; ++t) {
            block.col(t).setConstant(std::sin(2.0 * M_PI * 125.0 * (256 * b + t) / 1000.0));
        }
        QVERIFY(!psdDb);
        psdDb = noise.estimate_spectrum(block);
    }
    QVERIFY(psdDb);
    QCOMPARE(psdDb->rows(), static_cast<Index>(5));
    QCOMPARE(psdDb->cols(), static_cast<Index>(129));
    VectorXd power = psdDb->row(0).unaryExpr([](double db) { return std::pow(10.0, db / 10.0); }).transpose();
    const double tonePower = power.segment(30, 5).sum() * 1000.0 / 256.0;
    QVERIFY2(std::abs(tonePower - 0.5) < 0.02, qPrintable(QString("tone power %1, expected 0.5").arg(tonePower)));
}

//=============================================================================================================

void TestInvRtLibrary::rtNoise_lifecycle()
{
    // Blocks of any length: the spectrum of three blocks is the spectrum of their concatenation
    MatrixXd data(3, 600);
    for (int t = 0; t < data.cols(); ++t) {
        data.col(t) << std::sin(0.3 * t), std::cos(0.05 * t * t / 600.0), 0.01 * t;
    }
    RtNoise whole(128, 600.0, 1);
    const MatrixXd expected = whole.estimate_spectrum(data).value();
    RtNoise blocks(128, 600.0, 3);
    QVERIFY(!blocks.estimate_spectrum(data.leftCols(50)));
    QVERIFY(!blocks.estimate_spectrum(data.middleCols(50, 300)));
    const std::optional<MatrixXd> result = blocks.estimate_spectrum(data.rightCols(250));
    QVERIFY(result);
    QVERIFY((*result - expected).cwiseAbs().maxCoeff() < 1e-9);

    // After a spectrum the next one starts from nothing; a block with other channels starts over
    QVERIFY(!blocks.estimate_spectrum(data.leftCols(200)));
    QVERIFY(!blocks.estimate_spectrum(data.topRows(2)));
    QVERIFY(!blocks.estimate_spectrum(data.topRows(2)));
    QCOMPARE(blocks.estimate_spectrum(data.topRows(2)).value().rows(), static_cast<Index>(2));
}

//=============================================================================================================
// RT Processing: filterFile
//=============================================================================================================

void TestInvRtLibrary::rtprocessing_filterFile()
{
    if (!hasData())
        QSKIP("No test data");
    std::shared_ptr<FiffRawData> pRaw(new FiffRawData(rawPath().toStdString()));

    QTemporaryFile tmpFile;
    tmpFile.setAutoRemove(true);
    QVERIFY(tmpFile.open());

    bool ok = mne::filterFile(tmpFile.fileName().toStdU16String(), pRaw, 2, 10.0, 5.0, 2.0,
                              pRaw->info.sfreq, 512);
    QVERIFY(ok);
    QVERIFY(tmpFile.size() > 0);
}

//=============================================================================================================
// HPI Fit (from boost)
//=============================================================================================================

void TestInvRtLibrary::hpiFit_constructWithSensorSet()
{
    if (!hasData())
        QSKIP("No test data");
    FiffRawData raw(rawPath().toStdString());
    InvSensorSetCreator creator;
    // Only MEG channels have coils: a list with EEG/stim channels gives no sensor set
    QCOMPARE(creator.updateSensorSet(raw.info.chs, Accuracy::medium).ncoils(), 0);
    RowVectorXi megPicks = raw.info.pick_types(true, false, false);
    std::vector<FiffChInfo> megChs;
    megChs.reserve(static_cast<std::size_t>(megPicks.size()));
    for (int i = 0; i < megPicks.size(); ++i)
        megChs.push_back(raw.info.chs[megPicks(i)]);
    const InvSensorSet sensorSet = creator.updateSensorSet(megChs, Accuracy::medium);
    QCOMPARE(sensorSet.ncoils(), static_cast<int>(megPicks.size()));
    QVERIFY(sensorSet.np() > 0);
}

void TestInvRtLibrary::hpiFit_storeHeadPosition()
{
    MatrixXf matTransDevHead = MatrixXf::Identity(4, 4);
    matTransDevHead(0, 3) = 0.01f;
    matTransDevHead(1, 3) = -0.005f;
    MatrixXd matPosition = MatrixXd::Zero(1, 10);
    VectorXd vecGoF(4);
    vecGoF << 0.99, 0.98, 0.97, 0.96;
    std::vector<double> vecError = {0.001, 0.002, 0.001, 0.003};
    InvHpiFit::storeHeadPosition(0.5f, matTransDevHead, matPosition, vecGoF, vecError);
    // storeHeadPosition appends a new row; check the last row has data
    int lastRow = matPosition.rows() - 1;
    QVERIFY(lastRow >= 0);
    QVERIFY(matPosition(lastRow, 0) != 0.0 || matPosition(lastRow, 4) != 0.0);
}

void TestInvRtLibrary::hpiFit_fitWithSyntheticData()
{
    if (!hasData())
        QSKIP("No test data");
    FiffRawData raw(rawPath().toStdString());
    RowVectorXi megPicks = raw.info.pick_types(true, false, false);
    std::vector<FiffChInfo> megChs;
    megChs.reserve(static_cast<std::size_t>(megPicks.size()));
    for (int i = 0; i < megPicks.size(); ++i)
        megChs.push_back(raw.info.chs[megPicks(i)]);
    InvSensorSetCreator creator;
    InvSensorSet sensorSet = creator.updateSensorSet(megChs, Accuracy::medium);
    InvHpiFit hpiFit(sensorSet);
    std::vector<int> hpiFreqs = {154, 158, 162, 166};
    InvHpiModelParameters modelParams(hpiFreqs, static_cast<int>(raw.info.sfreq), 60, false);
    const int nMeg = megPicks.size();
    QCOMPARE(nMeg, sensorSet.ncoils());
    const int nSamples = static_cast<int>(raw.info.sfreq);

    // Four coils at known head positions; the device -> head transform is a 10 degree
    // rotation about z plus a 5 mm shift. Coil k oscillates at hpiFreqs[perm[k]].
    MatrixXd matCoilsHead(4, 3);
    matCoilsHead << 0.07, 0.0, 0.05, -0.07, 0.0, 0.05, 0.0, 0.08, 0.04, 0.0, -0.06, 0.07;
    const double angle = 10.0 * M_PI / 180.0;
    Matrix4f devHead = Matrix4f::Identity();
    devHead.topLeftCorner<3, 3>() << std::cos(angle), -std::sin(angle), 0, std::sin(angle), std::cos(angle), 0, 0, 0, 1;
    devHead.topRightCorner<3, 1>() << 0.003f, -0.002f, 0.004f;
    const FiffCoordTrans trueDevHead(FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD, devHead);
    const MatrixXd matCoilsDev = trueDevHead.apply_inverse_trans(matCoilsHead.cast<float>()).cast<double>();
    const std::vector<int> perm{2, 0, 3, 1};

    // Field of a magnetic dipole in vacuum, averaged over each sensor's integration points.
    auto field = [&](const Vector3d& pos, const Vector3d& moment) {
        VectorXd b(nMeg);
        const int np = sensorSet.np();
        for (int ch = 0; ch < nMeg; ++ch) {
            const MatrixXd rmag = sensorSet.rmag(ch);
            const MatrixXd cosmag = sensorSet.cosmag(ch);
            const RowVectorXd w = sensorSet.w(ch);
            double sum = 0.0;
            for (int p = 0; p < np; ++p) {
                const Vector3d r = rmag.row(p).transpose() - pos;
                const double rn = r.norm();
                const Vector3d bp = 1e-7 * (3.0 * r * r.dot(moment) - moment * rn * rn) / std::pow(rn, 5);
                sum += w(p) * bp.dot(cosmag.row(p).transpose());
            }
            b(ch) = sum;
        }
        return b;
    };
    const RowVectorXd t = RowVectorXd::LinSpaced(nSamples, 0.0, (nSamples - 1) / static_cast<double>(modelParams.iSampleFreq()));
    MatrixXd matData = MatrixXd::Zero(nMeg, nSamples);
    const Vector3d moments[4] = {{0.0, 0.0, 1.0}, {0.3, 0.0, 0.95}, {0.0, -0.4, 0.9}, {0.2, 0.2, 0.95}};
    for (int k = 0; k < 4; ++k) {
        const double f = hpiFreqs[perm[k]];
        // Coil 1 carries cosine, the others sine, so both amplitude branches are used.
        const RowVectorXd phase = 2.0 * M_PI * f * t;
        const RowVectorXd wave = k == 1 ? RowVectorXd(phase.array().cos()) : RowVectorXd(phase.array().sin());
        matData += field(matCoilsDev.row(k).transpose(), 1e-8 * moments[k]) * wave;
    }
    const MatrixXd matProjectors = MatrixXd::Identity(nMeg, nMeg);

    // Digitised coils listed in frequency order: data channel j is the coil at head position perm^-1(j).
    MatrixXd matDigitized(4, 3);
    for (int k = 0; k < 4; ++k)
        matDigitized.row(perm[k]) = matCoilsHead.row(k);

    HpiFitResult result;
    hpiFit.fit(matData, matProjectors, modelParams, matDigitized, true, result);
    QCOMPARE(result.hpiFreqs, hpiFreqs);
    QCOMPARE(result.GoF.size(), 4);
    for (int k = 0; k < 4; ++k) {
        QVERIFY2(result.GoF(k) > 1.0 - 1e-6, qPrintable(QString("GoF %1").arg(result.GoF(k))));
        QVERIFY2(result.errorDistances[k] < 1e-4, qPrintable(QString("coil %1 error %2 m").arg(k).arg(result.errorDistances[k])));
    }
    QVERIFY((result.devHeadTrans.trans - devHead).cwiseAbs().maxCoeff() < 1e-4f);

    // Warm start: a good previous transform seeds the fit from the digitised coils.
    hpiFit.fit(matData, matProjectors, modelParams, matDigitized, false, result);
    QVERIFY((result.devHeadTrans.trans - devHead).cwiseAbs().maxCoeff() < 1e-4f);

    // Each guard leaves the result untouched.
    HpiFitResult untouched;
    hpiFit.fit(matData, MatrixXd::Identity(3, 3), modelParams, matDigitized, untouched);
    hpiFit.fit(matData, matProjectors, modelParams, matDigitized.topRows(3), untouched);
    hpiFit.fit(MatrixXd(), MatrixXd(), modelParams, matDigitized, untouched);
    hpiFit.fit(matData.topRows(10), MatrixXd::Identity(10, 10), modelParams, matDigitized, untouched);
    QVERIFY(untouched.GoF.size() == 0);
}

void TestInvRtLibrary::hpiFitData_defaultConstruction()
{
    // InvHpiFit's documented defaults: 500 iterations, abort at an error of 1e-9
    InvHpiFitData data;
    QCOMPARE(data.m_iMaxIterations, 500);
    QVERIFY(qFuzzyCompare(data.m_fAbortError, 1e-9f));
    QCOMPARE(data.m_errorInfo.numIterations, 0);
}

//=============================================================================================================
// InvHpiModelParameters (from boost)
//=============================================================================================================

void TestInvRtLibrary::hpiModelParams_construction()
{
    std::vector<int> freqs = {154, 158, 162, 166};
    InvHpiModelParameters params(freqs, 1000, 60, false);
    QCOMPARE(params.vecHpiFreqs(), freqs);
    QCOMPARE(params.iSampleFreq(), 1000);
    QCOMPARE(params.iLineFreq(), 60);
    QCOMPARE(params.iNHpiCoils(), 4);
    QCOMPARE(params.bBasic(), false);
}

void TestInvRtLibrary::hpiModelParams_copyAndCompare()
{
    std::vector<int> freqs = {154, 158, 162, 166};
    InvHpiModelParameters params1(freqs, 1000, 60, false);
    InvHpiModelParameters params2(params1);
    QVERIFY(params1 == params2);
    QVERIFY(!(params1 != params2));
    InvHpiModelParameters params3(freqs, 2000, 50, true);
    QVERIFY(params1 != params3);
}

//=============================================================================================================
// InvSignalModel (from boost)
//=============================================================================================================

void TestInvRtLibrary::signalModel_construction()
{
    std::vector<int> freqs = {154, 158, 162, 166};
    InvHpiModelParameters params(freqs, 1000, 60, false);
    InvSignalModel model;
    int nSamples = 200;
    MatrixXd matData = MatrixXd::Random(1, nSamples);
    MatrixXd matFitted = model.fitData(params, matData);
    QVERIFY(matFitted.rows() > 0 && matFitted.cols() > 0);
}

//=============================================================================================================
// FilterKernel (from boost)
//=============================================================================================================

void TestInvRtLibrary::filterKernel_design_lpf()
{
    FilterKernel kernel("lpf_test", 0, 256, 40.0, 0.0, 0.0, 1000.0, 0);
    QVERIFY(kernel.coefficients().size() > 0);
}

void TestInvRtLibrary::filterKernel_design_hpf()
{
    FilterKernel kernel("hpf_test", 1, 256, 0.0, 1.0, 0.0, 1000.0, 0);
    QVERIFY(kernel.coefficients().size() > 0);
}

void TestInvRtLibrary::filterKernel_design_bpf()
{
    FilterKernel kernel("bpf_test", 2, 256, 40.0, 1.0, 0.0, 1000.0, 0);
    QVERIFY(kernel.coefficients().size() > 0);
}

//=============================================================================================================
// Filter apply (from boost)
//=============================================================================================================

void TestInvRtLibrary::filter_applyToRawData()
{
    if (!hasData())
        QSKIP("No test data");
    FiffRawData raw(rawPath().toStdString());
    MatrixXd data, times;
    raw.read_raw_segment(data, times, raw.first_samp, raw.first_samp + 4000);
    FilterKernel kernel("test_lp", 0, 256, 40.0, 0.0, 0.0, raw.info.sfreq, 0);
    MatrixXd subData = data.block(0, 0, qMin(5, static_cast<int>(data.rows())), data.cols());
    MatrixXd filtered = mne::filterData(subData, kernel);
    QVERIFY(filtered.rows() > 0);
    QVERIFY(filtered.cols() > 0);
}

void TestInvRtLibrary::filter_applyWithChannelSelection()
{
    if (!hasData())
        QSKIP("No test data");
    FiffRawData raw(rawPath().toStdString());
    MatrixXd data, times;
    raw.read_raw_segment(data, times, raw.first_samp, raw.first_samp + 2000);
    FilterKernel kernel("test_hp", 1, 128, 0.0, 1.0, 0.0, raw.info.sfreq, 0);
    MatrixXd subData = data.block(0, 0, qMin(3, static_cast<int>(data.rows())), data.cols());
    MatrixXd filtered = mne::filterData(subData, kernel);
    QVERIFY(filtered.rows() > 0);
}

//=============================================================================================================
// RtNoise extras (from coverage_push)
//=============================================================================================================

//=============================================================================================================
// FilterOverlapAdd (from coverage_push)
//=============================================================================================================

void TestInvRtLibrary::filterOverlapAdd_calculate()
{
    MatrixXd data = MatrixXd::Random(2, 2048);
    RowVectorXi picks(2);
    picks << 0, 1;
    mne::FilterOverlapAdd filterOA;
    MatrixXd result = filterOA.calculate(data, 0, 40.0, 0.0, 5.0, 1000.0, 128, 0,
                                         picks, true, false, false);
    QVERIFY(result.rows() == data.rows());
    QVERIFY(result.cols() > 0);
    filterOA.reset();
}

void TestInvRtLibrary::filterData_paramOverload()
{
    MatrixXd data = MatrixXd::Random(3, 1024);
    RowVectorXi picks(3);
    picks << 0, 1, 2;
    MatrixXd result = mne::filterData(data, 0, 40.0, 0.0, 5.0, 1000.0, 128, 0,
                                      picks, false, false);
    QVERIFY(result.rows() == data.rows());
    QVERIFY(result.cols() > 0);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestInvRtLibrary)

#include "test_inv_rt_library.moc"

#include <vector>
