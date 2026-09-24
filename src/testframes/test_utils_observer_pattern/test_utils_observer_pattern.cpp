//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_utils_observer_pattern.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Tests for Observer/Subject pattern, CircularBuffer edge cases,
 *           and MNELogger — the generic utility design patterns in utils.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/observerpattern.h>
#include <utils/generics/circularbuffer.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QtTest>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// TEST HELPERS
//=============================================================================================================

/**
 * Concrete observer for testing the observer pattern.
 */
class TestObserver : public IObserver
{
public:
    int updateCount = 0;
    Subject* lastSubject = nullptr;

    void update(Subject* pSubject) override {
        ++updateCount;
        lastSubject = pSubject;
    }
};

/**
 * Concrete subject for testing the observer pattern.
 */
class TestSubject : public Subject
{
public:
    TestSubject() : Subject() {}
};

//=============================================================================================================
/**
 * @brief Tests for Observer/Subject pattern, CircularBuffer, and MNELogger.
 */
class TestUtilsObserverPattern : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // ── Observer Pattern ──────────────────────────────────────────────
    void testAttachDetachObserver();
    void testNotifyObservers();
    void testNotifyDisabled();
    void testMultipleObservers();
    void testDetachAndReattach();

    // ── CircularBuffer edge cases ─────────────────────────────────────
    void testCircularBufferPauseResume();
    void testCircularBufferFreeElements();
    void testCircularBufferEigenMatrix();
    void testCircularBufferFillAndDrain();

    // ── MNELogger ─────────────────────────────────────────────
    void testMNELoggerInstall();

    void cleanupTestCase();
};

//=============================================================================================================

void TestUtilsObserverPattern::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
}

//=============================================================================================================
// Observer Pattern
//=============================================================================================================

void TestUtilsObserverPattern::testAttachDetachObserver()
{
    TestSubject subject;
    TestObserver observer;

    subject.attach(&observer);
    QCOMPARE(subject.observerNumDebug(), 1);

    subject.detach(&observer);
    QCOMPARE(subject.observerNumDebug(), 0);
}

//=============================================================================================================

void TestUtilsObserverPattern::testNotifyObservers()
{
    TestSubject subject;
    TestObserver observer;

    subject.attach(&observer);
    subject.notify();

    QCOMPARE(observer.updateCount, 1);
    QCOMPARE(observer.lastSubject, static_cast<Subject*>(&subject));

    subject.notify();
    QCOMPARE(observer.updateCount, 2);
}

//=============================================================================================================

void TestUtilsObserverPattern::testNotifyDisabled()
{
    TestSubject subject;
    TestObserver observer;

    subject.attach(&observer);

    bool oldState = Subject::notifyEnabled;
    Subject::notifyEnabled = false;
    subject.notify();
    QCOMPARE(observer.updateCount, 0);

    Subject::notifyEnabled = true;
    subject.notify();
    QCOMPARE(observer.updateCount, 1);

    // Restore original state
    Subject::notifyEnabled = oldState;
}

//=============================================================================================================

void TestUtilsObserverPattern::testMultipleObservers()
{
    TestSubject subject;
    TestObserver obs1, obs2, obs3;

    subject.attach(&obs1);
    subject.attach(&obs2);
    subject.attach(&obs3);
    QCOMPARE(subject.observerNumDebug(), 3);

    subject.notify();
    QCOMPARE(obs1.updateCount, 1);
    QCOMPARE(obs2.updateCount, 1);
    QCOMPARE(obs3.updateCount, 1);

    subject.detach(&obs2);
    QCOMPARE(subject.observerNumDebug(), 2);

    subject.notify();
    QCOMPARE(obs1.updateCount, 2);
    QCOMPARE(obs2.updateCount, 1); // Detached, should not be updated
    QCOMPARE(obs3.updateCount, 2);
}

//=============================================================================================================

void TestUtilsObserverPattern::testDetachAndReattach()
{
    TestSubject subject;
    TestObserver obs1;

    subject.attach(&obs1);
    QCOMPARE(subject.observerNumDebug(), 1);

    subject.detach(&obs1);
    QCOMPARE(subject.observerNumDebug(), 0);

    // Reattach should work
    subject.attach(&obs1);
    QCOMPARE(subject.observerNumDebug(), 1);

    subject.notify();
    QCOMPARE(obs1.updateCount, 1);
}

//=============================================================================================================
// CircularBuffer edge cases
//=============================================================================================================

void TestUtilsObserverPattern::testCircularBufferPauseResume()
{
    CircularBuffer<int> buffer(10);

    buffer.push(42);
    buffer.pause(true);

    // While paused, push should fail
    bool pushOk = buffer.push(99);
    QVERIFY(!pushOk);

    // Pop should also fail while paused
    int val = 0;
    bool popOk = buffer.pop(val);
    QVERIFY(!popOk);

    // Resume
    buffer.pause(false);
    // The 42 we pushed before pause should still be there
    popOk = buffer.pop(val);
    QVERIFY(popOk);
    QCOMPARE(val, 42);
}

//=============================================================================================================

void TestUtilsObserverPattern::testCircularBufferFreeElements()
{
    CircularBuffer<int> buffer(5);

    QCOMPARE(buffer.getFreeElementsWrite(), 5);
    QCOMPARE(buffer.getFreeElementsRead(), 0);

    buffer.push(1);
    buffer.push(2);

    QCOMPARE(buffer.getFreeElementsWrite(), 3);
    QCOMPARE(buffer.getFreeElementsRead(), 2);

    int val;
    buffer.pop(val);
    QCOMPARE(buffer.getFreeElementsWrite(), 4);
    QCOMPARE(buffer.getFreeElementsRead(), 1);
}

//=============================================================================================================

void TestUtilsObserverPattern::testCircularBufferEigenMatrix()
{
    CircularBuffer<MatrixXd> buffer(3);

    MatrixXd A(2, 2);
    A << 1, 2, 3, 4;

    QVERIFY(buffer.push(A));

    MatrixXd B;
    QVERIFY(buffer.pop(B));
    QVERIFY(B.isApprox(A));
}

//=============================================================================================================

void TestUtilsObserverPattern::testCircularBufferFillAndDrain()
{
    CircularBuffer<int> buffer(4);

    // Fill to capacity
    for (int i = 0; i < 4; ++i) {
        QVERIFY(buffer.push(i * 10));
    }
    QCOMPARE(buffer.getFreeElementsWrite(), 0);
    QCOMPARE(buffer.getFreeElementsRead(), 4);

    // Drain all
    for (int i = 0; i < 4; ++i) {
        int val;
        QVERIFY(buffer.pop(val));
        QCOMPARE(val, i * 10);
    }
    QCOMPARE(buffer.getFreeElementsWrite(), 4);
    QCOMPARE(buffer.getFreeElementsRead(), 0);

    // Buffer should be reusable after drain
    QVERIFY(buffer.push(999));
    int val;
    QVERIFY(buffer.pop(val));
    QCOMPARE(val, 999);
}

//=============================================================================================================
// MNELogger
//=============================================================================================================

void TestUtilsObserverPattern::testMNELoggerInstall()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);

    // Trigger different log levels
    qDebug() << "Test debug message from MNELogger test";
    qInfo() << "Test info message from MNELogger test";
    qWarning() << "Test warning message from MNELogger test";

    // If we got here without crashing, the logger works
    QVERIFY(true);
}

//=============================================================================================================

void TestUtilsObserverPattern::cleanupTestCase()
{
    qInfo() << "TestUtilsObserverPattern: All tests completed";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestUtilsObserverPattern)
#include "test_utils_observer_pattern.moc"
