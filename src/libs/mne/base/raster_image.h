//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     raster_image.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    An RGB image in memory with anti-aliased polygons, bitmap text and PNG output, in standard C++.
 *
 * A software rasteriser for images rendered without a window system, e.g. movie frames written by a
 * command-line tool, the counterpart of QImage with QPainter for those uses.
 */

#ifndef BASE_RASTER_IMAGE_H
#define BASE_RASTER_IMAGE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "base_global.h"
#include "error.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * An 8-bit RGB colour.
 *
 * @snippet ex_base/main.cpp raster_image_usage
 *
 * @since 3.0.0
 */
struct Rgb
{
    std::uint8_t r = 0; /**< Red. */
    std::uint8_t g = 0; /**< Green. */
    std::uint8_t b = 0; /**< Blue. */

    friend bool operator==(const Rgb&, const Rgb&) = default;
};

//=============================================================================================================
/**
 * A point in pixel coordinates; (0, 0) is the top-left corner of the top-left pixel.
 *
 * @snippet ex_base/main.cpp raster_image_usage
 *
 * @since 3.0.0
 */
struct PointF
{
    double x = 0.0; /**< Horizontal position, growing to the right. */
    double y = 0.0; /**< Vertical position, growing downwards. */
};

//=============================================================================================================
/**
 * @brief An RGB image in memory that fills polygons, draws text and writes PNG files.
 *
 * Polygons are filled with the non-zero winding rule and anti-aliased by 4 x 4 supersampling, so their edges
 * blend with what is underneath like QPainter's anti-aliased drawing.
 *
 * @snippet ex_base/main.cpp raster_image_usage
 *
 * @since 3.0.0
 */
class BASESHARED_EXPORT RasterImage
{
public:
    //=========================================================================================================
    /**
     * Creates an image filled with one colour.
     *
     * @param[in] width        Width in pixels; negative values count as 0.
     * @param[in] height       Height in pixels; negative values count as 0.
     * @param[in] background   Colour of every pixel.
     */
    RasterImage(int width, int height, Rgb background = {});

    /** @return The width in pixels. */
    int width() const
    {
        return m_width;
    }

    /** @return The height in pixels. */
    int height() const
    {
        return m_height;
    }

    //=========================================================================================================
    /**
     * @param[in] x   Column, 0 to width() - 1.
     * @param[in] y   Row, 0 to height() - 1.
     *
     * @return The colour of the pixel, or black outside the image.
     */
    Rgb pixel(int x, int y) const;

    //=========================================================================================================
    /**
     * Sets one pixel; pixels outside the image are ignored.
     *
     * @param[in] x       Column.
     * @param[in] y       Row.
     * @param[in] color   New colour.
     */
    void set_pixel(int x, int y, Rgb color);

    //=========================================================================================================
    /**
     * Sets every pixel to @p color.
     *
     * @param[in] color   New colour.
     */
    void fill(Rgb color);

    //=========================================================================================================
    /**
     * Fills a polygon, anti-aliased. Parts outside the image are clipped.
     *
     * @param[in] points    Corners in order; fewer than three fill nothing.
     * @param[in] color     Fill colour.
     * @param[in] opacity   Opacity of the fill, 0 (transparent) to 255 (opaque).
     */
    void fill_polygon(std::span<const PointF> points, Rgb color, int opacity = 255);

    //=========================================================================================================
    /**
     * Fills the area enclosed by several polygons at once, anti-aliased, with the non-zero winding rule over all
     * of them: polygons of the same orientation fill their union, so overlapping pieces of a stroke blend once.
     *
     * @param[in] polygons   Polygons, each with its corners in order; those with fewer than three are skipped.
     * @param[in] color      Fill colour.
     * @param[in] opacity    Opacity of the fill, 0 (transparent) to 255 (opaque).
     *
     * @since 3.0.0
     */
    void fill_polygons(std::span<const std::vector<PointF>> polygons, Rgb color, int opacity = 255);

    //=========================================================================================================
    /**
     * Mixes a colour into one pixel; pixels outside the image are ignored.
     *
     * @param[in] x       Column.
     * @param[in] y       Row.
     * @param[in] color   Colour to mix in.
     * @param[in] alpha   Weight of @p color, 0 (none) to 255 (replaces the pixel).
     *
     * @since 3.0.0
     */
    void blend_pixel(int x, int y, Rgb color, int alpha);

    //=========================================================================================================
    /**
     * Draws text with the built-in 5 x 7 pixel font, one pixel of spacing between characters.
     *
     * The font covers digits, letters (upper-case letters are drawn as lower-case ones) and the characters
     * space . , : ; + - = / % ( ) _ ; other characters are drawn as a box.
     *
     * @param[in] x          Left edge of the first character.
     * @param[in] baseline   Row below the last row of the characters.
     * @param[in] text       ASCII text.
     * @param[in] color      Text colour.
     * @param[in] scale      Size of a font pixel in image pixels, at least 1.
     */
    void draw_text(int x, int baseline, std::string_view text, Rgb color, int scale = 1);

    //=========================================================================================================
    /**
     * @return The pixels row by row from the top, three bytes (red, green, blue) per pixel.
     */
    std::span<const std::uint8_t> rgb() const
    {
        return m_rgb;
    }

    //=========================================================================================================
    /**
     * Writes the image as an 8-bit RGB PNG file.
     *
     * @param[in] path   Output file.
     *
     * @return Nothing, or an Io error if the file cannot be written.
     */
    [[nodiscard]] Expected<void> save_png(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * @return The image encoded as an 8-bit RGB PNG file.
     */
    std::vector<std::uint8_t> encode_png() const;

    //=========================================================================================================
    /**
     * Decodes a PNG file of 8-bit RGB or RGBA pixels without interlacing, e.g. one written by encode_png();
     * an alpha channel is dropped.
     *
     * @param[in] png   The file contents.
     *
     * @return The image, or a Format error for other or damaged files.
     *
     * @since 3.0.0
     */
    static Expected<RasterImage> decode_png(std::span<const std::uint8_t> png);

    //=========================================================================================================
    /**
     * Reads a PNG file as decode_png() does.
     *
     * @param[in] path   Input file.
     *
     * @return The image, an Io error if the file cannot be read, or a Format error.
     *
     * @since 3.0.0
     */
    static Expected<RasterImage> load_png(const std::filesystem::path& path);

private:
    void blend(int x, int y, Rgb color, int coverage);
    void fill_contours(std::span<const std::span<const PointF>> polygons, Rgb color, int opacity);

    int m_width = 0;
    int m_height = 0;
    std::vector<std::uint8_t> m_rgb;
};

} // namespace mne

#endif // BASE_RASTER_IMAGE_H
