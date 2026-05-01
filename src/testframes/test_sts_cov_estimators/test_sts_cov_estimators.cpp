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

    // OAS tests
    void testOasShrinkageRange();
    void testOasPositiveDefinite();
    void testOasSymmetry();

    // Diagonal fixed tests
    void testDiagFixedPositiveDefinite();
    void testDiagFixedIncreasedDiagonal();

    // PCA tests
    void testPcaDimensions();
    void testPcaAutoRank();
    void testPcaExplicitRank();
    void testPcaSymmetry();

    // Factor Analysis tests
    void testFactorAnalysisDimensions();
    void testFactorAnalysisSymmetry();
    void testFactorAnalysisPositiveDefinite();

    // Auto-select tests
    void testAutoSelectReturnsValid();
    void testAutoSelectMethodIndex();

    // Log-likelihood tests
    void testGaussianLogLikelihoodFinite();

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
// OAS tests
//=============================================================================================================

void TestStsCovEstimators::testOasShrinkageRange()
{
    MatrixXd data = generateGaussianData(20, 100);
    auto [cov, rho] = StsCovEstimators::oas(data);
    QVERIFY2(rho >= 0.0 && rho <= 1.0,
             qPrintable(QString("OAS rho=%1, expected [0,1]").arg(rho)));
}

//=============================================================================================================

void TestStsCovEstimators::testOasPositiveDefinite()
{
    MatrixXd data = generateGaussianData(30, 200);
    auto [cov, rho] = StsCovEstimators::oas(data);

    SelfAdjointEigenSolver<MatrixXd> solver(cov);
    double minEig = solver.eigenvalues().minCoeff();
    QVERIFY2(minEig > 0.0,
             qPrintable(QString("OAS min eigenvalue=%1, expected > 0").arg(minEig)));
}

//=============================================================================================================

void TestStsCovEstimators::testOasSymmetry()
{
    MatrixXd data = generateGaussianData(20, 100);
    auto [cov, rho] = StsCovEstimators::oas(data);
    double asymmetry = (cov - cov.transpose()).norm();
    QVERIFY2(asymmetry < 1e-12,
             qPrintable(QString("OAS asymmetry=%1").arg(asymmetry)));
}

//=============================================================================================================
// Diagonal fixed tests
//=============================================================================================================

void TestStsCovEstimators::testDiagFixedPositiveDefinite()
{
    MatrixXd data = generateRankDeficientData(64, 40, 30);
    auto [cov, reg] = StsCovEstimators::diagonalFixed(data, 0.1);

    SelfAdjointEigenSolver<MatrixXd> solver(cov);
    double minEig = solver.eigenvalues().minCoeff();
    QVERIFY2(minEig > 0.0,
             qPrintable(QString("DiagFixed min eigenvalue=%1").arg(minEig)));
}

//=============================================================================================================

void TestStsCovEstimators::testDiagFixedIncreasedDiagonal()
{
    MatrixXd data = generateGaussianData(10, 200);
    auto [covBase, reg0] = StsCovEstimators::diagonalFixed(data, 0.0);
    auto [covReg, reg1] = StsCovEstimators::diagonalFixed(data, 0.2);

    // Diagonal elements should be larger with regularisation
    for (int i = 0; i < 10; ++i) {
        QVERIFY2(covReg(i, i) >= covBase(i, i),
                 qPrintable(QString("DiagFixed: reg diagonal(%1)=%2 < base=%3")
                            .arg(i).arg(covReg(i, i)).arg(covBase(i, i))));
    }
}

//=============================================================================================================
// PCA tests
//=============================================================================================================

void TestStsCovEstimators::testPcaDimensions()
{
    int nCh = 20;
    MatrixXd data = generateGaussianData(nCh, 200);
    auto [cov, rank] = StsCovEstimators::pca(data);
    QCOMPARE(cov.rows(), nCh);
    QCOMPARE(cov.cols(), nCh);
}

//=============================================================================================================

void TestStsCovEstimators::testPcaAutoRank()
{
    // Rank-deficient data → PCA auto-rank should detect the true rank
    int nCh = 30;
    int trueRank = 10;
    MatrixXd data = generateRankDeficientData(nCh, 200, trueRank);
    auto [cov, detectedRank] = StsCovEstimators::pca(data);

    QVERIFY2(static_cast<int>(detectedRank) <= trueRank + 2,
             qPrintable(QString("PCA auto rank=%1, true rank=%2")
                        .arg(detectedRank).arg(trueRank)));
}

//=============================================================================================================

void TestStsCovEstimators::testPcaExplicitRank()
{
    int nCh = 20;
    int explicitRank = 5;
    MatrixXd data = generateGaussianData(nCh, 200);
    auto [cov, rank] = StsCovEstimators::pca(data, explicitRank);

    QCOMPARE(static_cast<int>(rank), explicitRank);

    // Check that the effective rank is <= explicitRank
    SelfAdjointEigenSolver<MatrixXd> solver(cov);
    int nonZero = 0;
    double maxEig = solver.eigenvalues().maxCoeff();
    for (int i = 0; i < nCh; ++i) {
        if (solver.eigenvalues()(i) > maxEig * 1e-10)
            ++nonZero;
    }
    QCOMPARE(nonZero, explicitRank);
}

//=============================================================================================================

void TestStsCovEstimators::testPcaSymmetry()
{
    MatrixXd data = generateGaussianData(15, 100);
    auto [cov, rank] = StsCovEstimators::pca(data);
    double asymmetry = (cov - cov.transpose()).norm();
    QVERIFY2(asymmetry < 1e-12,
             qPrintable(QString("PCA asymmetry=%1").arg(asymmetry)));
}

//=============================================================================================================
// Factor Analysis tests
//=============================================================================================================

void TestStsCovEstimators::testFactorAnalysisDimensions()
{
    int nCh = 15;
    MatrixXd data = generateGaussianData(nCh, 300);
    auto [cov, ll] = StsCovEstimators::factorAnalysis(data, 3);
    QCOMPARE(cov.rows(), nCh);
    QCOMPARE(cov.cols(), nCh);
}

//=============================================================================================================

void TestStsCovEstimators::testFactorAnalysisSymmetry()
{
    MatrixXd data = generateGaussianData(10, 200);
    auto [cov, ll] = StsCovEstimators::factorAnalysis(data, 3);
    double asymmetry = (cov - cov.transpose()).norm();
    QVERIFY2(asymmetry < 1e-12,
             qPrintable(QString("FA asymmetry=%1").arg(asymmetry)));
}

//=============================================================================================================

void TestStsCovEstimators::testFactorAnalysisPositiveDefinite()
{
    MatrixXd data = generateGaussianData(15, 300);
    auto [cov, ll] = StsCovEstimators::factorAnalysis(data, 3);

    SelfAdjointEigenSolver<MatrixXd> solver(cov);
    double minEig = solver.eigenvalues().minCoeff();
    QVERIFY2(minEig > 0.0,
             qPrintable(QString("FA min eigenvalue=%1").arg(minEig)));
}

//=============================================================================================================
// Auto-select tests
//=============================================================================================================

void TestStsCovEstimators::testAutoSelectReturnsValid()
{
    MatrixXd data = generateGaussianData(10, 200);
    auto [cov, bestMethod] = StsCovEstimators::autoSelect(data, 3);

    // Must be positive semi-definite and symmetric
    QCOMPARE(cov.rows(), static_cast<Eigen::Index>(10));
    QCOMPARE(cov.cols(), static_cast<Eigen::Index>(10));
    double asymmetry = (cov - cov.transpose()).norm();
    QVERIFY2(asymmetry < 1e-10,
             qPrintable(QString("Auto-select asymmetry=%1").arg(asymmetry)));
}

//=============================================================================================================

void TestStsCovEstimators::testAutoSelectMethodIndex()
{
    MatrixXd data = generateGaussianData(10, 200);
    auto [cov, bestMethod] = StsCovEstimators::autoSelect(data, 3);

    int method = static_cast<int>(bestMethod);
    QVERIFY2(method >= 0 && method <= 5,
             qPrintable(QString("Auto-select method=%1, expected [0,5]").arg(method)));
}

//=============================================================================================================
// Log-likelihood tests
//=============================================================================================================

void TestStsCovEstimators::testGaussianLogLikelihoodFinite()
{
    MatrixXd data = generateGaussianData(10, 200);
    MatrixXd cov = (data * data.transpose()) / 200.0;
    cov.diagonal().array() += 1e-6;  // ensure invertible

    double ll = StsCovEstimators::gaussianLogLikelihood(data, cov);
    QVERIFY2(std::isfinite(ll),
             qPrintable(QString("Log-likelihood=%1, expected finite").arg(ll)));
    QVERIFY2(ll < 0.0,
             qPrintable(QString("Log-likelihood=%1, expected negative").arg(ll)));
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
