//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     thread_pool.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Definition of @ref mne::ThreadPool.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "thread_pool.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <atomic>
#include <exception>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

/** One parallelFor call; helpers that start after the range is used up find nothing left and return. */
struct Loop
{
    std::size_t end = 0;
    std::size_t count = 0;
    std::size_t participants = 1;
    std::atomic<std::size_t> next{0};
    std::size_t finished = 0;
    std::exception_ptr error;
    std::mutex mutex;
    std::condition_variable allDone;
};

// Guided scheduling: each grab takes a share of what is left, so cores of unequal speed finish together.
void runChunks(Loop& loop, const std::function<void(std::size_t)>& body)
{
    for (;;) {
        std::size_t first = loop.next.load(std::memory_order_relaxed);
        std::size_t n = 0;
        do {
            if (first >= loop.end) {
                return;
            }
            n = std::max<std::size_t>(1, (loop.end - first) / (2 * loop.participants));
        } while (!loop.next.compare_exchange_weak(first, first + n, std::memory_order_relaxed));

        std::exception_ptr error;
        try {
            for (std::size_t i = first; i < first + n; ++i) {
                body(i);
            }
        } catch (...) {
            error = std::current_exception();
        }
        std::lock_guard<std::mutex> lock(loop.mutex);
        if (error && !loop.error) {
            loop.error = error;
        }
        loop.finished += n;
        if (loop.finished == loop.count) {
            loop.allDone.notify_all();
        }
    }
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ThreadPool::ThreadPool(std::size_t threads)
{
    if (threads == 0) {
        threads = std::max(1u, std::thread::hardware_concurrency());
    }
    m_workers.reserve(threads);
    for (std::size_t i = 0; i < threads; ++i) {
        m_workers.emplace_back([this] { workerLoop(); });
    }
}

//=============================================================================================================

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopping = true;
    }
    m_wake.notify_all();
    for (std::thread& worker : m_workers) {
        worker.join();
    }
}

//=============================================================================================================

ThreadPool& ThreadPool::global()
{
    static ThreadPool pool;
    return pool;
}

//=============================================================================================================

void ThreadPool::enqueue(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push_back(std::move(task));
    }
    m_wake.notify_one();
}

//=============================================================================================================

void ThreadPool::workerLoop()
{
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_wake.wait(lock, [this] { return m_stopping || !m_queue.empty(); });
            if (m_queue.empty()) {
                return;
            }
            task = std::move(m_queue.front());
            m_queue.pop_front();
        }
        task();
    }
}

//=============================================================================================================

void ThreadPool::parallelFor(std::size_t begin, std::size_t end, const std::function<void(std::size_t)>& body)
{
    if (end <= begin) {
        return;
    }
    auto loop = std::make_shared<Loop>();
    loop->end = end;
    loop->count = end - begin;
    loop->next = begin;
    const std::size_t helpers = std::min(threadCount(), loop->count - 1);
    loop->participants = helpers + 1;

    // The helpers share ownership of the loop and the body, so a helper that starts after return is harmless.
    auto shared = std::make_shared<std::function<void(std::size_t)>>(body);
    for (std::size_t h = 0; h < helpers; ++h) {
        enqueue([loop, shared] { runChunks(*loop, *shared); });
    }
    runChunks(*loop, body);

    std::unique_lock<std::mutex> lock(loop->mutex);
    loop->allDone.wait(lock, [&] { return loop->finished == loop->count; });
    if (loop->error) {
        std::rethrow_exception(loop->error);
    }
}
