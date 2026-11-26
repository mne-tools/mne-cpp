//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     average_layout.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of AverageLayout.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "average_layout.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Rect AverageLayout::box_at(Point position)
{
    return {160.0 * position.x - 60.0, -160.0 * position.y - 30.0, 120.0, 60.0};
}

//=============================================================================================================

void AverageLayout::set_tiles(std::vector<AverageTile> tiles)
{
    m_tiles = std::move(tiles);
}

//=============================================================================================================

Rect AverageLayout::bounds() const
{
    Rect area;
    for (const AverageTile& tile : m_tiles) {
        // The name sits above the box
        area = area.united(tile.box.adjusted(0, -tile.name_size, 0, 0));
    }
    return area;
}

//=============================================================================================================

int AverageLayout::tile_at(Point point) const
{
    for (std::size_t i = m_tiles.size(); i-- > 0;) {
        if (m_tiles[i].box.contains(point)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

//=============================================================================================================

void AverageLayout::draw(Canvas& canvas, const Font& font) const
{
    for (const AverageTile& tile : m_tiles) {
        tile.draw(canvas, font);
    }
}
