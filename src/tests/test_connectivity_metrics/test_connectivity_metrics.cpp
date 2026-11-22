// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <mne/connectivity/compute_connectivity.h>
#include <mne/connectivity/connectivitysettings.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_info.h>
#include <mne/connectivity/network/network.h>
#include <mne/connectivity/metrics/correlation.h>
#include <mne/connectivity/metrics/coherence.h>
#include <mne/connectivity/metrics/debiasedsquaredweightedphaselagindex.h>
#include <mne/connectivity/metrics/mvar_model.h>
#include <mne/connectivity/metrics/weightedphaselagindex.h>

#include <cmath>
#include <random>
#include <numbers>

using namespace mne;
using namespace Eigen;

class TestConnectivityMetrics : public QObject
{
    Q_OBJECT

private slots:

    void testConnectivitySettingsCtor()
    {
        ConnectivitySettings settings;
        QVERIFY(settings.empty());
        QCOMPARE(settings.size(), 0);
    }

    void testConnectivitySettingsAppendMatrix()
    {
        ConnectivitySettings settings;
        // 4 channels, 100 samples
        MatrixXd trial = MatrixXd::Random(4, 100);
        settings.append(trial);
        QCOMPARE(settings.size(), 1);
        QVERIFY(!settings.empty());
    }

    void testConnectivitySettingsAppendList()
    {
        ConnectivitySettings settings;
        std::vector<MatrixXd> trials;
        trials.push_back(MatrixXd::Random(4, 100));
        trials.push_back(MatrixXd::Random(4, 100));
        settings.append(trials);
        QCOMPARE(settings.size(), 2);
    }

    void testConnectivitySettingsSFreq()
    {
        ConnectivitySettings settings;
        settings.setSamplingFrequency(1000);
        QCOMPARE(settings.getSamplingFrequency(), 1000);
    }

    void testConnectivitySettingsFFTSize()
    {
        ConnectivitySettings settings;
        settings.setFFTSize(256);
        QCOMPARE(settings.getFFTSize(), 256);
        settings.setFFTSize(0);
        QCOMPARE(settings.getFFTSize(), 256);
        // The frequency resolution (1000 Hz / 256) is kept when the sampling frequency changes
        settings.setSamplingFrequency(1000);
        settings.setSamplingFrequency(500);
        QCOMPARE(settings.getFFTSize(), 128);
    }

    void testConnectivitySettingsWindowType()
    {
        ConnectivitySettings settings;
        settings.setWindowType("hanning");
        QCOMPARE(settings.getWindowType(), std::string("hanning"));
    }

    void testConnectivitySettingsMethods()
    {
        ConnectivitySettings settings;
        settings.setConnectivityMethods({"COR", "XCOR"});
        QCOMPARE(settings.getConnectivityMethods().size(), std::size_t(2));
        QCOMPARE(settings.getConnectivityMethods().at(0), std::string("COR"));
    }

    void testConnectivitySettingsNodePositions()
    {
        ConnectivitySettings settings;
        MatrixX3f pos(4, 3);
        pos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(pos);
        QCOMPARE(settings.getNodePositions().rows(), 4);

        // From the picked channels of a recording: EEG/MEG take their position, other channels sit at the origin
        mne::FiffInfo info;
        const QList<int> kinds{FIFFV_STIM_CH, FIFFV_EEG_CH, FIFFV_MEG_CH};
        for (int k = 0; k < kinds.size(); ++k) {
            mne::FiffChInfo ch;
            ch.kind = kinds[k];
            ch.chpos.r0 = Vector3f(k + 1.0f, 2.0f * k, -1.0f);
            info.chs.push_back(ch);
        }
        settings.setNodePositions(info, RowVector3i(1, 0, 2));
        MatrixX3f expected(3, 3);
        expected << 2, 2, -1, 0, 0, 0, 3, 4, -1;
        QCOMPARE(settings.getNodePositions(), expected);
    }

    void testConnectivitySettingsClearAll()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 100));
        settings.clearAllData();
        QCOMPARE(settings.size(), 0);
    }

    void testConnectivitySettingsClearIntermediate()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 100));
        settings.clearIntermediateData();
        // Data should still exist but intermediate cleared
        QCOMPARE(settings.size(), 1);
    }

    void testConnectivitySettingsRemoveFirst()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 100));
        settings.append(MatrixXd::Random(4, 100));
        settings.append(MatrixXd::Random(4, 100));
        settings.removeFirst(1);
        QCOMPARE(settings.size(), 2);
    }

    void testConnectivitySettingsRemoveLast()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 100));
        settings.append(MatrixXd::Random(4, 100));
        settings.removeLast(1);
        QCOMPARE(settings.size(), 1);

        // Removing trials takes their spectra out of the running sums; sums of another length are left alone
        const auto trial = [](double v) {
            ConnectivitySettings::IntermediateTrialData t;
            t.matData = MatrixXd::Zero(2, 4);
            t.matPsd = MatrixXd::Constant(2, 3, v);
            for (int i = 0; i < 2; ++i) {
                t.vecPairCsd.push_back({i, MatrixXcd::Constant(2, 3, v)});
                t.vecPairCsdNormalized.push_back({i, MatrixXcd::Constant(2, 3, v)});
                t.vecPairCsdImagSign.push_back({i, MatrixXd::Constant(2, 3, v)});
                t.vecPairCsdImagAbs.push_back({i, MatrixXd::Constant(2, 3, v)});
                t.vecPairCsdImagSqrd.push_back({i, MatrixXd::Constant(2, 3, v)});
            }
            return t;
        };
        ConnectivitySettings sums;
        for (const double v : {1.0, 2.0, 4.0})
            sums.append(trial(v));
        ConnectivitySettings::IntermediateSumData& sum = sums.getIntermediateSumData();
        const ConnectivitySettings::IntermediateTrialData total = trial(7.0);
        sum.matPsdSum = total.matPsd;
        sum.vecPairCsdSum = total.vecPairCsd;
        sum.vecPairCsdNormalizedSum = total.vecPairCsdNormalized;
        sum.vecPairCsdImagSignSum = total.vecPairCsdImagSign;
        sum.vecPairCsdImagAbsSum = total.vecPairCsdImagAbs;
        sum.vecPairCsdImagSqrdSum = {total.vecPairCsdImagSqrd.front()};
        sums.removeFirst(1);
        sums.removeLast(1);
        QCOMPARE(sums.size(), 1);
        QCOMPARE(sum.matPsdSum(1, 2), 2.0);
        QCOMPARE(sum.vecPairCsdSum[1].second(0, 0), std::complex<double>(2.0, 0.0));
        QCOMPARE(sum.vecPairCsdNormalizedSum[0].second(1, 1), std::complex<double>(2.0, 0.0));
        QCOMPARE(sum.vecPairCsdImagSignSum[1].second(0, 2), 2.0);
        QCOMPARE(sum.vecPairCsdImagAbsSum[0].second(1, 0), 2.0);
        QCOMPARE(sum.vecPairCsdImagSqrdSum[0].second(0, 0), 7.0);

        // More trials than stored, or none at all, removes nothing
        sums.removeFirst(2);
        sums.removeLast(2);
        QCOMPARE(sums.size(), 1);
        sums.clearAllData();
        sums.removeFirst(1);
        sums.removeLast(1);
        QVERIFY(sums.empty());
    }

    void testMvarModel()
    {
        // A two-channel AR(1) process where channel 0 drives channel 1
        Matrix2d coeff;
        coeff << 0.5, 0.0, 0.4, 0.3;
        std::seed_seq seed{7};
        std::mt19937 rng(seed);
        std::normal_distribution<double> noise;
        MatrixXd data = MatrixXd::Zero(2, 4000);
        for (int t = 1; t < data.cols(); ++t)
            data.col(t) = coeff * data.col(t - 1) + Vector2d(noise(rng), noise(rng));
        MvarModel model;
        model.fit(data);
        QCOMPARE(model.order(), 1);
        QVERIFY((model.coefficients()[0] - coeff).cwiseAbs().maxCoeff() < 0.05);
        QVERIFY((model.noiseCov() - Matrix2d::Identity()).cwiseAbs().maxCoeff() < 0.1);

        // Too few samples for the order leaves no model, whose transfer function is the identity
        model.fit(data.leftCols(3), 5);
        QCOMPARE(model.order(), 0);
        QVERIFY(model.coefficients().empty());
        const std::vector<MatrixXcd> h = model.transferFunction(Vector2d(0.1, 0.2));
        QCOMPARE(h.size(), 2);
        QVERIFY(h[1].isIdentity());

        // Too short or flat data to compare orders falls back to order one
        model.fit(data.leftCols(3));
        QCOMPARE(model.order(), 1);
        model.fit(MatrixXd::Ones(2, 50));
        QCOMPARE(model.order(), 1);
    }

    void testCalculateCorrelation()
    {
        ConnectivitySettings settings;
        // Create synthetic data: 4 channels, 200 samples, 2 trials
        settings.append(MatrixXd::Random(4, 200));
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(256);
        settings.setWindowType("hanning");

        settings.setConnectivityMethods({"COR"});

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        std::vector<Network> networks = compute_connectivity(settings);
        QCOMPARE(networks.size(), 1);

        // Trial-averaged Pearson r, as numpy.corrcoef per trial then mean
        MatrixXd expected = MatrixXd::Zero(4, 4);
        for (int t = 0; t < settings.size(); ++t) {
            const MatrixXd c = settings.at(t).matData.colwise() - settings.at(t).matData.rowwise().mean();
            const VectorXd n = c.rowwise().norm();
            expected += (c * c.transpose()).cwiseQuotient(n * n.transpose()) / settings.size();
        }
        const MatrixXd actual = networks.front().connectivity_matrix(EdgeSet::full, false);
        for (int i = 0; i < 4; ++i) {
            for (int j = i + 1; j < 4; ++j) {
                QVERIFY2(qAbs(actual(i, j) - expected(i, j)) < 1e-12,
                         qPrintable(QString("COR(%1,%2) %3 vs %4").arg(i).arg(j).arg(actual(i, j)).arg(expected(i, j))));
            }
        }
    }

    void testCalculateCrossCorrelation()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(256);
        settings.setWindowType("hanning");

        settings.setConnectivityMethods({"XCOR"});

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        std::vector<Network> networks = compute_connectivity(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateCoherence()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        settings.setConnectivityMethods({"COH"});

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        std::vector<Network> networks = compute_connectivity(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateImagCoherence()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        settings.setConnectivityMethods({"IMAGCOH"});

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        std::vector<Network> networks = compute_connectivity(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculatePLI()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        settings.setConnectivityMethods({"PLI"});

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        std::vector<Network> networks = compute_connectivity(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculatePLV()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        settings.setConnectivityMethods({"PLV"});

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        std::vector<Network> networks = compute_connectivity(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateWPLI()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        settings.setConnectivityMethods({"WPLI"});

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        std::vector<Network> networks = compute_connectivity(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateUSPLI()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        settings.setConnectivityMethods({"USPLI"});

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        std::vector<Network> networks = compute_connectivity(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateDSWPLI()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        settings.setConnectivityMethods({"DSWPLI"});

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        std::vector<Network> networks = compute_connectivity(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateMultipleMethods()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        settings.setConnectivityMethods({"COR", "COH", "PLI"});

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        std::vector<Network> networks = compute_connectivity(settings);
        QCOMPARE(networks.size(), 3);
    }

    void testDebiasedWpliFindsALaggedPairAndAccumulatesTrials()
    {
        // Two channels share a 10 Hz rhythm a quarter cycle apart (random phase per trial), a third is noise.
        // nfft > signal length takes the zero-padded path; the band covers the 10 Hz bins of a 256-point FFT.
        const double sfreq = 100.0;
        std::seed_seq seed{3};
        std::mt19937 rng(seed);
        std::normal_distribution<double> noise(0.0, 0.1);
        std::uniform_real_distribution<double> phase(0.0, 2.0 * std::numbers::pi);
        std::vector<MatrixXd> trials;
        for (int t = 0; t < 20; ++t) {
            const double phi = phase(rng);
            MatrixXd trial(3, 200);
            for (int k = 0; k < 200; ++k) {
                const double w = 2.0 * std::numbers::pi * 10.0 * k / sfreq;
                trial(0, k) = std::sin(w + phi) + noise(rng);
                trial(1, k) = std::sin(w + phi - std::numbers::pi / 2.0) + noise(rng);
                trial(2, k) = noise(rng);
            }
            trials.push_back(trial);
        }
        const auto settingsFor = [sfreq](std::span<const MatrixXd> data, bool keep) {
            ConnectivitySettings settings;
            settings.setSamplingFrequency(static_cast<int>(sfreq));
            settings.setFFTSize(256);
            settings.setWindowType("hanning");
            settings.set_frequency_bins(24, 4);
            settings.set_keep_trial_data(keep);
            settings.append(data);
            return settings;
        };
        ConnectivitySettings all = settingsFor(trials, false);
        const MatrixXd fresh = DebiasedSquaredWeightedPhaseLagIndex::calculate(all).connectivity_matrix();

        // Kept per-trial spectra let an appended trial give what computing all at once gives
        ConnectivitySettings growing = settingsFor(std::span(trials).first(19), true);
        DebiasedSquaredWeightedPhaseLagIndex::calculate(growing);
        growing.append(trials.back());
        const MatrixXd accumulated = DebiasedSquaredWeightedPhaseLagIndex::calculate(growing).connectivity_matrix();

        // Coherence finds the cross-spectra a phase-lag metric kept and still computes the spectra
        ConnectivitySettings shared = settingsFor(trials, true);
        WeightedPhaseLagIndex::calculate(shared);
        const MatrixXd coherence = Coherence::calculate(shared).connectivity_matrix();
        ConnectivitySettings alone = settingsFor(trials, false);
        QVERIFY((coherence - Coherence::calculate(alone).connectivity_matrix()).cwiseAbs().maxCoeff() < 1e-12);

        // Another analysis of the whole spectrum leaves the band of the first untouched
        ConnectivitySettings wide = settingsFor(trials, false);
        wide.set_frequency_bins(-1, -1);
        QCOMPARE(DebiasedSquaredWeightedPhaseLagIndex::calculate(wide).used_freq_bins(), 129);
        QCOMPARE(all.frequency_bins(), std::make_pair(24, 4));
        const MatrixXd again = DebiasedSquaredWeightedPhaseLagIndex::calculate(all).connectivity_matrix();
        QVERIFY((again - fresh).cwiseAbs().maxCoeff() < 1e-12);

        QVERIFY2(fresh(0, 1) > 0.9, qPrintable(QString::number(fresh(0, 1))));
        QVERIFY2(std::abs(fresh(0, 2)) < 0.3 && std::abs(fresh(1, 2)) < 0.3, qPrintable(QString("%1 %2").arg(fresh(0, 2)).arg(fresh(1, 2))));
        QVERIFY2((accumulated - fresh).cwiseAbs().maxCoeff() < 1e-9, qPrintable(QString::number((accumulated - fresh).cwiseAbs().maxCoeff())));
    }

    void testCoherenceListsEdgesInNodeOrder()
    {
        // The rows of the cross-spectra are reduced in parallel; the edges must still come out row by row
        const int channels = 24;
        MatrixXd data(channels, 256);
        for (int i = 0; i < channels; ++i)
            for (int k = 0; k < data.cols(); ++k)
                data(i, k) = std::sin(0.05 * (i + 1) * k) + std::cos(0.013 * (k + 3 * i));
        for (const std::string method : {"COH", "IMAGCOH"}) {
            ConnectivitySettings settings;
            settings.setSamplingFrequency(1000);
            settings.setFFTSize(256);
            settings.setWindowType("hanning");
            settings.setConnectivityMethods({method});
            for (int trial = 0; trial < 4; ++trial)
                settings.append(data);
            const std::vector<Network> networks = compute_connectivity(settings);
            QCOMPARE(networks.size(), std::size_t(1));
            const std::vector<NetworkEdge>& edges = networks.front().edges();
            QCOMPARE(edges.size(), std::size_t(channels * (channels - 1) / 2));
            for (std::size_t k = 1; k < edges.size(); ++k) {
                const auto previous = std::make_pair(edges[k - 1].source(), edges[k - 1].target());
                const auto current = std::make_pair(edges[k].source(), edges[k].target());
                QVERIFY2(previous < current, qPrintable(QString("%1 edge %2").arg(QString::fromStdString(method)).arg(k)));
            }
        }
    }
};

QTEST_GUILESS_MAIN(TestConnectivityMetrics)
#include "test_connectivity_metrics.moc"
