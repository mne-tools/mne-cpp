//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_rtcmne_cmne.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     April, 2026
 * @brief    Tests for the CMNE (Contextual MNE) routing inside the rtcmne real-time
 *           inverse plugin. The plugin itself can only be exercised in a full MNE Scan
 *           runtime, so this test focuses on the algorithmic dispatch: it drives the
 *           InvCMNE solver through the same compute() entry-point used by the plugin
 *           and validates the result shape. Without CMNE_MODEL_CHECKPOINT it runs the
 *           untrained cmne_smoke.onnx (see make_cmne_smoke_model.py).
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
    const QString sShipped = QStringLiteral(MNE_CMNE_SMOKE_MODEL);
    return QFileInfo::exists(sShipped) ? sShipped : QString();
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

#ifdef MNE_USE_ONNXRUNTIME
    // A model that fails to load falls back to the moving average silently, so prove inference ran.
    const MatrixXd matSources = MatrixXd::Random(nSources, nTimes).cwiseAbs();
    const MatrixXd matLstm = InvCMNE::applyLstmCorrection(matSources, sCheckpoint, settings.lookBack);
    const MatrixXd matMovingAverage = InvCMNE::applyLstmCorrection(matSources, QString(), settings.lookBack);
    QVERIFY(!matLstm.rightCols(nTimes - settings.lookBack).isApprox(matMovingAverage.rightCols(nTimes - settings.lookBack)));
#endif
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestRtcMneCmne)
#include "test_rtcmne_cmne.moc"
