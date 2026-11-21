//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_memory.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqMemoryLocation: where the samples of a ring live (host, pinned host, GPU, managed).
 *
 * A ring can be the memory a PCIe card or a network adapter writes by DMA, including GPU memory written
 * by GPUDirect RDMA. The library never dereferences sample memory the CPU may not touch; ring positions,
 * events and segment headers always live in host memory, so the stream protocol is the same everywhere.
 */

#ifndef ACQ_MEMORY_H
#define ACQ_MEMORY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Kind of memory that holds the samples of a ring.
 */
enum class AcqMemoryKind : std::uint8_t
{
    Host = 0,       /**< Pageable host memory. */
    HostPinned = 1, /**< Page-locked host memory registered with a device or GPU (PCIe DMA target). */
    Device = 2,     /**< GPU or accelerator memory (GPUDirect RDMA target); not addressable by the CPU. */
    Managed = 3     /**< Unified memory, migrated on access by the driver. */
};

//=============================================================================================================
/**
 * Location of ring memory: its kind and the device it belongs to.
 */
struct AcqMemoryLocation
{
    AcqMemoryKind kind = AcqMemoryKind::Host; /**< Kind of memory. */
    int device = -1;                          /**< GPU or accelerator ordinal; -1 for Host. */

    /** @return true when the CPU may read and write the samples directly. */
    [[nodiscard]] constexpr bool hostAccessible() const noexcept
    {
        return kind != AcqMemoryKind::Device;
    }

    friend constexpr bool operator==(const AcqMemoryLocation&, const AcqMemoryLocation&) noexcept = default;
};

} // namespace mne

#endif // ACQ_MEMORY_H
