//=============================================================================================================
/**
 * @file     test_source_morph.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for SourceMorph class.
 */

#include <inv/morph/source_morph.h>
#include <inv/inv_source_estimate.h>

#include <QtTest>
#include <QObject>
#include <Eigen/Core>
#include <Eigen/Sparse>

using namespace INVLIB;
using namespace Eigen;

class TestSourceMorph : public QObject
{
    Q_OBJECT

private slots:
    void testComputeAndApply()
    {
        // Simple identity morph
        int nFrom = 5, nTo = 5, nTimes = 10;

        VectorXi vertsFrom(nFrom), vertsTo(nTo);
        for (int i = 0; i < nFrom; ++i) vertsFrom(i) = i;
        for (int i = 0; i < nTo; ++i) vertsTo(i) = i;

        // Identity sparse matrix
        SparseMatrix<double> morphMap(nTo, nFrom);
        for (int i = 0; i < nTo; ++i) morphMap.insert(i, i) = 1.0;
        morphMap.makeCompressed();

        SourceMorph morph;
        morph.compute(vertsFrom, vertsTo, morphMap);
        QVERIFY(morph.isComputed());
        QCOMPARE(morph.nVerticesTo(), nTo);

        // Create source estimate
        MatrixXd data = MatrixXd::Ones(nFrom, nTimes);
        InvSourceEstimate stc(data, vertsFrom, 0.0f, 0.001f);

        InvSourceEstimate morphed = morph.apply(stc);

        QVERIFY(!morphed.isEmpty());
        QCOMPARE(morphed.data.rows(), static_cast<Index>(nTo));
        QCOMPARE(morphed.data.cols(), static_cast<Index>(nTimes));

        // Identity morph should preserve data
        QVERIFY((morphed.data - data).norm() < 1e-10);
    }

    void testDownsampling()
    {
        // Morph from 10 to 5 vertices (average pairs)
        int nFrom = 10, nTo = 5, nTimes = 4;

        VectorXi vertsFrom(nFrom), vertsTo(nTo);
        for (int i = 0; i < nFrom; ++i) vertsFrom(i) = i;
        for (int i = 0; i < nTo; ++i) vertsTo(i) = i;

        SparseMatrix<double> morphMap(nTo, nFrom);
        for (int i = 0; i < nTo; ++i) {
            morphMap.insert(i, 2*i) = 0.5;
            morphMap.insert(i, 2*i+1) = 0.5;
        }
        morphMap.makeCompressed();

        SourceMorph morph;
        morph.compute(vertsFrom, vertsTo, morphMap);

        MatrixXd data = MatrixXd::Ones(nFrom, nTimes) * 4.0;
        InvSourceEstimate stc(data, vertsFrom, 0.0f, 0.001f);

        InvSourceEstimate morphed = morph.apply(stc);

        QCOMPARE(morphed.data.rows(), static_cast<Index>(nTo));
        // Average of 4.0 and 4.0 should be 4.0
        QVERIFY(std::abs(morphed.data(0, 0) - 4.0) < 1e-10);
    }

    void testNotComputed()
    {
        SourceMorph morph;
        QVERIFY(!morph.isComputed());

        InvSourceEstimate stc;
        InvSourceEstimate result = morph.apply(stc);
        QVERIFY(result.isEmpty());
    }

    void testDimensionMismatch()
    {
        VectorXi vertsFrom(3), vertsTo(2);
        vertsFrom << 0, 1, 2;
        vertsTo << 0, 1;

        SparseMatrix<double> morphMap(2, 3);
        morphMap.insert(0, 0) = 1.0;
        morphMap.insert(1, 1) = 1.0;
        morphMap.makeCompressed();

        SourceMorph morph;
        morph.compute(vertsFrom, vertsTo, morphMap);
        QVERIFY(morph.isComputed());

        // Wrong number of source rows
        MatrixXd data = MatrixXd::Ones(5, 4); // 5 != 3
        InvSourceEstimate stc(data, VectorXi::LinSpaced(5, 0, 4), 0.0f, 0.001f);

        InvSourceEstimate result = morph.apply(stc);
        QVERIFY(result.isEmpty());
    }

    void testPreservesMetadata()
    {
        int n = 3, nTimes = 5;
        VectorXi verts(n);
        for (int i = 0; i < n; ++i) verts(i) = i;

        SparseMatrix<double> morphMap(n, n);
        for (int i = 0; i < n; ++i) morphMap.insert(i, i) = 1.0;
        morphMap.makeCompressed();

        SourceMorph morph;
        morph.compute(verts, verts, morphMap);

        MatrixXd data = MatrixXd::Random(n, nTimes);
        InvSourceEstimate stc(data, verts, 0.1f, 0.002f);
        stc.method = InvEstimateMethod::dSPM;
        stc.sourceSpaceType = InvSourceSpaceType::Surface;

        InvSourceEstimate morphed = morph.apply(stc);

        QCOMPARE(morphed.tmin, 0.1f);
        QCOMPARE(morphed.tstep, 0.002f);
        QVERIFY(morphed.method == InvEstimateMethod::dSPM);
        QVERIFY(morphed.sourceSpaceType == InvSourceSpaceType::Surface);
    }

    void testEmptyInput()
    {
        VectorXi v(2);
        v << 0, 1;
        SparseMatrix<double> m(2, 2);
        m.insert(0, 0) = 1.0;
        m.insert(1, 1) = 1.0;
        m.makeCompressed();

        SourceMorph morph;
        morph.compute(v, v, m);

        InvSourceEstimate empty;
        InvSourceEstimate result = morph.apply(empty);
        QVERIFY(result.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestSourceMorph)
#include "test_source_morph.moc"
