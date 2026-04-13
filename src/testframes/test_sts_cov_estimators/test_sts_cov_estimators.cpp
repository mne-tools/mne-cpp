//=============================================================================================================
/**
 * @file     test_sts_cov_estimators.cpp
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
 * @brief    Tests for Ledoit-Wolf shrinkage covariance estimator.
 *           Uses synthetic data with known covariance structure.
 *           Reference: Ledoit & Wolf (2004), "A well-conditioned estimator for
 *           large-dimensional covariance matrices".
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <sts/sts_cov_estimators.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Cholesky>
#include <Eigen/Eigenvalues>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace STSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestStsCovEstimators
 *
 * @brief Tests for Ledoit-Wolf shrinkage covariance estimator.
 */
class TestStsCovEstimators : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // Ledoit-Wolf tests
    void testLedoitWolfShrinkageRange();
    void testLedoitWolfPositiveDefinite();
    void testLedoitWolfFullRankData();
    void testLedoitWolfRankDeficient();
    void testLedoitWolfConditionNumber();
    void testLedoitWolfSymmetry();
    void testLedoitWolfDimensions();
    void testLedoitWolfIdentityLimit();

    void cleanupTestCase();

private:
    MatrixXd generateGaussianData(int nChannels, int nSamples, unsigned int seed = 42) const;
    MatrixXd generateRankDeficientData(int nChannels, int nSamples, int rank, unsigned int seed = 42) const;
};

//=============================================================================================================

void TestStsCovEstimators::initTestCase()
{
}

//=============================================================================================================

void TestStsCovEstimators::testLedoitWolfShrinkageRange()
{
    // Shrinkage coefficient alpha must be in [0, 1]
    MatrixXd data = generateGaussianData(20, 100);
    auto [cov, alpha] = StsCovEstimators::ledoitWolf(data);
    QVERIFY2(alpha >= 0.0 && alpha <= 1.0,
             qPrintable(QString("Shrinkage alpha=%1, expected [0,1]").arg(alpha)));
}

//=============================================================================================================

void TestStsCovEstimators::testLedoitWolfPositiveDefinite()
{
    // Result must be positive definite (all eigenvalues > 0)
    MatrixXd data = generateGaussianData(30, 200);
    auto [cov, alpha] = StsCovEstimators::ledoitWolf(data);

    SelfAdjointEigenSolver<MatrixXd> solver(cov);
    double minEig = solver.eigenvalues().minCoeff();
    QVERIFY2(minEig > 0.0,
             qPrintable(QString("Min eigenvalue=%1, expected > 0").arg(minEig)));
}

//=============================================================================================================

void TestStsCovEstimators::testLedoitWolfFullRankData()
{
    // With n_samples >> n_channels and well-structured Toeplitz covariance,
    // alpha should be small (sample covariance is already well-conditioned)
    int p = 10;
    int n = 5000;

    // Generate Toeplitz-correlated data: Sigma(i,j) = 0.8^|i-j|
    std::mt19937 gen(42);
    std::normal_distribution<> dist(0.0, 1.0);

    MatrixXd trueCov = MatrixXd::Identity(p, p);
    for (int i = 0; i < p; ++i)
        for (int j = 0; j < p; ++j)
            trueCov(i, j) = std::pow(0.8, std::abs(i - j));

    Eigen::LLT<MatrixXd> llt(trueCov);
    MatrixXd L = llt.matrixL();

    MatrixXd whitenoise(p, n);
    for (int i = 0; i < p; ++i)
        for (int j = 0; j < n; ++j)
            whitenoise(i, j) = dist(gen);

    MatrixXd data = L * whitenoise;
    data.colwise() -= data.rowwise().mean();

    auto [cov, alpha] = StsCovEstimators::ledoitWolf(data);

    QVERIFY2(alpha < 0.3,
             qPrintable(QString("For Toeplitz-structured data, alpha=%1 should be small").arg(alpha)));

    // Output covariance should closely match the true structure
    double maxDiff = (cov - trueCov).cwiseAbs().maxCoeff();
    QVERIFY2(maxDiff < 0.5,
             qPrintable(QString("Estimated cov should match Toeplitz, maxDiff=%1").arg(maxDiff)));
}

//=============================================================================================================

void TestStsCovEstimators::testLedoitWolfRankDeficient()
{
    // With rank < n_channels, alpha should be closer to 1
    // (heavy shrinkage needed to regularize)
    int nCh = 64;
    int nSamples = 40;  // fewer samples than channels → severely rank-deficient
    MatrixXd data = generateRankDeficientData(nCh, nSamples, 30);
    auto [cov, alpha] = StsCovEstimators::ledoitWolf(data);

    // Verify result is still positive definite
    SelfAdjointEigenSolver<MatrixXd> solver(cov);
    double minEig = solver.eigenvalues().minCoeff();
    QVERIFY2(minEig > 0.0,
             qPrintable(QString("Rank-deficient: min eigenvalue=%1, expected > 0").arg(minEig)));

    // Alpha should be substantial for rank-deficient data
    QVERIFY2(alpha > 0.1,
             qPrintable(QString("Rank-deficient data should have alpha > 0.1, got %1").arg(alpha)));
}

//=============================================================================================================

void TestStsCovEstimators::testLedoitWolfConditionNumber()
{
    // Ledoit-Wolf should produce a well-conditioned covariance matrix
    // even for data with very high sample covariance condition number
    int nCh = 64;
    int nSamples = 50;
    MatrixXd data = generateRankDeficientData(nCh, nSamples, 30);
    auto [cov, alpha] = StsCovEstimators::ledoitWolf(data);

    SelfAdjointEigenSolver<MatrixXd> solver(cov);
    double maxEig = solver.eigenvalues().maxCoeff();
    double minEig = solver.eigenvalues().minCoeff();
    double condNum = maxEig / minEig;

    QVERIFY2(condNum < 1e6,
             qPrintable(QString("Condition number=%1, expected < 1e6").arg(condNum)));
}

//=============================================================================================================

void TestStsCovEstimators::testLedoitWolfSymmetry()
{
    // Output covariance must be symmetric
    MatrixXd data = generateGaussianData(20, 100);
    auto [cov, alpha] = StsCovEstimators::ledoitWolf(data);

    double asymmetry = (cov - cov.transpose()).norm();
    QVERIFY2(asymmetry < 1e-12,
             qPrintable(QString("Asymmetry norm=%1, expected ~0").arg(asymmetry)));
}

//=============================================================================================================

void TestStsCovEstimators::testLedoitWolfDimensions()
{
    // Output dimensions must match n_channels × n_channels
    int nCh = 25;
    MatrixXd data = generateGaussianData(nCh, 200);
    auto [cov, alpha] = StsCovEstimators::ledoitWolf(data);

    QCOMPARE(cov.rows(), nCh);
    QCOMPARE(cov.cols(), nCh);
}

//=============================================================================================================

void TestStsCovEstimators::testLedoitWolfIdentityLimit()
{
    // For large n_samples with i.i.d. standard normal data, the shrinkage
    // target is trace(S)/p * I ≈ I, and alpha ≈ 0, so result ≈ sample cov ≈ I
    MatrixXd data = generateGaussianData(10, 10000);
    auto [cov, alpha] = StsCovEstimators::ledoitWolf(data);

    // Diagonal should be close to 1.0
    for (int i = 0; i < 10; ++i) {
        QVERIFY2(std::abs(cov(i, i) - 1.0) < 0.1,
                 qPrintable(QString("Diagonal(%1)=%2, expected ~1.0").arg(i).arg(cov(i, i))));
    }
}

//=============================================================================================================

void TestStsCovEstimators::cleanupTestCase()
{
}

//=============================================================================================================

MatrixXd TestStsCovEstimators::generateGaussianData(int nChannels, int nSamples, unsigned int seed) const
{
    std::mt19937 gen(seed);
    std::normal_distribution<double> dist(0.0, 1.0);

    MatrixXd data(nChannels, nSamples);
    for (int i = 0; i < nChannels; ++i)
        for (int j = 0; j < nSamples; ++j)
            data(i, j) = dist(gen);

    // Zero-mean
    data.colwise() -= data.rowwise().mean();
    return data;
}

//=============================================================================================================

MatrixXd TestStsCovEstimators::generateRankDeficientData(int nChannels, int nSamples, int rank, unsigned int seed) const
{
    std::mt19937 gen(seed);
    std::normal_distribution<double> dist(0.0, 1.0);

    // Generate low-rank data: nChannels × rank mixing matrix × rank × nSamples sources
    MatrixXd mixing(nChannels, rank);
    MatrixXd sources(rank, nSamples);

    for (int i = 0; i < nChannels; ++i)
        for (int j = 0; j < rank; ++j)
            mixing(i, j) = dist(gen);

    for (int i = 0; i < rank; ++i)
        for (int j = 0; j < nSamples; ++j)
            sources(i, j) = dist(gen);

    MatrixXd data = mixing * sources;
    data.colwise() -= data.rowwise().mean();
    return data;
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestStsCovEstimators)
#include "test_sts_cov_estimators.moc"
