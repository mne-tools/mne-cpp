//=============================================================================================================
/**
 * @file     test_rtcmne_cmne.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     April, 2026
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
 * @brief    Tests for the CMNE (Contextual MNE) routing inside the rtcmne real-time
 *           inverse plugin. The plugin itself can only be exercised in a full MNE Scan
 *           runtime, so this test focuses on the algorithmic dispatch: it drives the
 *           InvCMNE solver through the same compute() entry-point used by the plugin
 *           and validates the result shape. When no model checkpoint is available the
 *           test is SKIPPED via QSKIP rather than failing — CMNE requires a trained
 *           ONNX model that is not shipped with the repository.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/minimum_norm/inv_cmne.h>
#include <inv/minimum_norm/inv_cmne_settings.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QString>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>
#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestRtcMneCmne
 *
 * @brief Tests for the CMNE routing in the rtcmne real-time inverse plugin.
 */
class TestRtcMneCmne : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testSettingsRoundTrip();
    void testCmneInverseSmoke();

    void cleanupTestCase();

private:
    QString resolveModelCheckpoint() const;

    MatrixXd buildSyntheticGain(int nChannels, int nSources, unsigned int seed = 1) const;
    MatrixXd buildDiagonalCov(int n, double variance = 1.0) const;
};

//=============================================================================================================
// IMPLEMENTATION
//=============================================================================================================

void TestRtcMneCmne::initTestCase()
{
}

//=============================================================================================================

void TestRtcMneCmne::cleanupTestCase()
{
}

//=============================================================================================================

QString TestRtcMneCmne::resolveModelCheckpoint() const
{
    const QString sFromEnv = QProcessEnvironment::systemEnvironment().value(QStringLiteral("CMNE_MODEL_CHECKPOINT"));
    if(!sFromEnv.isEmpty() && QFileInfo::exists(sFromEnv)) {
        return sFromEnv;
    }
    return QString();
}

//=============================================================================================================

MatrixXd TestRtcMneCmne::buildSyntheticGain(int nChannels, int nSources, unsigned int seed) const
{
    std::mt19937 gen(seed);
    std::normal_distribution<double> nd(0.0, 1.0);
    MatrixXd m(nChannels, nSources);
    for(int r = 0; r < nChannels; ++r) {
        for(int c = 0; c < nSources; ++c) {
            m(r, c) = nd(gen);
        }
    }
    return m;
}

//=============================================================================================================

MatrixXd TestRtcMneCmne::buildDiagonalCov(int n, double variance) const
{
    return MatrixXd::Identity(n, n) * variance;
}

//=============================================================================================================

void TestRtcMneCmne::testSettingsRoundTrip()
{
    InvCMNESettings settings;
    settings.onnxModelPath     = QStringLiteral("/tmp/imaginary_checkpoint.onnx");
    settings.lambda2           = 1.0 / 9.0;
    settings.numSources        = 16;
    settings.lookBack          = 4;
    settings.method            = 1;
    settings.looseOriConstraint = 0.2;

    QCOMPARE(settings.onnxModelPath, QStringLiteral("/tmp/imaginary_checkpoint.onnx"));
    QCOMPARE(settings.numSources, 16);
    QCOMPARE(settings.lookBack, 4);
    QCOMPARE(settings.method, 1);
    QVERIFY(std::abs(settings.lambda2 - 1.0 / 9.0) < 1e-12);
    QVERIFY(std::abs(settings.looseOriConstraint - 0.2) < 1e-12);
}

//=============================================================================================================

void TestRtcMneCmne::testCmneInverseSmoke()
{
    const QString sCheckpoint = resolveModelCheckpoint();
    if(sCheckpoint.isEmpty()) {
        QSKIP("No CMNE model checkpoint available (set CMNE_MODEL_CHECKPOINT to a valid .onnx file).");
    }

    const int nChannels = 32;
    const int nSources  = 16;
    const int nTimes    = 16;

    const MatrixXd matGain     = buildSyntheticGain(nChannels, nSources);
    const MatrixXd matNoiseCov = buildDiagonalCov(nChannels);
    const MatrixXd matSrcCov   = buildDiagonalCov(nSources);
    const MatrixXd matEvoked   = MatrixXd::Random(nChannels, nTimes);

    InvCMNESettings settings;
    settings.onnxModelPath = sCheckpoint;
    settings.numSources    = nSources;
    settings.lookBack      = 4;
    settings.lambda2       = 1.0 / 9.0;

    const InvCMNEResult res = InvCMNE::compute(matEvoked, matGain, matNoiseCov, matSrcCov, settings);

    QVERIFY(res.matKernelDspm.rows() == nSources);
    QVERIFY(res.matKernelDspm.cols() == nChannels);
    QVERIFY(res.stcDspm.data.rows() == nSources || res.stcDspm.data.rows() == 0);
    QVERIFY(res.stcCmne.data.rows() == nSources || res.stcCmne.data.rows() == 0);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestRtcMneCmne)
#include "test_rtcmne_cmne.moc"
