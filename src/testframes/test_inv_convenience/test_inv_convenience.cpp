//=============================================================================================================
/**
 * @file     test_inv_convenience.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    Tests for inverse convenience functions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/inv_convenience.h>
#include <inv/beamformer/inv_lcmv.h>
#include <inv/beamformer/inv_dics.h>
#include <mne/mne_inverse_operator.h>
#include <fiff/fiff_cov.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Eigenvalues>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for inverse convenience functions.
 *
 * Note: Full integration tests with actual inverse operators require
 * test data files. These unit tests focus on computeWhitener and
 * applyInverseEpochs with synthetic data.
 */
class TestInvConvenience : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}

    //--- computeWhitener tests ---

    void testWhitenerDimensions()
    {
        FiffCov cov;
        cov.dim = 5;
        cov.data = MatrixXd::Identity(5, 5);
        cov.kind = 1;
        cov.diag = false;

        // Pre-compute eigendecomposition
        SelfAdjointEigenSolver<MatrixXd> solver(cov.data);
        cov.eig = solver.eigenvalues();
        cov.eigvec = solver.eigenvectors();

        auto [whitener, rank] = computeWhitener(cov);

        QCOMPARE(whitener.rows(), static_cast<Eigen::Index>(5));
        QCOMPARE(whitener.cols(), static_cast<Eigen::Index>(5));
        QCOMPARE(rank, 5);
    }

    void testWhitenerIdentityCov()
    {
        // For identity covariance, whitener should be identity (or orthogonal equivalent)
        FiffCov cov;
        cov.dim = 4;
        cov.data = MatrixXd::Identity(4, 4);
        cov.kind = 1;
        cov.diag = false;

        SelfAdjointEigenSolver<MatrixXd> solver(cov.data);
        cov.eig = solver.eigenvalues();
        cov.eigvec = solver.eigenvectors();

        auto [W, rank] = computeWhitener(cov);
        QCOMPARE(rank, 4);

        // W * W^T should be identity for identity covariance
        MatrixXd WWT = W * W.transpose();
        double diff = (WWT - MatrixXd::Identity(4, 4)).norm();
        QVERIFY2(diff < 1e-10,
                 qPrintable(QString("W*W^T should be identity, diff=%1").arg(diff)));
    }

    void testWhitenerProducesIdentityCov()
    {
        // Generate a random covariance matrix
        std::mt19937 gen(42);
        std::normal_distribution<> dist(0.0, 1.0);

        int p = 6;
        MatrixXd A(p, p);
        for (int i = 0; i < p; ++i)
            for (int j = 0; j < p; ++j)
                A(i, j) = dist(gen);

        MatrixXd trueCov = A * A.transpose() / static_cast<double>(p);

        FiffCov cov;
        cov.dim = p;
        cov.data = trueCov;
        cov.kind = 1;
        cov.diag = false;

        SelfAdjointEigenSolver<MatrixXd> solver(cov.data);
        cov.eig = solver.eigenvalues();
        cov.eigvec = solver.eigenvectors();

        auto [W, rank] = computeWhitener(cov);

        // W * C * W^T should be approximately identity
        MatrixXd whitened = W * trueCov * W.transpose();
        double diagDiff = 0.0;
        for (int i = 0; i < rank; ++i) {
            diagDiff += std::abs(whitened(i, i) - 1.0);
        }
        diagDiff /= static_cast<double>(rank);

        QVERIFY2(diagDiff < 0.1,
                 qPrintable(QString("Avg diagonal diff=%1, expected < 0.1").arg(diagDiff)));
    }

    void testWhitenerAutoRank()
    {
        // Create rank-deficient covariance
        int p = 8;
        int trueRank = 4;

        MatrixXd A = MatrixXd::Zero(p, p);
        for (int i = 0; i < trueRank; ++i) {
            A(i, i) = static_cast<double>(i + 1);
        }

        FiffCov cov;
        cov.dim = p;
        cov.data = A;
        cov.kind = 1;
        cov.diag = false;

        SelfAdjointEigenSolver<MatrixXd> solver(cov.data);
        cov.eig = solver.eigenvalues();
        cov.eigvec = solver.eigenvectors();

        auto [W, rank] = computeWhitener(cov);
        QCOMPARE(rank, trueRank);
    }

    void testWhitenerExplicitRank()
    {
        FiffCov cov;
        cov.dim = 5;
        cov.data = MatrixXd::Identity(5, 5);
        cov.kind = 1;
        cov.diag = false;

        SelfAdjointEigenSolver<MatrixXd> solver(cov.data);
        cov.eig = solver.eigenvalues();
        cov.eigvec = solver.eigenvectors();

        auto [W, rank] = computeWhitener(cov, 3);
        QCOMPARE(rank, 3);
    }

    void testWhitenerEmptyCov()
    {
        FiffCov cov;
        auto [W, rank] = computeWhitener(cov);
        QCOMPARE(rank, 0);
        QVERIFY(W.rows() == 0 || W.cols() == 0);
    }

    //--- applyInverseEpochs tests ---

    void testApplyInverseEpochsEmpty()
    {
        QList<MatrixXd> epochs;
        MNEInverseOperator inv;

        QList<InvSourceEstimate> results = applyInverseEpochs(epochs, inv, 1.0f / 9.0f);
        QVERIFY(results.isEmpty());
    }

    //--- computeSourcePsd tests ---

    void testSourcePsdDimensions()
    {
        // Create a simple source estimate: 3 sources, 256 time points at 256 Hz
        const int nSrc = 3;
        const int nTimes = 256;
        const float sfreq = 256.0f;

        MatrixXd data = MatrixXd::Random(nSrc, nTimes);
        VectorXi verts(nSrc);
        verts << 0, 1, 2;

        InvSourceEstimate stc(data, verts, 0.0f, 1.0f / sfreq);

        auto [psd, freqs] = computeSourcePsd(stc, sfreq);

        // nFft = nTimes = 256, so nFreqs = 129 (0 to Nyquist)
        QCOMPARE(psd.rows(), static_cast<Eigen::Index>(nSrc));
        QCOMPARE(psd.cols(), freqs.size());
        QVERIFY(freqs.size() > 0);
        // First frequency should be 0
        QVERIFY(std::abs(freqs(0)) < 1e-6);
        // Last frequency should be Nyquist = sfreq/2
        QVERIFY(std::abs(freqs(freqs.size() - 1) - sfreq / 2.0) < 1.0);
    }

    void testSourcePsdSineWave()
    {
        // Create a pure sine wave at 10 Hz, verify peak is at 10 Hz
        const int nSrc = 1;
        const int nTimes = 1024;
        const float sfreq = 256.0f;
        const double targetFreq = 10.0;

        MatrixXd data(nSrc, nTimes);
        for (int t = 0; t < nTimes; ++t) {
            data(0, t) = std::sin(2.0 * M_PI * targetFreq * t / sfreq);
        }

        VectorXi verts(1);
        verts << 0;
        InvSourceEstimate stc(data, verts, 0.0f, 1.0f / sfreq);

        auto [psd, freqs] = computeSourcePsd(stc, sfreq);

        // Find peak frequency
        int peakIdx = 0;
        double peakVal = psd(0, 0);
        for (int f = 1; f < freqs.size(); ++f) {
            if (psd(0, f) > peakVal) {
                peakVal = psd(0, f);
                peakIdx = f;
            }
        }

        double peakFreq = freqs(peakIdx);
        QVERIFY2(std::abs(peakFreq - targetFreq) < 1.0,
                 qPrintable(QString("Peak at %1 Hz, expected %2 Hz").arg(peakFreq).arg(targetFreq)));
    }

    void testSourcePsdFreqRange()
    {
        const int nSrc = 2;
        const int nTimes = 512;
        const float sfreq = 256.0f;

        MatrixXd data = MatrixXd::Random(nSrc, nTimes);
        VectorXi verts(nSrc);
        verts << 0, 1;
        InvSourceEstimate stc(data, verts, 0.0f, 1.0f / sfreq);

        auto [psd, freqs] = computeSourcePsd(stc, sfreq, 8.0f, 30.0f);

        QVERIFY(freqs(0) >= 8.0);
        QVERIFY(freqs(freqs.size() - 1) <= 30.0);
    }

    void testSourcePsdEmpty()
    {
        InvSourceEstimate stc;
        auto [psd, freqs] = computeSourcePsd(stc, 256.0f);
        QCOMPARE(psd.size(), static_cast<Eigen::Index>(0));
    }

    void testSourcePsdPositive()
    {
        const int nSrc = 2;
        const int nTimes = 256;

        MatrixXd data = MatrixXd::Random(nSrc, nTimes);
        VectorXi verts(nSrc);
        verts << 0, 1;
        InvSourceEstimate stc(data, verts, 0.0f, 1.0f / 256.0f);

        auto [psd, freqs] = computeSourcePsd(stc, 256.0f);

        for (int i = 0; i < psd.rows(); ++i)
            for (int j = 0; j < psd.cols(); ++j)
                QVERIFY2(psd(i, j) >= 0.0, "PSD values must be non-negative");
    }

    //--- computeSourceBandPower tests ---

    void testBandPowerDimensions()
    {
        const int nSrc = 3;
        const int nTimes = 512;
        const float sfreq = 256.0f;

        MatrixXd data = MatrixXd::Random(nSrc, nTimes);
        VectorXi verts(nSrc);
        verts << 0, 1, 2;
        InvSourceEstimate stc(data, verts, 0.0f, 1.0f / sfreq);

        QMap<QString, QPair<float, float>> bands;
        bands["alpha"] = QPair<float, float>(8.0f, 13.0f);
        bands["beta"] = QPair<float, float>(13.0f, 30.0f);

        auto result = computeSourceBandPower(stc, sfreq, bands);

        QCOMPARE(result.size(), 2);
        QVERIFY(result.contains("alpha"));
        QVERIFY(result.contains("beta"));
        QCOMPARE(result["alpha"].size(), static_cast<Eigen::Index>(nSrc));
        QCOMPARE(result["beta"].size(), static_cast<Eigen::Index>(nSrc));
    }

    void testBandPowerPositive()
    {
        const int nSrc = 2;
        const int nTimes = 512;
        const float sfreq = 256.0f;

        MatrixXd data = MatrixXd::Random(nSrc, nTimes);
        VectorXi verts(nSrc);
        verts << 0, 1;
        InvSourceEstimate stc(data, verts, 0.0f, 1.0f / sfreq);

        QMap<QString, QPair<float, float>> bands;
        bands["theta"] = QPair<float, float>(4.0f, 8.0f);

        auto result = computeSourceBandPower(stc, sfreq, bands);

        for (int i = 0; i < result["theta"].size(); ++i) {
            QVERIFY2(result["theta"](i) >= 0.0, "Band power must be non-negative");
        }
    }

    void testBandPowerSineConcentration()
    {
        // A sine at 10 Hz: alpha band should have much more power than beta
        const int nSrc = 1;
        const int nTimes = 1024;
        const float sfreq = 256.0f;

        MatrixXd data(nSrc, nTimes);
        for (int t = 0; t < nTimes; ++t) {
            data(0, t) = std::sin(2.0 * M_PI * 10.0 * t / sfreq);
        }
        VectorXi verts(1);
        verts << 0;
        InvSourceEstimate stc(data, verts, 0.0f, 1.0f / sfreq);

        QMap<QString, QPair<float, float>> bands;
        bands["alpha"] = QPair<float, float>(8.0f, 13.0f);
        bands["beta"] = QPair<float, float>(20.0f, 30.0f);

        auto result = computeSourceBandPower(stc, sfreq, bands);

        QVERIFY2(result["alpha"](0) > result["beta"](0) * 10.0,
                 qPrintable(QString("Alpha=%1 should >> beta=%2")
                            .arg(result["alpha"](0)).arg(result["beta"](0))));
    }

    void testBandPowerEmpty()
    {
        InvSourceEstimate stc;
        QMap<QString, QPair<float, float>> bands;
        bands["alpha"] = QPair<float, float>(8.0f, 13.0f);

        auto result = computeSourceBandPower(stc, 256.0f, bands);
        QVERIFY(result.isEmpty());
    }

    void cleanupTestCase() {}
};

//=============================================================================================================

QTEST_GUILESS_MAIN(TestInvConvenience)
#include "test_inv_convenience.moc"
