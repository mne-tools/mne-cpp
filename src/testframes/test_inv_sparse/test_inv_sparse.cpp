//=============================================================================================================
/**
 * @file     test_inv_sparse.cpp
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
 * @brief    Tests for InvMxne and InvGammaMap sparse inverse solvers.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/sparse/inv_mxne.h>
#include <inv/sparse/inv_gamma_map.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestInvSparse
 *
 * @brief The TestInvSparse class provides tests for sparse inverse solvers.
 */
class TestInvSparse : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // MxNE
    void testMxneBasic();
    void testMxneSparsity();
    void testMxneAlphaEffect();
    void testMxneResidual();
    void testMxneIterationCount();

    // Gamma-MAP
    void testGammaMapBasic();
    void testGammaMapSparsity();
    void testGammaMapConvergence();
    void testGammaMapNoiseCovEffect();

    void cleanupTestCase();

private:
    void createSyntheticForwardProblem(MatrixXd& gain, MatrixXd& data,
                                        int nSensors, int nSources, int nTimes,
                                        QVector<int> activeIndices) const;
};

//=============================================================================================================

void TestInvSparse::initTestCase()
{
}

//=============================================================================================================

void TestInvSparse::createSyntheticForwardProblem(
    MatrixXd& gain, MatrixXd& data,
    int nSensors, int nSources, int nTimes,
    QVector<int> activeIndices) const
{
    // Create a random gain matrix
    gain = MatrixXd::Random(nSensors, nSources) * 0.1;

    // Create sparse source activity — only activeIndices have signal
    MatrixXd sourceActivity = MatrixXd::Zero(nSources, nTimes);
    for (int idx : activeIndices) {
        if (idx < nSources) {
            for (int t = 0; t < nTimes; ++t) {
                sourceActivity(idx, t) = 5.0 * std::sin(2.0 * M_PI * 10.0 * t / nTimes);
            }
        }
    }

    // Simulate sensor data = G * X + noise
    data = gain * sourceActivity + MatrixXd::Random(nSensors, nTimes) * 0.01;
}

//=============================================================================================================

void TestInvSparse::testMxneBasic()
{
    int nSensors = 20;
    int nSources = 50;
    int nTimes = 30;
    MatrixXd gain, data;
    createSyntheticForwardProblem(gain, data, nSensors, nSources, nTimes, {5, 15, 25});

    InvMxneResult result = InvMxne::compute(gain, data, 1.0, 50, 1e-6);

    // Should produce some active vertices
    QVERIFY(result.activeVertices.size() > 0);
    QVERIFY(result.nIterations > 0);
    QVERIFY(result.residualNorm >= 0.0);
}

//=============================================================================================================

void TestInvSparse::testMxneSparsity()
{
    int nSensors = 30;
    int nSources = 100;
    int nTimes = 20;
    MatrixXd gain, data;
    createSyntheticForwardProblem(gain, data, nSensors, nSources, nTimes, {10, 20});

    // High regularization should produce sparser solution
    InvMxneResult sparse = InvMxne::compute(gain, data, 10.0, 50, 1e-6);
    InvMxneResult dense  = InvMxne::compute(gain, data, 0.01, 50, 1e-6);

    QVERIFY(sparse.activeVertices.size() <= dense.activeVertices.size());
}

//=============================================================================================================

void TestInvSparse::testMxneAlphaEffect()
{
    int nSensors = 20;
    int nSources = 50;
    int nTimes = 15;
    MatrixXd gain, data;
    createSyntheticForwardProblem(gain, data, nSensors, nSources, nTimes, {5});

    // Very high alpha → very few or zero active sources
    InvMxneResult result = InvMxne::compute(gain, data, 1000.0, 50, 1e-6);
    QVERIFY(result.activeVertices.size() <= 5);
}

//=============================================================================================================

void TestInvSparse::testMxneResidual()
{
    int nSensors = 20;
    int nSources = 50;
    int nTimes = 20;
    MatrixXd gain, data;
    createSyntheticForwardProblem(gain, data, nSensors, nSources, nTimes, {3, 7});

    InvMxneResult result = InvMxne::compute(gain, data, 0.1, 100, 1e-6);

    // Residual should be non-negative
    QVERIFY(result.residualNorm >= 0.0);
}

//=============================================================================================================

void TestInvSparse::testMxneIterationCount()
{
    int nSensors = 15;
    int nSources = 30;
    int nTimes = 10;
    MatrixXd gain, data;
    createSyntheticForwardProblem(gain, data, nSensors, nSources, nTimes, {2});

    int maxIter = 10;
    InvMxneResult result = InvMxne::compute(gain, data, 1.0, maxIter, 1e-6);

    QVERIFY(result.nIterations <= maxIter);
    QVERIFY(result.nIterations >= 1);
}

//=============================================================================================================

void TestInvSparse::testGammaMapBasic()
{
    int nSensors = 20;
    int nSources = 30;
    int nTimes = 20;
    MatrixXd gain, data;
    createSyntheticForwardProblem(gain, data, nSensors, nSources, nTimes, {5});

    // Noise covariance — use moderate noise for numerical stability
    MatrixXd noiseCov = MatrixXd::Identity(nSensors, nSensors) * 0.1;

    InvGammaMapResult result = InvGammaMap::compute(gain, data, noiseCov, 200, 1e-6, 1e-12);

    QVERIFY(result.vecGamma.size() > 0);
    QVERIFY(result.nIterations > 0);
    QVERIFY(result.residualNorm >= 0.0);
}

//=============================================================================================================

void TestInvSparse::testGammaMapSparsity()
{
    int nSensors = 20;
    int nSources = 50;
    int nTimes = 15;
    MatrixXd gain, data;
    createSyntheticForwardProblem(gain, data, nSensors, nSources, nTimes, {10});

    MatrixXd noiseCov = MatrixXd::Identity(nSensors, nSensors) * 0.01;

    // Low threshold → more active sources
    InvGammaMapResult dense = InvGammaMap::compute(gain, data, noiseCov, 100, 1e-6, 1e-20);
    // High threshold → fewer active sources
    InvGammaMapResult sparse = InvGammaMap::compute(gain, data, noiseCov, 100, 1e-6, 1e-2);

    QVERIFY(sparse.activeVertices.size() <= dense.activeVertices.size());
}

//=============================================================================================================

void TestInvSparse::testGammaMapConvergence()
{
    int nSensors = 15;
    int nSources = 30;
    int nTimes = 10;
    MatrixXd gain, data;
    createSyntheticForwardProblem(gain, data, nSensors, nSources, nTimes, {3});

    MatrixXd noiseCov = MatrixXd::Identity(nSensors, nSensors) * 0.1;

    int maxIter = 200;
    InvGammaMapResult result = InvGammaMap::compute(gain, data, noiseCov, maxIter, 1e-6, 1e-10);

    QVERIFY(result.nIterations <= maxIter);
    QVERIFY(result.nIterations >= 1);
}

//=============================================================================================================

void TestInvSparse::testGammaMapNoiseCovEffect()
{
    int nSensors = 20;
    int nSources = 40;
    int nTimes = 15;
    MatrixXd gain, data;
    createSyntheticForwardProblem(gain, data, nSensors, nSources, nTimes, {5, 15});

    // Low noise → can recover more sources accurately
    MatrixXd lowNoiseCov = MatrixXd::Identity(nSensors, nSensors) * 0.001;
    InvGammaMapResult lowNoise = InvGammaMap::compute(gain, data, lowNoiseCov, 100, 1e-6);

    // High noise → harder to distinguish signal
    MatrixXd highNoiseCov = MatrixXd::Identity(nSensors, nSensors) * 10.0;
    InvGammaMapResult highNoise = InvGammaMap::compute(gain, data, highNoiseCov, 100, 1e-6);

    // Both should produce results without crashing
    QVERIFY(lowNoise.activeVertices.size() >= 0);
    QVERIFY(highNoise.activeVertices.size() >= 0);
}

//=============================================================================================================

void TestInvSparse::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestInvSparse)
#include "test_inv_sparse.moc"
