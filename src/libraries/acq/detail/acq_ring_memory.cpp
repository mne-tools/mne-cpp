//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_ring_memory.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Ring memory from the operating system: VirtualAlloc on Windows, mmap on POSIX, mirrored
 *           with section views (Windows) or a shared memory object mapped twice (POSIX).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_ring_memory.h"

#if defined(_WIN32)
#define ACQ_RING_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif __has_include(<sys/mman.h>) && !defined(__EMSCRIPTEN__)
#define ACQ_RING_MMAP
#include <sys/mman.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/syscall.h>
#endif
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#endif

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::detail;

namespace
{

std::size_t roundUp(std::size_t value, std::size_t multiple) noexcept
{
    return (value + multiple - 1) / multiple * multiple;
}

#if defined(ACQ_RING_WINDOWS)
std::byte* virtualAlloc(void* address, std::size_t bytes, int numaNode, int& boundNode) noexcept
{
    constexpr DWORD kType = MEM_RESERVE | MEM_COMMIT;
    ULONG highest = 0;
    if (numaNode >= 0 && GetNumaHighestNodeNumber(&highest) != 0 && static_cast<ULONG>(numaNode) <= highest) {
        void* p = VirtualAllocExNuma(GetCurrentProcess(), address, bytes, kType, PAGE_READWRITE,
                                     static_cast<DWORD>(numaNode));
        if (p != nullptr) {
            boundNode = numaNode;
            return static_cast<std::byte*>(p);
        }
    }
    return static_cast<std::byte*>(VirtualAlloc(address, bytes, kType, PAGE_READWRITE));
}
#endif

#if defined(ACQ_RING_MMAP) && defined(__linux__) && defined(SYS_mbind)
bool bindToNode(void* address, std::size_t bytes, int node) noexcept
{
    constexpr int kMaxNodes = 1024;
    constexpr int kBits = static_cast<int>(8 * sizeof(unsigned long));
    if (node < 0 || node >= kMaxNodes) {
        return false;
    }
    unsigned long mask[kMaxNodes / kBits] = {};
    mask[node / kBits] = 1UL << (node % kBits);
    constexpr long kMpolBind = 2;
    return syscall(SYS_mbind, address, bytes, kMpolBind, mask, static_cast<unsigned long>(kMaxNodes) + 1, 0UL) == 0;
}
#endif

//=============================================================================================================
// MIRROR MAPPING: the ring's pages mapped twice back to back
//=============================================================================================================

#if defined(ACQ_RING_WINDOWS)
using VirtualAlloc2Fn = PVOID(WINAPI*)(HANDLE, PVOID, SIZE_T, ULONG, ULONG, MEM_EXTENDED_PARAMETER*, ULONG);
using MapViewOfFile3Fn = PVOID(WINAPI*)(HANDLE, HANDLE, PVOID, ULONG64, SIZE_T, ULONG, ULONG, MEM_EXTENDED_PARAMETER*,
                                        ULONG);

// Placeholders (Windows 10 1803 and later), resolved at run time so older systems still load the library.
bool mapMirror(RingMemory& out, std::size_t bytes, std::size_t alignment, int numaNode) noexcept
{
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    HMODULE kernel = GetModuleHandleW(L"kernelbase.dll");
    const auto virtualAlloc2 =
        kernel != nullptr ? reinterpret_cast<VirtualAlloc2Fn>(GetProcAddress(kernel, "VirtualAlloc2")) : nullptr;
    const auto mapViewOfFile3 =
        kernel != nullptr ? reinterpret_cast<MapViewOfFile3Fn>(GetProcAddress(kernel, "MapViewOfFile3")) : nullptr;
    if (virtualAlloc2 == nullptr || mapViewOfFile3 == nullptr || bytes % info.dwAllocationGranularity != 0 ||
        alignment > info.dwAllocationGranularity) {
        return false;
    }
    const auto size = static_cast<ULONGLONG>(bytes);
    const DWORD high = static_cast<DWORD>(size >> 32U);
    const DWORD low = static_cast<DWORD>(size & 0xFFFFFFFFULL);
    ULONG highest = 0;
    const bool numa = numaNode >= 0 && GetNumaHighestNodeNumber(&highest) != 0 && static_cast<ULONG>(numaNode) <= highest;
    HANDLE section = numa ? CreateFileMappingNumaW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, high, low, nullptr,
                                                   static_cast<DWORD>(numaNode))
                          : CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, high, low, nullptr);
    if (section == nullptr) {
        return false;
    }
    auto* placeholder = static_cast<std::byte*>(
        virtualAlloc2(nullptr, nullptr, 2 * bytes, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0));
    if (placeholder == nullptr) {
        CloseHandle(section);
        return false;
    }
    VirtualFree(placeholder, bytes, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    void* first = mapViewOfFile3(section, nullptr, placeholder, 0, bytes, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE,
                                 nullptr, 0);
    void* second = mapViewOfFile3(section, nullptr, placeholder + bytes, 0, bytes, MEM_REPLACE_PLACEHOLDER,
                                  PAGE_READWRITE, nullptr, 0);
    if (first == nullptr || second == nullptr) {
        if (first != nullptr) {
            UnmapViewOfFile(first);
        } else {
            VirtualFree(placeholder, 0, MEM_RELEASE);
        }
        if (second != nullptr) {
            UnmapViewOfFile(second);
        } else {
            VirtualFree(placeholder + bytes, 0, MEM_RELEASE);
        }
        CloseHandle(section);
        return false;
    }
    out.data = placeholder;
    out.bytes = bytes;
    out.section = section;
    out.numaNode = numa ? numaNode : -1;
    out.mirrored = true;
    return true;
}
#elif defined(ACQ_RING_MMAP)
/** @return A file descriptor of an unnamed shared memory object of @p bytes, or -1. */
int sharedMemoryObject(std::size_t bytes) noexcept
{
#if defined(__linux__)
    const int fd = memfd_create("mne_acq_ring", MFD_CLOEXEC);
#else
    // A name is needed only until it is unlinked; it must stay below PSHMNAMLEN (31) on macOS.
    static std::atomic<unsigned> counter{0};
    int fd = -1;
    for (int attempt = 0; attempt < 16 && fd < 0; ++attempt) {
        std::array<char, 32> name{};
        std::snprintf(name.data(), name.size(), "/mneacq.%d.%u", static_cast<int>(getpid()),
                      counter.fetch_add(1, std::memory_order_relaxed));
        fd = shm_open(name.data(), O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name.data());
        } else if (errno != EEXIST) {
            break;
        }
    }
#endif
    if (fd >= 0 && ftruncate(fd, static_cast<off_t>(bytes)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

bool mapMirror(RingMemory& out, std::size_t bytes, std::size_t alignment, int numaNode) noexcept
{
    const auto page = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    alignment = std::max(alignment, page);
    if (bytes % page != 0) {
        return false;
    }
    const int fd = sharedMemoryObject(bytes);
    if (fd < 0) {
        return false;
    }
    // Reserve the whole aligned range first, so the two views cannot be split by another mapping.
    const std::size_t total = 2 * bytes + alignment - page;
    void* reserved = mmap(nullptr, total, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (reserved == MAP_FAILED) {
        close(fd);
        return false;
    }
    auto* first = static_cast<std::byte*>(reserved);
    auto* data = first + (roundUp(reinterpret_cast<std::uintptr_t>(first), alignment) - reinterpret_cast<std::uintptr_t>(first));
    const bool mapped = mmap(data, bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) != MAP_FAILED &&
        mmap(data + bytes, bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) != MAP_FAILED;
    close(fd);
    if (!mapped) {
        munmap(first, total);
        return false;
    }
    const auto lead = static_cast<std::size_t>(data - first);
    if (lead > 0) {
        munmap(first, lead);
    }
    if (total - lead > 2 * bytes) {
        munmap(data + 2 * bytes, total - lead - 2 * bytes);
    }
#if defined(__linux__) && defined(SYS_mbind)
    if (bindToNode(data, bytes, numaNode)) {
        out.numaNode = numaNode;
    }
#endif
    static_cast<void>(numaNode);
    out.data = data;
    out.bytes = bytes;
    out.mirrored = true;
    return true;
}
#else
bool mapMirror(RingMemory&, std::size_t, std::size_t, int) noexcept
{
    return false;
}
#endif

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

RingMemory mne::detail::allocateRingMemory(std::size_t bytes, std::size_t alignment, bool hugePages,
                                           int numaNode, bool mirror) noexcept
{
    RingMemory out;
    if (mirror && !hugePages && mapMirror(out, bytes, alignment, numaNode)) {
        return out;
    }
    if (hugePages) {
        alignment = std::max(alignment, kHugePageBytes);
    }
#if defined(ACQ_RING_WINDOWS)
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    out.bytes = roundUp(bytes, hugePages ? kHugePageBytes : static_cast<std::size_t>(info.dwPageSize));
    if (alignment <= info.dwAllocationGranularity) {
        out.data = virtualAlloc(nullptr, out.bytes, numaNode, out.numaNode);
        return out;
    }
    // Reserve a larger range to find an aligned address, release it and map there; another thread may take the
    // address in between, so retry.
    for (int attempt = 0; attempt < 16 && out.data == nullptr; ++attempt) {
        void* probe = VirtualAlloc(nullptr, out.bytes + alignment, MEM_RESERVE, PAGE_NOACCESS);
        if (probe == nullptr) {
            break;
        }
        const auto aligned = roundUp(reinterpret_cast<std::uintptr_t>(probe), alignment);
        VirtualFree(probe, 0, MEM_RELEASE);
        out.numaNode = -1;
        out.data = virtualAlloc(reinterpret_cast<void*>(aligned), out.bytes, numaNode, out.numaNode);
    }
    return out;
#elif defined(ACQ_RING_MMAP)
    const auto page = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    alignment = std::max(alignment, page);
    out.bytes = roundUp(bytes, hugePages ? kHugePageBytes : page);
    const std::size_t total = out.bytes + alignment - page;
    void* mapped = mmap(nullptr, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapped == MAP_FAILED) {
        return out;
    }
    auto* first = static_cast<std::byte*>(mapped);
    out.data = first + (roundUp(reinterpret_cast<std::uintptr_t>(first), alignment) - reinterpret_cast<std::uintptr_t>(first));
    const auto lead = static_cast<std::size_t>(out.data - first);
    if (lead > 0) {
        munmap(first, lead);
    }
    if (total - lead > out.bytes) {
        munmap(out.data + out.bytes, total - lead - out.bytes);
    }
#if defined(MADV_HUGEPAGE)
    out.hugePages = hugePages && madvise(out.data, out.bytes, MADV_HUGEPAGE) == 0;
#endif
#if defined(__linux__) && defined(SYS_mbind)
    if (bindToNode(out.data, out.bytes, numaNode)) {
        out.numaNode = numaNode;
    }
#endif
    static_cast<void>(numaNode);
    return out;
#else
    static_cast<void>(numaNode);
    out.bytes = roundUp(bytes, alignment);
    out.data = static_cast<std::byte*>(std::aligned_alloc(alignment, out.bytes));
    if (out.data != nullptr) {
        std::memset(out.data, 0, out.bytes);
    }
    return out;
#endif
}

//=============================================================================================================

void mne::detail::lockRingMemory(RingMemory& memory) noexcept
{
    if (memory.data == nullptr || memory.locked) {
        return;
    }
#if defined(ACQ_RING_WINDOWS)
    memory.locked = VirtualLock(memory.data, memory.bytes) != 0 &&
        (!memory.mirrored || VirtualLock(memory.data + memory.bytes, memory.bytes) != 0);
#elif defined(ACQ_RING_MMAP)
    memory.locked = mlock(memory.data, memory.mirrored ? 2 * memory.bytes : memory.bytes) == 0;
#endif
}

//=============================================================================================================

void mne::detail::releaseRingMemory(RingMemory& memory) noexcept
{
    if (memory.data == nullptr) {
        return;
    }
#if defined(ACQ_RING_WINDOWS)
    if (memory.mirrored) {
        UnmapViewOfFile(memory.data);
        UnmapViewOfFile(memory.data + memory.bytes);
        CloseHandle(memory.section);
    } else {
        if (memory.locked) {
            VirtualUnlock(memory.data, memory.bytes);
        }
        VirtualFree(memory.data, 0, MEM_RELEASE);
    }
#elif defined(ACQ_RING_MMAP)
    const std::size_t span = memory.mirrored ? 2 * memory.bytes : memory.bytes;
    if (memory.locked) {
        munlock(memory.data, span);
    }
    munmap(memory.data, span);
#else
    std::free(memory.data);
#endif
    memory = RingMemory();
}
