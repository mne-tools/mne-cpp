//=============================================================================================================
/**
 * @file     test_dsp_ica.cpp
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
 * @brief    Unit tests for the ICA (FastICA) class.
 */

#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <cmath>

#include <dsp/ica.h>

using namespace UTILSLIB;
using namespace Eigen;

class TestDspIca : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================
    // Dimensions
    //=========================================================================
    void resultDimensions_allComponents()
    {
        // 5 channels, 500 samples, extract all 5 components
        MatrixXd data = MatrixXd::Random(5, 500);
        IcaResult res = ICA::run(data, -1, 200, 1e-4, 42);

        QCOMPARE(res.matMixing.rows(),   5);
        QCOMPARE(res.matMixing.cols(),   5);
        QCOMPARE(res.matUnmixing.rows(), 5);
        QCOMPARE(res.matUnmixing.cols(), 5);
        QCOMPARE(res.matSources.rows(),  5);
        QCOMPARE(res.matSources.cols(),  500);
        QCOMPARE(res.vecMean.size(),     5);
    }

    void resultDimensions_reducedComponents()
    {
        // 10 channels, 1000 samples, extract 3 components
        MatrixXd data = MatrixXd::Random(10, 1000);
        IcaResult res = ICA::run(data, 3, 200, 1e-4, 42);

        QCOMPARE(res.matMixing.rows(),   10);
        QCOMPARE(res.matMixing.cols(),   3);
        QCOMPARE(res.matUnmixing.rows(), 3);
        QCOMPARE(res.matUnmixing.cols(), 10);
        QCOMPARE(res.matSources.rows(),  3);
        QCOMPARE(res.matSources.cols(),  1000);
    }

    //=========================================================================
    // Source independence: off-diagonal correlations should be small
    //=========================================================================
    void sourceUncorrelated()
    {
        // Mix two independent sinusoids
        const int N = 2000;
        const int nCh = 2;
        MatrixXd sources(nCh, N);
        for (int i = 0; i < N; ++i) {
            sources(0, i) = std::sin(2.0 * M_PI * 5.0  * i / 1000.0);
            sources(1, i) = std::sin(2.0 * M_PI * 13.0 * i / 1000.0);
        }

        // Random mixing matrix
        MatrixXd A(nCh, nCh);
        A << 0.8, 0.4,
             0.2, 0.9;
        MatrixXd mixed = A * sources;

        IcaResult res = ICA::run(mixed, nCh, 300, 1e-5, 1);
        QVERIFY(res.bConverged);

        // Normalise recovered sources to unit variance for correlation check
        MatrixXd S = res.matSources;
        for (int r = 0; r < nCh; ++r) {
            double s = S.row(r).norm();
            if (s > 1e-12) S.row(r) /= s;
        }

        // Correlation matrix (should be close to identity up to permutation/sign)
        MatrixXd corr = S * S.transpose() / N;
        for (int r = 0; r < nCh; ++r) {
            for (int c = 0; c < nCh; ++c) {
                if (r != c) {
                    QVERIFY2(std::abs(corr(r, c)) < 0.15,
                             qPrintable(QString("Off-diagonal correlation too large: %1").arg(corr(r, c))));
                }
            }
        }
    }

    //=========================================================================
    // Reconstruction quality: exclude zero components => perfect reconstruction
    //=========================================================================
    void reconstructionNoExclusion()
    {
        MatrixXd data = MatrixXd::Random(4, 400);
        IcaResult res = ICA::run(data, 4, 200, 1e-4, 7);

        MatrixXd recon = ICA::excludeComponents(data, res, {});
        // Should be (near) identical to input
        double maxDiff = (recon - data).cwiseAbs().maxCoeff();
        QVERIFY2(maxDiff < 1e-8,
                 qPrintable(QString("Max diff with no exclusion: %1").arg(maxDiff)));
    }

    void reconstructionWithExclusion()
    {
        // Construct data with a known "artifact" component
        const int N = 1000;
        const int nCh = 3;
        MatrixXd sources(nCh, N);
        for (int i = 0; i < N; ++i) {
            sources(0, i) = std::sin(2 * M_PI * 50.0 * i / 1000.0);  // "artifact" at 50 Hz
            sources(1, i) = std::sin(2 * M_PI * 10.0 * i / 1000.0);
            sources(2, i) = std::sin(2 * M_PI * 3.0  * i / 1000.0);
        }
        MatrixXd A(nCh, nCh);
        A << 1.0, 0.3, 0.1,
             0.2, 1.0, 0.4,
             0.1, 0.5, 1.0;
        MatrixXd mixed = A * sources;

        IcaResult res = ICA::run(mixed, nCh, 300, 1e-5, 42);

        // Exclude first component (regardless of which physical source it maps to)
        MatrixXd cleaned = ICA::excludeComponents(mixed, res, {0});

        // Reconstruction should differ from original but have same dimensions
        QCOMPARE(cleaned.rows(), mixed.rows());
        QCOMPARE(cleaned.cols(), mixed.cols());
    }

    //=========================================================================
    // applyUnmixing consistency
    //=========================================================================
    void applyUnmixingConsistency()
    {
        MatrixXd data = MatrixXd::Random(5, 600);
        IcaResult res = ICA::run(data, 5, 200, 1e-4, 99);

        // Sources from run() and from applyUnmixing() should match
        MatrixXd S2 = ICA::applyUnmixing(data, res);
        double maxDiff = (S2 - res.matSources).cwiseAbs().maxCoeff();
        QVERIFY2(maxDiff < 1e-10,
                 qPrintable(QString("applyUnmixing mismatch: %1").arg(maxDiff)));
    }

    //=========================================================================
    // Mean centering: vecMean should be the row-wise mean of the input
    //=========================================================================
    void meanCentering()
    {
        MatrixXd data = MatrixXd::Random(4, 300);
        // Add a large offset so the mean is non-trivial
        data.rowwise() += Eigen::RowVectorXd::LinSpaced(300, 1.0, 5.0);

        IcaResult res = ICA::run(data, 4, 100, 1e-4, 0);
        VectorXd expectedMean = data.rowwise().mean();

        double maxErr = (res.vecMean - expectedMean).cwiseAbs().maxCoeff();
        QVERIFY2(maxErr < 1e-10,
                 qPrintable(QString("Mean mismatch: %1").arg(maxErr)));
    }

    //=========================================================================
    // Edge: single component
    //=========================================================================
    void singleComponent()
    {
        MatrixXd data = MatrixXd::Random(5, 500);
        IcaResult res = ICA::run(data, 1, 200, 1e-4, 5);

        QCOMPARE(res.matSources.rows(), 1);
        QCOMPARE(res.matSources.cols(), 500);
        QCOMPARE(res.matMixing.rows(),  5);
        QCOMPARE(res.matMixing.cols(),  1);
    }
};

QTEST_MAIN(TestDspIca)
#include "test_dsp_ica.moc"
