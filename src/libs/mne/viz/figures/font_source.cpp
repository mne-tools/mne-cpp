//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     font_source.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of FontSource and shape_text.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "font_source.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::shared_ptr<const FontFace> FontSource::fallback_face(const Font& /*font*/, char32_t /*code_point*/) const
{
    return nullptr;
}

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

double mne::viz::font_pixel_size(const Font& font, double default_pixel_size)
{
    if (font.pixel_size > 0.0) {
        return font.pixel_size;
    }
    if (font.point_size > 0.0) {
        return font.point_size * 96.0 / 72.0;
    }
    return default_pixel_size;
}

//=============================================================================================================

GlyphRun mne::viz::shape_text(const FontSource& fonts, const Font& font, std::string_view text, double pixel_size)
{
    GlyphRun run;
    run.face = fonts.face(font);
    run.pixel_size = pixel_size;
    if (!run.face) {
        return run;
    }
    const FontFace& primary = *run.face;
    const double primaryScale = pixel_size / primary.units_per_em();
    run.ascent = std::round(primary.ascender() * primaryScale);
    run.descent = std::round(-primary.descender() * primaryScale);

    double pen = 0.0;
    const FontFace* previousFace = nullptr;
    std::uint32_t previous = 0;
    std::shared_ptr<const FontFace> lastFallback;
    char32_t lastFallbackFor = 0;
    for (const char32_t c : decode_utf8(text)) {
        std::shared_ptr<const FontFace> face = run.face;
        std::uint32_t glyph = primary.glyph_index(c);
        if (glyph == 0 && c >= 0x20) {
            if (lastFallbackFor != c) {
                lastFallback = fonts.fallback_face(font, c);
                lastFallbackFor = c;
            }
            if (lastFallback) {
                if (const std::uint32_t other = lastFallback->glyph_index(c); other != 0) {
                    face = lastFallback;
                    glyph = other;
                }
            }
        }
        const double scale = pixel_size / face->units_per_em();
        if (previousFace == face.get()) {
            pen += face->kerning(previous, glyph) * scale;
        }
        run.glyphs.push_back({face, glyph, pen});
        pen += face->advance(glyph) * scale;
        previousFace = face.get();
        previous = glyph;
    }
    run.advance = pen;
    return run;
}
