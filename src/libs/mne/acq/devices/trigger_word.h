//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trigger_word.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TriggerWord: events from the level of an amplifier's trigger input.
 */

#ifndef TRIGGER_WORD_H
#define TRIGGER_WORD_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <optional>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/**
 * Follows the trigger word sample by sample. Its value at the start is taken as idle, as inputs may idle high
 * or low; a change to any other value is an event.
 */
class TriggerWord
{
public:
    /** Forgets the idle value, for a new start. */
    void reset() noexcept
    {
        m_idle.reset();
    }

    /**
     * @param[in] word   The word of the next sample.
     *
     * @return true if @p word starts an event.
     */
    [[nodiscard]] bool changed(std::int32_t word) noexcept
    {
        if (!m_idle) {
            m_idle = word;
            m_word = word;
        }
        const bool event = word != m_word && word != *m_idle;
        m_word = word;
        return event;
    }

private:
    std::optional<std::int32_t> m_idle;
    std::int32_t m_word = 0;
};

} // namespace mne::detail

#endif // TRIGGER_WORD_H
