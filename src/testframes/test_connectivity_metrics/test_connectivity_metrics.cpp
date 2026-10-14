// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <connectivity/connectivity.h>
#include <connectivity/connectivitysettings.h>
#include <connectivity/network/network.h>
#include <connectivity/metrics/correlation.h>
#include <connectivity/metrics/abstractmetric.h>
#include <connectivity/metrics/debiasedsquaredweightedphaselagindex.h>

#include <cmath>
#include <random>

using namespace CONNECTIVITYLIB;
using namespace Eigen;

class TestConnectivityMetrics : public QObject
{
    Q_OBJECT

private slots:

    void testConnectivitySettingsCtor()
    {
        ConnectivitySettings settings;
        QVERIFY(settings.isEmpty());
        QCOMPARE(settings.size(), 0);
    }

    void testConnectivitySettingsAppendMatrix()
    {
        ConnectivitySettings settings;
        // 4 channels, 100 samples
        MatrixXd trial = MatrixXd::Random(4, 100);
        settings.append(trial);
        QCOMPARE(settings.size(), 1);
        QVERIFY(!settings.isEmpty());
    }

    void testConnectivitySettingsAppendList()
    {
        ConnectivitySettings settings;
        QList<MatrixXd> trials;
        trials.append(MatrixXd::Random(4, 100));
        trials.append(MatrixXd::Random(4, 100));
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
    }

    void testConnectivitySettingsWindowType()
    {
        ConnectivitySettings settings;
        settings.setWindowType("hanning");
        QCOMPARE(settings.getWindowType(), QString("hanning"));
    }

    void testConnectivitySettingsMethods()
    {
        ConnectivitySettings settings;
        QStringList methods;
        methods << "COR" << "XCOR";
        settings.setConnectivityMethods(methods);
        QCOMPARE(settings.getConnectivityMethods().size(), 2);
        QCOMPARE(settings.getConnectivityMethods().at(0), QString("COR"));
    }

    void testConnectivitySettingsNodePositions()
    {
        ConnectivitySettings settings;
        MatrixX3f pos(4, 3);
        pos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(pos);
        QCOMPARE(settings.getNodePositions().rows(), 4);
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

        QStringList methods;
        methods << "COR";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);

        // Trial-averaged Pearson r, as numpy.corrcoef per trial then mean
        MatrixXd expected = MatrixXd::Zero(4, 4);
        for (int t = 0; t < settings.size(); ++t) {
            const MatrixXd c = settings.at(t).matData.colwise() - settings.at(t).matData.rowwise().mean();
            const VectorXd n = c.rowwise().norm();
            expected += (c * c.transpose()).cwiseQuotient(n * n.transpose()) / settings.size();
        }
        const MatrixXd actual = networks.first().getFullConnectivityMatrix(false);
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

        QStringList methods;
        methods << "XCOR";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateCoherence()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "COH";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateImagCoherence()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "IMAGCOH";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculatePLI()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "PLI";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculatePLV()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "PLV";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateWPLI()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "WPLI";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateUSPLI()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "USPLI";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateDSWPLI()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "DSWPLI";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
        QCOMPARE(networks.size(), 1);
    }

    void testCalculateMultipleMethods()
    {
        ConnectivitySettings settings;
        settings.append(MatrixXd::Random(4, 200));
        settings.setSamplingFrequency(1000);
        settings.setFFTSize(128);
        settings.setWindowType("hanning");

        QStringList methods;
        methods << "COR" << "COH" << "PLI";
        settings.setConnectivityMethods(methods);

        MatrixX3f nodePos(4, 3);
        nodePos << 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1;
        settings.setNodePositions(nodePos);

        QList<Network> networks = Connectivity::calculate(settings);
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
        std::uniform_real_distribution<double> phase(0.0, 2.0 * M_PI);
        QList<MatrixXd> trials;
        for (int t = 0; t < 20; ++t) {
            const double phi = phase(rng);
            MatrixXd trial(3, 200);
            for (int k = 0; k < 200; ++k) {
                const double w = 2.0 * M_PI * 10.0 * k / sfreq;
                trial(0, k) = std::sin(w + phi) + noise(rng);
                trial(1, k) = std::sin(w + phi - M_PI / 2.0) + noise(rng);
                trial(2, k) = noise(rng);
            }
            trials << trial;
        }
        const auto settingsFor = [sfreq](const QList<MatrixXd>& data) {
            ConnectivitySettings settings;
            settings.setSamplingFrequency(static_cast<int>(sfreq));
            settings.setFFTSize(256);
            settings.setWindowType("hanning");
            settings.append(data);
            return settings;
        };
        const bool storage = AbstractMetric::m_bStorageModeIsActive;
        const int start = AbstractMetric::m_iNumberBinStart, amount = AbstractMetric::m_iNumberBinAmount;
        AbstractMetric::m_iNumberBinStart = 24;
        AbstractMetric::m_iNumberBinAmount = 4;

        AbstractMetric::m_bStorageModeIsActive = false;
        ConnectivitySettings all = settingsFor(trials);
        const MatrixXd fresh = DebiasedSquaredWeightedPhaseLagIndex::calculate(all).getFullConnectivityMatrix();

        // In storage mode the per-trial spectra are kept, so adding a trial gives what computing all at once gives
        AbstractMetric::m_bStorageModeIsActive = true;
        ConnectivitySettings growing = settingsFor(trials.mid(0, 19));
        DebiasedSquaredWeightedPhaseLagIndex::calculate(growing);
        growing.append(trials.last());
        const MatrixXd accumulated = DebiasedSquaredWeightedPhaseLagIndex::calculate(growing).getFullConnectivityMatrix();

        AbstractMetric::m_bStorageModeIsActive = storage;
        AbstractMetric::m_iNumberBinStart = start;
        AbstractMetric::m_iNumberBinAmount = amount;

        QVERIFY2(fresh(0, 1) > 0.9, qPrintable(QString::number(fresh(0, 1))));
        QVERIFY2(std::abs(fresh(0, 2)) < 0.3 && std::abs(fresh(1, 2)) < 0.3, qPrintable(QString("%1 %2").arg(fresh(0, 2)).arg(fresh(1, 2))));
        QVERIFY2((accumulated - fresh).cwiseAbs().maxCoeff() < 1e-9, qPrintable(QString::number((accumulated - fresh).cwiseAbs().maxCoeff())));
    }
};

QTEST_GUILESS_MAIN(TestConnectivityMetrics)
#include "test_connectivity_metrics.moc"
