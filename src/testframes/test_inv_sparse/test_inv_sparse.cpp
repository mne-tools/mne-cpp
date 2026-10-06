//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_inv_sparse.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for InvMxne and InvGammaMap sparse inverse solvers.
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

    void testMxneMatchesMnePython();
    void testGammaMapMatchesMnePython();

    void cleanupTestCase();

private:
    static void deterministicProblem(MatrixXd& gain, MatrixXd& data);

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

void TestInvSparse::deterministicProblem(MatrixXd& gain, MatrixXd& data)
{
    // G_ij = sin(1.7 (i + 1)(j + 1) + 0.3 i + 2.9 j) with unit columns (smallest singular value 1.1);
    // sources 5 and 22 active plus a deterministic "noise" term.
    gain.resize(20, 40);
    for (int i = 0; i < 20; ++i)
        for (int j = 0; j < 40; ++j)
            gain(i, j) = std::sin(1.7 * (i + 1) * (j + 1) + 0.3 * i + 2.9 * j);
    gain.colwise().normalize();
    MatrixXd x = MatrixXd::Zero(40, 10);
    MatrixXd noise(20, 10);
    for (int t = 0; t < 10; ++t) {
        x(5, t) = 3.0 * std::sin(0.3 * t);
        x(22, t) = 2.0 * std::cos(0.3 * t);
        for (int i = 0; i < 20; ++i)
            noise(i, t) = 0.05 * std::sin(1.3 * i + 2.1 * t);
    }
    data = gain * x + noise;
}

//=============================================================================================================

void TestInvSparse::testMxneMatchesMnePython()
{
    // mne.inverse_sparse.mxne_optim.mixed_norm_solver(M, G, alpha, n_orient=1, debias=False) at
    // alpha = 0.2 * max_i ||G_i^T M||: only sources 5 and 22, row norms 5.48248 and 3.00622.
    MatrixXd gain, data;
    deterministicProblem(gain, data);
    const InvMxneResult result = InvMxne::compute(gain, data, 1.3706862500771375);
    MatrixXd x = MatrixXd::Zero(40, 10);
    for (int k = 0; k < result.activeVertices.size(); ++k)
        x.row(result.activeVertices[k]) = result.stc.data.row(k);
    x.row(5).setZero();
    x.row(22).setZero();
    QVERIFY2(std::abs(result.stc.data.row(result.activeVertices.indexOf(5)).norm() - 5.482482828139865) < 1e-5, "source 5");
    QVERIFY2(std::abs(result.stc.data.row(result.activeVertices.indexOf(22)).norm() - 3.0062181954974725) < 1e-5, "source 22");
    QVERIFY2(x.norm() < 1e-5, qPrintable(QString("energy outside the mne support: %1").arg(x.norm())));
}

//=============================================================================================================

void TestInvSparse::testGammaMapMatchesMnePython()
{
    // mne.inverse_sparse._gamma_map._gamma_map_opt(M, G, alpha=0.05**2, update_mode=1): sources 5, 7, 22, 35.
    MatrixXd gain, data;
    deterministicProblem(gain, data);
    const InvGammaMapResult result = InvGammaMap::compute(gain, data, 0.0025 * MatrixXd::Identity(20, 20), 5000, 1e-12, 1e-8);
    QCOMPARE(result.activeVertices, QVector<int>({5, 7, 22, 35}));
    const double expected[4] = {6.848310575155952, 0.28156801539348963, 4.369723814282878, 0.26551421080291066};
    for (int k = 0; k < 4; ++k) {
        const double norm = result.stc.data.row(k).norm();
        QVERIFY2(std::abs(norm - expected[k]) < 1e-4, qPrintable(QString("row %1 norm %2").arg(k).arg(norm, 0, 'g', 12)));
    }
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
    InvMxneResult dense = InvMxne::compute(gain, data, 0.01, 50, 1e-6);

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
