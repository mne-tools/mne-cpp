//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fiff_byte_swap.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Tests for FIFF byte-swap utility functions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_byte_swap.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
/**
 * @brief Tests for byte-swap functions in fiff_byte_swap.h.
 */
class TestFiffByteSwap : public QObject
{
    Q_OBJECT

public:
    TestFiffByteSwap();

private slots:
    void initTestCase();

    // swap_short
    void testSwapShortIdentity();
    void testSwapShortKnown();
    void testSwapShortNeg();

    // swap_int
    void testSwapIntIdentity();
    void testSwapIntKnown();
    void testSwapIntNeg();

    // swap_intp
    void testSwapIntpIdentity();
    void testSwapIntpKnown();

    // swap_long
    void testSwapLongIdentity();
    void testSwapLongKnown();

    // swap_longp
    void testSwapLongpIdentity();
    void testSwapLongpKnown();

    // swap_float
    void testSwapFloatIdentity();
    void testSwapFloatKnown();

    // swap_floatp
    void testSwapFloatpIdentity();
    void testSwapFloatpKnown();

    // swap_doublep
    void testSwapDoublepIdentity();
    void testSwapDoublepKnown();

    void cleanupTestCase();
};

//=============================================================================================================

TestFiffByteSwap::TestFiffByteSwap()
{
}

//=============================================================================================================

void TestFiffByteSwap::initTestCase()
{
}

//=============================================================================================================
// swap_short tests
//=============================================================================================================

void TestFiffByteSwap::testSwapShortIdentity()
{
    qint16 val = 0x0102;
    QCOMPARE(swap_short(swap_short(val)), val);
}

void TestFiffByteSwap::testSwapShortKnown()
{
    qint16 val = 0x0102;
    qint16 swapped = swap_short(val);
    QCOMPARE(swapped, static_cast<qint16>(0x0201));
}

void TestFiffByteSwap::testSwapShortNeg()
{
    qint16 val = -1;
    QCOMPARE(swap_short(val), static_cast<qint16>(-1));
}

//=============================================================================================================
// swap_int tests
//=============================================================================================================

void TestFiffByteSwap::testSwapIntIdentity()
{
    qint32 val = 0x01020304;
    QCOMPARE(swap_int(swap_int(val)), val);
}

void TestFiffByteSwap::testSwapIntKnown()
{
    qint32 val = 0x01020304;
    qint32 swapped = swap_int(val);
    QCOMPARE(swapped, static_cast<qint32>(0x04030201));
}

void TestFiffByteSwap::testSwapIntNeg()
{
    qint32 val = -1;
    QCOMPARE(swap_int(val), static_cast<qint32>(-1));
}

//=============================================================================================================
// swap_intp tests
//=============================================================================================================

void TestFiffByteSwap::testSwapIntpIdentity()
{
    qint32 val = 0x01020304;
    qint32 copy = val;
    swap_intp(&copy);
    swap_intp(&copy);
    QCOMPARE(copy, val);
}

void TestFiffByteSwap::testSwapIntpKnown()
{
    qint32 val = 0x01020304;
    swap_intp(&val);
    QCOMPARE(val, static_cast<qint32>(0x04030201));
}

//=============================================================================================================
// swap_long tests
//=============================================================================================================

void TestFiffByteSwap::testSwapLongIdentity()
{
    qint64 val = Q_INT64_C(0x0102030405060708);
    QCOMPARE(swap_long(swap_long(val)), val);
}

void TestFiffByteSwap::testSwapLongKnown()
{
    qint64 val = Q_INT64_C(0x0102030405060708);
    qint64 swapped = swap_long(val);
    QCOMPARE(swapped, Q_INT64_C(0x0807060504030201));
}

//=============================================================================================================
// swap_longp tests
//=============================================================================================================

void TestFiffByteSwap::testSwapLongpIdentity()
{
    qint64 val = Q_INT64_C(0x0102030405060708);
    qint64 copy = val;
    swap_longp(&copy);
    swap_longp(&copy);
    QCOMPARE(copy, val);
}

void TestFiffByteSwap::testSwapLongpKnown()
{
    qint64 val = Q_INT64_C(0x0102030405060708);
    swap_longp(&val);
    QCOMPARE(val, Q_INT64_C(0x0807060504030201));
}

//=============================================================================================================
// swap_float tests
//=============================================================================================================

void TestFiffByteSwap::testSwapFloatIdentity()
{
    float val = 3.14f;
    QCOMPARE(swap_float(swap_float(val)), val);
}

void TestFiffByteSwap::testSwapFloatKnown()
{
    float val = 1.0f;
    float swapped = swap_float(val);
    // swap_float(swap_float(x)) == x, and swapped != val for non-palindrome byte patterns
    QVERIFY(swapped != val);
    QCOMPARE(swap_float(swapped), val);
}

//=============================================================================================================
// swap_floatp tests
//=============================================================================================================

void TestFiffByteSwap::testSwapFloatpIdentity()
{
    float val = 2.71828f;
    float copy = val;
    swap_floatp(&copy);
    swap_floatp(&copy);
    QCOMPARE(copy, val);
}

void TestFiffByteSwap::testSwapFloatpKnown()
{
    float val = 1.0f;
    float copy = val;
    swap_floatp(&copy);
    QVERIFY(copy != val);
    swap_floatp(&copy);
    QCOMPARE(copy, val);
}

//=============================================================================================================
// swap_doublep tests
//=============================================================================================================

void TestFiffByteSwap::testSwapDoublepIdentity()
{
    double val = 3.141592653589793;
    double copy = val;
    swap_doublep(&copy);
    swap_doublep(&copy);
    QCOMPARE(copy, val);
}

void TestFiffByteSwap::testSwapDoublepKnown()
{
    double val = 1.0;
    double copy = val;
    swap_doublep(&copy);
    QVERIFY(copy != val);
    swap_doublep(&copy);
    QCOMPARE(copy, val);
}

//=============================================================================================================

void TestFiffByteSwap::cleanupTestCase()
{
    qInfo() << "TestFiffByteSwap: All tests completed";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffByteSwap)
#include "test_fiff_byte_swap.moc"
