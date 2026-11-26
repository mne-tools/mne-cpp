//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     raster_renderer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of to_raster.
 *
 * Strokes become polygons (one quadrilateral per segment plus caps and joins) that RasterImage fills together,
 * so overlaps blend once. Clips are convex polygons in image pixels: shapes are cut against them, text and
 * images are tested per pixel. Gradients, text and images are drawn through a coverage mask.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "raster_renderer.h"
#include "detail/text_layout.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <span>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;

namespace
{

using Polygon = std::vector<Point>;

constexpr int kGlyphHeight = 7;  // RasterImage's bitmap font
constexpr int kGlyphAdvance = 6; // 5 columns and one of spacing

double cross(Point a, Point b)
{
    return a.x * b.y - a.y * b.x;
}

double dot(Point a, Point b)
{
    return a.x * b.x + a.y * b.y;
}

double signedArea(const Polygon& polygon)
{
    double area = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        area += cross(polygon[i], polygon[(i + 1) % polygon.size()]);
    }
    return area / 2.0;
}

// Keeps the part of the polygon where dot(normal, p) >= offset (Sutherland-Hodgman, one edge)
Polygon clipHalfPlane(const Polygon& polygon, Point normal, double offset)
{
    Polygon out;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const Point a = polygon[i];
        const Point b = polygon[(i + 1) % polygon.size()];
        const double da = dot(normal, a) - offset;
        const double db = dot(normal, b) - offset;
        if (da >= 0.0) {
            out.push_back(a);
        }
        if ((da >= 0.0) != (db >= 0.0)) {
            out.push_back(a + (b - a) * (da / (da - db)));
        }
    }
    return out;
}

// Cuts the polygon with a convex clip polygon of either orientation
Polygon clipConvex(Polygon polygon, const Polygon& clip)
{
    const double orientation = signedArea(clip) >= 0.0 ? 1.0 : -1.0;
    for (std::size_t i = 0; i < clip.size() && polygon.size() >= 3; ++i) {
        const Point a = clip[i];
        const Point b = clip[(i + 1) % clip.size()];
        // The interior lies on the same side of every edge
        const Point normal{-(b.y - a.y) * orientation, (b.x - a.x) * orientation};
        polygon = clipHalfPlane(polygon, normal, dot(normal, a));
    }
    return polygon;
}

bool insideConvex(Point p, const Polygon& clip)
{
    const double orientation = signedArea(clip) >= 0.0 ? 1.0 : -1.0;
    for (std::size_t i = 0; i < clip.size(); ++i) {
        const Point a = clip[i];
        const Point b = clip[(i + 1) % clip.size()];
        if (cross(b - a, p - a) * orientation < 0.0) {
            return false;
        }
    }
    return true;
}

Polygon ellipsePolygon(Point center, double rx, double ry, double startDegrees, double spanDegrees, int segments)
{
    Polygon points;
    for (int i = 0; i <= segments; ++i) {
        const double angle = (startDegrees + spanDegrees * i / segments) * std::numbers::pi / 180.0;
        points.push_back({center.x + rx * std::cos(angle), center.y + ry * std::sin(angle)});
    }
    return points;
}

int segmentsFor(double radius)
{
    return std::clamp(static_cast<int>(std::ceil(2.0 * std::numbers::pi * radius / 3.0)), 12, 256);
}

Polygon roundedRectPolygon(const Rect& r, double rx, double ry)
{
    rx = std::min(rx, r.width / 2.0);
    ry = std::min(ry, r.height / 2.0);
    if (rx <= 0.0 || ry <= 0.0) {
        return {{r.left(), r.top()}, {r.right(), r.top()}, {r.right(), r.bottom()}, {r.left(), r.bottom()}};
    }
    const int n = std::max(2, segmentsFor(std::max(rx, ry)) / 4);
    Polygon points;
    const auto corner = [&](Point c, double start) {
        const Polygon arc = ellipsePolygon(c, rx, ry, start, 90.0, n);
        points.insert(points.end(), arc.begin(), arc.end());
    };
    corner({r.right() - rx, r.top() + ry}, 270.0);
    corner({r.right() - rx, r.bottom() - ry}, 0.0);
    corner({r.left() + rx, r.bottom() - ry}, 90.0);
    corner({r.left() + rx, r.top() + ry}, 180.0);
    return points;
}

std::vector<double> dashPattern(LineStyle style)
{
    switch (style) {
        case LineStyle::Dash:
            return {4, 2};
        case LineStyle::Dot:
            return {1, 2};
        case LineStyle::DashDot:
            return {4, 2, 1, 2};
        case LineStyle::DashDotDot:
            return {4, 2, 1, 2, 1, 2};
        default:
            return {};
    }
}

// Splits a polyline into the pieces its dash pattern draws
std::vector<Polygon> dashPieces(const Polygon& line, const std::vector<double>& pattern, double unit)
{
    if (pattern.empty()) {
        return {line};
    }
    std::vector<Polygon> pieces;
    std::size_t index = 0;
    double left = pattern[0] * unit;
    bool on = true;
    Polygon current{line.front()};
    for (std::size_t i = 1; i < line.size(); ++i) {
        Point a = line[i - 1];
        const Point b = line[i];
        double length = std::hypot(b.x - a.x, b.y - a.y);
        while (length > 0.0) {
            const double step = std::min(left, length);
            const Point p = a + (b - a) * (step / length);
            if (on) {
                current.push_back(p);
            }
            length -= step;
            left -= step;
            a = p;
            if (left <= 1e-9) {
                if (on && current.size() >= 2) {
                    pieces.push_back(current);
                }
                on = !on;
                index = (index + 1) % pattern.size();
                left = pattern[index] * unit;
                current = {a};
            }
        }
    }
    if (on && current.size() >= 2) {
        pieces.push_back(current);
    }
    return pieces;
}

// The outline of a stroke as polygons of one orientation
std::vector<Polygon> strokePolygons(const Polygon& points, bool closed, const Pen& pen, double width)
{
    std::vector<Polygon> polygons;
    const double h = width / 2.0;
    Polygon line = points;
    if (closed && points.size() >= 2) {
        line.push_back(points.front());
    }
    for (const Polygon& piece : dashPieces(line, dashPattern(pen.style), width)) {
        const bool wholeClosed = closed && pen.style == LineStyle::Solid;
        for (std::size_t i = 1; i < piece.size(); ++i) {
            Point a = piece[i - 1];
            Point b = piece[i];
            const double length = std::hypot(b.x - a.x, b.y - a.y);
            if (length <= 0.0) {
                continue;
            }
            const Point d = (b - a) * (1.0 / length);
            if (pen.cap == LineCap::Square && !wholeClosed) {
                if (i == 1) {
                    a = a - d * h;
                }
                if (i + 1 == piece.size()) {
                    b = b + d * h;
                }
            }
            const Point n{-d.y * h, d.x * h};
            polygons.push_back({a + n, b + n, b - n, a - n});
        }
        // Joins: a disc where segments meet (round), else the triangles that close the outer gap
        const std::size_t first = wholeClosed ? 0 : 1;
        for (std::size_t i = first; i + 1 < piece.size(); ++i) {
            const Point v = piece[i];
            const Point prev = i == 0 ? piece[piece.size() - 2] : piece[i - 1];
            const Point next = piece[i + 1];
            if (pen.join == LineJoin::Round) {
                polygons.push_back(ellipsePolygon(v, h, h, 0.0, 360.0, segmentsFor(h)));
                continue;
            }
            const double l1 = std::hypot(v.x - prev.x, v.y - prev.y);
            const double l2 = std::hypot(next.x - v.x, next.y - v.y);
            if (l1 <= 0.0 || l2 <= 0.0) {
                continue;
            }
            const Point n1{-(v.y - prev.y) / l1 * h, (v.x - prev.x) / l1 * h};
            const Point n2{-(next.y - v.y) / l2 * h, (next.x - v.x) / l2 * h};
            polygons.push_back({v, v + n1, v + n2});
            polygons.push_back({v, v - n2, v - n1});
        }
        if (pen.cap == LineCap::Round && !wholeClosed) {
            polygons.push_back(ellipsePolygon(piece.front(), h, h, 0.0, 360.0, segmentsFor(h)));
            polygons.push_back(ellipsePolygon(piece.back(), h, h, 0.0, 360.0, segmentsFor(h)));
        }
    }
    return polygons;
}

class Renderer
{
public:
    Renderer(const DisplayList& list, const RasterOptions& options)
    : m_scale(options.scale > 0.0 ? options.scale : 1.0)
    , m_image(static_cast<int>(std::ceil(list.width * m_scale)), static_cast<int>(std::ceil(list.height * m_scale)),
              rgb(list.background.value_or(options.background)))
    {
        if (list.background && list.background->a < 255) {
            const Color under = options.background;
            m_image.fill(rgb(under));
            fillPolygons({{{0, 0}, {list.width * m_scale, 0}, {list.width * m_scale, list.height * m_scale}, {0, list.height * m_scale}}},
                         *list.background);
        }
    }

    RasterImage render(const DisplayList& list)
    {
        for (const Command& command : list.commands) {
            m_device = command.transform * Transform::scaling(m_scale, m_scale);
            m_opacity = command.opacity;
            m_clips.clear();
            for (const Clip& clip : command.clips) {
                const Transform t = clip.transform * Transform::scaling(m_scale, m_scale);
                const Rect& r = clip.rect;
                m_clips.push_back({t.map({r.left(), r.top()}), t.map({r.right(), r.top()}), t.map({r.right(), r.bottom()}),
                                   t.map({r.left(), r.bottom()})});
            }
            std::visit([&](const auto& shape) { draw(shape); }, command.shape);
        }
        return std::move(m_image);
    }

private:
    static Rgb rgb(Color c)
    {
        return {c.r, c.g, c.b};
    }

    // An alpha value times the opacity of the current command
    int alpha(int value) const
    {
        return static_cast<int>(std::lround(value * m_opacity));
    }

    Polygon toDevice(const Polygon& local) const
    {
        Polygon device;
        device.reserve(local.size());
        for (const Point& p : local) {
            device.push_back(m_device.map(p));
        }
        return device;
    }

    // Clipped polygons in device pixels, all oriented the same way
    std::vector<Polygon> prepare(std::vector<Polygon> polygons) const
    {
        std::vector<Polygon> out;
        for (Polygon& polygon : polygons) {
            for (const Polygon& clip : m_clips) {
                polygon = clipConvex(std::move(polygon), clip);
            }
            if (polygon.size() < 3) {
                continue;
            }
            if (signedArea(polygon) < 0.0) {
                std::reverse(polygon.begin(), polygon.end());
            }
            out.push_back(std::move(polygon));
        }
        return out;
    }

    void fillPolygons(const std::vector<Polygon>& polygons, Color color)
    {
        std::vector<std::vector<PointF>> contours;
        for (const Polygon& polygon : prepare(polygons)) {
            std::vector<PointF> contour;
            for (const Point& p : polygon) {
                contour.push_back({p.x, p.y});
            }
            contours.push_back(std::move(contour));
        }
        m_image.fill_polygons(contours, rgb(color), alpha(color.a));
    }

    void fillBrush(const Polygon& local, const Brush& brush)
    {
        if (!brush.visible()) {
            return;
        }
        if (brush.style == Brush::Style::Solid) {
            fillPolygons({toDevice(local)}, brush.color);
            return;
        }
        // A gradient: the polygon's coverage, coloured per pixel
        const std::vector<Polygon> polygons = prepare({toDevice(local)});
        const Point start = m_device.map(brush.start);
        const Point axis = m_device.map(brush.end) - start;
        const double length2 = dot(axis, axis);
        paintCoverage(polygons, [&](double x, double y) {
            return brush.color_at(length2 > 0.0 ? dot(Point{x, y} - start, axis) / length2 : 0.0);
        });
    }

    template<class ColorAt>
    void paintCoverage(const std::vector<Polygon>& polygons, ColorAt colorAt)
    {
        if (polygons.empty()) {
            return;
        }
        double minX = m_image.width();
        double minY = m_image.height();
        double maxX = 0.0;
        double maxY = 0.0;
        for (const Polygon& polygon : polygons) {
            for (const Point& p : polygon) {
                minX = std::min(minX, p.x);
                minY = std::min(minY, p.y);
                maxX = std::max(maxX, p.x);
                maxY = std::max(maxY, p.y);
            }
        }
        const int x0 = std::max(0, static_cast<int>(std::floor(minX)));
        const int y0 = std::max(0, static_cast<int>(std::floor(minY)));
        const int x1 = std::min(m_image.width(), static_cast<int>(std::ceil(maxX)) + 1);
        const int y1 = std::min(m_image.height(), static_cast<int>(std::ceil(maxY)) + 1);
        if (x0 >= x1 || y0 >= y1) {
            return;
        }
        RasterImage mask(x1 - x0, y1 - y0);
        std::vector<std::vector<PointF>> contours;
        for (const Polygon& polygon : polygons) {
            std::vector<PointF> contour;
            for (const Point& p : polygon) {
                contour.push_back({p.x - x0, p.y - y0});
            }
            contours.push_back(std::move(contour));
        }
        mask.fill_polygons(contours, {255, 255, 255});
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                const int coverage = mask.pixel(x - x0, y - y0).r;
                if (coverage > 0) {
                    const Color c = colorAt(x + 0.5, y + 0.5);
                    m_image.blend_pixel(x, y, rgb(c), alpha(coverage * c.a / 255));
                }
            }
        }
    }

    void stroke(const Polygon& local, bool closed, const Pen& pen)
    {
        if (!pen.visible() || local.size() < 2) {
            return;
        }
        std::vector<Polygon> polygons;
        if (pen.width > 0.0) {
            for (const Polygon& polygon : strokePolygons(local, closed, pen, pen.width)) {
                polygons.push_back(toDevice(polygon));
            }
        } else {
            polygons = strokePolygons(toDevice(local), closed, pen, 1.0);
        }
        fillPolygons(polygons, pen.color);
    }

    void draw(const LineShape& line)
    {
        stroke({line.from, line.to}, false, line.pen);
    }

    void draw(const PathShape& path)
    {
        const Polygon points = path.cubic ? flattenCubic(path.points) : path.points;
        if (path.closed && points.size() >= 3) {
            fillBrush(points, path.brush);
        }
        stroke(points, path.closed, path.pen);
    }

    // Cubic segments as polylines, with steps of about two device pixels
    Polygon flattenCubic(const Polygon& points) const
    {
        Polygon line{points.front()};
        for (std::size_t i = 1; i + 2 < points.size(); i += 3) {
            const Point p0 = points[i - 1];
            const Point c1 = points[i];
            const Point c2 = points[i + 1];
            const Point p1 = points[i + 2];
            double length = 0.0;
            for (const auto& [a, b] : {std::pair{p0, c1}, std::pair{c1, c2}, std::pair{c2, p1}}) {
                const Point d = m_device.map(b) - m_device.map(a);
                length += std::hypot(d.x, d.y);
            }
            const int steps = std::clamp(static_cast<int>(length / 2.0), 1, 256);
            for (int k = 1; k <= steps; ++k) {
                const double t = static_cast<double>(k) / steps;
                const double u = 1.0 - t;
                line.push_back(p0 * (u * u * u) + c1 * (3 * u * u * t) + c2 * (3 * u * t * t) + p1 * (t * t * t));
            }
        }
        return line;
    }

    void draw(const RectShape& rect)
    {
        const Polygon polygon = roundedRectPolygon(rect.rect, rect.rx, rect.ry);
        fillBrush(polygon, rect.brush);
        stroke(polygon, true, rect.pen);
    }

    void draw(const EllipseShape& ellipse)
    {
        const double radius = std::max(ellipse.rx, ellipse.ry) * std::sqrt(std::abs(m_device.m11 * m_device.m22 - m_device.m12 * m_device.m21));
        Polygon polygon = ellipsePolygon(ellipse.center, ellipse.rx, ellipse.ry, 0.0, 360.0, segmentsFor(radius));
        polygon.pop_back();
        fillBrush(polygon, ellipse.brush);
        stroke(polygon, true, ellipse.pen);
    }

    // Draws a source of size w x h placed by `place` (source pixel to local coordinates) through the device transform
    template<class Sample>
    void paintMapped(double w, double h, const Transform& place, Sample sample)
    {
        const Transform toDevice = place * m_device;
        const Transform fromDevice = toDevice.inverted();
        const Point corners[4] = {toDevice.map({0, 0}), toDevice.map({w, 0}), toDevice.map({w, h}), toDevice.map({0, h})};
        double minX = corners[0].x;
        double maxX = corners[0].x;
        double minY = corners[0].y;
        double maxY = corners[0].y;
        for (const Point& c : corners) {
            minX = std::min(minX, c.x);
            maxX = std::max(maxX, c.x);
            minY = std::min(minY, c.y);
            maxY = std::max(maxY, c.y);
        }
        const int x0 = std::max(0, static_cast<int>(std::floor(minX)));
        const int y0 = std::max(0, static_cast<int>(std::floor(minY)));
        const int x1 = std::min(m_image.width(), static_cast<int>(std::ceil(maxX)));
        const int y1 = std::min(m_image.height(), static_cast<int>(std::ceil(maxY)));
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                const Point center{x + 0.5, y + 0.5};
                if (std::any_of(m_clips.begin(), m_clips.end(), [&](const Polygon& clip) { return !insideConvex(center, clip); })) {
                    continue;
                }
                const Point source = fromDevice.map(center);
                if (source.x < 0.0 || source.y < 0.0 || source.x >= w || source.y >= h) {
                    continue;
                }
                const Color c = sample(source.x, source.y);
                if (c.a > 0) {
                    m_image.blend_pixel(x, y, rgb(c), alpha(c.a));
                }
            }
        }
    }

    void draw(const TextShape& text)
    {
        const int pixelSize = static_cast<int>(std::lround(text.font.pixel_size > 0.0       ? text.font.pixel_size
                                                               : text.font.point_size > 0.0 ? text.font.point_size * 96.0 / 72.0
                                                                                            : 13.0));
        const int scale = std::max(1, static_cast<int>(std::lround(pixelSize / 12.0)));
        int characters = 0;
        for (const char c : text.text) {
            if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) {
                ++characters;
            }
        }
        const int width = std::max(1, characters * kGlyphAdvance * scale - scale);
        const int ascent = kGlyphHeight * scale;
        RasterImage glyphs(width, ascent);
        std::string ascii;
        for (const char c : text.text) {
            if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) {
                ascii += (static_cast<unsigned char>(c) < 0x80) ? c : '?';
            }
        }
        glyphs.draw_text(0, ascent, ascii, {255, 255, 255}, scale);
        const Point origin = detail::text_origin(text, width, ascent, 2.0 * scale);
        const Color color = text.color;
        paintMapped(width, ascent, Transform::translation(origin.x, origin.y - ascent), [&](double sx, double sy) {
            const int coverage = glyphs.pixel(static_cast<int>(sx), static_cast<int>(sy)).r;
            return Color{color.r, color.g, color.b, static_cast<std::uint8_t>(coverage * color.a / 255)};
        });
    }

    void draw(const ImageShape& shape)
    {
        const Image& image = *shape.image;
        const Transform place = Transform::scaling(shape.target.width / image.width, shape.target.height / image.height) *
            Transform::translation(shape.target.x, shape.target.y);
        paintMapped(image.width, image.height, place, [&](double sx, double sy) {
            if (!shape.smooth) {
                return image.pixel(static_cast<int>(sx), static_cast<int>(sy));
            }
            // Bilinear between the four nearest pixel centres
            const double fx = std::clamp(sx - 0.5, 0.0, image.width - 1.0);
            const double fy = std::clamp(sy - 0.5, 0.0, image.height - 1.0);
            const int ix = static_cast<int>(fx);
            const int iy = static_cast<int>(fy);
            const int jx = std::min(ix + 1, image.width - 1);
            const int jy = std::min(iy + 1, image.height - 1);
            const double tx = fx - ix;
            const double ty = fy - iy;
            const Color a = image.pixel(ix, iy);
            const Color b = image.pixel(jx, iy);
            const Color c = image.pixel(ix, jy);
            const Color d = image.pixel(jx, jy);
            const auto mix = [&](std::uint8_t p, std::uint8_t q, std::uint8_t r, std::uint8_t s) {
                return static_cast<std::uint8_t>(std::lround((p * (1 - tx) + q * tx) * (1 - ty) + (r * (1 - tx) + s * tx) * ty));
            };
            return Color{mix(a.r, b.r, c.r, d.r), mix(a.g, b.g, c.g, d.g), mix(a.b, b.b, c.b, d.b), mix(a.a, b.a, c.a, d.a)};
        });
    }

    double m_scale;
    RasterImage m_image;
    Transform m_device;
    double m_opacity = 1.0;
    std::vector<Polygon> m_clips;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

RasterImage mne::viz::to_raster(const DisplayList& list, const RasterOptions& options)
{
    return Renderer(list, options).render(list);
}
