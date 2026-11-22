//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_rtserver_observer_pattern.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Tests for the Subject/IObserver pattern and the ICommand interface.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <rtserver_connector/command_pattern.h>
#include <rtserver_connector/observer_pattern.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <memory>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// TEST HELPERS
//=============================================================================================================

namespace
{

/** Counts its notifications and runs an optional action from inside update(). */
class TestObserver : public IObserver
{
public:
    void update(Subject* subject) override
    {
        ++updateCount;
        lastSubject = subject;
        if (onUpdate) {
            onUpdate();
        }
    }

    int updateCount = 0;
    Subject* lastSubject = nullptr;
    std::function<void()> onUpdate;
};

class TestSubject : public Subject
{
};

/** Appends a value to a log when executed. */
class LogCommand : public ICommand
{
public:
    LogCommand(std::vector<int>& log, int value)
    : m_log(log)
    , m_value(value)
    {
    }

    void execute() override
    {
        m_log.push_back(m_value);
    }

private:
    std::vector<int>& m_log;
    int m_value;
};

} // namespace

//=============================================================================================================
/**
 * @brief Tests for the Subject/IObserver pattern and the ICommand interface.
 */
class TestRtServerObserverPattern : public QObject
{
    Q_OBJECT

private slots:
    void testAttachDetach();
    void testNotify();
    void testNotifyDisabled();
    void testMultipleObserversInOrder();
    void testDetachDuringNotify();
    void testCommand();
};

//=============================================================================================================

void TestRtServerObserverPattern::testAttachDetach()
{
    TestSubject subject;
    TestObserver observer;

    subject.attach(&observer);
    subject.attach(&observer); // twice: still one entry
    subject.attach(nullptr);
    QCOMPARE(subject.observers().size(), std::size_t(1));

    TestObserver stranger;
    subject.detach(&stranger); // not attached: no effect
    QCOMPARE(subject.observers().size(), std::size_t(1));

    subject.detach(&observer);
    QVERIFY(subject.observers().empty());

    subject.attach(&observer);
    subject.notify();
    QCOMPARE(observer.updateCount, 1);
}

//=============================================================================================================

void TestRtServerObserverPattern::testNotify()
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

void TestRtServerObserverPattern::testNotifyDisabled()
{
    TestSubject subject;
    TestSubject other;
    TestObserver observer;
    subject.attach(&observer);
    other.attach(&observer);

    QVERIFY(subject.notify_enabled());
    subject.set_notify_enabled(false);
    subject.notify();
    QCOMPARE(observer.updateCount, 0);

    other.notify(); // suspending one subject leaves the others alone
    QCOMPARE(observer.updateCount, 1);

    subject.set_notify_enabled(true);
    subject.notify();
    QCOMPARE(observer.updateCount, 2);
}

//=============================================================================================================

void TestRtServerObserverPattern::testMultipleObserversInOrder()
{
    TestSubject subject;
    std::vector<int> order;
    TestObserver obs1;
    TestObserver obs2;
    TestObserver obs3;
    obs1.onUpdate = [&] {
        order.push_back(1);
    };
    obs2.onUpdate = [&] {
        order.push_back(2);
    };
    obs3.onUpdate = [&] {
        order.push_back(3);
    };

    subject.attach(&obs3);
    subject.attach(&obs1);
    subject.attach(&obs2);
    subject.notify();
    QCOMPARE(order, (std::vector<int>{3, 1, 2}));

    subject.detach(&obs1);
    subject.notify();
    QCOMPARE(obs1.updateCount, 1);
    QCOMPARE(obs2.updateCount, 2);
    QCOMPARE(obs3.updateCount, 2);
}

//=============================================================================================================

void TestRtServerObserverPattern::testDetachDuringNotify()
{
    TestSubject subject;
    TestObserver first;
    TestObserver second;
    auto late = std::make_unique<TestObserver>();

    // The first observer detaches itself and attaches another one while it is being notified
    first.onUpdate = [&] {
        subject.detach(&first);
        subject.attach(late.get());
    };
    subject.attach(&first);
    subject.attach(&second);

    subject.notify();
    QCOMPARE(first.updateCount, 1);
    QCOMPARE(second.updateCount, 1); // still notified in this round
    QCOMPARE(late->updateCount, 0);  // joins from the next round on

    subject.notify();
    QCOMPARE(first.updateCount, 1);
    QCOMPARE(second.updateCount, 2);
    QCOMPARE(late->updateCount, 1);
}

//=============================================================================================================

void TestRtServerObserverPattern::testCommand()
{
    std::vector<int> log;
    std::vector<std::unique_ptr<ICommand>> queue;
    queue.push_back(std::make_unique<LogCommand>(log, 1));
    queue.push_back(std::make_unique<LogCommand>(log, 2));
    QVERIFY(log.empty()); // nothing runs until executed
    for (const auto& command : queue) {
        command->execute();
    }
    QCOMPARE(log, (std::vector<int>{1, 2}));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestRtServerObserverPattern)
#include "test_rtserver_observer_pattern.moc"
