//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_control_queue.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqControlQueue: the bounded lock-free queue that carries control events from backend threads.
 *
 * Any number of backend threads (the acquisition thread among them) push and the device's users pop, without locks
 * or allocation (Dmitry Vyukov's bounded multi-producer multi-consumer queue). A full queue drops the new event
 * and counts it, so a consumer that stops polling never stalls acquisition.
 */

#ifndef ACQ_CONTROL_QUEUE_H
#define ACQ_CONTROL_QUEUE_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4324) // padding from cache-line alignment is intended
#endif

namespace mne::detail
{

//=============================================================================================================
/**
 * Bounded lock-free queue of trivially copyable records.
 *
 * @tparam T          Record type.
 * @tparam Capacity   Number of records, a power of two.
 */
template<typename T, std::size_t Capacity>
class AcqControlQueue
{
    static_assert(std::is_trivially_copyable_v<T>, "records are copied without locks");
    static_assert(Capacity >= 2 && (Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

public:
    AcqControlQueue() noexcept
    {
        for (std::size_t i = 0; i < Capacity; ++i) {
            m_cells[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    AcqControlQueue(const AcqControlQueue&) = delete;
    AcqControlQueue& operator=(const AcqControlQueue&) = delete;

    /**
     * @param[in] record   The record to append.
     *
     * @return Whether it was queued; false (and counted as dropped) if the queue is full.
     */
    bool push(const T& record) noexcept
    {
        std::size_t position = m_enqueue.load(std::memory_order_relaxed);
        Cell* cell = nullptr;
        while (true) {
            cell = &m_cells[position & (Capacity - 1)];
            const std::size_t sequence = cell->sequence.load(std::memory_order_acquire);
            const auto difference = static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(position);
            if (difference == 0) {
                if (m_enqueue.compare_exchange_weak(position, position + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (difference < 0) {
                m_dropped.fetch_add(1, std::memory_order_relaxed);
                return false;
            } else {
                position = m_enqueue.load(std::memory_order_relaxed);
            }
        }
        cell->record = record;
        cell->sequence.store(position + 1, std::memory_order_release);
        return true;
    }

    /**
     * @param[out] record   The oldest record, if any.
     *
     * @return Whether a record was taken.
     */
    bool pop(T& record) noexcept
    {
        std::size_t position = m_dequeue.load(std::memory_order_relaxed);
        Cell* cell = nullptr;
        while (true) {
            cell = &m_cells[position & (Capacity - 1)];
            const std::size_t sequence = cell->sequence.load(std::memory_order_acquire);
            const auto difference = static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(position + 1);
            if (difference == 0) {
                if (m_dequeue.compare_exchange_weak(position, position + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (difference < 0) {
                return false;
            } else {
                position = m_dequeue.load(std::memory_order_relaxed);
            }
        }
        record = cell->record;
        cell->sequence.store(position + Capacity, std::memory_order_release);
        return true;
    }

    /** @return Records dropped because the queue was full. */
    [[nodiscard]] std::uint64_t dropped() const noexcept
    {
        return m_dropped.load(std::memory_order_relaxed);
    }

private:
    struct Cell
    {
        std::atomic<std::size_t> sequence{0};
        T record{};
    };

    std::array<Cell, Capacity> m_cells;
    alignas(64) std::atomic<std::size_t> m_enqueue{0};
    alignas(64) std::atomic<std::size_t> m_dequeue{0};
    std::atomic<std::uint64_t> m_dropped{0};
};

} // namespace mne::detail

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif // ACQ_CONTROL_QUEUE_H
