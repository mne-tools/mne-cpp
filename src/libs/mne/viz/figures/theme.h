//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     theme.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Theme: colours, font sizes and metrics shared by the toolkit-free views of all backends.
 */

#ifndef VIZ_THEME_H
#define VIZ_THEME_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "style.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief The look of toolkit-free views such as FormView: one calm palette, font sizes and metrics.
 *
 * All lengths are in logical units. Backends draw the same Theme, so a panel looks alike in every window.
 *
 * @since 3.0.0
 */
struct VIZSHARED_EXPORT Theme
{
    Color background{245, 246, 248};  /**< Behind everything. */
    Color surface{255, 255, 255};     /**< Cards that hold groups of rows. */
    Color field{246, 247, 249};       /**< Fill of text fields, steppers and other controls. */
    Color text{31, 35, 40};           /**< Labels and values. */
    Color muted_text{107, 114, 128};  /**< Group titles, units and secondary text. */
    Color accent{47, 111, 222};       /**< Switches that are on, buttons and the focus. */
    Color accent_text{255, 255, 255}; /**< Text on the accent colour. */
    Color border{221, 225, 230};      /**< Outlines and separators. */
    Color positive{26, 127, 75};      /**< Success, e.g. "Ready". */
    Color warning{178, 106, 0};       /**< Needs attention. */
    Color negative{201, 60, 55};      /**< Failure. */
    std::string font_family;          /**< Font family; empty for the backend's default. */
    double font_size = 13.0;          /**< Pixel size of labels and values. */
    double title_font_size = 18.0;    /**< Pixel size of titles. */
    double small_font_size = 11.0;    /**< Pixel size of group titles. */
    double padding = 16.0;            /**< Margin around the content and inside cards. */
    double spacing = 8.0;             /**< Gap between related elements. */
    double corner_radius = 8.0;       /**< Corner radius of cards; controls use half of it. */
    double row_height = 36.0;         /**< Height of one row of a card. */
    double editor_width = 200.0;      /**< Preferred width of the editors on the right of rows. */

    //=========================================================================================================
    /**
     * @return The default light theme.
     *
     * @since 3.0.0
     */
    static Theme light();

    //=========================================================================================================
    /**
     * @return A dark variant of the light theme with the same metrics.
     *
     * @since 3.0.0
     */
    static Theme dark();

    //=========================================================================================================
    /**
     * @param[in] pixel_size    Pixel size of the font.
     * @param[in] bold          Whether the font is bold.
     *
     * @return A font of the theme's family.
     *
     * @since 3.0.0
     */
    Font font(double pixel_size, bool bold = false) const;
};

} // namespace mne::viz

#endif // VIZ_THEME_H
