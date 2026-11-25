//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     average_tile.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AverageTile draws the evoked responses of one channel in a small box of a sensor layout.
 */

#ifndef SCENE_AVERAGE_TILE_H
#define SCENE_AVERAGE_TILE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "canvas.h"
#include "scene_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

/**
 * @brief The evoked responses of one channel in its box of a sensor layout, as MNE-Python's plot_evoked_topo.
 *
 * Draws a red stimulus line and zero line, the averages scaled so that the channel's range fills half the box
 * above and below the zero line, and the channel name above the box. Lines are one device pixel wide whatever
 * the zoom of the layout. Bad channels are drawn faint.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT AverageTile
{
public:
    /** One average. */
    struct Trace
    {
        std::vector<double> samples; /**< Values, one per sample. */
        Color color;                 /**< Line colour. */
    };

    Rect box;                  /**< The box in layout coordinates. */
    std::string name;          /**< Channel name, drawn above the box. */
    double name_size = 3.0;    /**< Point size of the name. */
    double max_value = 1.0;    /**< Value drawn at the top edge. */
    bool bad = false;          /**< Bad channels are drawn faint. */
    int stim_sample = 0;       /**< Sample of the stimulus. */
    int columns = 0;           /**< Samples per average, which place the stimulus line; no lines if 0. */
    std::vector<Trace> traces; /**< Averages, drawn in order. */

    /**
     * Draws the tile with the current transform of @p canvas.
     *
     * @param[in] canvas   Canvas.
     * @param[in] font     Font of the name; its size is replaced by name_size.
     */
    void draw(Canvas& canvas, const Font& font) const;
};

} // namespace mne::scene

#endif // SCENE_AVERAGE_TILE_H
