//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     average_layout.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AverageLayout places the AverageTiles of the channels at their positions in a sensor layout.
 */

#ifndef SCENE_AVERAGE_LAYOUT_H
#define SCENE_AVERAGE_LAYOUT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "average_tile.h"
#include "canvas.h"
#include "scene_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

/**
 * @brief The evoked responses of many channels, each in a tile at its place in a sensor layout.
 *
 * A channel at layout position (x, y) gets a 120 x 60 box centred on (160 x, -160 y) in scene coordinates, y
 * pointing down, as MNE-Python's plot_evoked_topo arranges them.
 *
 * @snippet ex_scene/main.cpp average_layout_usage
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT AverageLayout
{
public:
    /**
     * @param[in] position   Position of a channel in layout units, y pointing up.
     *
     * @return The box of its tile in scene coordinates.
     */
    static Rect box_at(Point position);

    /**
     * Sets the tiles, drawn in this order; their boxes are usually from box_at().
     *
     * @param[in] tiles   The tiles.
     */
    void set_tiles(std::vector<AverageTile> tiles);

    /** @return The tiles. */
    const std::vector<AverageTile>& tiles() const
    {
        return m_tiles;
    }

    /** @return The tiles, to change their data. */
    std::vector<AverageTile>& tiles()
    {
        return m_tiles;
    }

    /** @return The area of the tiles and their names in scene coordinates; empty without tiles. */
    Rect bounds() const;

    /**
     * @param[in] point   Point in scene coordinates.
     *
     * @return The index of the topmost tile whose box contains @p point, or -1.
     */
    int tile_at(Point point) const;

    /**
     * Draws the tiles in scene coordinates with the current transform of @p canvas.
     *
     * @param[in, out] canvas   Canvas to draw on.
     * @param[in]      font     Font of the channel names.
     */
    void draw(Canvas& canvas, const Font& font) const;

private:
    std::vector<AverageTile> m_tiles;
};

} // namespace mne::scene

#endif // SCENE_AVERAGE_LAYOUT_H
