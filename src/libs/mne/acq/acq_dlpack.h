//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_dlpack.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Optional, header-only DLPack export of acquisition views (zero copy to NumPy, PyTorch, CuPy, ...).
 *
 * mne_acq itself depends only on the C++20 standard library. Code that exchanges views through DLPack includes
 * this header and provides DLPack 1.x (dlpack/dlpack.h); nothing here is compiled into the library.
 */

#ifndef ACQ_DLPACK_H
#define ACQ_DLPACK_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq.h"
#include <dlpack/dlpack.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>

static_assert(DLPACK_MAJOR_VERSION >= 1, "acq_dlpack.h needs DLPack 1.0 or newer (DLManagedTensorVersioned)");

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Exports one part of @p view as a read-only DLPack tensor of shape channels x samples, without a copy.
 * Strides follow the ring layout (interleaved or planar); raw device values, including digital bits.
 * GPU memory is described as CUDA (Device -> kDLCUDA, HostPinned -> kDLCUDAHost, Managed -> kDLCUDAManaged).
 *
 * The tensor points into the ring: release @p view only after the consumer is done with it, and check
 * the result of release(): Overrun means the producer overwrote the samples meanwhile.
 *
 * @param[in] view   An acquired view.
 * @param[in] part   Part index; mirror-mapped rings always give one part.
 *
 * @return The tensor, owned by the caller until handed to a consumer (its deleter frees only the
 *         descriptor), or InvalidParameter (no such part), UnsupportedFormat (Int24Packed and ComplexInt16
 *         have no DLPack type), OutOfMemory.
 */
[[nodiscard]] inline AcqExpected<DLManagedTensorVersioned*> toDLPack(const AcqBlockView& view, std::size_t part = 0)
{
    if (part >= view.partCount()) {
        return AcqErrorCode::InvalidParameter;
    }
    DLDataType dtype{};
    dtype.lanes = 1;
    switch (view.format()) {
        case AcqSampleFormat::Int16:
            dtype.code = kDLInt;
            dtype.bits = 16;
            break;
        case AcqSampleFormat::Int32:
            dtype.code = kDLInt;
            dtype.bits = 32;
            break;
        case AcqSampleFormat::Float32:
            dtype.code = kDLFloat;
            dtype.bits = 32;
            break;
        case AcqSampleFormat::Float64:
            dtype.code = kDLFloat;
            dtype.bits = 64;
            break;
        case AcqSampleFormat::ComplexFloat32:
            dtype.code = kDLComplex;
            dtype.bits = 64;
            break;
        default:
            return AcqErrorCode::UnsupportedFormat;
    }
    const AcqBlockPart& run = view.part(part);
    const auto bytes = static_cast<std::ptrdiff_t>(bytesPerSample(view.format()));
    if (run.channelStride % bytes != 0 || run.sampleStride % bytes != 0) {
        return AcqErrorCode::UnsupportedFormat; // DLPack strides count elements
    }
    DLDevice device{kDLCPU, 0};
    const AcqMemoryLocation memory = view.memory();
    const std::int32_t ordinal = memory.device < 0 ? 0 : memory.device;
    switch (memory.kind) {
        case AcqMemoryKind::Host:
            break;
        case AcqMemoryKind::HostPinned:
            device = {kDLCUDAHost, ordinal};
            break;
        case AcqMemoryKind::Device:
            device = {kDLCUDA, ordinal};
            break;
        case AcqMemoryKind::Managed:
            device = {kDLCUDAManaged, ordinal};
            break;
    }

    struct Holder
    {
        DLManagedTensorVersioned managed; // every field is set below
        std::int64_t shape[2]{};
        std::int64_t strides[2]{};
    };
    auto* holder = new (std::nothrow) Holder;
    if (holder == nullptr) {
        return AcqErrorCode::OutOfMemory;
    }
    holder->shape[0] = static_cast<std::int64_t>(view.channelCount());
    holder->shape[1] = static_cast<std::int64_t>(run.samples);
    holder->strides[0] = run.channelStride / bytes;
    holder->strides[1] = run.sampleStride / bytes;
    DLManagedTensorVersioned& m = holder->managed;
    m.version = {DLPACK_MAJOR_VERSION, DLPACK_MINOR_VERSION};
    m.manager_ctx = holder;
    m.deleter = [](DLManagedTensorVersioned* self) {
        std::unique_ptr<Holder>(static_cast<Holder*>(self->manager_ctx)).reset();
    };
    m.flags = DLPACK_FLAG_BITMASK_READ_ONLY;
    m.dl_tensor.data = const_cast<std::byte*>(run.data); // read-only by the flag above
    m.dl_tensor.device = device;
    m.dl_tensor.ndim = 2;
    m.dl_tensor.dtype = dtype;
    m.dl_tensor.shape = holder->shape;
    m.dl_tensor.strides = holder->strides;
    m.dl_tensor.byte_offset = 0;
    return &m;
}

} // namespace mne

#endif // ACQ_DLPACK_H
