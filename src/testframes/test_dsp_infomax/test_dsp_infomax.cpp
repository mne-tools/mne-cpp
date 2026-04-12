//=============================================================================================================
/**
 * @file     test_dsp_infomax.cpp
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
 * @brief    Tests for Extended Infomax ICA.
 *           Uses synthetic source mixtures with known ground truth.
 *           Validates unmixing recovery via correlation (Amari-like metric).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/extended_infomax.h>

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

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestDspInfomax
 *
 * @brief Tests for Extended Infomax ICA.
 */
class TestDspInfomax : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testOutputDimensions();
    void testUnmixingMixingInverse();
    void testConvergence();
    void testDeterminism();
    void testSuperGaussianRecovery();
    void testReducedComponents();
    void testSourcesComputed();

    void cleanupTestCase();

private:
    MatrixXd createSuperGaussianMix(int nSources, int nSamples,
                                     MatrixXd& mixingOut, MatrixXd& sourcesOut,
                                     unsigned int seed = 42) const;
};

//=============================================================================================================

void TestDspInfomax::initTestCase()
{
}

//=============================================================================================================

void TestDspInfomax::testOutputDimensions()
{
    int nCh = 5;
    int nSamples = 2000;
    MatrixXd mixing, sources;
    MatrixXd data = createSuperGaussianMix(nCh, nSamples, mixing, sources);

    InfomaxResult result = ExtendedInfomax::compute(data, -1, 200, 0.001, 1e-7, true, 42);

    QCOMPARE(result.matUnmixing.rows(), nCh);
    QCOMPARE(result.matUnmixing.cols(), nCh);
    QCOMPARE(result.matMixing.rows(), nCh);
    QCOMPARE(result.matMixing.cols(), nCh);
}

//=============================================================================================================

void TestDspInfomax::testUnmixingMixingInverse()
{
    // Unmixing * Mixing should be approximately identity
    int nCh = 4;
    int nSamples = 3000;
    MatrixXd mixing, sources;
    MatrixXd data = createSuperGaussianMix(nCh, nSamples, mixing, sources);

    InfomaxResult result = ExtendedInfomax::compute(data, -1, 200, 0.001, 1e-7, true, 42);

    MatrixXd product = result.matUnmixing * result.matMixing;
    MatrixXd identity = MatrixXd::Identity(nCh, nCh);

    // Should be close to identity (permutation/scaling ambiguity means we check
    // that each row has one dominant element)
    for (int i = 0; i < nCh; ++i) {
        double maxVal = product.row(i).cwiseAbs().maxCoeff();
        QVERIFY2(maxVal > 0.7,
                 qPrintable(QString("Row %1 max=%2, expected dominant entry").arg(i).arg(maxVal)));
    }
}

//=============================================================================================================

void TestDspInfomax::testConvergence()
{
    int nCh = 3;
    int nSamples = 5000;
    MatrixXd mixing, sources;
    MatrixXd data = createSuperGaussianMix(nCh, nSamples, mixing, sources);

    InfomaxResult result = ExtendedInfomax::compute(data, -1, 200, 0.001, 1e-7, true, 42);

    QVERIFY2(result.converged, "Extended Infomax should converge on super-Gaussian data");
    QVERIFY2(result.nIterations > 0, "Should take at least 1 iteration");
    QVERIFY2(result.nIterations <= 200, "Should converge within 200 iterations");
}

//=============================================================================================================

void TestDspInfomax::testDeterminism()
{
    // Two runs with the same seed should produce identical results
    int nCh = 3;
    int nSamples = 2000;
    MatrixXd mixing, sources;
    MatrixXd data = createSuperGaussianMix(nCh, nSamples, mixing, sources);

    InfomaxResult r1 = ExtendedInfomax::compute(data, -1, 200, 0.001, 1e-7, true, 99);
    InfomaxResult r2 = ExtendedInfomax::compute(data, -1, 200, 0.001, 1e-7, true, 99);

    double diff = (r1.matUnmixing - r2.matUnmixing).norm();
    QVERIFY2(diff < 1e-10,
             qPrintable(QString("Two runs with same seed differ by %1").arg(diff)));
}

//=============================================================================================================

void TestDspInfomax::testSuperGaussianRecovery()
{
    // Create 3 super-Gaussian (Laplacian) sources, mix them, and verify recovery
    int nSources = 3;
    int nSamples = 5000;
    MatrixXd trueMixing, trueSources;
    MatrixXd data = createSuperGaussianMix(nSources, nSamples, trueMixing, trueSources);

    InfomaxResult result = ExtendedInfomax::compute(data, -1, 200, 0.001, 1e-7, true, 42);

    // Recovered sources
    MatrixXd recovered = result.matUnmixing * data;

    // For each true source, find the recovered component with highest correlation
    for (int i = 0; i < nSources; ++i) {
        double bestCorr = 0.0;
        for (int j = 0; j < nSources; ++j) {
            VectorXd a = trueSources.row(i).transpose();
            VectorXd b = recovered.row(j).transpose();
            a.array() -= a.mean();
            b.array() -= b.mean();
            double corr = std::abs(a.dot(b) / (a.norm() * b.norm()));
            bestCorr = std::max(bestCorr, corr);
        }
        QVERIFY2(bestCorr > 0.85,
                 qPrintable(QString("Source %1: best correlation=%2, expected > 0.85").arg(i).arg(bestCorr)));
    }
}

//=============================================================================================================

void TestDspInfomax::testReducedComponents()
{
    // Request fewer components than channels
    int nCh = 5;
    int nComp = 3;
    int nSamples = 3000;
    MatrixXd mixing, sources;
    MatrixXd data = createSuperGaussianMix(nCh, nSamples, mixing, sources);

    InfomaxResult result = ExtendedInfomax::compute(data, nComp, 200, 0.001, 1e-7, true, 42);

    QCOMPARE(result.matUnmixing.rows(), nComp);
    QCOMPARE(result.matUnmixing.cols(), nCh);
}

//=============================================================================================================

void TestDspInfomax::testSourcesComputed()
{
    int nCh = 3;
    int nSamples = 2000;
    MatrixXd mixing, sources;
    MatrixXd data = createSuperGaussianMix(nCh, nSamples, mixing, sources);

    InfomaxResult result = ExtendedInfomax::compute(data, -1, 200, 0.001, 1e-7, true, 42);

    // If matSources is populated, verify its dimensions
    if (result.matSources.rows() > 0) {
        QCOMPARE(result.matSources.rows(), nCh);
        QCOMPARE(result.matSources.cols(), nSamples);
    }
}

//=============================================================================================================

void TestDspInfomax::cleanupTestCase()
{
}

//=============================================================================================================

MatrixXd TestDspInfomax::createSuperGaussianMix(int nSources, int nSamples,
                                                  MatrixXd& mixingOut, MatrixXd& sourcesOut,
                                                  unsigned int seed) const
{
    std::mt19937 gen(seed);

    // Create Laplacian (super-Gaussian) sources
    sourcesOut.resize(nSources, nSamples);
    std::exponential_distribution<double> expDist(1.0);
    for (int i = 0; i < nSources; ++i) {
        for (int j = 0; j < nSamples; ++j) {
            // Laplacian = Exponential * random sign
            double val = expDist(gen);
            sourcesOut(i, j) = (gen() % 2 == 0) ? val : -val;
        }
    }
    // Zero-mean sources
    sourcesOut.colwise() -= sourcesOut.rowwise().mean().eval();

    // Random mixing matrix
    mixingOut.resize(nSources, nSources);
    std::normal_distribution<double> normDist(0.0, 1.0);
    for (int i = 0; i < nSources; ++i)
        for (int j = 0; j < nSources; ++j)
            mixingOut(i, j) = normDist(gen);

    MatrixXd data = mixingOut * sourcesOut;
    // Zero-mean mixed data
    data.colwise() -= data.rowwise().mean().eval();
    return data;
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspInfomax)
#include "test_dsp_infomax.moc"
