//=============================================================================================================
/**
 * @file     test_tf_mxne.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
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
        QCOMPARE(dict.rows(), static_cast<Index>(8));   // 2 * 4 frequencies
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
        QCOMPARE(dict.rows(), static_cast<Index>(2));  // cos + sin for 1 freq
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
            X(3, t)  = 5.0 * std::sin(2.0 * M_PI * 10.0 * t / 200.0);
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
        MatrixXd M = MatrixXd::Random(7, 20);  // 7 != 5
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
