//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     raster_renderer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Renders a DisplayList into a RasterImage, without a window system.
 *
 * The software backend of the scene library: tools render figures into PNG files with it, and tests compare
 * its pixels with golden images. Shapes are anti-aliased; text uses RasterImage's built-in bitmap font, so it
 * is legible but not typeset.
 */

#ifndef VIZ_RASTER_RENDERER_H
#define VIZ_RASTER_RENDERER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "display_list.h"
#include "../viz_global.h"
#include <mne/base/raster_image.h>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * Options of to_raster().
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
struct RasterOptions
{
    Color background = Color::white(); /**< Fill of a figure without background. */
    double scale = 1.0;                /**< Image pixels per figure pixel, e.g. 2 for a high-resolution image. */
};

/**
 * Renders @p list into an image of its size times @p options.scale.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @param[in] list      Figure.
 * @param[in] options   Background and scale.
 *
 * @return The image.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT RasterImage to_raster(const DisplayList& list, const RasterOptions& options = {});

} // namespace mne::viz

#endif // VIZ_RASTER_RENDERER_H
