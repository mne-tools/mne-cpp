//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_ring_memory.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Ring memory from the operating system: alignment, huge pages, NUMA binding and locking.
 */

#ifndef ACQ_RING_MEMORY_H
#define ACQ_RING_MEMORY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <cstddef>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** Huge page size the ring is aligned and sized to when huge pages are requested. */
inline constexpr std::size_t kHugePageBytes = std::size_t(2) << 20;

/**
 * Memory owned by a ring. Huge pages, NUMA binding and locking are best effort and reported here.
 */
struct RingMemory
{
    std::byte* data = nullptr; /**< Aligned start; nullptr if the allocation failed. */
    std::size_t bytes = 0;     /**< Mapped bytes, a multiple of the page size. */
    bool hugePages = false;    /**< Whether the range is backed by huge pages. */
    int numaNode = -1;         /**< NUMA node the range is bound to, -1 if not bound. */
    bool locked = false;       /**< Whether the range is locked in physical memory. */
    bool mirrored = false;     /**< Whether [data + bytes, data + 2 bytes) maps the same pages again. */
    void* section = nullptr;   /**< Windows: the section object behind a mirrored range. */
};

/**
 * Maps zeroed memory for a ring.
 *
 * @param[in] bytes       Usable bytes.
 * @param[in] alignment   Start alignment, a power of two.
 * @param[in] hugePages   Request huge pages (start and size then aligned to kHugePageBytes).
 * @param[in] numaNode    Bind to this node, or -1.
 * @param[in] mirror      Map the pages a second time right after the first, so any run of up to @p bytes is
 *                        contiguous; only when @p bytes is a multiple of the mapping granularity and huge
 *                        pages are not requested, otherwise an ordinary range.
 *
 * @return The memory; data is nullptr when the system has no memory left.
 */
RingMemory allocateRingMemory(std::size_t bytes, std::size_t alignment, bool hugePages, int numaNode,
                              bool mirror) noexcept;

/** Locks @p memory (both views of a mirrored range) in physical memory; sets RingMemory::locked. */
void lockRingMemory(RingMemory& memory) noexcept;

/** Unlocks and unmaps @p memory. */
void releaseRingMemory(RingMemory& memory) noexcept;

} // namespace mne::detail

#endif // ACQ_RING_MEMORY_H
