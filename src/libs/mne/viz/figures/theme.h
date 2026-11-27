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
    Color background{245, 246, 248};                  /**< Behind everything. */
    Color surface{255, 255, 255};                     /**< Cards that hold groups of rows. */
    Color field{255, 255, 255};                       /**< Fill of text fields, steppers and other controls. */
    Color field_border{208, 213, 220};                /**< Outline of controls. */
    Color text{24, 32, 42};                           /**< Labels and values. */
    Color muted_text{93, 104, 118};                   /**< Section titles, units and secondary text. */
    Color accent{31, 111, 235};                       /**< Checked boxes, buttons and the focus. */
    Color accent_text{255, 255, 255};                 /**< Text on the accent colour. */
    Color border{227, 230, 234};                      /**< Outline of cards. */
    Color separator{237, 239, 242};                   /**< Lines between the rows of a card. */
    Color positive{26, 127, 75};                      /**< Success, e.g. "Ready". */
    Color warning{178, 94, 0};                        /**< Needs attention. */
    Color negative{192, 54, 44};                      /**< Failure. */
    std::string font_family = "Source Sans 3";        /**< Font family of text; empty for the backend's default. */
    std::string mono_font_family = "Source Code Pro"; /**< Font family of codes such as colours; empty for monospace. */
    double font_size = 14.0;                          /**< Pixel size of labels and values. */
    double title_font_size = 20.0;                    /**< Pixel size of titles. */
    double small_font_size = 12.0;                    /**< Pixel size of section titles and status chips. */
    double padding = 20.0;                            /**< Margin around the content and inside cards. */
    double spacing = 8.0;                             /**< Gap between related elements. */
    double corner_radius = 6.0;                       /**< Corner radius of cards; controls use 4 px. */
    double row_height = 44.0;                         /**< Height of one row of a card. */
    double control_height = 30.0;                     /**< Height of controls in a row. */
    double editor_width = 200.0;                      /**< Preferred width of the editors on the right of rows. */

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

    //=========================================================================================================
    /**
     * @param[in] pixel_size    Pixel size of the font.
     *
     * @return A font of the theme's monospace family.
     *
     * @since 3.0.0
     */
    Font mono_font(double pixel_size) const;
};

} // namespace mne::viz

#endif // VIZ_THEME_H
