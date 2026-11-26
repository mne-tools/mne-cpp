//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     evoked_conditions.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Bookkeeping of the conditions (averages) an evoked view shows.
 */

#ifndef VIZ_EVOKED_CONDITIONS_H
#define VIZ_EVOKED_CONDITIONS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <map>
#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * Brings a per-condition setting (colour, visibility, ...) in line with the conditions an evoked view now shows.
 * A new condition gets the setting it had when it was last shown, else @p initial; the settings of conditions
 * no longer shown move to @p retired.
 *
 * @param[in, out] current      Setting per shown condition.
 * @param[in, out] retired      Setting per condition shown before.
 * @param[in] conditions        The conditions shown now.
 * @param[in] initial           Setting of a condition never shown before.
 *
 * @return Whether the number of shown conditions changed.
 *
 * @since 3.0.0
 */
template<typename T>
bool reconcile_conditions(std::map<std::string, T>& current, std::map<std::string, T>& retired,
                          const std::vector<std::string>& conditions, const T& initial)
{
    const std::size_t size_before = current.size();

    for (const std::string& condition : conditions) {
        if (current.contains(condition))
            continue;
        const auto it = retired.find(condition);
        current.insert_or_assign(condition, it != retired.end() ? it->second : initial);
    }

    std::erase_if(current, [&](const auto& item) {
        if (std::ranges::find(conditions, item.first) != conditions.end())
            return false;
        retired.insert_or_assign(item.first, item.second);
        return true;
    });

    return size_before != current.size();
}

//=============================================================================================================
/**
 * Counts the samples before the stimulus onset: those before the first time point that is exactly 0.
 *
 * @param[in] times   Time of each sample in seconds.
 *
 * @return The number of pre-stimulus samples, all samples if none is at 0.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT int pre_stimulus_samples(std::span<const float> times);

} // namespace mne::viz

#endif // VIZ_EVOKED_CONDITIONS_H
