//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *
 * @file     test_base_circular_buffer.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.9
 * @date     August, 2022
 * @brief    Tests the bounded blocking queue mne::CircularBuffer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/circular_buffer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <chrono>
#include <thread>
#include <type_traits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

//=============================================================================================================
/**
 * @brief Tests mne::CircularBuffer.
 */
class TestCircularBuffer : public QObject
{
    Q_OBJECT

private slots:
    void testIsNotCopyable();
    void testPushPopInFifoOrder();
    void testCapacityAndSize();
    void testFullAndEmptyTimeOut();
    void testClear();
    void testMovesMatrices();
    void testBlocksUntilTheOtherThreadMoves();
};

//=============================================================================================================

void TestCircularBuffer::testIsNotCopyable()
{
    static_assert(!std::is_copy_constructible_v<CircularBuffer<int>>);
    static_assert(!std::is_copy_assignable_v<CircularBuffer<int>>);
}

//=============================================================================================================

void TestCircularBuffer::testPushPopInFifoOrder()
{
    CircularBuffer<int> buffer(10);
    const std::array<int, 3> block{1, 2, 3};

    QVERIFY(buffer.push(5));
    QVERIFY(buffer.push(block));

    int value = 0;
    for (const int expected : {5, 1, 2, 3}) {
        QVERIFY(buffer.pop(value));
        QCOMPARE(value, expected);
    }
}

//=============================================================================================================

void TestCircularBuffer::testCapacityAndSize()
{
    CircularBuffer<int> buffer(5);
    QCOMPARE(buffer.capacity(), std::size_t(5));
    QCOMPARE(buffer.size(), std::size_t(0));

    buffer.push(1);
    buffer.push(2);
    QCOMPARE(buffer.size(), std::size_t(2));

    int value = 0;
    buffer.pop(value);
    QCOMPARE(buffer.size(), std::size_t(1));
}

//=============================================================================================================

void TestCircularBuffer::testFullAndEmptyTimeOut()
{
    CircularBuffer<int> buffer(2, 10ms);
    int value = 0;
    QVERIFY(!buffer.pop(value));

    QVERIFY(buffer.push(5));
    QVERIFY(buffer.push(10));
    QVERIFY(!buffer.push(15));
    QVERIFY(!buffer.push(std::array<int, 1>{15}));
    QCOMPARE(buffer.size(), std::size_t(2));
}

//=============================================================================================================

void TestCircularBuffer::testClear()
{
    CircularBuffer<int> buffer(2, 10ms);
    buffer.push(5);
    buffer.push(10);
    buffer.clear();

    int value = 0;
    QVERIFY(!buffer.pop(value));
    QVERIFY(buffer.push(20));
    QVERIFY(buffer.pop(value));
    QCOMPARE(value, 20);
}

//=============================================================================================================

void TestCircularBuffer::testMovesMatrices()
{
    CircularBuffer<Eigen::MatrixXd> buffer(3);
    Eigen::MatrixXd a(2, 2);
    a << 1, 2, 3, 4;

    QVERIFY(buffer.push(Eigen::MatrixXd(a)));
    Eigen::MatrixXd b;
    QVERIFY(buffer.pop(b));
    QVERIFY(b.isApprox(a));

    // A push that times out leaves the element it was given, so the caller can retry it
    CircularBuffer<Eigen::MatrixXd> full(1, 10ms);
    QVERIFY(full.push(Eigen::MatrixXd(a)));
    Eigen::MatrixXd retry = a;
    QVERIFY(!full.push(std::move(retry)));
    QVERIFY(retry.isApprox(a)); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move): a timed-out push does not move
}

//=============================================================================================================

void TestCircularBuffer::testBlocksUntilTheOtherThreadMoves()
{
    CircularBuffer<int> buffer(1, 5s);
    QVERIFY(buffer.push(1));

    std::thread consumer([&buffer] {
        std::this_thread::sleep_for(20ms);
        int value = 0;
        buffer.pop(value);
    });
    // Waits for the consumer to free the only slot rather than failing.
    QVERIFY(buffer.push(2));
    consumer.join();

    int value = 0;
    QVERIFY(buffer.pop(value));
    QCOMPARE(value, 2);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestCircularBuffer)
#include "test_base_circular_buffer.moc"
