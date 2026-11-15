//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_base_thread_pool.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Tests ThreadPool and benchmarks it against QtConcurrent.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/thread_pool.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtConcurrent>
#include <QtTest>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <atomic>
#include <chrono>
#include <cmath>
#include <numeric>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================

namespace
{

constexpr int kBenchItems = 1 << 16;

/** A few microseconds of floating-point work per item, typical of a per-channel or per-permutation step. */
double work(int i)
{
    double acc = 0.0;
    for (int k = 1; k <= 200; ++k) {
        acc += std::sin(static_cast<double>(i) * 1.0e-3 + static_cast<double>(k));
    }
    return acc;
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestUtilsThreadPool
 *
 * @brief Tests ThreadPool.
 */
class TestUtilsThreadPool : public QObject
{
    Q_OBJECT

private slots:
    void submitReturnsResultsAndRunsOnWorkers();
    void submitPropagatesExceptions();
    void parallelForCoversEveryIndexOnce_data();
    void parallelForCoversEveryIndexOnce();
    void parallelForRethrowsAndFinishes();
    void parallelForNestsWithoutDeadlock();
    void parallelMapKeepsOrder();
    void destructorRunsQueuedTasks();
    void globalPoolUsesEveryCore();
    void benchmarkMap_data();
    void benchmarkMap();
};

//=============================================================================================================

void TestUtilsThreadPool::submitReturnsResultsAndRunsOnWorkers()
{
    ThreadPool pool(2);
    QCOMPARE(pool.threadCount(), std::size_t(2));
    const std::thread::id caller = std::this_thread::get_id();
    auto answer = pool.submit([] { return 6 * 7; });
    auto where = pool.submit([] { return std::this_thread::get_id(); });
    std::atomic<bool> ran{false};
    auto none = pool.submit([&ran] { ran = true; });
    QCOMPARE(answer.get(), 42);
    QVERIFY(where.get() != caller);
    none.get();
    QVERIFY(ran);
}

//=============================================================================================================

void TestUtilsThreadPool::submitPropagatesExceptions()
{
    ThreadPool pool(1);
    auto failing = pool.submit([]() -> int { throw std::runtime_error("bad channel"); });
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, static_cast<void>(failing.get()));
    QCOMPARE(pool.submit([] { return 1; }).get(), 1); // the worker survived
}

//=============================================================================================================

void TestUtilsThreadPool::parallelForCoversEveryIndexOnce_data()
{
    QTest::addColumn<int>("threads");
    QTest::addColumn<int>("begin");
    QTest::addColumn<int>("end");
    QTest::newRow("empty") << 4 << 5 << 5;
    QTest::newRow("reversed") << 4 << 9 << 3;
    QTest::newRow("one") << 4 << 0 << 1;
    QTest::newRow("fewer than threads") << 8 << 10 << 13;
    QTest::newRow("many") << 4 << 0 << 100003;
    QTest::newRow("single thread") << 1 << 0 << 1000;
}

//=============================================================================================================

void TestUtilsThreadPool::parallelForCoversEveryIndexOnce()
{
    QFETCH(int, threads);
    QFETCH(int, begin);
    QFETCH(int, end);
    ThreadPool pool(static_cast<std::size_t>(threads));
    const std::size_t count = end > begin ? static_cast<std::size_t>(end - begin) : 0;
    std::vector<std::atomic<int>> hits(count + 1);
    pool.parallelFor(static_cast<std::size_t>(begin), static_cast<std::size_t>(end), [&](std::size_t i) {
        hits[i - static_cast<std::size_t>(begin)].fetch_add(1);
    });
    for (std::size_t i = 0; i < count; ++i) {
        QCOMPARE(hits[i].load(), 1);
    }
    QCOMPARE(hits[count].load(), 0);
}

//=============================================================================================================

void TestUtilsThreadPool::parallelForRethrowsAndFinishes()
{
    ThreadPool pool(4);
    std::atomic<int> visited{0};
    QVERIFY_THROWS_EXCEPTION(std::out_of_range, pool.parallelFor(0, 1000, [&](std::size_t i) {
        visited.fetch_add(1);
        if (i == 500) {
            throw std::out_of_range("index 500");
        }
    }));
    // Every chunk ran to its end or its exception before parallelFor returned.
    QVERIFY(visited.load() >= 1);
    QVERIFY(visited.load() <= 1000);
    std::atomic<int> after{0};
    pool.parallelFor(0, 100, [&](std::size_t) { after.fetch_add(1); });
    QCOMPARE(after.load(), 100);
}

//=============================================================================================================

void TestUtilsThreadPool::parallelForNestsWithoutDeadlock()
{
    // Each outer item occupies a worker and starts an inner loop on the same pool; the callers work through it.
    ThreadPool pool(2);
    std::atomic<int> total{0};
    pool.parallelFor(0, 8, [&](std::size_t) {
        pool.parallelFor(0, 50, [&](std::size_t) { total.fetch_add(1); });
    });
    QCOMPARE(total.load(), 8 * 50);

    auto nested = pool.submit([&pool] { return pool.parallelMap(std::vector<int>{1, 2, 3}, [](int v) { return v * v; }); });
    QCOMPARE(nested.get(), (std::vector<int>{1, 4, 9}));
}

//=============================================================================================================

void TestUtilsThreadPool::parallelMapKeepsOrder()
{
    ThreadPool pool(4);
    std::vector<int> input(10000);
    std::iota(input.begin(), input.end(), 0);
    const std::vector<double> mapped = pool.parallelMap(input, [](int v) { return 0.5 * v; });
    QCOMPARE(mapped.size(), input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        QCOMPARE(mapped[i], 0.5 * static_cast<double>(input[i]));
    }
    QVERIFY(pool.parallelMap(std::vector<int>{}, [](int v) { return v; }).empty());
}

//=============================================================================================================

void TestUtilsThreadPool::destructorRunsQueuedTasks()
{
    std::atomic<int> done{0};
    {
        ThreadPool pool(1);
        for (int i = 0; i < 20; ++i) {
            pool.submit([&done] {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                done.fetch_add(1);
            });
        }
    }
    QCOMPARE(done.load(), 20);
}

//=============================================================================================================

void TestUtilsThreadPool::globalPoolUsesEveryCore()
{
    const std::size_t cores = std::max(1u, std::thread::hardware_concurrency());
    QCOMPARE(ThreadPool::global().threadCount(), cores);
    QCOMPARE(&ThreadPool::global(), &ThreadPool::global());
    QCOMPARE(ThreadPool(0).threadCount(), cores);
}

//=============================================================================================================

void TestUtilsThreadPool::benchmarkMap_data()
{
    QTest::addColumn<bool>("qt");
    QTest::newRow("QtConcurrent::blockingMapped") << true;
    QTest::newRow("ThreadPool::parallelMap") << false;
}

//=============================================================================================================

void TestUtilsThreadPool::benchmarkMap()
{
    QFETCH(bool, qt);
    std::vector<int> input(kBenchItems);
    std::iota(input.begin(), input.end(), 0);
    std::vector<double> out;
    if (qt) {
        QBENCHMARK
        {
            out = QtConcurrent::blockingMapped<std::vector<double>>(input, work);
        }
    } else {
        QBENCHMARK
        {
            out = ThreadPool::global().parallelMap(input, work);
        }
    }
    QCOMPARE(out.size(), input.size());
    QCOMPARE(out[1234], work(1234));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestUtilsThreadPool)
#include "test_base_thread_pool.moc"
