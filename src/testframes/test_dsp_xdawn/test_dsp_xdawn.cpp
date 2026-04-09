//=============================================================================================================
/**
 * @file     test_dsp_xdawn.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Unit tests for xDAWN spatial filtering.
 */

#include <QtTest/QtTest>

#include <dsp/xdawn.h>

#include <Eigen/Dense>

#include <random>

using namespace UTILSLIB;
using namespace MNELIB;
using namespace Eigen;

namespace {

struct SyntheticXdawnData
{
    QVector<MNEEpochData> epochs;
    MatrixXd              targetTemplate;
};

SyntheticXdawnData makeSyntheticEpochs()
{
    const int nEpochs = 40;
    const int nCh = 6;
    const int nSamp = 160;
    const int targetEvent = 1;

    std::mt19937 rng(42);
    std::normal_distribution<double> noiseDist(0.0, 0.8);

    VectorXd spatialPattern(nCh);
    spatialPattern << 2.2, -1.3, 0.8, 0.0, 0.4, -0.2;

    RowVectorXd erp = RowVectorXd::Zero(nSamp);
    for (int t = 0; t < nSamp; ++t) {
        const double x = (static_cast<double>(t) - 70.0) / 10.0;
        erp(t) = std::exp(-0.5 * x * x);
    }

    SyntheticXdawnData out;
    out.targetTemplate = spatialPattern * erp;
    out.epochs.reserve(nEpochs);

    for (int i = 0; i < nEpochs; ++i) {
        MNEEpochData ep;
        ep.event = (i < nEpochs / 2) ? targetEvent : 2;
        ep.tmin = -0.1f;
        ep.tmax = 0.5f;
        ep.bReject = false;
        ep.epoch = MatrixXd::Zero(nCh, nSamp);

        for (int ch = 0; ch < nCh; ++ch) {
            for (int t = 0; t < nSamp; ++t) {
                ep.epoch(ch, t) = noiseDist(rng);
            }
        }

        if (ep.event == targetEvent) {
            ep.epoch += out.targetTemplate;
        }

        out.epochs.append(ep);
    }

    return out;
}

double correlation(const RowVectorXd& a, const RowVectorXd& b)
{
    RowVectorXd ac = a.array() - a.mean();
    RowVectorXd bc = b.array() - b.mean();
    const double denom = ac.norm() * bc.norm();
    if (denom < 1e-12) {
        return 0.0;
    }
    return ac.dot(bc) / denom;
}

double rayleighQuotient(const VectorXd& w, const MatrixXd& signalCov, const MatrixXd& noiseCov)
{
    const double denom = (w.transpose() * noiseCov * w)(0, 0);
    if (std::abs(denom) < 1e-12) {
        return 0.0;
    }

    return (w.transpose() * signalCov * w)(0, 0) / denom;
}

} // anonymous namespace

class TestDspXdawn : public QObject
{
    Q_OBJECT

private slots:
    void fit_resultDimensions()
    {
        SyntheticXdawnData synth = makeSyntheticEpochs();
        XdawnResult res = Xdawn::fit(synth.epochs, 1, 3);

        QVERIFY(res.bValid);
        QCOMPARE(res.matFilters.rows(), 6);
        QCOMPARE(res.matFilters.cols(), 3);
        QCOMPARE(res.matPatterns.rows(), 6);
        QCOMPARE(res.matPatterns.cols(), 3);
        QCOMPARE(res.matTargetEvoked.rows(), 6);
        QCOMPARE(res.matTargetEvoked.cols(), 160);
    }

    void apply_componentDimensions()
    {
        SyntheticXdawnData synth = makeSyntheticEpochs();
        XdawnResult res = Xdawn::fit(synth.epochs, 1, 4);
        MatrixXd comp = Xdawn::apply(synth.epochs.first().epoch, res);

        QCOMPARE(comp.rows(), 4);
        QCOMPARE(comp.cols(), 160);
    }

    void firstComponent_maximizesGeneralizedSnr()
    {
        SyntheticXdawnData synth = makeSyntheticEpochs();
        XdawnResult res = Xdawn::fit(synth.epochs, 1, 4);
        QVERIFY(res.bValid);

        const VectorXd firstFilter = res.matFilters.col(0);
        const VectorXd secondFilter = res.matFilters.col(1);

        const double firstScore = rayleighQuotient(firstFilter, res.matSignalCov, res.matNoiseCov);
        const double secondScore = rayleighQuotient(secondFilter, res.matSignalCov, res.matNoiseCov);

        double meanRawScore = 0.0;
        for (int ch = 0; ch < res.matFilters.rows(); ++ch) {
            VectorXd sensorAxis = VectorXd::Zero(res.matFilters.rows());
            sensorAxis(ch) = 1.0;
            meanRawScore += rayleighQuotient(sensorAxis, res.matSignalCov, res.matNoiseCov);
        }
        meanRawScore /= static_cast<double>(res.matFilters.rows());

        QVERIFY2(firstScore >= secondScore,
                 qPrintable(QString("First xDAWN filter score (%1) should be >= second filter score (%2).")
                            .arg(firstScore)
                            .arg(secondScore)));
        QVERIFY2(firstScore > meanRawScore,
                 qPrintable(QString("First xDAWN filter score (%1) did not exceed mean raw-channel score (%2).")
                            .arg(firstScore)
                            .arg(meanRawScore)));
    }

    void fit_missingTarget_returnsInvalid()
    {
        SyntheticXdawnData synth = makeSyntheticEpochs();
        XdawnResult res = Xdawn::fit(synth.epochs, 99, 2);

        QVERIFY(!res.bValid);
        QVERIFY(res.matFilters.size() == 0);
    }
};

QTEST_MAIN(TestDspXdawn)
#include "test_dsp_xdawn.moc"
