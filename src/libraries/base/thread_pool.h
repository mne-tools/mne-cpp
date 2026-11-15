//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     thread_pool.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Fixed-size worker pool with futures and blocking parallel loops, in standard C++.
 */

#ifndef BASE_THREAD_POOL_H
#define BASE_THREAD_POOL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "base_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Runs tasks on a fixed set of worker threads.
 *
 * parallelFor() and parallelMap() block until done, and the calling thread works through the range too, so they
 * may be nested or called from a task of the same pool without deadlocking.
 *
 * @snippet ex_base/main.cpp thread_pool_usage
 */
class BASESHARED_EXPORT ThreadPool
{
public:
    //=========================================================================================================
    /**
     * Starts the workers.
     *
     * @param[in] threads   Number of workers; 0 for one per hardware thread.
     */
    explicit ThreadPool(std::size_t threads = 0);

    /** Runs the tasks still queued, then joins the workers. */
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /** @return The pool shared by the libraries, with one worker per hardware thread. */
    static ThreadPool& global();

    /** @return The number of workers. */
    std::size_t threadCount() const
    {
        return m_workers.size();
    }

    //=========================================================================================================
    /**
     * Queues @p task.
     *
     * @param[in] task   Callable without arguments.
     *
     * @return Its result, or the exception it threw.
     */
    template<typename F>
    std::future<std::invoke_result_t<F>> submit(F task)
    {
        using R = std::invoke_result_t<F>;
        auto packaged = std::make_shared<std::packaged_task<R()>>(std::move(task));
        std::future<R> result = packaged->get_future();
        enqueue([packaged] { (*packaged)(); });
        return result;
    }

    //=========================================================================================================
    /**
     * Calls @p body(i) for every i in [@p begin, @p end) and returns when all calls have finished.
     *
     * @param[in] begin   First index.
     * @param[in] end     One past the last index; an empty range does nothing.
     * @param[in] body    Called once per index, from any thread.
     *
     * Rethrows the first exception a call threw; indices after it in the same chunk are skipped.
     */
    void parallelFor(std::size_t begin, std::size_t end, const std::function<void(std::size_t)>& body);

    //=========================================================================================================
    /**
     * @param[in] input   The values.
     * @param[in] f       Called once per value, from any thread.
     *
     * @return f applied to every value of @p input, in the order of @p input.
     */
    template<typename T, typename F>
    auto parallelMap(const std::vector<T>& input, F f) -> std::vector<std::invoke_result_t<F, const T&>>
    {
        std::vector<std::invoke_result_t<F, const T&>> output(input.size());
        parallelFor(0, input.size(), [&](std::size_t i) { output[i] = f(input[i]); });
        return output;
    }

private:
    void enqueue(std::function<void()> task);
    void workerLoop();

    std::vector<std::thread> m_workers;
    std::deque<std::function<void()>> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    bool m_stopping = false;
};

} // namespace mne

#endif // BASE_THREAD_POOL_H
