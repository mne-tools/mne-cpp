//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_backend_impl.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Private state of AcqBackend, shared with AcqDevice.
 */

#ifndef ACQ_BACKEND_IMPL_H
#define ACQ_BACKEND_IMPL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../acq_backend.h"

#include <atomic>
#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

struct AcqBackend::Impl
{
    std::vector<AcqStream> streams;
    std::vector<std::unique_ptr<AcqWriter>> writers;
    std::vector<std::unique_ptr<AcqReader>> deviceReaders;
    bool sealed = false; // set when an AcqDevice takes the backend; no streams are added afterwards

    // First reported error wins: 0 = none, 1 = being written, 2 = readable.
    std::atomic<int> errorState{0};
    AcqError error;

    [[nodiscard]] bool failed() const noexcept
    {
        return errorState.load(std::memory_order_acquire) == 2;
    }
};

namespace detail
{

/** Gives AcqDevice access to the private state of the backend it owns. */
struct BackendAccess
{
    static AcqBackend::Impl& state(const AcqBackend& backend) noexcept
    {
        return *backend.m_pImpl;
    }
};

} // namespace detail

} // namespace mne

#endif // ACQ_BACKEND_IMPL_H
