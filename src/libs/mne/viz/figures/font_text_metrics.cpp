//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     font_text_metrics.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of FontTextMetrics.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "font_text_metrics.h"

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

FontTextMetrics::FontTextMetrics(std::shared_ptr<const FontSource> fonts, double default_pixel_size)
: m_fonts(std::move(fonts))
, m_approximate(default_pixel_size)
, m_defaultPixelSize(default_pixel_size > 0.0 ? default_pixel_size : 13.0)
{
}

//=============================================================================================================

double FontTextMetrics::pixel_size(const Font& font) const
{
    return font_pixel_size(font, m_defaultPixelSize);
}

//=============================================================================================================

double FontTextMetrics::advance(const Font& font, std::string_view text) const
{
    if (m_fonts) {
        if (const GlyphRun run = shape_text(*m_fonts, font, text, pixel_size(font)); run.face) {
            return run.advance;
        }
    }
    return m_approximate.advance(font, text);
}

//=============================================================================================================

double FontTextMetrics::ascent(const Font& font) const
{
    if (m_fonts) {
        if (const GlyphRun run = shape_text(*m_fonts, font, {}, pixel_size(font)); run.face) {
            return run.ascent;
        }
    }
    return m_approximate.ascent(font);
}

//=============================================================================================================

double FontTextMetrics::descent(const Font& font) const
{
    if (m_fonts) {
        if (const GlyphRun run = shape_text(*m_fonts, font, {}, pixel_size(font)); run.face) {
            return run.descent;
        }
    }
    return m_approximate.descent(font);
}

//=============================================================================================================

double FontTextMetrics::cap_height(const Font& font) const
{
    if (m_fonts) {
        if (const GlyphRun run = shape_text(*m_fonts, font, {}, pixel_size(font)); run.face) {
            return run.cap_height;
        }
    }
    return m_approximate.cap_height(font);
}
