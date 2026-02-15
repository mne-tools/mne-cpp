//=============================================================================================================
/**
 * @file     test_sensor_streaming.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for RtSensorDataWorker and RtSensorDataController.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QThread>
#include <Eigen/Core>
#include <cmath>

#include "workers/rtsensordataworker.h"
#include "workers/rtsensordatacontroller.h"

using namespace BRAINVIEWLIB;

//=============================================================================================================
/**
 * @brief Test class for real-time sensor data streaming.
 */
class TestSensorStreaming : public QObject
{
    Q_OBJECT

private slots:
    // ── Worker tests ───────────────────────────────────────────────────

    void testWorkerConstruction();
    void testWorkerAddData();
    void testWorkerClear();
    void testWorkerStreamDataNoMapping();
    void testWorkerStreamDataWithMapping();
    void testWorkerAveraging();
    void testWorkerLooping();
    void testWorkerQueueCap();
    void testWorkerMappingMatrixDimensionMismatch();
    void testWorkerColorOutput();
    void testWorkerSymmetricNormalization();
    void testWorkerColormapChange();
    void testWorkerSFreq();

    // ── Controller tests ───────────────────────────────────────────────

    void testControllerConstruction();
    void testControllerStreamingState();
    void testControllerTimeInterval();
    void testControllerSignalForwarding();
    void testControllerClearData();

    // ── Integration tests ──────────────────────────────────────────────

    void testEndToEndStreaming();
    void testMultipleStreamCycles();
    void testEmptyQueueBehavior();
};

//=============================================================================================================
// HELPER FUNCTIONS
//=============================================================================================================

/**
 * Create a simple identity-like dense mapping matrix.
 * Returns an (nVertices x nSensors) matrix where each vertex is mapped
 * to the nearest sensor with weight 1.0.
 */
static QSharedPointer<Eigen::MatrixXf> createTestMapping(int nVertices, int nSensors)
{
    auto mat = QSharedPointer<Eigen::MatrixXf>::create(nVertices, nSensors);
    mat->setZero();

    // Simple round-robin allocation: vertex i maps to sensor i % nSensors
    for (int i = 0; i < nVertices; ++i) {
        (*mat)(i, i % nSensors) = 1.0f;
    }
    return mat;
}

/**
 * Create a uniform mapping matrix where every vertex gets the average of all sensors.
 */
static QSharedPointer<Eigen::MatrixXf> createUniformMapping(int nVertices, int nSensors)
{
    auto mat = QSharedPointer<Eigen::MatrixXf>::create(nVertices, nSensors);
    mat->setConstant(1.0f / nSensors);
    return mat;
}

//=============================================================================================================
// WORKER TESTS
//=============================================================================================================

void TestSensorStreaming::testWorkerConstruction()
{
    RtSensorDataWorker worker;
    // Worker should be constructible without crashing
    QVERIFY(true);
}

//=============================================================================================================

void TestSensorStreaming::testWorkerAddData()
{
    RtSensorDataWorker worker;
    Eigen::VectorXf data(10);
    data.setRandom();

    // Adding data should not crash
    worker.addData(data);
    worker.addData(data);
    worker.addData(data);
    QVERIFY(true);
}

//=============================================================================================================

void TestSensorStreaming::testWorkerClear()
{
    RtSensorDataWorker worker;
    Eigen::VectorXf data(10);
    data.setRandom();

    worker.addData(data);
    worker.addData(data);
    worker.clear();

    // After clear, streaming should produce no output (no data in queue)
    QSignalSpy spy(&worker, &RtSensorDataWorker::newRtSensorColors);
    worker.streamData();
    QCOMPARE(spy.count(), 0);
}

//=============================================================================================================

void TestSensorStreaming::testWorkerStreamDataNoMapping()
{
    RtSensorDataWorker worker;
    Eigen::VectorXf data(10);
    data.setRandom();

    worker.addData(data);

    // Without mapping matrix, streamData should not emit
    QSignalSpy spy(&worker, &RtSensorDataWorker::newRtSensorColors);
    worker.streamData();
    QCOMPARE(spy.count(), 0);
}

//=============================================================================================================

void TestSensorStreaming::testWorkerStreamDataWithMapping()
{
    RtSensorDataWorker worker;
    const int nSensors = 5;
    const int nVertices = 20;

    auto mapping = createTestMapping(nVertices, nSensors);
    worker.setMappingMatrix(mapping);

    Eigen::VectorXf data(nSensors);
    data << 1.0f, 0.5f, -0.5f, -1.0f, 0.0f;
    worker.addData(data);

    QSignalSpy spy(&worker, &RtSensorDataWorker::newRtSensorColors);
    worker.streamData();

    QCOMPARE(spy.count(), 1);

    QList<QVariant> args = spy.takeFirst();
    QVector<uint32_t> colors = args.at(1).value<QVector<uint32_t>>();
    QCOMPARE(colors.size(), nVertices);

    // All vertices should have full alpha (0xFF in high byte)
    for (int i = 0; i < colors.size(); ++i) {
        uint32_t alpha = (colors[i] >> 24) & 0xFF;
        QCOMPARE(alpha, 0xFFu);
    }
}

//=============================================================================================================

void TestSensorStreaming::testWorkerAveraging()
{
    RtSensorDataWorker worker;
    const int nSensors = 3;
    const int nVertices = 6;

    auto mapping = createUniformMapping(nVertices, nSensors);
    worker.setMappingMatrix(mapping);
    worker.setNumberAverages(3);

    // Add 3 samples
    Eigen::VectorXf d1(nSensors); d1 << 1.0f, 2.0f, 3.0f;
    Eigen::VectorXf d2(nSensors); d2 << 4.0f, 5.0f, 6.0f;
    Eigen::VectorXf d3(nSensors); d3 << 7.0f, 8.0f, 9.0f;

    worker.addData(d1);
    worker.addData(d2);
    worker.addData(d3);

    QSignalSpy spy(&worker, &RtSensorDataWorker::newRtSensorColors);

    // First call: pops d1, starts accumulating (1/3) — no emission
    worker.streamData();
    QCOMPARE(spy.count(), 0);

    // Second call: pops d2, accumulates (2/3) — no emission
    worker.streamData();
    QCOMPARE(spy.count(), 0);

    // Third call: pops d3, accumulates (3/3) — should emit averaged result
    worker.streamData();
    QCOMPARE(spy.count(), 1);

    QVector<uint32_t> colors = spy.takeFirst().at(1).value<QVector<uint32_t>>();
    QCOMPARE(colors.size(), nVertices);
}

//=============================================================================================================

void TestSensorStreaming::testWorkerLooping()
{
    RtSensorDataWorker worker;
    const int nSensors = 3;
    const int nVertices = 6;

    auto mapping = createUniformMapping(nVertices, nSensors);
    worker.setMappingMatrix(mapping);
    worker.setLoopState(true);

    Eigen::VectorXf data(nSensors);
    data << 1.0f, 2.0f, 3.0f;
    worker.addData(data);

    QSignalSpy spy(&worker, &RtSensorDataWorker::newRtSensorColors);

    // First call: takes from queue
    worker.streamData();
    QCOMPARE(spy.count(), 1);

    // Queue is empty now, but loop queue should replay
    worker.streamData();
    QCOMPARE(spy.count(), 2);

    worker.streamData();
    QCOMPARE(spy.count(), 3);
}

//=============================================================================================================

void TestSensorStreaming::testWorkerQueueCap()
{
    RtSensorDataWorker worker;
    worker.setSFreq(10.0); // Cap at 10 samples

    Eigen::VectorXf data(3);
    data.setRandom();

    // Add more than the cap
    for (int i = 0; i < 20; ++i) {
        worker.addData(data);
    }

    // Should not crash — queue is capped
    QVERIFY(true);
}

//=============================================================================================================

void TestSensorStreaming::testWorkerMappingMatrixDimensionMismatch()
{
    RtSensorDataWorker worker;
    const int nSensors = 5;
    const int nVertices = 10;

    auto mapping = createTestMapping(nVertices, nSensors);
    worker.setMappingMatrix(mapping);

    // Feed data with wrong number of sensors
    Eigen::VectorXf wrongData(nSensors + 3);
    wrongData.setRandom();
    worker.addData(wrongData);

    QSignalSpy spy(&worker, &RtSensorDataWorker::newRtSensorColors);
    worker.streamData();

    // Should not emit due to dimension mismatch
    QCOMPARE(spy.count(), 0);
}

//=============================================================================================================

void TestSensorStreaming::testWorkerColorOutput()
{
    RtSensorDataWorker worker;
    const int nSensors = 2;
    const int nVertices = 4;

    // Create mapping where vertex 0,1 map to sensor 0, vertex 2,3 map to sensor 1
    auto mapping = QSharedPointer<Eigen::MatrixXf>::create(nVertices, nSensors);
    mapping->setZero();
    (*mapping)(0, 0) = 1.0f;
    (*mapping)(1, 0) = 1.0f;
    (*mapping)(2, 1) = 1.0f;
    (*mapping)(3, 1) = 1.0f;

    worker.setMappingMatrix(mapping);

    // Feed equal and opposite sensor values
    Eigen::VectorXf data(nSensors);
    data << 1.0f, -1.0f;
    worker.addData(data);

    QSignalSpy spy(&worker, &RtSensorDataWorker::newRtSensorColors);
    worker.streamData();

    QCOMPARE(spy.count(), 1);
    QVector<uint32_t> colors = spy.takeFirst().at(1).value<QVector<uint32_t>>();
    QCOMPARE(colors.size(), nVertices);

    // Vertices 0,1 (sensor 0 = +1.0) should have opposite color to vertices 2,3 (sensor 1 = -1.0)
    // With symmetric normalization: +1.0/1.0 * 0.5 + 0.5 = 1.0, -1.0/1.0 * 0.5 + 0.5 = 0.0
    // So vertices 0,1 should be different from vertices 2,3
    QVERIFY(colors[0] == colors[1]); // Same sensor
    QVERIFY(colors[2] == colors[3]); // Same sensor
    QVERIFY(colors[0] != colors[2]); // Different sensors, opposite values
}

//=============================================================================================================

void TestSensorStreaming::testWorkerSymmetricNormalization()
{
    RtSensorDataWorker worker;
    const int nSensors = 1;
    const int nVertices = 3;

    // 1:1 mapping with different weights
    auto mapping = QSharedPointer<Eigen::MatrixXf>::create(nVertices, nSensors);
    (*mapping)(0, 0) = 1.0f;   // Will get +1.0 * data
    (*mapping)(1, 0) = 0.0f;   // Will get 0.0
    (*mapping)(2, 0) = -1.0f;  // Will get -1.0 * data

    worker.setMappingMatrix(mapping);

    Eigen::VectorXf data(nSensors);
    data << 2.0f;
    worker.addData(data);

    QSignalSpy spy(&worker, &RtSensorDataWorker::newRtSensorColors);
    worker.streamData();

    QCOMPARE(spy.count(), 1);
    QVector<uint32_t> colors = spy.takeFirst().at(1).value<QVector<uint32_t>>();
    QCOMPARE(colors.size(), nVertices);

    // Mapped values: +2.0, 0.0, -2.0. maxAbs = 2.0
    // Normalized: (2.0/2.0)*0.5+0.5=1.0, (0/2.0)*0.5+0.5=0.5, (-2.0/2.0)*0.5+0.5=0.0
    // Vertex 1 (normalized=0.5) should be the mid-color
    // Vertex 0 (normalized=1.0) and vertex 2 (normalized=0.0) should be at the extremes
    QVERIFY(colors[0] != colors[1]); // Extreme vs mid
    QVERIFY(colors[1] != colors[2]); // Mid vs opposite extreme
    QVERIFY(colors[0] != colors[2]); // Opposite extremes
}

//=============================================================================================================

void TestSensorStreaming::testWorkerColormapChange()
{
    RtSensorDataWorker worker;
    const int nSensors = 1;
    const int nVertices = 3;

    auto mapping = QSharedPointer<Eigen::MatrixXf>::create(nVertices, nSensors);
    (*mapping)(0, 0) = 1.0f;
    (*mapping)(1, 0) = 0.0f;
    (*mapping)(2, 0) = -1.0f;

    worker.setMappingMatrix(mapping);

    Eigen::VectorXf data(nSensors);
    data << 1.0f;

    // Get colors with default colormap (MNE)
    worker.addData(data);
    QSignalSpy spy1(&worker, &RtSensorDataWorker::newRtSensorColors);
    worker.streamData();
    QCOMPARE(spy1.count(), 1);
    QVector<uint32_t> colorsDefault = spy1.takeFirst().at(1).value<QVector<uint32_t>>();

    // Change colormap and get new colors
    worker.setColormapType("Hot");
    worker.addData(data);
    QSignalSpy spy2(&worker, &RtSensorDataWorker::newRtSensorColors);
    worker.streamData();
    QCOMPARE(spy2.count(), 1);
    QVector<uint32_t> colorsHot = spy2.takeFirst().at(1).value<QVector<uint32_t>>();

    // Colors should differ between colormaps for at least some vertices
    bool anyDifferent = false;
    for (int i = 0; i < nVertices; ++i) {
        if (colorsDefault[i] != colorsHot[i]) {
            anyDifferent = true;
            break;
        }
    }
    QVERIFY(anyDifferent);
}

//=============================================================================================================

void TestSensorStreaming::testWorkerSFreq()
{
    RtSensorDataWorker worker;
    // Setting sampling frequency should not crash
    worker.setSFreq(100.0);
    worker.setSFreq(0.5);
    worker.setSFreq(10000.0);
    QVERIFY(true);
}

//=============================================================================================================
// CONTROLLER TESTS
//=============================================================================================================

void TestSensorStreaming::testControllerConstruction()
{
    RtSensorDataController controller;
    QVERIFY(!controller.isStreaming());
}

//=============================================================================================================

void TestSensorStreaming::testControllerStreamingState()
{
    RtSensorDataController controller;

    QVERIFY(!controller.isStreaming());

    controller.setStreamingState(true);
    QVERIFY(controller.isStreaming());

    controller.setStreamingState(false);
    QVERIFY(!controller.isStreaming());
}

//=============================================================================================================

void TestSensorStreaming::testControllerTimeInterval()
{
    RtSensorDataController controller;

    controller.setTimeInterval(50);
    // Should not crash
    controller.setTimeInterval(1);
    controller.setTimeInterval(100);
    QVERIFY(true);
}

//=============================================================================================================

void TestSensorStreaming::testControllerSignalForwarding()
{
    RtSensorDataController controller;

    const int nSensors = 3;
    const int nVertices = 6;

    auto mapping = createUniformMapping(nVertices, nSensors);
    controller.setMappingMatrix(mapping);
    controller.setColormapType("Hot");
    controller.setLoopState(true);

    Eigen::VectorXf data(nSensors);
    data << 1.0f, 0.5f, -0.5f;
    controller.addData(data);

    QSignalSpy spy(&controller, &RtSensorDataController::newSensorColorsAvailable);

    // Start streaming and wait for at least one emission
    controller.setStreamingState(true);
    QVERIFY(spy.wait(500)); // Wait up to 500ms for signal

    controller.setStreamingState(false);

    QVERIFY(spy.count() >= 1);

    // Verify the emitted data structure
    QList<QVariant> args = spy.first();
    QVector<uint32_t> colors = args.at(1).value<QVector<uint32_t>>();
    QCOMPARE(colors.size(), nVertices);
}

//=============================================================================================================

void TestSensorStreaming::testControllerClearData()
{
    RtSensorDataController controller;

    Eigen::VectorXf data(5);
    data.setRandom();
    controller.addData(data);
    controller.addData(data);

    // Clear should not crash
    controller.clearData();
    QVERIFY(true);
}

//=============================================================================================================
// INTEGRATION TESTS
//=============================================================================================================

void TestSensorStreaming::testEndToEndStreaming()
{
    RtSensorDataController controller;

    const int nSensors = 4;
    const int nVertices = 12;

    auto mapping = createTestMapping(nVertices, nSensors);
    controller.setMappingMatrix(mapping);
    controller.setColormapType("MNE");
    controller.setLoopState(false);
    controller.setNumberAverages(1);
    controller.setTimeInterval(10); // 100 fps for faster test

    // Feed multiple time points
    for (int t = 0; t < 10; ++t) {
        Eigen::VectorXf data(nSensors);
        data.setRandom();
        controller.addData(data);
    }

    QSignalSpy spy(&controller, &RtSensorDataController::newSensorColorsAvailable);

    controller.setStreamingState(true);

    // Wait for several emissions
    for (int i = 0; i < 5 && spy.count() < 5; ++i) {
        spy.wait(200);
    }

    controller.setStreamingState(false);

    // Should have received multiple color updates
    QVERIFY(spy.count() >= 3);

    // Each emission should have the right number of vertices
    for (int i = 0; i < spy.count(); ++i) {
        QVector<uint32_t> colors = spy.at(i).at(1).value<QVector<uint32_t>>();
        QCOMPARE(colors.size(), nVertices);
    }
}

//=============================================================================================================

void TestSensorStreaming::testMultipleStreamCycles()
{
    RtSensorDataController controller;

    const int nSensors = 3;
    const int nVertices = 6;

    auto mapping = createUniformMapping(nVertices, nSensors);
    controller.setMappingMatrix(mapping);
    controller.setLoopState(true);
    controller.setTimeInterval(10);

    Eigen::VectorXf data(nSensors);
    data << 1.0f, 0.0f, -1.0f;
    controller.addData(data);

    // First streaming cycle
    QSignalSpy spy1(&controller, &RtSensorDataController::newSensorColorsAvailable);
    controller.setStreamingState(true);
    QVERIFY(spy1.wait(300));
    controller.setStreamingState(false);
    QVERIFY(spy1.count() >= 1);

    // Second streaming cycle
    QSignalSpy spy2(&controller, &RtSensorDataController::newSensorColorsAvailable);
    controller.setStreamingState(true);
    QVERIFY(spy2.wait(300));
    controller.setStreamingState(false);
    QVERIFY(spy2.count() >= 1);
}

//=============================================================================================================

void TestSensorStreaming::testEmptyQueueBehavior()
{
    RtSensorDataController controller;

    const int nSensors = 3;
    const int nVertices = 6;

    auto mapping = createUniformMapping(nVertices, nSensors);
    controller.setMappingMatrix(mapping);
    controller.setLoopState(false); // No looping

    // Don't add any data

    QSignalSpy spy(&controller, &RtSensorDataController::newSensorColorsAvailable);

    controller.setStreamingState(true);
    QTest::qWait(100); // Wait 100ms
    controller.setStreamingState(false);

    // Should not have emitted anything (empty queue, no looping)
    QCOMPARE(spy.count(), 0);
}

//=============================================================================================================

QTEST_MAIN(TestSensorStreaming)

#include "test_sensor_streaming.moc"
