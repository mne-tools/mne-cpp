//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     canvas.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Canvas records drawing calls in the style of QPainter into a DisplayList.
 */

#ifndef VIZ_CANVAS_H
#define VIZ_CANVAS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "display_list.h"
#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief Records drawing calls into a DisplayList.
 *
 * The calls and their state (pen, brush, font, transform, clip, save and restore) follow QPainter, so code
 * that painted a widget becomes code that builds a figure any backend can draw. Text takes the pen's colour.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT Canvas
{
public:
    //=========================================================================================================
    /**
     * Starts an empty figure.
     *
     * @param[in] width        Width in pixels.
     * @param[in] height       Height in pixels.
     * @param[in] background   Fill of the whole figure, if any.
     */
    Canvas(double width, double height, std::optional<Color> background = std::nullopt);

    /** @return The width in pixels. */
    double width() const
    {
        return m_list.width;
    }

    /** @return The height in pixels. */
    double height() const
    {
        return m_list.height;
    }

    /** Saves pen, brush, font, transform, clip, antialiasing and opacity; restore() brings them back. */
    void save();

    /** Restores the state of the matching save(); without one it does nothing. */
    void restore();

    /**
     * Moves the origin by (@p dx, @p dy) in the current coordinates.
     *
     * @param[in] dx   Horizontal offset.
     * @param[in] dy   Vertical offset.
     */
    void translate(double dx, double dy);

    /**
     * Rotates the coordinates by @p degrees, clockwise on screen.
     *
     * @param[in] degrees   Angle in degrees, clockwise on screen.
     */
    void rotate(double degrees);

    /**
     * Scales the coordinates by @p sx horizontally and @p sy vertically.
     *
     * @param[in] sx   Horizontal factor.
     * @param[in] sy   Vertical factor.
     */
    void scale(double sx, double sy);

    /** @return The current transform from drawing coordinates to figure pixels. */
    const Transform& transform() const
    {
        return m_state.transform;
    }

    /**
     * Limits drawing to @p rect (in the current coordinates), within any earlier clip.
     *
     * @param[in] rect   Rectangle in the current coordinates.
     */
    void clip_rect(const Rect& rect);

    /**
     * Turns anti-aliased edges on or off (off by default, as in QPainter).
     *
     * @param[in] on   Whether edges are anti-aliased.
     */
    void set_antialiasing(bool on);

    /**
     * Sets the opacity of what is drawn next, 0 (invisible) to 1 (default), like QPainter::setOpacity.
     *
     * @param[in] opacity   Opacity, 0 to 1.
     */
    void set_opacity(double opacity);

    /**
     * Sets the outline and text colour.
     *
     * @param[in] pen   Pen.
     */
    void set_pen(const Pen& pen);

    /**
     * Sets a solid one-pixel pen of @p color.
     *
     * @param[in] color   Line and text colour.
     */
    void set_pen(Color color);

    /**
     * Sets the fill.
     *
     * @param[in] brush   Brush.
     */
    void set_brush(const Brush& brush);

    /**
     * Sets the font.
     *
     * @param[in] font   Font of the text drawn next.
     */
    void set_font(const Font& font);

    /** @return The current pen. */
    const Pen& pen() const
    {
        return m_state.pen;
    }

    /** @return The current brush. */
    const Brush& brush() const
    {
        return m_state.brush;
    }

    /** @return The current font. */
    const Font& font() const
    {
        return m_state.font;
    }

    /**
     * Draws a line from @p from to @p to with the pen.
     *
     * @param[in] from   Start point.
     * @param[in] to   End point.
     */
    void draw_line(Point from, Point to);

    /**
     * Draws connected lines through @p points with the pen.
     *
     * @param[in] points   Points in order.
     */
    void draw_polyline(std::span<const Point> points);

    /**
     * Draws connected cubic Bézier segments with the pen.
     *
     * @param[in] points   The start, then for each segment two control points and its end (3 n + 1 points).
     */
    void draw_bezier(std::span<const Point> points);

    /**
     * Draws a closed polygon: filled with the brush, outlined with the pen.
     *
     * @param[in] points   Points in order.
     */
    void draw_polygon(std::span<const Point> points);

    /**
     * Draws a rectangle: filled with the brush, outlined with the pen.
     *
     * @param[in] rect   Rectangle in the current coordinates.
     */
    void draw_rect(const Rect& rect);

    /**
     * Draws a rectangle with corners rounded by the radii @p rx and @p ry.
     *
     * @param[in] rect   Rectangle in the current coordinates.
     * @param[in] rx   Horizontal radius.
     * @param[in] ry   Vertical radius.
     */
    void draw_rounded_rect(const Rect& rect, double rx, double ry);

    /**
     * Draws an ellipse around @p center with the radii @p rx and @p ry.
     *
     * @param[in] center   Centre.
     * @param[in] rx   Horizontal radius.
     * @param[in] ry   Vertical radius.
     */
    void draw_ellipse(Point center, double rx, double ry);

    /**
     * Fills @p rect with @p brush, without outline and without changing the brush.
     *
     * @param[in] rect   Rectangle in the current coordinates.
     * @param[in] brush   Brush.
     */
    void fill_rect(const Rect& rect, const Brush& brush);

    /**
     * Draws @p text placed in @p box by @p align, in the pen's colour.
     *
     * @param[in] box   Rectangle the text is aligned in.
     * @param[in] align   Alignment in the rectangle.
     * @param[in] text   UTF-8 text.
     */
    void draw_text(const Rect& box, Align align, std::string_view text);

    /**
     * Draws @p text with its baseline starting at @p baseline.
     *
     * @param[in] baseline   Start of the baseline.
     * @param[in] text   UTF-8 text.
     */
    void draw_text(Point baseline, std::string_view text);

    /**
     * Draws @p image scaled into @p target.
     *
     * @param[in] target   Rectangle the image fills.
     * @param[in] image   Image.
     * @param[in] smooth   Bilinear instead of nearest-pixel scaling.
     */
    void draw_image(const Rect& target, std::shared_ptr<const Image> image, bool smooth = false);

    /** @return The figure recorded so far. */
    const DisplayList& display_list() const
    {
        return m_list;
    }

    /** @return The recorded figure, moved out; the canvas is empty afterwards. */
    DisplayList take();

private:
    struct State
    {
        Pen pen;
        Brush brush;
        Font font;
        Transform transform;
        std::vector<Clip> clips;
        bool antialias = false;
        double opacity = 1.0;
    };

    void add(Command::Shape shape);

    DisplayList m_list;
    State m_state;
    std::vector<State> m_saved;
};

} // namespace mne::viz

#endif // VIZ_CANVAS_H
