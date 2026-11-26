//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     map_value.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Lookup with a fallback for standard maps, shared by the mne_disp sources; not installed.
 */

#ifndef DISPLIB_DETAIL_MAP_VALUE_H
#define DISPLIB_DETAIL_MAP_VALUE_H

namespace mne::viz::qt
{
namespace detail
{

/** The value stored under @p key in @p map, or @p fallback when there is none, like QMap::value(). */
template<class Map, class Key>
typename Map::mapped_type valueOr(const Map& map, const Key& key, typename Map::mapped_type fallback = {})
{
    const auto it = map.find(key);
    return it != map.end() ? it->second : fallback;
}

} // namespace detail
} // namespace mne::viz::qt

#endif // DISPLIB_DETAIL_MAP_VALUE_H
