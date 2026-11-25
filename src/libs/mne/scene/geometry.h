//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     geometry.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Points, rectangles and affine transforms of the scene library.
 *
 * Coordinates are in pixels with the origin in the top-left corner and y growing downwards, the convention of
 * QPainter, SVG and image files. Transform follows QTransform: a transform maps a point, and composing two
 * transforms with @c * applies the left one first.
 */

#ifndef SCENE_GEOMETRY_H
#define SCENE_GEOMETRY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scene_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <numbers>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

/**
 * A point, or a vector between two points.
 *
 * @since 3.0.0
 */
struct Point
{
    double x = 0.0; /**< Horizontal position, growing to the right. */
    double y = 0.0; /**< Vertical position, growing downwards. */

    friend bool operator==(const Point&, const Point&) = default;

    friend Point operator+(Point a, Point b)
    {
        return {a.x + b.x, a.y + b.y};
    }

    friend Point operator-(Point a, Point b)
    {
        return {a.x - b.x, a.y - b.y};
    }

    friend Point operator*(Point a, double s)
    {
        return {a.x * s, a.y * s};
    }
};

/**
 * An axis-aligned rectangle given by its top-left corner and its size.
 *
 * @since 3.0.0
 */
struct Rect
{
    double x = 0.0;      /**< Left edge. */
    double y = 0.0;      /**< Top edge. */
    double width = 0.0;  /**< Width; the rectangle is empty when it is not positive. */
    double height = 0.0; /**< Height; the rectangle is empty when it is not positive. */

    friend bool operator==(const Rect&, const Rect&) = default;

    /** @return The left edge. */
    double left() const
    {
        return x;
    }

    /** @return The top edge. */
    double top() const
    {
        return y;
    }

    /** @return The right edge, x + width. */
    double right() const
    {
        return x + width;
    }

    /** @return The bottom edge, y + height. */
    double bottom() const
    {
        return y + height;
    }

    /** @return The centre. */
    Point center() const
    {
        return {x + width / 2.0, y + height / 2.0};
    }

    /** @return Whether width or height is not positive. */
    bool empty() const
    {
        return !(width > 0.0) || !(height > 0.0);
    }

    /**
     * @param[in] dx1   Added to the left edge.
     * @param[in] dy1   Added to the top edge.
     * @param[in] dx2   Added to the right edge.
     * @param[in] dy2   Added to the bottom edge.
     *
     * @return The rectangle with moved edges, like QRectF::adjusted.
     */
    Rect adjusted(double dx1, double dy1, double dx2, double dy2) const
    {
        return {x + dx1, y + dy1, width - dx1 + dx2, height - dy1 + dy2};
    }
};

/**
 * An affine transform of the plane with the layout and composition rules of QTransform.
 *
 * A point (x, y) maps to (m11 x + m21 y + dx, m12 x + m22 y + dy).
 *
 * @since 3.0.0
 */
struct Transform
{
    double m11 = 1.0; /**< Horizontal scaling. */
    double m12 = 0.0; /**< Vertical shearing. */
    double m21 = 0.0; /**< Horizontal shearing. */
    double m22 = 1.0; /**< Vertical scaling. */
    double dx = 0.0;  /**< Horizontal translation. */
    double dy = 0.0;  /**< Vertical translation. */

    friend bool operator==(const Transform&, const Transform&) = default;

    /** @return Whether this is the identity. */
    bool identity() const
    {
        return *this == Transform{};
    }

    /**
     * @param[in] p   Point.
     *
     * @return The image of @p p.
     */
    Point map(Point p) const
    {
        return {m11 * p.x + m21 * p.y + dx, m12 * p.x + m22 * p.y + dy};
    }

    /**
     * Composition: @p a applied first, then @p b.
     *
     * @param[in] a   First transform.
     * @param[in] b   Second transform.
     *
     * @return The combined transform.
     */
    friend Transform operator*(const Transform& a, const Transform& b)
    {
        return {a.m11 * b.m11 + a.m12 * b.m21, a.m11 * b.m12 + a.m12 * b.m22,
                a.m21 * b.m11 + a.m22 * b.m21, a.m21 * b.m12 + a.m22 * b.m22,
                a.dx * b.m11 + a.dy * b.m21 + b.dx, a.dx * b.m12 + a.dy * b.m22 + b.dy};
    }

    /**
     * @return The inverse transform, or the identity if this one is singular.
     */
    Transform inverted() const
    {
        const double det = m11 * m22 - m12 * m21;
        if (det == 0.0 || !std::isfinite(det)) {
            return {};
        }
        Transform inverse{m22 / det, -m12 / det, -m21 / det, m11 / det, 0.0, 0.0};
        inverse.dx = -(dx * inverse.m11 + dy * inverse.m21);
        inverse.dy = -(dx * inverse.m12 + dy * inverse.m22);
        return inverse;
    }

    /**
     * @param[in] tx   Horizontal translation.
     * @param[in] ty   Vertical translation.
     *
     * @return A translation by (@p tx, @p ty).
     */
    static Transform translation(double tx, double ty)
    {
        return {1.0, 0.0, 0.0, 1.0, tx, ty};
    }

    /**
     * @param[in] sx   Horizontal factor.
     * @param[in] sy   Vertical factor.
     *
     * @return A scaling by @p sx horizontally and @p sy vertically.
     */
    static Transform scaling(double sx, double sy)
    {
        return {sx, 0.0, 0.0, sy, 0.0, 0.0};
    }

    /**
     * @param[in] degrees   Angle in degrees, clockwise on screen.
     *
     * @return A rotation by @p degrees, clockwise on screen (y grows downwards), like QTransform::rotate.
     */
    static Transform rotation(double degrees)
    {
        // Exact values for quarter turns, so text rotated by -90 degrees stays on whole pixels
        const double turns = degrees / 90.0;
        if (turns == std::floor(turns)) {
            const int quarter = ((static_cast<int>(turns) % 4) + 4) % 4;
            constexpr double kCos[4] = {1.0, 0.0, -1.0, 0.0};
            constexpr double kSin[4] = {0.0, 1.0, 0.0, -1.0};
            return {kCos[quarter], kSin[quarter], -kSin[quarter], kCos[quarter], 0.0, 0.0};
        }
        const double radians = degrees * std::numbers::pi / 180.0;
        const double c = std::cos(radians);
        const double s = std::sin(radians);
        return {c, s, -s, c, 0.0, 0.0};
    }
};

} // namespace mne::scene

#endif // SCENE_GEOMETRY_H
