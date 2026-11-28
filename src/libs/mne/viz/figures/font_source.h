//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     font_source.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FontSource picks font faces for viz::Font requests; shape_text() places the glyphs of a string.
 *
 * The software renderer draws, and FontTextMetrics measures, the same GlyphRun, so layouts measure exactly
 * what is drawn.
 */

#ifndef VIZ_FONT_SOURCE_H
#define VIZ_FONT_SOURCE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "font_face.h"
#include "style.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief Chooses the font face that draws a viz::Font, and faces for characters it lacks.
 *
 * Implementations are thread-safe.
 *
 * @snippet ex_scene/main.cpp font_library_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT FontSource
{
public:
    virtual ~FontSource() = default;

    //=========================================================================================================
    /**
     * @param[in] font   Requested family, weight and style; the size is ignored.
     *
     * @return The face that draws @p font, or nullptr if there is none.
     *
     * @since 3.0.0
     */
    virtual std::shared_ptr<const FontFace> face(const Font& font) const = 0;

    //=========================================================================================================
    /**
     * @param[in] font         Requested font, whose face() lacks @p code_point.
     * @param[in] code_point   Character to draw.
     *
     * @return Another face that has a glyph for @p code_point, or nullptr to draw the missing-glyph box.
     *
     * @since 3.0.0
     */
    virtual std::shared_ptr<const FontFace> fallback_face(const Font& font, char32_t code_point) const;
};

/**
 * @brief A glyph placed on the baseline.
 *
 * @since 3.0.0
 */
struct PlacedGlyph
{
    std::shared_ptr<const FontFace> face; /**< Face the glyph belongs to. */
    std::uint32_t glyph = 0;              /**< Glyph index in face. */
    double x = 0.0;                       /**< Pen position in pixels from the start of the run. */
};

/**
 * @brief The glyphs of a string at a pixel size.
 *
 * @since 3.0.0
 */
struct GlyphRun
{
    std::shared_ptr<const FontFace> face; /**< The primary face; nullptr if the source has none for the font. */
    std::vector<PlacedGlyph> glyphs;      /**< Glyphs in order, one per character. */
    double pixel_size = 0.0;              /**< Pixels per em. */
    double advance = 0.0;                 /**< Width of the run in pixels, kerning included. */
    double ascent = 0.0;                  /**< Ascent of the primary face in whole pixels. */
    double descent = 0.0;                 /**< Descent of the primary face in whole pixels, positive. */
    double cap_height = 0.0;              /**< Height of capitals of the primary face in pixels. */
};

//=============================================================================================================
/**
 * Places the glyphs of a string: advances in fractional pixels, pair kerning within a face, and characters
 * missing from the primary face taken from FontSource::fallback_face().
 *
 * @param[in] fonts        Source of the faces.
 * @param[in] font         Font of the text.
 * @param[in] text         UTF-8 text.
 * @param[in] pixel_size   Pixels per em.
 *
 * @return The run; without a face if @p fonts has none for @p font.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT GlyphRun shape_text(const FontSource& fonts, const Font& font, std::string_view text, double pixel_size);

//=============================================================================================================
/**
 * @param[in] font                 Font.
 * @param[in] default_pixel_size   Size of a font without size.
 *
 * @return The pixel size of @p font, its point size converted at 96 dots per inch.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT double font_pixel_size(const Font& font, double default_pixel_size = 13.0);

} // namespace mne::viz

#endif // VIZ_FONT_SOURCE_H
