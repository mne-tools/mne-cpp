//=============================================================================================================
/**
 * @file     test_utils_circularbuffer.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.9
 * @date     August, 2022
 *
 * @section  LICENSE
 *
 * Copyright (C) 2022, Gabriel Motta. All rights reserved.
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
 * @brief     Test for the circular buffer.
 *
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
        QVERIFY(resultArray[i] == resultArray[i]);
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
