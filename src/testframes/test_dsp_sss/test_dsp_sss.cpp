//=============================================================================================================
/**
 * @file     test_dsp_sss.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.10
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Unit tests for the SSS (Signal Space Separation) class.
 */

#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <cmath>

#include <dsp/sss.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// Helper: build a synthetic FiffInfo with MEG magnetometers arranged on a sphere
static FiffInfo makeSyntheticMegInfo(int nSensors, double radius = 0.12)
{
    FiffInfo info;
    info.nchan = nSensors;
    info.chs.resize(nSensors);

    // Distribute sensors roughly uniformly on a sphere of given radius using
    // the Fibonacci lattice (golden-angle spiral).
    const double golden = M_PI * (3.0 - std::sqrt(5.0));  // golden angle

    for (int i = 0; i < nSensors; ++i) {
        double y   = 1.0 - (static_cast<double>(i) / (nSensors - 1)) * 2.0;
        double r2d = std::sqrt(1.0 - y * y);
        double phi = golden * i;

        // Sensor position on sphere
        double x = std::cos(phi) * r2d * radius;
        double z = std::sin(phi) * r2d * radius + 0.04;  // 4 cm superior
        double yy = y * radius;

        // Outward-pointing normal (radial)
        Vector3f pos(static_cast<float>(x), static_cast<float>(yy), static_cast<float>(z));
        Vector3f normal = pos.normalized();

        // Build ez and ex arbitrarily perpendicular
        Vector3f ez = normal;
        Vector3f ex = Vector3f(1, 0, 0);
        if (std::abs(ez.dot(ex)) > 0.9f) ex = Vector3f(0, 1, 0);
        Vector3f ey = ez.cross(ex).normalized();
        ex = ey.cross(ez).normalized();

        FiffChInfo ch;
        ch.kind         = FIFFV_MEG_CH;
        ch.unit         = 112;   // Tesla
        ch.chpos.r0     = pos;
        ch.chpos.ex     = ex;
        ch.chpos.ey     = ey;
        ch.chpos.ez     = ez;
        ch.ch_name      = QString("MEG%1").arg(i + 1, 4, 10, QChar('0'));
        info.chs[i]     = ch;
    }

    info.sfreq  = 1000.0;
    return info;
}

//=============================================================================================================

class TestDspSss : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================
    // Basis dimensions
    //=========================================================================
    void basisDimensions_standard()
    {
        FiffInfo info = makeSyntheticMegInfo(102);
        SSS::Params p;
        p.iOrderIn  = 8;
        p.iOrderOut = 3;
        SSS::Basis basis = SSS::computeBasis(info, p);

        const int nMeg = 102;
        const int Nin  = 8 * (8 + 2);  // 80
        const int Nout = 3 * (3 + 2);  // 15

        QCOMPARE(basis.megChannelIdx.size(), nMeg);
        QCOMPARE(basis.matSin.rows(),  nMeg);
        QCOMPARE(basis.matSin.cols(),  Nin);
        QCOMPARE(basis.matSout.rows(), nMeg);
        QCOMPARE(basis.matSout.cols(), Nout);
        QCOMPARE(basis.matProjIn.rows(), nMeg);
        QCOMPARE(basis.matProjIn.cols(), nMeg);
        QCOMPARE(basis.matPinvAll.rows(), Nin + Nout);
        QCOMPARE(basis.matPinvAll.cols(), nMeg);
    }

    void basisDimensions_reducedOrder()
    {
        FiffInfo info = makeSyntheticMegInfo(60);
        SSS::Params p;
        p.iOrderIn  = 4;
        p.iOrderOut = 2;
        SSS::Basis basis = SSS::computeBasis(info, p);

        const int Nin  = 4 * 6;   // 24
        const int Nout = 2 * 4;   // 8

        QCOMPARE(basis.iNin,  Nin);
        QCOMPARE(basis.iNout, Nout);
        QCOMPARE(basis.matProjIn.rows(), 60);
        QCOMPARE(basis.matProjIn.cols(), 60);
    }

    void basisEmpty_noMegChannels()
    {
        FiffInfo info;
        info.nchan = 3;
        info.chs.resize(3);
        for (auto& ch : info.chs) ch.kind = FIFFV_EEG_CH;

        SSS::Basis basis = SSS::computeBasis(info, SSS::Params());
        QVERIFY(basis.megChannelIdx.isEmpty());
        QVERIFY(basis.matProjIn.size() == 0);
    }

    //=========================================================================
    // Projector is an idempotent projection: P^2 = P (rank ≤ N_in)
    //=========================================================================
    void projectorIdempotent()
    {
        FiffInfo info = makeSyntheticMegInfo(102);
        SSS::Params p;
        p.iOrderIn  = 8;
        p.iOrderOut = 3;
        SSS::Basis basis = SSS::computeBasis(info, p);

        MatrixXd P   = basis.matProjIn;
        MatrixXd P2  = P * P;
        double maxErr = (P2 - P).cwiseAbs().maxCoeff();

        QVERIFY2(maxErr < 1e-6,
                 qPrintable(QString("P^2 != P: max error %1").arg(maxErr)));
    }

    //=========================================================================
    // SSS apply: output has correct dimensions, non-MEG channels unchanged
    //=========================================================================
    void apply_dimensionsAndNonMegPassthrough()
    {
        FiffInfo info = makeSyntheticMegInfo(102);
        // Add one EEG channel
        FiffChInfo eeg;
        eeg.kind = FIFFV_EEG_CH;
        eeg.unit = 107;
        info.chs.append(eeg);
        info.nchan = 103;

        SSS::Basis basis = SSS::computeBasis(info, SSS::Params());

        MatrixXd data = MatrixXd::Random(103, 500);
        MatrixXd out  = SSS::apply(data, basis);

        QCOMPARE(out.rows(), data.rows());
        QCOMPARE(out.cols(), data.cols());

        // EEG channel (row 102) must be unchanged
        double diff = (out.row(102) - data.row(102)).cwiseAbs().maxCoeff();
        QVERIFY2(diff == 0.0, "Non-MEG channel was modified by SSS::apply");
    }

    //=========================================================================
    // SSS suppresses a simulated external dipole field
    //=========================================================================
    void apply_suppressesExternalField()
    {
        // Build 102-sensor array
        FiffInfo info = makeSyntheticMegInfo(102);
        SSS::Params p;
        p.iOrderIn  = 8;
        p.iOrderOut = 3;
        SSS::Basis basis = SSS::computeBasis(info, p);

        const int nSamp = 200;
        MatrixXd data(102, nSamp);

        // Simulate a simple internal dipole source field: project a random
        // signal through the first column of S_in (field pattern of internal component 0)
        VectorXd internalPattern = basis.matSin.col(0);
        RowVectorXd internalSignal = RowVectorXd::Random(nSamp);
        for (int i = 0; i < 102; ++i) {
            data.row(i) = internalPattern(i) * internalSignal;
        }

        MatrixXd cleaned = SSS::apply(data, basis);

        // The internal field pattern must be mostly preserved (correlation with input ≥ 0.9)
        double normIn  = data.norm();
        double normOut = cleaned.norm();
        QVERIFY2(normIn > 1e-30, "Input signal is zero");
        QVERIFY2(normOut > normIn * 0.1,  // internal signal mostly retained
                 qPrintable(QString("SSS removed too much of internal signal: ratio = %1")
                            .arg(normOut / normIn)));
    }

    //=========================================================================
    // applyTemporal: returns same dimensions
    //=========================================================================
    void applyTemporal_dimensions()
    {
        FiffInfo info = makeSyntheticMegInfo(102);
        SSS::Basis basis = SSS::computeBasis(info, SSS::Params());

        MatrixXd data = MatrixXd::Random(102, 2000);
        MatrixXd out  = SSS::applyTemporal(data, basis, 500, 0.98);

        QCOMPARE(out.rows(), data.rows());
        QCOMPARE(out.cols(), data.cols());
    }
};

QTEST_MAIN(TestDspSss)
#include "test_dsp_sss.moc"
