//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     style.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Colours, pens, brushes, fonts and text alignment of the scene library.
 *
 * The defaults match QPainter's (a pen is solid, one pixel wide, with square caps and bevel joins), so a
 * figure drawn through the QPainter backend looks like one drawn with QPainter directly.
 */

#ifndef VIZ_STYLE_H
#define VIZ_STYLE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "geometry.h"
#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * An 8-bit RGBA colour, not premultiplied.
 *
 * @since 3.0.0
 */
struct Color
{
    std::uint8_t r = 0;   /**< Red. */
    std::uint8_t g = 0;   /**< Green. */
    std::uint8_t b = 0;   /**< Blue. */
    std::uint8_t a = 255; /**< Opacity, 0 (transparent) to 255 (opaque). */

    friend bool operator==(const Color&, const Color&) = default;

    /** @return Opaque black. */
    static constexpr Color black()
    {
        return {0, 0, 0, 255};
    }

    /** @return Opaque white. */
    static constexpr Color white()
    {
        return {255, 255, 255, 255};
    }

    /** @return Fully transparent black. */
    static constexpr Color transparent()
    {
        return {0, 0, 0, 0};
    }
};

/**
 * How a pen dashes its line; the patterns are those of Qt::PenStyle, in multiples of the pen width.
 *
 * @since 3.0.0
 */
enum class LineStyle
{
    None,      /**< No line. */
    Solid,     /**< Continuous line. */
    Dash,      /**< Dashes 4, gaps 2. */
    Dot,       /**< Dots 1, gaps 2. */
    DashDot,   /**< Dash 4, gap 2, dot 1, gap 2. */
    DashDotDot /**< Dash 4, gap 2, dot 1, gap 2, dot 1, gap 2. */
};

/**
 * The end of an open line.
 *
 * @since 3.0.0
 */
enum class LineCap
{
    Flat,   /**< Ends at the end point. */
    Square, /**< Extends half the pen width past the end point. */
    Round   /**< A half circle around the end point. */
};

/**
 * The corner between two segments.
 *
 * @since 3.0.0
 */
enum class LineJoin
{
    Miter, /**< Extended edges meet in a point. */
    Bevel, /**< The outer corner is cut. */
    Round  /**< A circular arc. */
};

/**
 * The outline of shapes and the colour of text.
 *
 * @since 3.0.0
 */
struct Pen
{
    Color color = Color::black();       /**< Line and text colour. */
    double width = 1.0;                 /**< Line width; 0 is one device pixel whatever the transform. */
    LineStyle style = LineStyle::Solid; /**< Dash pattern, or None for no outline. */
    LineCap cap = LineCap::Square;      /**< End of open lines. */
    LineJoin join = LineJoin::Bevel;    /**< Corners. */

    friend bool operator==(const Pen&, const Pen&) = default;

    /**
     * @param[in] textColor   Colour of text drawn with the pen.
     *
     * @return A pen that draws no outline (text still takes its colour).
     */
    static Pen none(Color textColor = Color::black())
    {
        return {textColor, 1.0, LineStyle::None};
    }

    /** @return Whether the pen draws a line. */
    bool visible() const
    {
        return style != LineStyle::None && color.a > 0;
    }
};

/**
 * One colour of a gradient.
 *
 * @since 3.0.0
 */
struct GradientStop
{
    double position = 0.0; /**< 0 at the start point to 1 at the end point. */
    Color color;           /**< Colour at the position. */

    friend bool operator==(const GradientStop&, const GradientStop&) = default;
};

/**
 * The fill of shapes: nothing, one colour, or a linear gradient.
 *
 * @since 3.0.0
 */
struct Brush
{
    /** Kind of fill. */
    enum class Style
    {
        None,          /**< No fill. */
        Solid,         /**< One colour. */
        LinearGradient /**< Colours along the line from start to end, constant across it, padded beyond. */
    };

    Style style = Style::None;       /**< Kind of fill. */
    Color color;                     /**< Colour of a solid fill. */
    Point start;                     /**< Start of a gradient, in the coordinates of the shape. */
    Point end;                       /**< End of a gradient, in the coordinates of the shape. */
    std::vector<GradientStop> stops; /**< Gradient colours, by increasing position. */

    friend bool operator==(const Brush&, const Brush&) = default;

    /**
     * @param[in] color   Fill colour.
     *
     * @return A solid fill with @p color.
     */
    static Brush solid(Color color)
    {
        Brush brush;
        brush.style = Style::Solid;
        brush.color = color;
        return brush;
    }

    /**
     * @param[in] start   Start point, in the coordinates of the shape.
     * @param[in] end   End point, in the coordinates of the shape.
     * @param[in] stops   Colours by increasing position.
     *
     * @return A linear gradient from @p start to @p end.
     */
    static Brush linear_gradient(Point start, Point end, std::vector<GradientStop> stops)
    {
        Brush brush;
        brush.style = Style::LinearGradient;
        brush.start = start;
        brush.end = end;
        brush.stops = std::move(stops);
        return brush;
    }

    /** @return Whether the brush fills anything. */
    bool visible() const
    {
        return (style == Style::Solid && color.a > 0) || (style == Style::LinearGradient && !stops.empty());
    }

    /**
     * @param[in] t   Position along a gradient, 0 to 1 (clamped).
     *
     * @return The gradient colour at @p t, interpolated linearly between the stops.
     */
    VIZSHARED_EXPORT Color color_at(double t) const;
};

/**
 * A font request; a backend picks the closest font it has.
 *
 * Point sizes are converted with the backend's resolution, pixel sizes are exact. With neither size set,
 * the backend uses its default font size.
 *
 * @since 3.0.0
 */
struct Font
{
    std::string family;      /**< Family name, empty for the backend's default family. */
    double point_size = 0.0; /**< Size in points, 0 if unset. */
    double pixel_size = 0.0; /**< Size in pixels, 0 if unset; wins over point_size. */
    bool bold = false;       /**< Bold weight. */
    bool italic = false;     /**< Italic style. */

    friend bool operator==(const Font&, const Font&) = default;
};

/**
 * Placement of text in a rectangle, with the values of Qt::AlignmentFlag.
 *
 * @since 3.0.0
 */
enum class Align : unsigned
{
    Left = 0x1,       /**< Left edge. */
    Right = 0x2,      /**< Right edge. */
    HCenter = 0x4,    /**< Centred horizontally. */
    Top = 0x20,       /**< Top edge. */
    Bottom = 0x40,    /**< Bottom edge. */
    VCenter = 0x80,   /**< Centred vertically. */
    Baseline = 0x100, /**< The baseline at the top edge (text drawn at a point). */
    Center = 0x84     /**< HCenter | VCenter. */
};

/**
 * @param[in] a   First alignment.
 * @param[in] b   Second alignment.
 *
 * @return The union of two alignments.
 */
constexpr Align operator|(Align a, Align b)
{
    return static_cast<Align>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}

/**
 * @param[in] align   Alignment.
 * @param[in] flag   Flag to test for.
 *
 * @return Whether @p align contains all bits of @p flag.
 */
constexpr bool has_flag(Align align, Align flag)
{
    return (static_cast<unsigned>(align) & static_cast<unsigned>(flag)) == static_cast<unsigned>(flag);
}

} // namespace mne::viz

#endif // VIZ_STYLE_H
