//=============================================================================================================
/**
 * @file     test_dsp_rt_cov.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.10
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
 * @brief    Unit tests for RtCov.
 */

#include <QtTest/QtTest>

#include <dsp/rt/rt_cov.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace Eigen;

namespace {

QSharedPointer<FiffInfo> makeSyntheticInfo()
{
    QSharedPointer<FiffInfo> info(new FiffInfo);

    for(int i = 0; i < 2; ++i) {
        FiffChInfo ch;
        ch.kind = FIFFV_EEG_CH;
        ch.unit = FIFF_UNIT_V;
        ch.ch_name = QString("EEG%1").arg(i + 1, 3, 10, QChar('0'));
        ch.range = 1.0f;
        ch.cal = 1.0f;
        ch.chpos.coil_type = FIFFV_COIL_EEG;
        ch.chpos.r0 << 0.01f * (i + 1), 0.0f, 0.02f;
        info->chs.append(ch);
        info->ch_names.append(ch.ch_name);
    }

    info->nchan = info->chs.size();
    return info;
}

}

class TestDspRtCov : public QObject
{
    Q_OBJECT

private slots:
    void returnsEmptyUntilEnoughSamples()
    {
        RtCov cov(makeSyntheticInfo());

        MatrixXd block(2, 2);
        block << 1.0, 2.0,
                 2.0, 4.0;

        FiffCov result = cov.estimateCovariance(block, 3);
        QVERIFY(result.isEmpty());
    }

    void computesExpectedRegularizedCovariance()
    {
        RtCov cov(makeSyntheticInfo());

        MatrixXd block(2, 3);
        block << 1.0, 2.0, 3.0,
                 2.0, 4.0, 6.0;

        FiffCov result = cov.estimateCovariance(block, 3);
        QVERIFY(!result.isEmpty());

        QCOMPARE(result.dim, 2);
        QCOMPARE(result.nfree, 3);
        QCOMPARE(result.names.size(), 2);
        QCOMPARE(result.names.at(0), QString("EEG001"));
        QCOMPARE(result.names.at(1), QString("EEG002"));

        // Raw sample covariance is [[1, 2], [2, 4]]. EEG regularization adds 10% of mean(diag)=0.25 to the diagonal.
        QVERIFY(qAbs(result.data(0, 0) - 1.25) < 1e-9);
        QVERIFY(qAbs(result.data(0, 1) - 2.00) < 1e-9);
        QVERIFY(qAbs(result.data(1, 0) - 2.00) < 1e-9);
        QVERIFY(qAbs(result.data(1, 1) - 4.25) < 1e-9);
    }

    void resetsAfterCompletedEstimate()
    {
        RtCov cov(makeSyntheticInfo());

        MatrixXd first(2, 3);
        first << 1.0, 2.0, 3.0,
                 3.0, 2.0, 1.0;
        QVERIFY(!cov.estimateCovariance(first, 3).isEmpty());

        MatrixXd second(2, 1);
        second << 10.0,
                  20.0;
        QVERIFY(cov.estimateCovariance(second, 2).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestDspRtCov)
#include "test_dsp_rt_cov.moc"
