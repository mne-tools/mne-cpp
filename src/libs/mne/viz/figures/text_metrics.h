//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     text_metrics.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TextMetrics measures text for layout; ApproximateTextMetrics does so without fonts.
 *
 * Figures size margins and legends by their text. A window-system backend measures with its real fonts (the
 * QPainter backend with QFontMetrics); headless rendering and golden tests use ApproximateTextMetrics, which
 * gives the same numbers on every platform.
 */

#ifndef VIZ_TEXT_METRICS_H
#define VIZ_TEXT_METRICS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "style.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief Measures text in a font.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT TextMetrics
{
public:
    virtual ~TextMetrics() = default;

    /**
     * @param[in] font   Font of the text.
     *
     * @return The font size in pixels.
     */
    virtual double pixel_size(const Font& font) const = 0;

    /**
     * @param[in] font   Font of the text.
     * @param[in] text   UTF-8 text.
     *
     * @return The width of @p text in @p font, in pixels.
     */
    virtual double advance(const Font& font, std::string_view text) const = 0;

    /**
     * @param[in] font   Font of the text.
     *
     * @return The distance from the baseline to the top of the tallest glyphs.
     */
    virtual double ascent(const Font& font) const = 0;

    /**
     * @param[in] font   Font of the text.
     *
     * @return The distance from the baseline to the bottom of the lowest glyphs.
     */
    virtual double descent(const Font& font) const = 0;

    /**
     * @param[in] font   Font of the text.
     *
     * @return The line height, ascent() + descent() (+ leading).
     */
    virtual double height(const Font& font) const
    {
        return ascent(font) + descent(font);
    }
};

/**
 * @brief Text metrics from the font size alone, identical on every platform.
 *
 * A character advances 0.6 of the pixel size (wide enough for digits in common sans-serif fonts), the ascent
 * is 0.8 and the descent 0.2 of it; results are rounded to whole pixels like QFontMetrics. A font without
 * a size has the default pixel size, and points convert at 96 dots per inch.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT ApproximateTextMetrics : public TextMetrics
{
public:
    /**
     * @param[in] defaultPixelSize   Pixel size of a font without size.
     */
    explicit ApproximateTextMetrics(double defaultPixelSize = 13.0);

    double pixel_size(const Font& font) const override;
    double advance(const Font& font, std::string_view text) const override;
    double ascent(const Font& font) const override;
    double descent(const Font& font) const override;

private:
    double m_defaultPixelSize;
};

/**
 * @param[in] font      Font.
 * @param[in] factor    Scale factor of its size.
 * @param[in] metrics   Size of a font without size.
 *
 * @return @p font with its point or pixel size times @p factor.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT Font scaled_font(const Font& font, double factor, const TextMetrics& metrics);

/**
 * @param[in] font      Font.
 * @param[in] points    Points to add to its size (negative to shrink it), at least 1 point remains.
 * @param[in] metrics   Size of a font without size.
 *
 * @return @p font, its size changed by @p points like QFont::setPointSize(pointSize() + points).
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT Font resized_font(const Font& font, double points, const TextMetrics& metrics);

} // namespace mne::viz

#endif // VIZ_TEXT_METRICS_H
