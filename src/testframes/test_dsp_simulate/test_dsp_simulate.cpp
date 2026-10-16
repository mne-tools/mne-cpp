//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_simulate.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Tests for simulation utilities.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/simulate.h>
#include <inv/inv_source_estimate.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_constants.h>
#include <mne/mne_forward_solution.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace INVLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================

class TestDspSimulate : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
    }

    //--- simulateStc tests ---

    void testSimulateStcDimensions()
    {
        VectorXi allVerts(20);
        for (int i = 0; i < 20; ++i)
            allVerts(i) = i * 100;

        VectorXi activeVerts(3);
        activeVerts << 0, 500, 1000; // vertices 0, 5, 10

        SimulateStcParams params;
        params.sfreq = 1000.0f;
        params.tmin = 0.0f;
        params.duration = 0.5f;

        InvSourceEstimate stc = simulateStc(activeVerts, allVerts, params);

        QVERIFY(!stc.isEmpty());
        QCOMPARE(stc.data.rows(), static_cast<Eigen::Index>(20));
        QCOMPARE(stc.data.cols(), static_cast<Eigen::Index>(500));
        QCOMPARE(stc.vertices.size(), static_cast<Eigen::Index>(20));
    }

    void testSimulateStcActiveVerticesNonZero()
    {
        VectorXi allVerts(10);
        for (int i = 0; i < 10; ++i)
            allVerts(i) = i;

        VectorXi activeVerts(2);
        activeVerts << 3, 7;

        SimulateStcParams params;
        params.sfreq = 100.0f;
        params.duration = 1.0f;

        InvSourceEstimate stc = simulateStc(activeVerts, allVerts, params);

        QVERIFY(!stc.isEmpty());

        // Active vertices should have non-zero data
        QVERIFY(stc.data.row(3).norm() > 0.0);
        QVERIFY(stc.data.row(7).norm() > 0.0);

        // Inactive vertices should be zero
        QVERIFY(stc.data.row(0).norm() < 1e-15);
        QVERIFY(stc.data.row(1).norm() < 1e-15);
        QVERIFY(stc.data.row(5).norm() < 1e-15);
    }

    void testSimulateStcReproducibility()
    {
        VectorXi allVerts(5);
        for (int i = 0; i < 5; ++i)
            allVerts(i) = i;
        VectorXi activeVerts(1);
        activeVerts << 2;

        SimulateStcParams params;
        params.seed = 123;
        params.sfreq = 100.0f;
        params.duration = 0.1f;

        InvSourceEstimate stc1 = simulateStc(activeVerts, allVerts, params);
        InvSourceEstimate stc2 = simulateStc(activeVerts, allVerts, params);

        QVERIFY((stc1.data - stc2.data).norm() < 1e-15);
    }

    void testSimulateStcInvalidVertex()
    {
        VectorXi allVerts(3);
        allVerts << 0, 1, 2;
        VectorXi activeVerts(1);
        activeVerts << 999; // not in allVerts

        InvSourceEstimate stc = simulateStc(activeVerts, allVerts);
        QVERIFY(stc.isEmpty());
    }

    //--- simulateStcFromWaveforms tests ---

    void testSimulateFromWaveforms()
    {
        VectorXi allVerts(5);
        for (int i = 0; i < 5; ++i)
            allVerts(i) = i;
        VectorXi activeVerts(2);
        activeVerts << 1, 3;

        MatrixXd waveforms(2, 100);
        waveforms.row(0) = VectorXd::Ones(100).transpose();
        waveforms.row(1) = VectorXd::Ones(100).transpose() * 2.0;

        InvSourceEstimate stc = simulateStcFromWaveforms(waveforms, activeVerts, allVerts, 0.0f, 0.001f);

        QVERIFY(!stc.isEmpty());
        QCOMPARE(stc.data.rows(), static_cast<Eigen::Index>(5));
        QCOMPARE(stc.data.cols(), static_cast<Eigen::Index>(100));

        // Check waveforms placed correctly
        QVERIFY(std::abs(stc.data(1, 50) - 1.0) < 1e-10);
        QVERIFY(std::abs(stc.data(3, 50) - 2.0) < 1e-10);
        QVERIFY(std::abs(stc.data(0, 50)) < 1e-15);
    }

    void testSimulateFromWaveformsMismatch()
    {
        VectorXi allVerts(3);
        allVerts << 0, 1, 2;
        VectorXi activeVerts(2);
        activeVerts << 0, 1;

        MatrixXd waveforms(3, 10); // 3 rows but only 2 active verts

        InvSourceEstimate stc = simulateStcFromWaveforms(waveforms, activeVerts, allVerts);
        QVERIFY(stc.isEmpty());
    }

    //--- simulateEvoked tests ---

    void testSimulateEvokedNoiselessDimensions()
    {
        // Create a minimal forward solution
        const int nChan = 10;
        const int nSrc = 5;
        const int nTimes = 50;

        MNEForwardSolution fwd;
        fwd.sol = new FiffNamedMatrix();
        fwd.sol->data = MatrixXd::Random(nChan, nSrc);

        // Create source estimate
        VectorXi verts(nSrc);
        for (int i = 0; i < nSrc; ++i)
            verts(i) = i;
        MatrixXd srcData = MatrixXd::Ones(nSrc, nTimes);
        InvSourceEstimate stc(srcData, verts, 0.0f, 0.001f);

        // Create info
        FiffInfo info;
        info.sfreq = 1000.0;
        info.nchan = nChan;
        for (int i = 0; i < nChan; ++i) {
            FiffChInfo ch;
            ch.ch_name = QString("CH%1").arg(i);
            info.chs.append(ch);
            info.ch_names.append(ch.ch_name);
        }

        FiffEvoked evoked = simulateEvokedNoiseless(fwd, stc, info);

        QCOMPARE(evoked.data.rows(), static_cast<Eigen::Index>(nChan));
        QCOMPARE(evoked.data.cols(), static_cast<Eigen::Index>(nTimes));
        QCOMPARE(evoked.times.size(), static_cast<Eigen::Index>(nTimes));
    }

    void testSimulateEvokedForwardProjection()
    {
        // Verify data = G * stc
        const int nChan = 3;
        const int nSrc = 2;
        const int nTimes = 4;

        MNEForwardSolution fwd;
        fwd.sol = new FiffNamedMatrix();
        fwd.sol->data = MatrixXd::Identity(nChan, nSrc).block(0, 0, nChan, nSrc);
        // G = [[1,0],[0,1],[0,0]]

        VectorXi verts(nSrc);
        verts << 0, 1;
        MatrixXd srcData(nSrc, nTimes);
        srcData << 1, 2, 3, 4,
            5, 6, 7, 8;
        InvSourceEstimate stc(srcData, verts, 0.0f, 0.001f);

        FiffInfo info;
        info.sfreq = 1000.0;
        info.nchan = nChan;
        for (int i = 0; i < nChan; ++i) {
            FiffChInfo ch;
            ch.ch_name = QString("CH%1").arg(i);
            info.chs.append(ch);
            info.ch_names.append(ch.ch_name);
        }

        FiffEvoked evoked = simulateEvokedNoiseless(fwd, stc, info);

        // Row 0 should equal source 0 data
        QVERIFY(std::abs(evoked.data(0, 0) - 1.0) < 1e-10);
        QVERIFY(std::abs(evoked.data(0, 3) - 4.0) < 1e-10);
        // Row 1 should equal source 1 data
        QVERIFY(std::abs(evoked.data(1, 0) - 5.0) < 1e-10);
        // Row 2 should be zero (no gain)
        QVERIFY(std::abs(evoked.data(2, 0)) < 1e-10);
    }

    void testSimulateEvokedWithNoise()
    {
        const int nChan = 5;
        const int nSrc = 3;
        const int nTimes = 100;

        MNEForwardSolution fwd;
        fwd.sol = new FiffNamedMatrix();
        fwd.sol->data = MatrixXd::Random(nChan, nSrc) * 0.1;

        VectorXi verts(nSrc);
        for (int i = 0; i < nSrc; ++i)
            verts(i) = i;
        MatrixXd srcData = MatrixXd::Zero(nSrc, nTimes);
        InvSourceEstimate stc(srcData, verts, 0.0f, 0.001f);

        FiffInfo info;
        info.sfreq = 1000.0;
        info.nchan = nChan;
        for (int i = 0; i < nChan; ++i) {
            FiffChInfo ch;
            ch.ch_name = QString("CH%1").arg(i);
            ch.kind = FIFFV_EEG_CH;
            info.chs.append(ch);
            info.ch_names.append(ch.ch_name);
        }

        // Identity noise covariance
        FiffCov noiseCov;
        noiseCov.dim = nChan;
        noiseCov.names = info.ch_names;
        noiseCov.data = MatrixXd::Identity(nChan, nChan) * 1e-24;
        noiseCov.kind = 1;

        FiffEvoked evoked = simulateEvoked(fwd, stc, info, noiseCov, 1, 42);

        // Source is zero, so evoked should be purely noise
        QVERIFY(evoked.data.norm() > 0.0); // noise should be non-zero
        QCOMPARE(evoked.nave, 1);
    }

    /**
     * Reference values produced by mne.cov.compute_whitener(cov, info, pca=True, return_colorer=True) (mne 1.11),
     * the colourer mne.simulation.simulate_evoked draws its noise with: colorer @ colorer.T / outer(scale, scale)
     * for channels M1-M3 (mag) and E1-E2 (eeg), C = (A^T A) * outer(scale, scale), scale 1e-13 / 1e-6.
     * MEG and EEG noise are independent; the noise is divided by sqrt(nave).
     */
    void testSimulateEvokedNoiseMatchesMnePython()
    {
        const int nChan = 5;
        const int nTimes = 40000;
        const int nave = 4;
        MatrixXd a(5, 5);
        a << 1.0, 0.3, 0.1, 0.5, 0.2, 0.0, 1.0, 0.2, 0.1, 0.4, 0.0, 0.0, 1.0, 0.3, 0.1, 0.0, 0.0, 0.0, 1.0, 0.6, 0.0, 0.0, 0.0, 0.0, 1.0;
        MatrixXd expected(5, 5);
        expected << 1.0, 0.3, 0.1, 0.0, 0.0, 0.3, 1.09, 0.23, 0.0, 0.0, 0.1, 0.23, 1.05, 0.0, 0.0, 0.0, 0.0, 0.0, 1.35, 0.77, 0.0, 0.0, 0.0, 0.77, 1.57;
        const VectorXd scale = (VectorXd(5) << 1e-13, 1e-13, 1e-13, 1e-6, 1e-6).finished();

        FiffInfo info;
        info.sfreq = 1000.0;
        const QStringList names{"M1", "M2", "M3", "E1", "E2"};
        for (int i = 0; i < nChan; ++i) {
            FiffChInfo ch;
            ch.ch_name = names[i];
            ch.kind = i < 3 ? FIFFV_MEG_CH : FIFFV_EEG_CH;
            ch.unit = i < 3 ? FIFF_UNIT_T : FIFF_UNIT_V;
            info.chs.append(ch);
            info.ch_names.append(names[i]);
        }
        info.nchan = nChan;

        FiffCov cov;
        cov.kind = FIFFV_MNE_NOISE_COV;
        cov.dim = nChan;
        cov.names = names;
        cov.data = (a.transpose() * a).cwiseProduct(scale * scale.transpose());

        MNEForwardSolution fwd;
        fwd.sol = new FiffNamedMatrix();
        fwd.sol->data = MatrixXd::Zero(nChan, 1);
        const InvSourceEstimate stc(MatrixXd::Zero(1, nTimes), VectorXi::Zero(1), 0.0f, 0.001f);

        const FiffEvoked evoked = simulateEvoked(fwd, stc, info, cov, nave, 7);
        QCOMPARE(evoked.data.rows(), Eigen::Index(nChan));
        const MatrixXd empirical = (evoked.data * evoked.data.transpose() / nTimes * nave).cwiseQuotient(scale * scale.transpose());
        QVERIFY2((empirical - expected).cwiseAbs().maxCoeff() < 0.05,
                 qPrintable(QString("max deviation %1, M1-E1 %2").arg((empirical - expected).cwiseAbs().maxCoeff()).arg(empirical(0, 3))));
    }

    void testSimulateEvokedEmptyForward()
    {
        MNEForwardSolution fwd;
        InvSourceEstimate stc;
        FiffInfo info;
        FiffCov cov;

        FiffEvoked evoked = simulateEvoked(fwd, stc, info, cov);
        QCOMPARE(evoked.data.size(), static_cast<Eigen::Index>(0));
    }

    void cleanupTestCase()
    {
    }
};

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspSimulate)
#include "test_dsp_simulate.moc"
