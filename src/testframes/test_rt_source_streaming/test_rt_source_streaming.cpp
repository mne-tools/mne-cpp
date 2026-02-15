//=============================================================================================================
/**
 * @file     test_rt_source_streaming.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    Unit tests for RtSourceDataWorker and RtSourceDataController.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <workers/rtsourcedataworker.h>
#include <workers/rtsourcedatacontroller.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QSignalSpy>
#include <QVector>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BRAINVIEWLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestRtSourceStreaming
 *
 * @brief Unit tests for real-time source data streaming pipeline.
 */
class TestRtSourceStreaming : public QObject
{
    Q_OBJECT

public:
    TestRtSourceStreaming();

private slots:
    void initTestCase();
    void testWorkerAddDataAndQueue();
    void testWorkerStreamWithoutInterpolation();
    void testWorkerStreamWithIdentityInterpolation();
    void testWorkerThresholds();
    void testWorkerColormapSwitch();
    void testWorkerLooping();
    void testControllerLifecycle();
    void testControllerStreaming();
    void testControllerStreamingInterval();
    void cleanupTestCase();

private:
    /**
     * Build a simple identity-like sparse matrix (nVerts x nSources).
     * Each source maps to one vertex with weight 1.0.
     */
    QSharedPointer<SparseMatrix<float>> makeIdentityInterpMat(int nVerts, int nSources);

    int m_iNumSourcesLh = 10;     /**< Test LH source count. */
    int m_iNumSourcesRh = 8;      /**< Test RH source count. */
    int m_iNumVertsLh = 20;       /**< Test LH vertex count. */
    int m_iNumVertsRh = 16;       /**< Test RH vertex count. */
};

//=============================================================================================================

TestRtSourceStreaming::TestRtSourceStreaming()
{
}

//=============================================================================================================

void TestRtSourceStreaming::initTestCase()
{
    qDebug() << "TestRtSourceStreaming: starting tests with"
             << m_iNumSourcesLh << "LH sources," << m_iNumSourcesRh << "RH sources,"
             << m_iNumVertsLh << "LH verts," << m_iNumVertsRh << "RH verts.";
}

//=============================================================================================================

QSharedPointer<SparseMatrix<float>> TestRtSourceStreaming::makeIdentityInterpMat(int nVerts, int nSources)
{
    auto mat = QSharedPointer<SparseMatrix<float>>::create(nVerts, nSources);
    QVector<Triplet<float>> triplets;
    for (int s = 0; s < nSources && s < nVerts; ++s) {
        triplets.append(Triplet<float>(s, s, 1.0f));
    }
    mat->setFromTriplets(triplets.begin(), triplets.end());
    return mat;
}

//=============================================================================================================

void TestRtSourceStreaming::testWorkerAddDataAndQueue()
{
    RtSourceDataWorker worker;

    // Add several data vectors
    int totalSources = m_iNumSourcesLh + m_iNumSourcesRh;
    VectorXd data1 = VectorXd::Constant(totalSources, 0.5);
    VectorXd data2 = VectorXd::Constant(totalSources, 1.0);
    VectorXd data3 = VectorXd::Random(totalSources);

    worker.addData(data1);
    worker.addData(data2);
    worker.addData(data3);

    // streamData without interpolation matrices should still work (emit nothing or handle gracefully)
    QSignalSpy spy(&worker, &RtSourceDataWorker::newRtSmoothedData);
    worker.streamData();

    // Without interpolation matrices set, we don't expect color output
    // but the worker should not crash
    QVERIFY2(true, "Worker did not crash when streaming without interpolation matrices.");
}

//=============================================================================================================

void TestRtSourceStreaming::testWorkerStreamWithoutInterpolation()
{
    RtSourceDataWorker worker;

    int totalSources = m_iNumSourcesLh + m_iNumSourcesRh;
    worker.addData(VectorXd::Constant(totalSources, 0.3));

    QSignalSpy spy(&worker, &RtSourceDataWorker::newRtSmoothedData);
    worker.streamData();

    // Without interpolation matrices, the signal should not be emitted
    // (or emitted with empty arrays) — just verify no crash
    QVERIFY(spy.count() >= 0);
}

//=============================================================================================================

void TestRtSourceStreaming::testWorkerStreamWithIdentityInterpolation()
{
    RtSourceDataWorker worker;

    // Set identity interpolation matrices
    auto interpLh = makeIdentityInterpMat(m_iNumVertsLh, m_iNumSourcesLh);
    auto interpRh = makeIdentityInterpMat(m_iNumVertsRh, m_iNumSourcesRh);
    worker.setInterpolationMatrixLeft(interpLh);
    worker.setInterpolationMatrixRight(interpRh);

    // Set thresholds so values map nicely
    worker.setThresholds(0.0, 0.5, 1.0);

    // Add data: all sources at 0.8
    int totalSources = m_iNumSourcesLh + m_iNumSourcesRh;
    VectorXd data = VectorXd::Constant(totalSources, 0.8);
    worker.addData(data);

    // Stream
    QSignalSpy spy(&worker, &RtSourceDataWorker::newRtSmoothedData);
    worker.streamData();

    // Should have emitted exactly one frame
    QCOMPARE(spy.count(), 1);

    // Extract arguments
    QList<QVariant> args = spy.takeFirst();
    QVector<uint32_t> colorsLh = args.at(0).value<QVector<uint32_t>>();
    QVector<uint32_t> colorsRh = args.at(1).value<QVector<uint32_t>>();

    // Color arrays should match vertex counts
    QCOMPARE(colorsLh.size(), m_iNumVertsLh);
    QCOMPARE(colorsRh.size(), m_iNumVertsRh);

    // Source-mapped vertices (first nSources) should have non-zero alpha (not fully transparent)
    for (int i = 0; i < m_iNumSourcesLh; ++i) {
        uint32_t c = colorsLh[i];
        uint8_t alpha = (c >> 24) & 0xFF;
        QVERIFY2(alpha > 0, qPrintable(QString("LH vertex %1 has zero alpha").arg(i)));
    }
    for (int i = 0; i < m_iNumSourcesRh; ++i) {
        uint32_t c = colorsRh[i];
        uint8_t alpha = (c >> 24) & 0xFF;
        QVERIFY2(alpha > 0, qPrintable(QString("RH vertex %1 has zero alpha").arg(i)));
    }
}

//=============================================================================================================

void TestRtSourceStreaming::testWorkerThresholds()
{
    RtSourceDataWorker worker;

    auto interpLh = makeIdentityInterpMat(m_iNumVertsLh, m_iNumSourcesLh);
    auto interpRh = makeIdentityInterpMat(m_iNumVertsRh, m_iNumSourcesRh);
    worker.setInterpolationMatrixLeft(interpLh);
    worker.setInterpolationMatrixRight(interpRh);

    // Set tight threshold: only values > 0.9 should be visible
    worker.setThresholds(0.9, 0.95, 1.0);

    // Add data with value below threshold
    int totalSources = m_iNumSourcesLh + m_iNumSourcesRh;
    VectorXd dataLow = VectorXd::Constant(totalSources, 0.1);
    worker.addData(dataLow);

    QSignalSpy spy(&worker, &RtSourceDataWorker::newRtSmoothedData);
    worker.streamData();

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QVector<uint32_t> colorsLh = args.at(0).value<QVector<uint32_t>>();

    // Values below min threshold should be transparent (alpha = 0)
    for (int i = 0; i < m_iNumSourcesLh; ++i) {
        uint32_t c = colorsLh[i];
        uint8_t alpha = (c >> 24) & 0xFF;
        QCOMPARE(alpha, (uint8_t)0);
    }

    // Now test with value above threshold
    VectorXd dataHigh = VectorXd::Constant(totalSources, 0.98);
    worker.addData(dataHigh);

    QSignalSpy spy2(&worker, &RtSourceDataWorker::newRtSmoothedData);
    worker.streamData();

    QCOMPARE(spy2.count(), 1);
    QList<QVariant> args2 = spy2.takeFirst();
    QVector<uint32_t> colorsLh2 = args2.at(0).value<QVector<uint32_t>>();

    for (int i = 0; i < m_iNumSourcesLh; ++i) {
        uint32_t c = colorsLh2[i];
        uint8_t alpha = (c >> 24) & 0xFF;
        QVERIFY2(alpha > 0, qPrintable(QString("LH vertex %1 should be visible above threshold").arg(i)));
    }
}

//=============================================================================================================

void TestRtSourceStreaming::testWorkerColormapSwitch()
{
    RtSourceDataWorker worker;

    auto interpLh = makeIdentityInterpMat(m_iNumVertsLh, m_iNumSourcesLh);
    auto interpRh = makeIdentityInterpMat(m_iNumVertsRh, m_iNumSourcesRh);
    worker.setInterpolationMatrixLeft(interpLh);
    worker.setInterpolationMatrixRight(interpRh);
    worker.setThresholds(0.0, 0.5, 1.0);

    int totalSources = m_iNumSourcesLh + m_iNumSourcesRh;
    VectorXd data = VectorXd::Constant(totalSources, 0.7);

    // Get colors with "Hot" colormap
    worker.setColormapType("Hot");
    worker.addData(data);
    QSignalSpy spyHot(&worker, &RtSourceDataWorker::newRtSmoothedData);
    worker.streamData();
    QCOMPARE(spyHot.count(), 1);
    QVector<uint32_t> colorsHot = spyHot.takeFirst().at(0).value<QVector<uint32_t>>();

    // Get colors with "Jet" colormap
    worker.setColormapType("Jet");
    worker.addData(data);
    QSignalSpy spyJet(&worker, &RtSourceDataWorker::newRtSmoothedData);
    worker.streamData();
    QCOMPARE(spyJet.count(), 1);
    QVector<uint32_t> colorsJet = spyJet.takeFirst().at(0).value<QVector<uint32_t>>();

    // Colors should differ between colormaps (at least for the first source-mapped vertex)
    bool bDiffer = false;
    for (int i = 0; i < m_iNumSourcesLh; ++i) {
        if (colorsHot[i] != colorsJet[i]) {
            bDiffer = true;
            break;
        }
    }
    QVERIFY2(bDiffer, "Hot and Jet colormaps should produce different colors for the same input.");
}

//=============================================================================================================

void TestRtSourceStreaming::testWorkerLooping()
{
    RtSourceDataWorker worker;

    auto interpLh = makeIdentityInterpMat(m_iNumVertsLh, m_iNumSourcesLh);
    auto interpRh = makeIdentityInterpMat(m_iNumVertsRh, m_iNumSourcesRh);
    worker.setInterpolationMatrixLeft(interpLh);
    worker.setInterpolationMatrixRight(interpRh);
    worker.setThresholds(0.0, 0.5, 1.0);
    worker.setLoopState(true);

    int totalSources = m_iNumSourcesLh + m_iNumSourcesRh;
    VectorXd data = VectorXd::Constant(totalSources, 0.6);
    worker.addData(data);

    QSignalSpy spy(&worker, &RtSourceDataWorker::newRtSmoothedData);

    // Stream multiple times — looping should allow replaying the same data
    for (int i = 0; i < 5; ++i) {
        worker.streamData();
    }

    // With looping enabled, all 5 calls should succeed and emit data
    QVERIFY2(spy.count() == 5,
             qPrintable(QString("Expected 5 emissions with looping, got %1").arg(spy.count())));
}

//=============================================================================================================

void TestRtSourceStreaming::testControllerLifecycle()
{
    // Test that a controller can be created and destroyed without issues
    {
        RtSourceDataController controller;
        QVERIFY(!controller.isStreaming());

        controller.setStreamingState(true);
        QVERIFY(controller.isStreaming());

        controller.setStreamingState(false);
        QVERIFY(!controller.isStreaming());
    }
    // Destructor runs here — should not crash
    QVERIFY2(true, "Controller lifecycle (create/start/stop/destroy) completed without crash.");
}

//=============================================================================================================

void TestRtSourceStreaming::testControllerStreaming()
{
    RtSourceDataController controller;

    auto interpLh = makeIdentityInterpMat(m_iNumVertsLh, m_iNumSourcesLh);
    auto interpRh = makeIdentityInterpMat(m_iNumVertsRh, m_iNumSourcesRh);
    controller.setInterpolationMatrixLeft(interpLh);
    controller.setInterpolationMatrixRight(interpRh);
    controller.setThresholds(0.0, 0.5, 1.0);
    controller.setLoopState(true);

    // Push data
    int totalSources = m_iNumSourcesLh + m_iNumSourcesRh;
    for (int i = 0; i < 10; ++i) {
        VectorXd data = VectorXd::Constant(totalSources, 0.5 + 0.05 * i);
        controller.addData(data);
    }

    // Set up spy for output signal
    QSignalSpy spy(&controller, &RtSourceDataController::newSmoothedDataAvailable);

    // Start streaming with a fast interval
    controller.setTimeInterval(10);  // 10ms
    controller.setStreamingState(true);

    // Wait for some frames to be produced
    // Use a short event loop wait
    QTest::qWait(200);  // 200ms should give ~20 timer ticks

    controller.setStreamingState(false);

    // We should have received at least a few color frames
    QVERIFY2(spy.count() >= 3,
             qPrintable(QString("Expected at least 3 streaming frames, got %1").arg(spy.count())));

    // Verify first frame has correct structure
    QList<QVariant> firstArgs = spy.first();
    QVector<uint32_t> colorsLh = firstArgs.at(0).value<QVector<uint32_t>>();
    QVector<uint32_t> colorsRh = firstArgs.at(1).value<QVector<uint32_t>>();
    QCOMPARE(colorsLh.size(), m_iNumVertsLh);
    QCOMPARE(colorsRh.size(), m_iNumVertsRh);
}

//=============================================================================================================

void TestRtSourceStreaming::testControllerStreamingInterval()
{
    RtSourceDataController controller;

    auto interpLh = makeIdentityInterpMat(m_iNumVertsLh, m_iNumSourcesLh);
    auto interpRh = makeIdentityInterpMat(m_iNumVertsRh, m_iNumSourcesRh);
    controller.setInterpolationMatrixLeft(interpLh);
    controller.setInterpolationMatrixRight(interpRh);
    controller.setThresholds(0.0, 0.5, 1.0);
    controller.setLoopState(true);

    int totalSources = m_iNumSourcesLh + m_iNumSourcesRh;
    for (int i = 0; i < 5; ++i) {
        controller.addData(VectorXd::Constant(totalSources, 0.7));
    }

    // Stream at 50ms interval — expect fewer frames than 10ms
    QSignalSpy spy50(&controller, &RtSourceDataController::newSmoothedDataAvailable);
    controller.setTimeInterval(50);
    controller.setStreamingState(true);
    QTest::qWait(300);
    controller.setStreamingState(false);
    int count50 = spy50.count();

    // Re-add data for second run
    controller.clearData();
    for (int i = 0; i < 5; ++i) {
        controller.addData(VectorXd::Constant(totalSources, 0.7));
    }

    // Stream at 10ms interval — expect more frames
    QSignalSpy spy10(&controller, &RtSourceDataController::newSmoothedDataAvailable);
    controller.setTimeInterval(10);
    controller.setStreamingState(true);
    QTest::qWait(300);
    controller.setStreamingState(false);
    int count10 = spy10.count();

    qDebug() << "Frames at 50ms interval:" << count50 << ", at 10ms interval:" << count10;
    // 10ms interval should produce more frames than 50ms
    QVERIFY2(count10 > count50,
             qPrintable(QString("Expected more frames at 10ms (%1) than 50ms (%2)").arg(count10).arg(count50)));
}

//=============================================================================================================

void TestRtSourceStreaming::cleanupTestCase()
{
    qDebug() << "TestRtSourceStreaming: all tests completed.";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestRtSourceStreaming)
#include "test_rt_source_streaming.moc"
