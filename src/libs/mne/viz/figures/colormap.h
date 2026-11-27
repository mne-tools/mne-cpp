//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     colormap.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Colour maps (Jet, Hot, Bone, RedBlue, Cool, Viridis, ...) from values in [0, 1] to colours.
 *
 * The maps of mne::viz::qt::ColorMap, moved here so figures can be coloured without Qt.
 */

#ifndef VIZ_COLORMAP_H
#define VIZ_COLORMAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "style.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <optional>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * The colour maps.
 *
 * @since 3.0.0
 */
enum class Colormap
{
    Jet,            /**< Blue, cyan, yellow, red (MATLAB's jet). */
    Hot,            /**< Black, red, yellow, white. */
    HotNegative1,   /**< Hot, starting later. */
    HotNegative2,   /**< Hot, starting even later. */
    Bone,           /**< Grey-blue. */
    RedBlue,        /**< Blue at -1, white at 0, red at 1. */
    Cool,           /**< Cyan to magenta. */
    Viridis,        /**< Perceptually uniform, purple to yellow. */
    ViridisNegated, /**< Viridis reversed. */
    Mne             /**< MNE's diverging field map: cyan, blue, grey at 0.5, red, yellow (since 3.0.0). */
};

/**
 * @param[in] name   "Jet", "Hot", "HotNegative1", "HotNegative2", "Bone", "RedBlue", "Cool", "Viridis",
 *                   "ViridisNegated" or "MNE".
 *
 * @return The colour map of that name, or none.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT std::optional<Colormap> colormap_from_name(std::string_view name);

/**
 * @param[in] map     Colour map.
 * @param[in] value   Position in the map, 0 to 1; RedBlue takes -1 to 1.
 *
 * @return The opaque colour at @p value. Values outside the range wrap each channel to 8 bits as qRgb does;
 *         Viridis gives transparent black outside [0, 1].
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT Color map_color(Colormap map, double value);

/**
 * @param[in] name    Colour map name as for colormap_from_name(); unknown names give Jet.
 * @param[in] value   Position in the map, 0 to 1 for every map (RedBlue included).
 *
 * @return The colour at @p value.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT Color map_color(std::string_view name, double value);

/**
 * Colour from hue, saturation and lightness.
 *
 * @param[in] hue          Hue in [0, 1), wrapping around.
 * @param[in] saturation   Saturation in [0, 1].
 * @param[in] lightness    Lightness in [0, 1].
 * @param[in] alpha        Opacity in [0, 1].
 * @return The colour.
 * @since 3.0.0
 */
VIZSHARED_EXPORT Color color_from_hsl(double hue, double saturation, double lightness, double alpha = 1.0);

/**
 * Darker version of a colour, scaling its value (HSV) by 100 / @p factor like QColor::darker.
 *
 * @param[in] color    The colour.
 * @param[in] factor   Darkening in percent; 150 gives a colour at two thirds of the value. 100 or less returns @p color.
 * @return The darker colour, with the opacity of @p color.
 * @since 3.0.0
 */
VIZSHARED_EXPORT Color darker(Color color, int factor = 200);

} // namespace mne::viz

#endif // VIZ_COLORMAP_H
