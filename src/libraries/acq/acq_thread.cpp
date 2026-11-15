//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_thread.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Definitions of the thread placement and priority functions declared in @ref acq_thread.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_thread.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <fstream>
#include <future>
#include <string>
#include <thread>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <avrt.h>
#elif defined(__APPLE__) && defined(__clang__) // the Mach headers of the SDK parse with Clang only
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#include <pthread/qos.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

namespace
{
#if defined(__linux__) || (defined(__APPLE__) && defined(__clang__))
AcqError systemError(const char* what, int code)
{
    if (code == EPERM || code == EACCES) {
        return AcqError(AcqErrorCode::PermissionDenied, std::string(what) + ": " + std::strerror(code));
    }
    return AcqError(AcqErrorCode::DeviceIo, std::string(what) + ": " + std::strerror(code));
}
#endif

#if defined(__APPLE__) && defined(__clang__)
thread_local bool g_optedOutOfQos = false; // set by the time-constraint policy
#endif

#if defined(_WIN32)
thread_local HANDLE g_mmcss = nullptr; // MMCSS registration of this thread, undone by Normal
#endif

#if defined(__linux__)
/** Parses a kernel CPU list such as "0-3,8,10-11". @return false on malformed input. */
bool parseCpuList(const std::string& text, std::vector<unsigned>& out)
{
    const char* p = text.data();
    const char* end = p + text.size();
    while (p < end && *p != '\n') {
        unsigned first = 0;
        auto r = std::from_chars(p, end, first);
        if (r.ec != std::errc()) {
            return false;
        }
        unsigned last = first;
        p = r.ptr;
        if (p < end && *p == '-') {
            r = std::from_chars(p + 1, end, last);
            if (r.ec != std::errc() || last < first) {
                return false;
            }
            p = r.ptr;
        }
        for (unsigned c = first; c <= last; ++c) {
            out.push_back(c);
        }
        if (p < end && *p == ',') {
            ++p;
        }
    }
    return true;
}
#endif
} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::acqPinThisThread(std::span<const unsigned> cpus) noexcept
{
#if defined(__linux__)
    if (cpus.empty()) {
        return AcqError(AcqErrorCode::InvalidParameter, "no CPU given");
    }
    cpu_set_t set;
    CPU_ZERO(&set);
    for (const unsigned c : cpus) {
        if (c >= static_cast<unsigned>(CPU_SETSIZE)) {
            return AcqError(AcqErrorCode::InvalidParameter, "CPU " + std::to_string(c) + " does not exist");
        }
        CPU_SET(c, &set);
    }
    const int rc = pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
    if (rc == EINVAL) {
        return AcqError(AcqErrorCode::InvalidParameter, "none of the CPUs is available to this process");
    }
    return rc == 0 ? AcqError() : systemError("pthread_setaffinity_np", rc);
#elif defined(_WIN32)
    if (cpus.empty()) {
        return AcqError(AcqErrorCode::InvalidParameter, "no CPU given");
    }
    GROUP_AFFINITY affinity{};
    affinity.Group = static_cast<WORD>(cpus.front() / 64);
    for (const unsigned c : cpus) {
        if (c / 64 != affinity.Group || c / 64 >= GetActiveProcessorGroupCount() ||
            c % 64 >= GetActiveProcessorCount(static_cast<WORD>(c / 64))) {
            return AcqError(AcqErrorCode::InvalidParameter,
                            "CPU " + std::to_string(c) + " does not exist or is in another processor group");
        }
        affinity.Mask |= KAFFINITY(1) << (c % 64);
    }
    if (SetThreadGroupAffinity(GetCurrentThread(), &affinity, nullptr) == 0) {
        return AcqError(AcqErrorCode::DeviceIo, "SetThreadGroupAffinity failed: " + std::to_string(GetLastError()));
    }
    return {};
#else
    static_cast<void>(cpus);
    return AcqError(AcqErrorCode::PlatformUnsupported, "this platform does not let threads choose their CPUs");
#endif
}

//=============================================================================================================

AcqError mne::acqSetThisThreadPriority(AcqThreadPriority priority) noexcept
{
#if defined(__linux__)
    const auto tid = static_cast<id_t>(syscall(SYS_gettid));
    if (priority == AcqThreadPriority::Realtime) {
        sched_param param{};
        param.sched_priority = std::min(80, sched_get_priority_max(SCHED_FIFO));
        const int rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
        return rc == 0 ? AcqError() : systemError("SCHED_FIFO", rc);
    }
    sched_param param{};
    if (const int rc = pthread_setschedparam(pthread_self(), SCHED_OTHER, &param); rc != 0) {
        return systemError("SCHED_OTHER", rc);
    }
    if (setpriority(PRIO_PROCESS, tid, priority == AcqThreadPriority::High ? -10 : 0) != 0) {
        return systemError("setpriority", errno);
    }
    return {};
#elif defined(_WIN32)
    if (g_mmcss != nullptr) {
        AvRevertMmThreadCharacteristics(g_mmcss);
        g_mmcss = nullptr;
    }
    int level = THREAD_PRIORITY_NORMAL;
    if (priority == AcqThreadPriority::Realtime) {
        DWORD task = 0;
        g_mmcss = AvSetMmThreadCharacteristicsW(L"Pro Audio", &task);
        if (g_mmcss == nullptr) {
            return AcqError(AcqErrorCode::DeviceIo, "MMCSS registration failed: " + std::to_string(GetLastError()));
        }
        level = THREAD_PRIORITY_TIME_CRITICAL;
    } else if (priority == AcqThreadPriority::High) {
        level = THREAD_PRIORITY_HIGHEST;
    }
    if (SetThreadPriority(GetCurrentThread(), level) == 0) {
        return AcqError(AcqErrorCode::DeviceIo, "SetThreadPriority failed: " + std::to_string(GetLastError()));
    }
    return {};
#elif defined(__APPLE__) && defined(__clang__)
    const thread_port_t self = pthread_mach_thread_np(pthread_self());
    if (priority == AcqThreadPriority::Realtime) {
        mach_timebase_info_data_t timebase{};
        mach_timebase_info(&timebase);
        const auto ticks = [&](double ns) {
            return static_cast<std::uint32_t>(ns * timebase.denom / timebase.numer);
        };
        thread_time_constraint_policy_data_t policy{};
        policy.period = ticks(1e6);
        policy.computation = ticks(5e5);
        policy.constraint = ticks(1e6);
        policy.preemptible = 1;
        const kern_return_t rc = thread_policy_set(self, THREAD_TIME_CONSTRAINT_POLICY,
                                                   reinterpret_cast<thread_policy_t>(&policy),
                                                   THREAD_TIME_CONSTRAINT_POLICY_COUNT);
        if (rc != KERN_SUCCESS) {
            return AcqError(AcqErrorCode::DeviceIo, "time-constraint policy: " + std::to_string(rc));
        }
        g_optedOutOfQos = true;
        return {};
    }
    if (g_optedOutOfQos) {
        // The time-constraint policy opted the thread out of QoS for good (so does the standard policy, hence only
        // here): the standard policy is all that is left.
        thread_standard_policy_data_t standard{};
        thread_policy_set(self, THREAD_STANDARD_POLICY, reinterpret_cast<thread_policy_t>(&standard),
                          THREAD_STANDARD_POLICY_COUNT);
        return priority == AcqThreadPriority::Normal
            ? AcqError()
            : AcqError(AcqErrorCode::InvalidState, "a macOS thread that was realtime cannot become high, only normal");
    }
    const int rc =
        pthread_set_qos_class_self_np(priority == AcqThreadPriority::High ? QOS_CLASS_USER_INTERACTIVE : QOS_CLASS_DEFAULT, 0);
    return rc == 0 ? AcqError() : systemError("pthread_set_qos_class_self_np", rc);
#else
    return priority == AcqThreadPriority::Normal
        ? AcqError()
        : AcqError(AcqErrorCode::PlatformUnsupported, "this platform has no thread priorities");
#endif
}

//=============================================================================================================

AcqError mne::acqApplyThreadPolicy(const AcqThreadPolicy& policy) noexcept
{
    if (!policy.cpus.empty()) {
        if (AcqError e = acqPinThisThread(policy.cpus); !e.ok()) {
            return e;
        }
    }
    return policy.priority == AcqThreadPriority::Normal ? AcqError() : acqSetThisThreadPriority(policy.priority);
}

//=============================================================================================================

int mne::acqCurrentCpu() noexcept
{
#if defined(__linux__)
    return sched_getcpu();
#elif defined(_WIN32)
    PROCESSOR_NUMBER n{};
    GetCurrentProcessorNumberEx(&n);
    return static_cast<int>(n.Group) * 64 + static_cast<int>(n.Number);
#else
    return -1;
#endif
}

//=============================================================================================================

AcqExpected<std::vector<unsigned>> mne::acqNumaNodeCpus(int node)
{
    if (node < 0) {
        return AcqError(AcqErrorCode::InvalidParameter, "NUMA node " + std::to_string(node) + " does not exist");
    }
    std::vector<unsigned> cpus;
#if defined(__linux__)
    std::ifstream file("/sys/devices/system/node/node" + std::to_string(node) + "/cpulist");
    std::string text;
    if (file && std::getline(file, text)) {
        if (!parseCpuList(text, cpus)) {
            return AcqError(AcqErrorCode::DeviceIo, "cannot parse the CPU list of NUMA node " + std::to_string(node));
        }
        return cpus;
    }
#elif defined(_WIN32)
    ULONG highest = 0;
    GROUP_AFFINITY affinity{};
    if (GetNumaHighestNodeNumber(&highest) != 0 && static_cast<ULONG>(node) <= highest &&
        GetNumaNodeProcessorMaskEx(static_cast<USHORT>(node), &affinity) != 0) {
        for (unsigned bit = 0; bit < 64; ++bit) {
            if ((affinity.Mask >> bit) & 1U) {
                cpus.push_back(static_cast<unsigned>(affinity.Group) * 64 + bit);
            }
        }
        return cpus;
    }
#endif
    // Without NUMA information every CPU belongs to node 0.
    if (node != 0) {
        return AcqError(AcqErrorCode::InvalidParameter, "NUMA node " + std::to_string(node) + " does not exist");
    }
    for (unsigned c = 0; c < std::max(1U, std::thread::hardware_concurrency()); ++c) {
        cpus.push_back(c);
    }
    return cpus;
}

//=============================================================================================================

AcqExpected<std::thread> mne::start_thread(const AcqThreadPolicy& policy, std::function<void()> body)
{
    // The promise's shared state outlives both ends, unlike a flag the thread could still notify after we return
    std::promise<AcqError> applied;
    std::future<AcqError> result = applied.get_future();
    std::thread thread([policy, body = std::move(body), applied = std::move(applied)]() mutable {
        AcqError error = acqApplyThreadPolicy(policy);
        const bool ok = error.ok();
        applied.set_value(std::move(error));
        if (ok) {
            body();
        }
    });
    if (AcqError error = result.get(); !error.ok()) {
        thread.join();
        return error;
    }
    return thread;
}
