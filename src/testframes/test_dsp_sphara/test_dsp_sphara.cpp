//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_sphara.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Unit tests for SPHARA projector construction.
 */

#include <QtTest/QtTest>

#include <dsp/sphara.h>

#include <Eigen/Core>

using namespace UTILSLIB;
using namespace Eigen;

class TestDspSphara : public QObject
{
    Q_OBJECT

private slots:
    void makeSpharaProjector_emptyBasis_returnsIdentity();
    void makeSpharaProjector_simpleProjector_matchesExpectedBlock();
    void makeSpharaProjector_skipReplicatesAcrossSensorGroups();
};

void TestDspSphara::makeSpharaProjector_emptyBasis_returnsIdentity()
{
    MatrixXd basis;
    VectorXi indices(3);
    indices << 0, 1, 2;

    MatrixXd projector = makeSpharaProjector(basis, indices, 3, 1);
    QVERIFY(projector.isApprox(MatrixXd::Identity(3, 3)));
}

void TestDspSphara::makeSpharaProjector_simpleProjector_matchesExpectedBlock()
{
    MatrixXd basis(3, 2);
    basis << 1.0, 0.0,
             0.0, 1.0,
             0.0, 0.0;

    VectorXi indices(3);
    indices << 2, 0, 3;

    MatrixXd projector = makeSpharaProjector(basis, indices, 5, 1);
    MatrixXd expected = MatrixXd::Identity(5, 5);
    expected(2, 2) = 1.0;
    expected(2, 0) = 0.0;
    expected(2, 3) = 0.0;
    expected(0, 2) = 0.0;
    expected(0, 0) = 0.0;
    expected(0, 3) = 0.0;
    expected(3, 2) = 0.0;
    expected(3, 0) = 0.0;
    expected(3, 3) = 0.0;

    QVERIFY(projector.isApprox(expected));
}

void TestDspSphara::makeSpharaProjector_skipReplicatesAcrossSensorGroups()
{
    MatrixXd basis(2, 1);
    basis << 1.0,
             0.0;

    VectorXi indices(4);
    indices << 0, 1, 2, 3;

    MatrixXd projector = makeSpharaProjector(basis, indices, 4, 1, 1);
    MatrixXd expected = MatrixXd::Identity(4, 4);
    expected(0, 0) = 1.0;
    expected(0, 2) = 0.0;
    expected(2, 0) = 0.0;
    expected(2, 2) = 0.0;
    expected(1, 1) = 1.0;
    expected(1, 3) = 0.0;
    expected(3, 1) = 0.0;
    expected(3, 3) = 0.0;

    QVERIFY(projector.isApprox(expected));
}

QTEST_MAIN(TestDspSphara)
#include "test_dsp_sphara.moc"
