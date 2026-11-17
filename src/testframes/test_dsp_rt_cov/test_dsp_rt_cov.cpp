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
#include <memory>

#include <dsp/rt/rt_cov.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

using namespace mne;
using namespace Eigen;

namespace
{

std::shared_ptr<FiffInfo> makeSyntheticInfo()
{
    std::shared_ptr<FiffInfo> info(new FiffInfo);

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

}

class TestDspRtCov : public QObject
{
    Q_OBJECT

private slots:
    void returnsNothingUntilEnoughSamples()
    {
        RtCov cov(*makeSyntheticInfo());

        MatrixXd block(2, 2);
        block << 1.0, 2.0,
            2.0, 4.0;

        QVERIFY(!cov.estimate_covariance(block, 3));
    }

    void computesExpectedRegularizedCovariance()
    {
        RtCov cov(*makeSyntheticInfo());

        MatrixXd block(2, 3);
        block << 1.0, 2.0, 3.0,
            2.0, 4.0, 6.0;

        const std::optional<FiffCov> result = cov.estimate_covariance(block, 3);
        QVERIFY(result);
        QCOMPARE(result->dim, 2);
        QCOMPARE(result->nfree, 3);
        QVERIFY(result->names == (StringList{"EEG001", "EEG002"}));

        // Raw sample covariance is [[1, 2], [2, 4]]. EEG regularization adds 10% of mean(diag)=0.25 to the diagonal.
        QVERIFY((result->data - (Matrix2d() << 1.25, 2.0, 2.0, 4.25).finished()).cwiseAbs().maxCoeff() < 1e-9);
    }

    void blocksGiveTheCovarianceOfTheirConcatenation()
    {
        MatrixXd all(2, 12);
        for (int t = 0; t < all.cols(); ++t) {
            all(0, t) = std::sin(0.7 * t);
            all(1, t) = 0.3 * t - std::cos(1.3 * t);
        }
        RtCov whole(*makeSyntheticInfo());
        const FiffCov expected = whole.estimate_covariance(all, 12).value();

        RtCov blocks(*makeSyntheticInfo());
        QVERIFY(!blocks.estimate_covariance(all.leftCols(5), 12));
        QVERIFY(!blocks.estimate_covariance(all.middleCols(5, 4), 12));
        const std::optional<FiffCov> result = blocks.estimate_covariance(all.rightCols(3), 12);
        QVERIFY(result);
        QCOMPARE(result->nfree, 12);
        QVERIFY((result->data - expected.data).cwiseAbs().maxCoeff() < 1e-12);
    }

    void resetsAfterCompletedEstimate()
    {
        RtCov cov(*makeSyntheticInfo());

        MatrixXd first(2, 3);
        first << 1.0, 2.0, 3.0,
            3.0, 2.0, 1.0;
        QVERIFY(cov.estimate_covariance(first, 3));

        MatrixXd second(2, 1);
        second << 10.0,
            20.0;
        QVERIFY(!cov.estimate_covariance(second, 2));
    }

    void needsMeasurementInfo()
    {
        RtCov cov{FiffInfo()};
        QVERIFY(!cov.estimate_covariance(MatrixXd::Ones(2, 10), 3));
    }
};

QTEST_GUILESS_MAIN(TestDspRtCov)
#include "test_dsp_rt_cov.moc"
