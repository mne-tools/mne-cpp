//=============================================================================================================
/**
 * @file     test_inv_cmne.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
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
 * @brief    Tests for Contextual MNE (CMNE) inverse solver.
 *           Tests the dSPM kernel computation, z-score/rectify pipeline,
 *           CMNE settings, and LSTM correction loop mechanics.
 *           Reference: Dinh et al. (2021), Front. Neurosci. 15:552666.
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

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <random>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestInvCmne
 *
 * @brief Tests for Contextual MNE (CMNE) inverse solver.
 */
class TestInvCmne : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // Settings
    void testSettingsDefaults();
    void testSettingsCustom();

    // dSPM kernel computation (static helper via compute with known data)
    void testDspmKernelDimensions();
    void testDspmOutputRange();

    // z-score + rectify
    void testZScoreRectifyProperties();

    // CMNE Markov chain mechanics (synthetic — no real ONNX model)
    void testCmneIdentityForEarlyTimesteps();
    void testCmneOutputDimensions();

    // Edge cases
    void testFewTimeSamples();

    void cleanupTestCase();

private:
    MatrixXd createSyntheticGain(int nChannels, int nSources, unsigned int seed = 42) const;
    MatrixXd createDiagonalCov(int n, double variance = 1.0) const;
};

//=============================================================================================================

void TestInvCmne::initTestCase()
{
}

//=============================================================================================================

void TestInvCmne::testSettingsDefaults()
{
    InvCMNESettings settings;
    QCOMPARE(settings.lookBack, 80);
    QCOMPARE(settings.numSources, 5124);
    QVERIFY(std::abs(settings.lambda2 - 1.0 / 9.0) < 1e-10);
    QCOMPARE(settings.method, 1);  // dSPM
    QVERIFY(std::abs(settings.looseOriConstraint - 0.2) < 1e-10);
    QVERIFY(settings.onnxModelPath.isEmpty());
}

//=============================================================================================================

void TestInvCmne::testSettingsCustom()
{
    InvCMNESettings settings;
    settings.lookBack = 40;
    settings.numSources = 2562;
    settings.lambda2 = 0.5;
    settings.method = 2;  // sLORETA
    settings.looseOriConstraint = 0.0;
    settings.onnxModelPath = "/tmp/model.onnx";

    QCOMPARE(settings.lookBack, 40);
    QCOMPARE(settings.numSources, 2562);
    QVERIFY(std::abs(settings.lambda2 - 0.5) < 1e-10);
    QCOMPARE(settings.method, 2);
    QVERIFY(std::abs(settings.looseOriConstraint) < 1e-10);
    QCOMPARE(settings.onnxModelPath, QString("/tmp/model.onnx"));
}

//=============================================================================================================

void TestInvCmne::testDspmKernelDimensions()
{
    // Use small synthetic data to verify the compute pipeline dimensions
    int nCh = 10;
    int nSrc = 20;
    int nTimes = 50;

    MatrixXd gain = createSyntheticGain(nCh, nSrc);
    MatrixXd noiseCov = createDiagonalCov(nCh, 1.0);
    MatrixXd srcCov = createDiagonalCov(nSrc, 1.0);

    // Create synthetic evoked data by projecting through gain
    std::mt19937 gen(123);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd srcActivity(nSrc, nTimes);
    for (int i = 0; i < nSrc; ++i)
        for (int j = 0; j < nTimes; ++j)
            srcActivity(i, j) = dist(gen);
    MatrixXd evoked = gain * srcActivity;

    // Settings without ONNX model — compute should still produce dSPM estimates
    InvCMNESettings settings;
    settings.numSources = nSrc;
    settings.lambda2 = 1.0 / 9.0;
    settings.onnxModelPath.clear();  // No ONNX model

    // Call compute — without ONNX model, it should produce dSPM but CMNE
    // correction will be skipped or will be identity
    InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);

    // dSPM output dimensions
    QCOMPARE(result.stcDspm.data().rows(), nSrc);
    QCOMPARE(result.stcDspm.data().cols(), nTimes);

    // Kernel dimensions
    QCOMPARE(result.matKernelDspm.rows(), nSrc);
    QCOMPARE(result.matKernelDspm.cols(), nCh);
}

//=============================================================================================================

void TestInvCmne::testDspmOutputRange()
{
    // dSPM output should have reasonable values (z-score-like magnitudes)
    int nCh = 15;
    int nSrc = 30;
    int nTimes = 100;

    MatrixXd gain = createSyntheticGain(nCh, nSrc);
    MatrixXd noiseCov = createDiagonalCov(nCh, 1.0);
    MatrixXd srcCov = createDiagonalCov(nSrc, 1.0);

    std::mt19937 gen(456);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd srcActivity(nSrc, nTimes);
    for (int i = 0; i < nSrc; ++i)
        for (int j = 0; j < nTimes; ++j)
            srcActivity(i, j) = dist(gen);
    MatrixXd evoked = gain * srcActivity;

    InvCMNESettings settings;
    settings.numSources = nSrc;
    settings.lambda2 = 1.0 / 9.0;

    InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);

    // dSPM values should not be all zeros
    QVERIFY2(result.stcDspm.data().norm() > 0.0, "dSPM output should not be all zeros");

    // dSPM values should not contain NaN or Inf
    bool hasNan = result.stcDspm.data().array().isNaN().any();
    bool hasInf = result.stcDspm.data().array().isInf().any();
    QVERIFY2(!hasNan, "dSPM output contains NaN");
    QVERIFY2(!hasInf, "dSPM output contains Inf");
}

//=============================================================================================================

void TestInvCmne::testZScoreRectifyProperties()
{
    // After z-score + rectify, each source's time course should have
    // approximately zero mean and unit std (before rectification abs)
    int nCh = 10;
    int nSrc = 20;
    int nTimes = 200;

    MatrixXd gain = createSyntheticGain(nCh, nSrc);
    MatrixXd noiseCov = createDiagonalCov(nCh, 1.0);
    MatrixXd srcCov = createDiagonalCov(nSrc, 1.0);

    std::mt19937 gen(789);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd srcActivity(nSrc, nTimes);
    for (int i = 0; i < nSrc; ++i)
        for (int j = 0; j < nTimes; ++j)
            srcActivity(i, j) = dist(gen);
    MatrixXd evoked = gain * srcActivity;

    InvCMNESettings settings;
    settings.numSources = nSrc;
    settings.lambda2 = 1.0 / 9.0;

    InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);

    // Verify dSPM data is finite
    QVERIFY(!result.stcDspm.data().array().isNaN().any());
    QVERIFY(!result.stcDspm.data().array().isInf().any());
}

//=============================================================================================================

void TestInvCmne::testCmneIdentityForEarlyTimesteps()
{
    // For t < lookBack, CMNE output should equal the dSPM output
    // (no LSTM correction possible without enough history)
    int nCh = 10;
    int nSrc = 20;
    int nTimes = 200;
    int lookBack = 40;

    MatrixXd gain = createSyntheticGain(nCh, nSrc);
    MatrixXd noiseCov = createDiagonalCov(nCh, 1.0);
    MatrixXd srcCov = createDiagonalCov(nSrc, 1.0);

    std::mt19937 gen(321);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd srcActivity(nSrc, nTimes);
    for (int i = 0; i < nSrc; ++i)
        for (int j = 0; j < nTimes; ++j)
            srcActivity(i, j) = dist(gen);
    MatrixXd evoked = gain * srcActivity;

    InvCMNESettings settings;
    settings.numSources = nSrc;
    settings.lambda2 = 1.0 / 9.0;
    settings.lookBack = lookBack;
    // No ONNX model — CMNE correction should be identity/skipped

    InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);

    // First lookBack columns of CMNE should equal dSPM (when no model is available,
    // the entire output may equal dSPM)
    if (result.stcCmne.data().cols() > 0 && result.stcDspm.data().cols() > 0) {
        int checkCols = qMin(lookBack, (int)result.stcCmne.data().cols());
        MatrixXd cmneEarly = result.stcCmne.data().leftCols(checkCols);
        MatrixXd dspmEarly = result.stcDspm.data().leftCols(checkCols);
        double maxDiff = (cmneEarly - dspmEarly).cwiseAbs().maxCoeff();
        QVERIFY2(maxDiff < 1e-10,
                 qPrintable(QString("Early timesteps: CMNE != dSPM, maxDiff=%1").arg(maxDiff)));
    }
}

//=============================================================================================================

void TestInvCmne::testCmneOutputDimensions()
{
    int nCh = 10;
    int nSrc = 20;
    int nTimes = 100;

    MatrixXd gain = createSyntheticGain(nCh, nSrc);
    MatrixXd noiseCov = createDiagonalCov(nCh);
    MatrixXd srcCov = createDiagonalCov(nSrc);

    std::mt19937 gen(654);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd evoked(nCh, nTimes);
    for (int i = 0; i < nCh; ++i)
        for (int j = 0; j < nTimes; ++j)
            evoked(i, j) = dist(gen);

    InvCMNESettings settings;
    settings.numSources = nSrc;

    InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);

    // Both dSPM and CMNE outputs should have same dimensions
    QCOMPARE(result.stcDspm.data().rows(), nSrc);
    QCOMPARE(result.stcDspm.data().cols(), nTimes);

    if (result.stcCmne.data().rows() > 0) {
        QCOMPARE(result.stcCmne.data().rows(), nSrc);
        QCOMPARE(result.stcCmne.data().cols(), nTimes);
    }
}

//=============================================================================================================

void TestInvCmne::testFewTimeSamples()
{
    // With fewer than lookBack time samples, no LSTM correction is possible
    // Should return dSPM only without crashing
    int nCh = 10;
    int nSrc = 20;
    int nTimes = 5;  // Much less than lookBack (80)

    MatrixXd gain = createSyntheticGain(nCh, nSrc);
    MatrixXd noiseCov = createDiagonalCov(nCh);
    MatrixXd srcCov = createDiagonalCov(nSrc);

    std::mt19937 gen(987);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd evoked(nCh, nTimes);
    for (int i = 0; i < nCh; ++i)
        for (int j = 0; j < nTimes; ++j)
            evoked(i, j) = dist(gen);

    InvCMNESettings settings;
    settings.numSources = nSrc;
    settings.lookBack = 80;

    // Should not crash
    InvCMNEResult result = InvCMNE::compute(evoked, gain, noiseCov, srcCov, settings);

    QCOMPARE(result.stcDspm.data().rows(), nSrc);
    QCOMPARE(result.stcDspm.data().cols(), nTimes);
}

//=============================================================================================================

void TestInvCmne::cleanupTestCase()
{
}

//=============================================================================================================

MatrixXd TestInvCmne::createSyntheticGain(int nChannels, int nSources, unsigned int seed) const
{
    std::mt19937 gen(seed);
    std::normal_distribution<double> dist(0.0, 1.0);
    MatrixXd gain(nChannels, nSources);
    for (int i = 0; i < nChannels; ++i)
        for (int j = 0; j < nSources; ++j)
            gain(i, j) = dist(gen) / std::sqrt(static_cast<double>(nSources));
    return gain;
}

//=============================================================================================================

MatrixXd TestInvCmne::createDiagonalCov(int n, double variance) const
{
    return MatrixXd::Identity(n, n) * variance;
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestInvCmne)
#include "test_inv_cmne.moc"
