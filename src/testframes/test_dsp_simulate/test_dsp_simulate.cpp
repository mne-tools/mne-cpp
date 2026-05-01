//=============================================================================================================
/**
 * @file     test_dsp_simulate.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
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
    void initTestCase() {}

    //--- simulateStc tests ---

    void testSimulateStcDimensions()
    {
        VectorXi allVerts(20);
        for (int i = 0; i < 20; ++i) allVerts(i) = i * 100;

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
        for (int i = 0; i < 10; ++i) allVerts(i) = i;

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
        for (int i = 0; i < 5; ++i) allVerts(i) = i;
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
        for (int i = 0; i < 5; ++i) allVerts(i) = i;
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
        for (int i = 0; i < nSrc; ++i) verts(i) = i;
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
        for (int i = 0; i < nSrc; ++i) verts(i) = i;
        MatrixXd srcData = MatrixXd::Zero(nSrc, nTimes);
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

        // Identity noise covariance
        FiffCov noiseCov;
        noiseCov.dim = nChan;
        noiseCov.data = MatrixXd::Identity(nChan, nChan) * 1e-24;
        noiseCov.kind = 1;

        FiffEvoked evoked = simulateEvoked(fwd, stc, info, noiseCov, 1, 42);

        // Source is zero, so evoked should be purely noise
        QVERIFY(evoked.data.norm() > 0.0); // noise should be non-zero
        QCOMPARE(evoked.nave, 1);
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

    void cleanupTestCase() {}
};

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspSimulate)
#include "test_dsp_simulate.moc"
