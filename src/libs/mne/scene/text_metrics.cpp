//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     text_metrics.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of ApproximateTextMetrics.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "text_metrics.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

namespace
{

// Code points of UTF-8 text: every byte except continuation bytes starts one
double codePoints(std::string_view text)
{
    double count = 0.0;
    for (const char c : text) {
        if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) {
            count += 1.0;
        }
    }
    return count;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ApproximateTextMetrics::ApproximateTextMetrics(double defaultPixelSize)
: m_defaultPixelSize(defaultPixelSize > 0.0 ? defaultPixelSize : 13.0)
{
}

//=============================================================================================================

double ApproximateTextMetrics::pixel_size(const Font& font) const
{
    if (font.pixel_size > 0.0) {
        return font.pixel_size;
    }
    if (font.point_size > 0.0) {
        return font.point_size * 96.0 / 72.0;
    }
    return m_defaultPixelSize;
}

//=============================================================================================================

double ApproximateTextMetrics::advance(const Font& font, std::string_view text) const
{
    const double perCharacter = (font.bold ? 0.65 : 0.6) * pixel_size(font);
    return std::round(perCharacter * codePoints(text));
}

//=============================================================================================================

double ApproximateTextMetrics::ascent(const Font& font) const
{
    return std::round(0.8 * pixel_size(font));
}

//=============================================================================================================

double ApproximateTextMetrics::descent(const Font& font) const
{
    return std::round(0.2 * pixel_size(font));
}

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

Font mne::scene::scaled_font(const Font& font, double factor, const TextMetrics& metrics)
{
    Font result = font;
    if (font.pixel_size > 0.0) {
        result.pixel_size = font.pixel_size * factor;
    } else if (font.point_size > 0.0) {
        result.point_size = font.point_size * factor;
    } else {
        result.pixel_size = metrics.pixel_size(font) * factor;
    }
    return result;
}

//=============================================================================================================

Font mne::scene::resized_font(const Font& font, double points, const TextMetrics& metrics)
{
    Font result = font;
    if (font.point_size > 0.0) {
        result.point_size = std::max(1.0, font.point_size + points);
    } else {
        result.pixel_size = std::max(1.0, metrics.pixel_size(font) + points * 96.0 / 72.0);
    }
    return result;
}
