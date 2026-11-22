//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_rt_cov.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.10
 * @date     March, 2026
 * @brief    Unit tests for RtCov.
 */

#include <QtTest/QtTest>

#include <cmath>
#include <limits>
#include <memory>

#include <mne/dsp/rt/rt_cov.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_constants.h>

using namespace mne;
using namespace Eigen;

namespace
{

std::shared_ptr<FiffInfo> makeSyntheticInfo(double sfreq = 0.0)
{
    std::shared_ptr<FiffInfo> info(new FiffInfo);
    info->sfreq = sfreq;

    for (int i = 0; i < 2; ++i) {
        FiffChInfo ch;
        ch.kind = FIFFV_EEG_CH;
        ch.unit = FIFF_UNIT_V;
        ch.ch_name = (QString("EEG%1").arg(i + 1, 3, 10, QChar('0'))).toStdString();
        ch.range = 1.0f;
        ch.cal = 1.0f;
        ch.chpos.coil_type = FIFFV_COIL_EEG;
        ch.chpos.r0 << 0.01f * (i + 1), 0.0f, 0.02f;
        info->chs.push_back(ch);
        info->ch_names.push_back(ch.ch_name);
    }

    info->nchan = static_cast<int>(info->chs.size());
    return info;
}

MatrixXd wave(Index samples)
{
    MatrixXd data(2, samples);
    for (Index t = 0; t < samples; ++t) {
        data(0, t) = std::sin(0.7 * static_cast<double>(t));
        data(1, t) = 0.3 * static_cast<double>(t) - std::cos(1.3 * static_cast<double>(t));
    }
    return data;
}

}

class TestDspRtCov : public QObject
{
    Q_OBJECT

private slots:
    void returnsNothingUntilTwoSamples()
    {
        RtCov cov(*makeSyntheticInfo());
        QVERIFY(!cov.covariance());
        cov.update(MatrixXd::Ones(2, 1));
        QVERIFY(!cov.covariance());
    }

    void computesExpectedRegularizedCovariance()
    {
        RtCov cov(*makeSyntheticInfo());

        MatrixXd block(2, 3);
        block << 1.0, 2.0, 3.0,
            2.0, 4.0, 6.0;
        cov.update(block);

        const std::optional<FiffCov> result = cov.covariance();
        QVERIFY(result);
        QCOMPARE(result->dim, 2);
        QCOMPARE(result->nfree, 3);
        QVERIFY(result->names == (StringList{"EEG001", "EEG002"}));

        // Raw sample covariance is [[1, 2], [2, 4]]. EEG regularization adds 10% of mean(diag)=0.25 to the diagonal.
        QVERIFY((result->data - (Matrix2d() << 1.25, 2.0, 2.0, 4.25).finished()).cwiseAbs().maxCoeff() < 1e-9);
    }

    void blocksGiveTheCovarianceOfTheirConcatenation_data()
    {
        QTest::addColumn<double>("halfLife");
        QTest::newRow("all samples") << std::numeric_limits<double>::infinity();
        QTest::newRow("half-life 4 samples") << 4.0;
    }

    void blocksGiveTheCovarianceOfTheirConcatenation()
    {
        QFETCH(double, halfLife);
        const MatrixXd all = wave(12);
        RtCov whole(*makeSyntheticInfo(1.0), halfLife);
        whole.update(all);
        const FiffCov expected = whole.covariance().value();

        RtCov blocks(*makeSyntheticInfo(1.0), halfLife);
        blocks.update(all.leftCols(5));
        blocks.update(all.middleCols(5, 4));
        blocks.update(all.rightCols(3));
        const FiffCov result = blocks.covariance().value();
        QCOMPARE(result.nfree, expected.nfree);
        QVERIFY((result.data - expected.data).cwiseAbs().maxCoeff() < 1e-12 * expected.data.cwiseAbs().maxCoeff());
    }

    void forgettingWeighsSamplesByTheirAge()
    {
        // Half-life 0.05 s at 100 Hz: a sample five samples older counts half as much
        const MatrixXd all = wave(40);
        RtCov cov(*makeSyntheticInfo(100.0), 0.05);
        cov.update(all.leftCols(17));
        cov.update(all.rightCols(23));
        const FiffCov result = cov.covariance().value();

        // numpy.cov(all, aweights=w) with w = 0.5 ** (age / 5)
        VectorXd w(all.cols());
        for (Index t = 0; t < all.cols(); ++t) {
            w(t) = std::pow(0.5, static_cast<double>(all.cols() - 1 - t) / 5.0);
        }
        const double v1 = w.sum();
        const double v2 = w.squaredNorm();
        const MatrixXd centred = all.colwise() - all * w / v1;
        const MatrixXd raw = centred * w.asDiagonal() * centred.transpose() / (v1 - v2 / v1);
        const MatrixXd expected = raw + MatrixXd::Identity(2, 2) * 0.1 * raw.diagonal().mean();
        QVERIFY((result.data - expected).cwiseAbs().maxCoeff() < 1e-12 * expected.cwiseAbs().maxCoeff());
        QCOMPARE(result.nfree, static_cast<int>(std::lround(v1 * v1 / v2)));
    }

    void followsAChangeInTheNoise()
    {
        // Unit variance for 10 s, then variance 9: a 0.1 s half-life has forgotten the first part after 1 s
        const double sfreq = 1000.0;
        RtCov tracking(*makeSyntheticInfo(sfreq), 0.1);
        RtCov everything(*makeSyntheticInfo(sfreq));
        for (int second = 0; second < 11; ++second) {
            MatrixXd block = MatrixXd::Zero(2, 1000);
            for (Index t = 0; t < block.cols(); ++t) {
                // Deterministic, uncorrelated, zero mean and unit variance per channel
                block(0, t) = std::sqrt(2.0) * std::sin(2.1 * static_cast<double>(t));
                block(1, t) = std::sqrt(2.0) * std::cos(2.9 * static_cast<double>(t));
            }
            if (second == 10) {
                block *= 3.0;
            }
            tracking.update(block);
            everything.update(block);
        }
        const double variance = tracking.covariance().value().data(0, 0) / 1.1;
        const double average = everything.covariance().value().data(0, 0) / 1.1;
        QVERIFY2(std::fabs(variance - 9.0) < 0.1, qPrintable(QString::number(variance)));
        QVERIFY2(average < 2.0, qPrintable(QString::number(average)));
    }

    void keepsPrecisionWithALargeOffset()
    {
        // 1 nV noise on a 10 mV offset; summing raw outer products loses it to cancellation
        MatrixXd block = wave(1000) * 1e-9;
        block.array() += 1e-2;
        RtCov cov(*makeSyntheticInfo());
        for (Index start = 0; start < block.cols(); start += 100) {
            cov.update(block.middleCols(start, 100));
        }
        const MatrixXd centred = block.colwise() - block.rowwise().mean();
        const MatrixXd raw = centred * centred.transpose() / static_cast<double>(block.cols() - 1);
        const double expected = raw(0, 1);
        const double result = cov.covariance().value().data(0, 1);
        QVERIFY2(std::fabs(result - expected) < 1e-6 * std::fabs(expected), qPrintable(QString("%1 vs %2").arg(result).arg(expected)));
    }

    void resetStartsOver()
    {
        RtCov cov(*makeSyntheticInfo());
        cov.update(wave(10));
        QVERIFY(cov.covariance());
        cov.reset();
        QVERIFY(!cov.covariance());
        cov.update(MatrixXd::Ones(2, 1));
        QVERIFY(!cov.covariance());
    }

    void needsMeasurementInfo()
    {
        RtCov cov{FiffInfo()};
        cov.update(MatrixXd::Ones(2, 10));
        QVERIFY(!cov.covariance());
    }
};

QTEST_GUILESS_MAIN(TestDspRtCov)
#include "test_dsp_rt_cov.moc"
