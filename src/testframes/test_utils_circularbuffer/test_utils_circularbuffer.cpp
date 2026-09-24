//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *
 * @file     test_utils_circularbuffer.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.9
 * @date     August, 2022
 * @brief     Test for the circular buffer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/circularbuffer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QDebug>
#include <QTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestCircularBuffer
 *
 * @brief The TestCircularBuffer class provides tests for verifying circular buffer functionality
 *
 */

class TestCircularBuffer : public QObject
{
    Q_OBJECT

private slots:
    void testBufferCreationDestruction();
    void testBufferPushingPopping();
    void testBufferCapacity();
};

//=============================================================================================================

void TestCircularBuffer::testBufferCreationDestruction()
{
    //Test Constructors/Destructors
    {
        CircularBuffer<float> testBuffer(10);
    }
    {
        CircularBuffer<int> *testBuffer = new CircularBuffer<int>(10);
        delete testBuffer;
    }
}

//=============================================================================================================

void TestCircularBuffer::testBufferPushingPopping()
{
    CircularBuffer<int> testBuffer(10);

    int testVal = 5;
    int testArray[3] = {1, 2, 3};

    //Verify Push
    QVERIFY(testBuffer.push(testVal));
    QVERIFY(testBuffer.push(testArray, 3));

    int resultVal = 0;
    int resultArray[3] = {0, 0, 0};

    QVERIFY(testBuffer.pop(resultVal));
    for (int i = 0; i < 3; ++i){
        QVERIFY(testBuffer.pop(resultArray[i]));
    }

    QVERIFY(resultVal == testVal);
    for (int i = 0; i < 3; ++i){
        QVERIFY(resultArray[i] == testArray[i]);
    }
}

//=============================================================================================================

void TestCircularBuffer::testBufferCapacity()
{
    CircularBuffer<int> testBuffer(2);
    int testSink = 0;

    QVERIFY(!testBuffer.pop(testSink));

    testBuffer.push(5);
    testBuffer.push(10);

    QVERIFY(!testBuffer.push(15));

    testBuffer.clear();

    QVERIFY(!testBuffer.pop(testSink));

    QVERIFY(testBuffer.push(20));

    QVERIFY(testBuffer.pop(testSink));

    QVERIFY(testSink == 20);

    QVERIFY(!testBuffer.pop(testSink));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestCircularBuffer);
#include "test_utils_circularbuffer.moc"
