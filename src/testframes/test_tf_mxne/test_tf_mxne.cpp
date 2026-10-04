//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_tf_mxne.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Tests for TF-MxNE sparse inverse solver.
 */

#include <inv/sparse/inv_tf_mxne.h>

#include <QtTest>
#include <QObject>
#include <Eigen/Core>
#include <cmath>

using namespace INVLIB;
using namespace Eigen;

class TestTfMxne : public QObject
{
    Q_OBJECT

private slots:
    void testGaborDictionary()
    {
        MatrixXd dict = InvTfMxne::buildGaborDictionary(100, 4, 1.0, 30.0, 200.0);
        QCOMPARE(dict.rows(), static_cast<Index>(8)); // 2 * 4 frequencies
        QCOMPARE(dict.cols(), static_cast<Index>(100));

        // Each row should be approximately unit norm
        for (int r = 0; r < dict.rows(); ++r) {
            QVERIFY2(std::abs(dict.row(r).norm() - 1.0) < 0.01,
                     qPrintable(QString("Row %1 norm=%2").arg(r).arg(dict.row(r).norm())));
        }
    }

    void testGaborSingleFreq()
    {
        MatrixXd dict = InvTfMxne::buildGaborDictionary(50, 1, 10.0, 10.0, 100.0);
        QCOMPARE(dict.rows(), static_cast<Index>(2)); // cos + sin for 1 freq
        QCOMPARE(dict.cols(), static_cast<Index>(50));
    }

    void testComputeRecoversSparseSource()
    {
        // Create a sparse problem: 2 active sources out of 20
        const int nCh = 10, nSrc = 20, nTimes = 50;

        MatrixXd G = MatrixXd::Random(nCh, nSrc) * 0.1;
        MatrixXd X = MatrixXd::Zero(nSrc, nTimes);

        // Active sources at indices 3 and 15
        for (int t = 0; t < nTimes; ++t) {
            X(3, t) = 5.0 * std::sin(2.0 * M_PI * 10.0 * t / 200.0);
            X(15, t) = 3.0 * std::cos(2.0 * M_PI * 20.0 * t / 200.0);
        }

        MatrixXd M = G * X;

        InvTfMxneParams params;
        params.dAlphaSpace = 0.05;
        params.dAlphaTime = 0.01;
        params.dSFreq = 200.0;
        params.iNFreqs = 4;
        params.dFMin = 5.0;
        params.dFMax = 30.0;
        params.iMaxIterations = 50;

        InvTfMxneResult result = InvTfMxne::compute(G, M, params);

        // Should find some active sources
        QVERIFY(!result.activeVertices.isEmpty());
        QVERIFY(result.nIterations > 0);
        QVERIFY(!result.stc.isEmpty());
    }

    void testComputeEmptyGain()
    {
        MatrixXd G(0, 0);
        MatrixXd M(0, 0);
        InvTfMxneResult result = InvTfMxne::compute(G, M);
        QVERIFY(result.activeVertices.isEmpty());
    }

    void testComputeDimensionMismatch()
    {
        MatrixXd G = MatrixXd::Random(5, 10);
        MatrixXd M = MatrixXd::Random(7, 20); // 7 != 5
        InvTfMxneResult result = InvTfMxne::compute(G, M);
        QVERIFY(result.stc.isEmpty());
    }

    void testHighRegularizationYieldsZero()
    {
        MatrixXd G = MatrixXd::Random(5, 10);
        MatrixXd M = MatrixXd::Random(5, 20) * 0.01;

        InvTfMxneParams params;
        params.dAlphaSpace = 100.0;
        params.dAlphaTime = 100.0;
        params.iMaxIterations = 10;

        InvTfMxneResult result = InvTfMxne::compute(G, M, params);

        // With very high regularization, should have no active sources
        QVERIFY(result.activeVertices.isEmpty() || result.stc.data.norm() < 0.01);
    }

    void testResultMethod()
    {
        MatrixXd G = MatrixXd::Random(5, 10);
        MatrixXd X = MatrixXd::Zero(10, 20);
        X(2, 10) = 10.0;
        MatrixXd M = G * X;

        InvTfMxneParams params;
        params.dAlphaSpace = 0.01;
        params.dAlphaTime = 0.001;

        InvTfMxneResult result = InvTfMxne::compute(G, M, params);

        if (!result.stc.isEmpty()) {
            QVERIFY(result.stc.method == InvEstimateMethod::MixedNorm);
        }
    }

    void testSolutionSatisfiesKkt()
    {
        // min_Z 0.5 ||M - G Z Phi||^2 + aS sum_j ||z_j|| + aT ||Z||_1 is optimal iff, with g = G^T R Phi^T:
        // nonzero z_ja: g_ja = aT sign(z_ja) + aS z_ja / ||z_j||; zero z_ja in an active row: |g_ja| <= aT;
        // inactive row: ||soft_aT(g_j)|| <= aS.
        const int nCh = 12, nSrc = 8, nTimes = 64;
        MatrixXd G(nCh, nSrc);
        for (int c = 0; c < nCh; ++c)
            for (int s = 0; s < nSrc; ++s)
                G(c, s) = std::sin(0.7 * (c + 1) * (s + 1)) + (c == s ? 1.0 : 0.0);
        InvTfMxneParams params;
        params.dSFreq = 200.0;
        params.iNFreqs = 3;
        params.dFMin = 8.0;
        params.dFMax = 30.0;
        params.dAlphaSpace = 0.3;
        params.dAlphaTime = 0.05;
        params.iMaxIterations = 20000;
        params.dTolerance = 1e-14;
        params.bDebias = false;
        const MatrixXd Phi = InvTfMxne::buildGaborDictionary(nTimes, params.iNFreqs, params.dFMin, params.dFMax, params.dSFreq);
        MatrixXd Ztrue = MatrixXd::Zero(nSrc, Phi.rows());
        Ztrue(1, 0) = 4.0;
        Ztrue(1, 3) = -2.0;
        Ztrue(5, 4) = 3.0;
        const MatrixXd M = G * Ztrue * Phi;

        const InvTfMxneResult result = InvTfMxne::compute(G, M, params);
        QVERIFY(result.nIterations < params.iMaxIterations);
        QVERIFY(result.activeVertices.contains(1) && result.activeVertices.contains(5));
        MatrixXd Z = MatrixXd::Zero(nSrc, Phi.rows());
        for (int i = 0; i < result.activeVertices.size(); ++i) {
            Z.row(result.activeVertices[i]) = result.tfCoefficients.row(i);
            QVERIFY((result.stc.data.row(i) - result.tfCoefficients.row(i) * Phi).norm() < 1e-12);
        }

        const MatrixXd g = G.transpose() * (M - G * Z * Phi) * Phi.transpose();
        const double tol = 1e-4;
        for (int j = 0; j < nSrc; ++j) {
            const double rowNorm = Z.row(j).norm();
            if (rowNorm == 0.0) {
                const RowVectorXd soft = g.row(j).unaryExpr([&](double v) { return std::copysign(std::max(0.0, std::abs(v) - params.dAlphaTime), v); });
                QVERIFY2(soft.norm() <= params.dAlphaSpace + tol, qPrintable(QString("inactive row %1: %2").arg(j).arg(soft.norm())));
                continue;
            }
            for (int a = 0; a < Z.cols(); ++a) {
                const double z = Z(j, a);
                const double err = z != 0.0 ? std::abs(g(j, a) - params.dAlphaTime * (z > 0 ? 1.0 : -1.0) - params.dAlphaSpace * z / rowNorm)
                                            : std::max(0.0, std::abs(g(j, a)) - params.dAlphaTime);
                QVERIFY2(err <= tol, qPrintable(QString("row %1 atom %2: KKT violation %3").arg(j).arg(a).arg(err)));
            }
        }

        // Debiasing refits the active set by least squares: G_A^T (M - G_A X) = 0.
        params.bDebias = true;
        const InvTfMxneResult debiased = InvTfMxne::compute(G, M, params);
        MatrixXd GA(nCh, debiased.activeVertices.size());
        for (int i = 0; i < debiased.activeVertices.size(); ++i)
            GA.col(i) = G.col(debiased.activeVertices[i]);
        QVERIFY((GA.transpose() * (M - GA * debiased.stc.data)).norm() <= 1e-10 * M.norm());
    }

    void testParamsDefaults()
    {
        InvTfMxneParams params;
        QCOMPARE(params.iMaxIterations, 100);
        QVERIFY(std::abs(params.dAlphaSpace - 0.5) < 1e-10);
        QVERIFY(std::abs(params.dAlphaTime - 0.1) < 1e-10);
        QCOMPARE(params.iNFreqs, 8);
        QVERIFY(params.bDebias);
    }
};

QTEST_GUILESS_MAIN(TestTfMxne)
#include "test_tf_mxne.moc"
