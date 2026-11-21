//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_ch_name.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Matching of channel names across naming conventions.
 *
 * Neuromag/MEGIN files name the same channel "MEG 0113" or "MEG0113", depending on the software that wrote them.
 * Names are read as stored; files of either convention are combined by matching names as MNE-C does: two names
 * denote the same channel if they are equal once spaces are dropped and letter case is ignored.
 */

#ifndef FIFF_CH_NAME_H
#define FIFF_CH_NAME_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/**
 * @param[in] a   A channel name.
 * @param[in] b   Another channel name.
 *
 * @return Whether @p a and @p b name the same channel: equal once spaces are dropped and letter case is ignored,
 *         so "MEG 0113" matches "MEG0113" and "meg0113".
 */
FIFFSHARED_EXPORT bool same_channel(std::string_view a, std::string_view b) noexcept;

/**
 * @param[in] names   Channel names.
 * @param[in] name    The channel to find.
 *
 * @return The index of @p name in @p names, an exact match before one by same_channel(); -1 if there is none.
 */
FIFFSHARED_EXPORT int channel_index(const mne::StringList& names, std::string_view name) noexcept;

/**
 * @param[in] names   Channel names.
 * @param[in] name    The channel to look for.
 *
 * @return Whether @p names has channel @p name, see channel_index().
 */
inline bool has_channel(const mne::StringList& names, std::string_view name) noexcept
{
    return channel_index(names, name) >= 0;
}

} // namespace mne

#endif // FIFF_CH_NAME_H
