//=============================================================================================================
/**
 * @file     test_dsp_sphara.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
