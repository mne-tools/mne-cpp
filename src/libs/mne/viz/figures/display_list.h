//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     display_list.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    A figure as an ordered list of drawing commands, independent of any window system.
 *
 * Every command carries the full state it is drawn with (transform, clip, antialiasing, pen, brush, font), so
 * a backend draws each command on its own: QPainter on a widget, an SVG document, or a RasterImage.
 */

#ifndef VIZ_DISPLAY_LIST_H
#define VIZ_DISPLAY_LIST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "geometry.h"
#include "../viz_global.h"
#include "style.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * An RGBA image, four bytes per pixel row by row from the top, not premultiplied.
 *
 * @since 3.0.0
 */
struct Image
{
    int width = 0;                  /**< Width in pixels. */
    int height = 0;                 /**< Height in pixels. */
    std::vector<std::uint8_t> rgba; /**< width * height * 4 bytes. */

    /**
     * @param[in] w   Width in pixels.
     * @param[in] h   Height in pixels.
     *
     * @return A transparent image of the size.
     */
    static Image transparent(int w, int h)
    {
        Image image;
        image.width = w > 0 ? w : 0;
        image.height = h > 0 ? h : 0;
        image.rgba.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4, 0);
        return image;
    }

    /**
     * @param[in] x   Column.
     * @param[in] y   Row.
     *
     * @return The colour of pixel (@p x, @p y), which must lie in the image.
     */
    Color pixel(int x, int y) const
    {
        const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4;
        return {rgba[i], rgba[i + 1], rgba[i + 2], rgba[i + 3]};
    }

    /**
     * Sets pixel (@p x, @p y), which must lie in the image.
     *
     * @param[in] x   Column.
     * @param[in] y   Row.
     * @param[in] color   Colour.
     */
    void set_pixel(int x, int y, Color color)
    {
        const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4;
        rgba[i] = color.r;
        rgba[i + 1] = color.g;
        rgba[i + 2] = color.b;
        rgba[i + 3] = color.a;
    }
};

/**
 * A straight line.
 *
 * @since 3.0.0
 */
struct LineShape
{
    Point from; /**< Start point. */
    Point to;   /**< End point. */
    Pen pen;    /**< Line. */
};

/**
 * A polyline or a chain of cubic Bézier segments, closed into a polygon or not.
 *
 * @since 3.0.0
 */
struct PathShape
{
    std::vector<Point> points; /**< Corners in order; for a cubic path the start, then two control points and the end of each segment. */
    bool closed = false;       /**< Whether the last point connects to the first. */
    bool cubic = false;        /**< Whether the points describe cubic Bézier segments. */
    Pen pen;                   /**< Outline. */
    Brush brush;               /**< Fill of a closed path. */
};

/**
 * A rectangle with optionally rounded corners.
 *
 * @since 3.0.0
 */
struct RectShape
{
    Rect rect;       /**< Rectangle. */
    double rx = 0.0; /**< Horizontal corner radius. */
    double ry = 0.0; /**< Vertical corner radius. */
    Pen pen;         /**< Outline. */
    Brush brush;     /**< Fill. */
};

/**
 * An ellipse.
 *
 * @since 3.0.0
 */
struct EllipseShape
{
    Point center;    /**< Centre. */
    double rx = 0.0; /**< Horizontal radius. */
    double ry = 0.0; /**< Vertical radius. */
    Pen pen;         /**< Outline. */
    Brush brush;     /**< Fill. */
};

/**
 * A line of text placed in a rectangle, or at a baseline point when the alignment has Align::Baseline.
 *
 * @since 3.0.0
 */
struct TextShape
{
    Rect box;                  /**< Rectangle the text is aligned in; for Align::Baseline its top-left corner is the start of the baseline. */
    Align align = Align::Left; /**< Placement in @c box. */
    std::string text;          /**< UTF-8 text. */
    Font font;                 /**< Font. */
    Color color;               /**< Colour. */
};

/**
 * An image scaled into a rectangle.
 *
 * @since 3.0.0
 */
struct ImageShape
{
    Rect target;                        /**< Rectangle the image fills. */
    std::shared_ptr<const Image> image; /**< Pixels. */
    bool smooth = false;                /**< Bilinear instead of nearest-pixel scaling. */
};

/**
 * A clip rectangle in the coordinates of its own transform.
 *
 * @since 3.0.0
 */
struct Clip
{
    Rect rect;           /**< Visible rectangle. */
    Transform transform; /**< Transform in effect when the clip was set. */

    friend bool operator==(const Clip&, const Clip&) = default;
};

/**
 * One drawing command with the state it is drawn with.
 *
 * @since 3.0.0
 */
struct Command
{
    using Shape = std::variant<LineShape, PathShape, RectShape, EllipseShape, TextShape, ImageShape>; /**< The shape kinds. */

    Shape shape;             /**< What is drawn. */
    Transform transform;     /**< Maps shape coordinates to figure pixels. */
    std::vector<Clip> clips; /**< Drawing is limited to the intersection of these. */
    bool antialias = false;  /**< Whether edges are anti-aliased (the QPainter backend honours it). */
    double opacity = 1.0;    /**< Opacity of the whole shape, 0 to 1, on top of its colours' own. */
};

/**
 * A figure: a size, an optional background and the commands in drawing order.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
struct DisplayList
{
    double width = 0.0;              /**< Width in pixels. */
    double height = 0.0;             /**< Height in pixels. */
    std::optional<Color> background; /**< Fill of the whole figure, if any. */
    std::vector<Command> commands;   /**< Commands in drawing order. */
};

} // namespace mne::viz

#endif // VIZ_DISPLAY_LIST_H
