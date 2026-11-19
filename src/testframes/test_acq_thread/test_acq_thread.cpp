//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_acq_thread.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests for thread pinning, priority and NUMA node CPUs, directly and through a backend's URI.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq.h>

#include <QtTest/QtTest>

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

namespace
{
#if defined(__APPLE__) || defined(__EMSCRIPTEN__)
constexpr bool kCanPin = false;
#else
constexpr bool kCanPin = true;
#endif
} // namespace

//=============================================================================================================

class TestAcqThread : public QObject
{
    Q_OBJECT

private slots:
    void testNumaNodeCpus();
    void testPinThisThread();
    void testPinRejectsBadCpus();
    void testPriority();
    void testStartThread();
    void testBackendThreadPolicy_data();
    void testBackendThreadPolicy();
};

//=============================================================================================================

void TestAcqThread::testNumaNodeCpus()
{
    auto cpus = acqNumaNodeCpus(0);
    QVERIFY(cpus);
    QVERIFY(!cpus->empty());
    QVERIFY(std::is_sorted(cpus->begin(), cpus->end()));
    QVERIFY(std::adjacent_find(cpus->begin(), cpus->end()) == cpus->end());
    QVERIFY(cpus->back() < std::max(1U, std::thread::hardware_concurrency()) + 4096U);
    QVERIFY(acqNumaNodeCpus(-1).error() == AcqErrorCode::InvalidParameter);
    QVERIFY(acqNumaNodeCpus(1 << 20).error() == AcqErrorCode::InvalidParameter);
}

//=============================================================================================================

void TestAcqThread::testPinThisThread()
{
    const std::vector<unsigned> all = acqNumaNodeCpus(0).value();
    AcqError pinned;
    AcqError unpinned;
    std::vector<int> seen;
    int target = -1;
    std::thread worker([&] {
        // The CPU the thread runs on is one this process may use, even in a container with a CPU quota.
        target = acqCurrentCpu();
        const unsigned cpu = target >= 0 ? static_cast<unsigned>(target) : all.back();
        pinned = acqPinThisThread(std::span<const unsigned>(&cpu, 1));
        // The scheduler may not move the thread off its only CPU, however often it yields.
        for (int i = 0; i < 1000; ++i) {
            seen.push_back(acqCurrentCpu());
            std::this_thread::yield();
        }
        unpinned = acqPinThisThread(all);
    });
    worker.join();
    if (!kCanPin) {
        QVERIFY(pinned.code() == AcqErrorCode::PlatformUnsupported);
        QVERIFY(std::all_of(seen.begin(), seen.end(), [](int cpu) { return cpu == -1; }));
        return;
    }
    QVERIFY2(pinned.ok(), pinned.text().c_str());
    QVERIFY(unpinned.ok());
    QVERIFY(target >= 0);
    QVERIFY(std::all_of(seen.begin(), seen.end(), [&](int cpu) { return cpu == target; }));
}

//=============================================================================================================

void TestAcqThread::testPinRejectsBadCpus()
{
    const AcqErrorCode expected = kCanPin ? AcqErrorCode::InvalidParameter : AcqErrorCode::PlatformUnsupported;
    QVERIFY(acqPinThisThread({}).code() == expected);
    const unsigned missing = 1U << 20;
    QVERIFY(acqPinThisThread(std::span<const unsigned>(&missing, 1)).code() == expected);
    AcqThreadPolicy policy;
    QVERIFY(acqApplyThreadPolicy(policy).ok()); // no CPUs, normal priority: nothing to change
    policy.cpus = {missing};
    QVERIFY(acqApplyThreadPolicy(policy).code() == expected);
}

//=============================================================================================================

void TestAcqThread::testPriority()
{
    using P = AcqThreadPriority;
    const std::vector<P> sequence = {P::Normal, P::High, P::Normal, P::Realtime, P::Normal, P::Realtime, P::High};
    std::vector<AcqError> results;
    std::thread worker([&] {
        for (const P p : sequence) {
            results.push_back(acqSetThisThreadPriority(p));
        }
    });
    worker.join();
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        const AcqError& e = results[i];
#if defined(__linux__)
        // Unprivileged users may lack CAP_SYS_NICE; that must be reported as such, not as another failure.
        const bool refused = sequence[i] != P::Normal && e.code() == AcqErrorCode::PermissionDenied;
#elif defined(__APPLE__)
        // The time-constraint policy opts a thread out of QoS; afterwards it can return to normal only.
        const bool refused = i == sequence.size() - 1 && e.code() == AcqErrorCode::InvalidState;
#else
        const bool refused = false;
#endif
        QVERIFY2(e.ok() || refused, (std::to_string(i) + ": " + e.text()).c_str());
    }
}

//=============================================================================================================

void TestAcqThread::testStartThread()
{
    std::atomic<int> runs{0};
    auto thread = start_thread({}, [&runs] { ++runs; });
    QVERIFY(thread);
    thread->join();
    QCOMPARE(runs.load(), 1);

    // A refused policy runs nothing and leaves no thread behind
    AcqThreadPolicy refused;
    refused.cpus = {1U << 20};
    auto none = start_thread(refused, [&runs] { ++runs; });
    QVERIFY(!none);
    QCOMPARE(none.error().code(), kCanPin ? AcqErrorCode::InvalidParameter : AcqErrorCode::PlatformUnsupported);
    QCOMPARE(runs.load(), 1);
}

//=============================================================================================================

void TestAcqThread::testBackendThreadPolicy_data()
{
    QTest::addColumn<QString>("params");
    QTest::addColumn<int>("openError");
    QTest::addColumn<int>("startError");
    const int none = int(AcqErrorCode::None);
    const int bad = int(AcqErrorCode::InvalidParameter);
    QTest::newRow("default") << "" << none << none;
    QTest::newRow("normal priority") << "&priority=normal" << none << none;
    QTest::newRow("unknown priority") << "&priority=turbo" << bad << none;
    QTest::newRow("cpu list") << "&cpus=0" << none << (kCanPin ? none : int(AcqErrorCode::PlatformUnsupported));
    QTest::newRow("not a number") << "&cpus=0,x" << bad << none;
    QTest::newRow("empty list") << "&cpus=" << bad << none;
    QTest::newRow("no such cpu") << "&cpus=1048576" << none << (kCanPin ? bad : int(AcqErrorCode::PlatformUnsupported));
}

void TestAcqThread::testBackendThreadPolicy()
{
    QFETCH(QString, params);
    QFETCH(int, openError);
    QFETCH(int, startError);
    auto device = AcqDevice::open("synthetic://?eeg=2&realtime=0&duration=1" + params.toStdString());
    QCOMPARE(int(device ? AcqErrorCode::None : device.error().code()), openError);
    if (!device) {
        return;
    }
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    const AcqError started = device->start();
    QCOMPARE(int(started.code()), startError);
    if (!started.ok()) {
        QCOMPARE(device->state(), AcqDeviceState::Opened); // a refused policy starts nothing
        return;
    }
    std::uint64_t samples = 0;
    while (auto view = reader->lease(1, 5s)) {
        samples += (*view)->sampleCount();
    }
    QCOMPARE(samples, std::uint64_t(1000));
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestAcqThread)
#include "test_acq_thread.moc"
