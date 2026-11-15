//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_thread.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Thread placement and priority for acquisition threads (control path).
 *
 * A thread that the scheduler moves to another core loses its caches, and on a multi-socket machine it may end up
 * reading the ring across the socket link. Pinning the producer and its readers to cores of the ring's NUMA node
 * (AcqStreamOptions::numaNode) keeps the data local; a raised priority keeps other processes from delaying them.
 */

#ifndef ACQ_THREAD_H
#define ACQ_THREAD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"
#include "acq_error.h"

#include <cstdint>
#include <functional>
#include <span>
#include <thread>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Scheduling class of an acquisition thread.
 */
enum class AcqThreadPriority : std::uint8_t
{
    Normal = 0,  /**< The operating system default. */
    High = 1,    /**< Above other threads: nice -10 (Linux), THREAD_PRIORITY_HIGHEST (Windows), user-interactive QoS
                      (macOS, which also keeps the thread on performance cores). */
    Realtime = 2 /**< SCHED_FIFO (Linux), MMCSS "Pro Audio" (Windows), time-constraint policy for 1 ms periods
                      (macOS). Spinning at this priority can starve the system; use it for threads that block. */
};

//=============================================================================================================
/**
 * Where and how an acquisition thread runs, e.g. a backend's producer thread (URI parameters cpus and priority).
 *
 * @snippet ex_acq_performance/main.cpp acq_thread_policy
 */
struct AcqThreadPolicy
{
    std::vector<unsigned> cpus;                             /**< Logical CPUs the thread may run on; empty = any. */
    AcqThreadPriority priority = AcqThreadPriority::Normal; /**< Scheduling class. */
};

//=============================================================================================================
/**
 * Restricts the calling thread to @p cpus.
 *
 * @param[in] cpus   Logical CPU numbers; on Windows all in one processor group of 64.
 *
 * @return Success, InvalidParameter (empty, unknown CPU, several Windows groups), PlatformUnsupported (macOS and
 *         WebAssembly, which have no affinity), or DeviceIo with the system's message.
 */
[[nodiscard]] ACQSHARED_EXPORT AcqError acqPinThisThread(std::span<const unsigned> cpus) noexcept;

//=============================================================================================================
/**
 * Sets the scheduling class of the calling thread.
 *
 * @param[in] priority   Scheduling class; Normal undoes High and Realtime.
 *
 * @return Success, PermissionDenied (Linux needs CAP_SYS_NICE or an rtprio/nice limit for High and Realtime),
 *         InvalidState (High on a macOS thread that was Realtime: it can only return to Normal),
 *         PlatformUnsupported (WebAssembly), or DeviceIo with the system's message.
 */
[[nodiscard]] ACQSHARED_EXPORT AcqError acqSetThisThreadPriority(AcqThreadPriority priority) noexcept;

//=============================================================================================================
/**
 * Applies @p policy to the calling thread: CPUs first, then priority.
 *
 * @param[in] policy   The policy; empty CPUs leave the placement alone.
 *
 * @return As acqPinThisThread() and acqSetThisThreadPriority().
 */
[[nodiscard]] ACQSHARED_EXPORT AcqError acqApplyThreadPolicy(const AcqThreadPolicy& policy) noexcept;

/** @return The logical CPU the calling thread runs on now, or -1 where the system does not tell (macOS). */
[[nodiscard]] ACQSHARED_EXPORT int acqCurrentCpu() noexcept;

//=============================================================================================================
/**
 * @param[in] node   NUMA node, e.g. AcqStreamStats::numaNode of a ring.
 *
 * @return The logical CPUs of @p node in ascending order (node 0 holds every CPU on a machine without NUMA), or
 *         InvalidParameter for a node that does not exist.
 */
[[nodiscard]] ACQSHARED_EXPORT AcqExpected<std::vector<unsigned>> acqNumaNodeCpus(int node);

//=============================================================================================================
/**
 * Starts a thread that applies @p policy and then runs @p body, e.g. a backend's producer loop. Returns once the
 * policy is applied, so a refused policy fails the start instead of the acquisition.
 *
 * @param[in] policy   CPUs and priority of the thread.
 * @param[in] body     What the thread runs; not run if the policy is refused.
 *
 * @return The running thread, or the error of acqApplyThreadPolicy() after the thread has ended.
 */
[[nodiscard]] ACQSHARED_EXPORT AcqExpected<std::thread> start_thread(const AcqThreadPolicy& policy, std::function<void()> body);

} // namespace mne

#endif // ACQ_THREAD_H
