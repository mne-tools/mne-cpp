//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     circular_buffer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2017
 * @brief    Bounded blocking queue that hands data from one thread to another.
 */

#ifndef BASE_CIRCULAR_BUFFER_H
#define BASE_CIRCULAR_BUFFER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <span>
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
 * @brief Fixed-capacity first-in first-out queue; push waits while it is full and pop while it is empty,
 *        each for at most the timeout.
 *
 * @snippet ex_base/main.cpp circular_buffer_usage
 */
template<typename T>
class CircularBuffer
{
public:
    //=========================================================================================================
    /**
     * @param[in] capacity   Number of elements the buffer holds.
     * @param[in] timeout    How long push and pop wait for room or data.
     */
    explicit CircularBuffer(std::size_t capacity, std::chrono::milliseconds timeout = std::chrono::seconds(1))
    : m_slots(capacity)
    , m_timeout(timeout)
    {
    }

    CircularBuffer(const CircularBuffer&) = delete;
    CircularBuffer& operator=(const CircularBuffer&) = delete;

    //=========================================================================================================
    /**
     * Appends @p element.
     *
     * @param[in] element   The element to append.
     *
     * @return Whether it was appended before the timeout.
     */
    template<class U>
    bool push(U&& element)
        requires std::is_constructible_v<T, U&&>
    {
        std::unique_lock lock(m_mutex);
        if (!m_notFull.wait_for(lock, m_timeout, [this] { return m_size < m_slots.size(); })) {
            return false;
        }
        m_slots[(m_head + m_size) % m_slots.size()] = std::forward<U>(element);
        ++m_size;
        m_notEmpty.notify_one();
        return true;
    }

    //=========================================================================================================
    /**
     * Appends all of @p elements, or none.
     *
     * @param[in] elements   The elements to append, in order.
     *
     * @return Whether there was room for all of them before the timeout.
     */
    bool push(std::span<const T> elements)
    {
        std::unique_lock lock(m_mutex);
        if (!m_notFull.wait_for(lock, m_timeout, [&] { return m_slots.size() - m_size >= elements.size(); })) {
            return false;
        }
        for (const T& element : elements) {
            m_slots[(m_head + m_size) % m_slots.size()] = element;
            ++m_size;
        }
        m_notEmpty.notify_all();
        return true;
    }

    //=========================================================================================================
    /**
     * Removes the oldest element.
     *
     * @param[out] element   Receives the element; unchanged if none arrived before the timeout.
     *
     * @return Whether an element was removed.
     */
    bool pop(T& element)
    {
        std::unique_lock lock(m_mutex);
        if (!m_notEmpty.wait_for(lock, m_timeout, [this] { return m_size > 0; })) {
            return false;
        }
        element = std::move(m_slots[m_head]);
        m_head = (m_head + 1) % m_slots.size();
        --m_size;
        m_notFull.notify_all();
        return true;
    }

    /** Drops all elements. */
    void clear()
    {
        std::lock_guard lock(m_mutex);
        m_head = 0;
        m_size = 0;
        m_notFull.notify_all();
    }

    /** @return The number of elements waiting to be popped. */
    std::size_t size() const
    {
        std::lock_guard lock(m_mutex);
        return m_size;
    }

    /** @return The number of elements the buffer holds. */
    std::size_t capacity() const noexcept
    {
        return m_slots.size();
    }

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_notFull;
    std::condition_variable m_notEmpty;
    std::vector<T> m_slots;
    std::size_t m_head = 0;
    std::size_t m_size = 0;
    std::chrono::milliseconds m_timeout;
};

} // namespace mne

#endif // BASE_CIRCULAR_BUFFER_H
