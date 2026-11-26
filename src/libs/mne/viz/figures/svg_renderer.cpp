//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     svg_renderer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of to_svg and write_svg.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "svg_renderer.h"
#include "detail/text_layout.h"
#include "text_metrics.h"
#include <mne/base/raster_image.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <fstream>
#include <span>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;

namespace
{

// A number with at most three decimals, without trailing zeros
std::string num(double value)
{
    if (!std::isfinite(value)) {
        return "0";
    }
    std::string text = std::format("{:.3f}", value);
    while (text.back() == '0') {
        text.pop_back();
    }
    if (text.back() == '.') {
        text.pop_back();
    }
    return text == "-0" ? "0" : text;
}

std::string escape(std::string_view text)
{
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        switch (c) {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            default:
                out += c;
        }
    }
    return out;
}

std::string hex(Color c)
{
    return std::format("#{:02x}{:02x}{:02x}", c.r, c.g, c.b);
}

std::string matrix(const Transform& t)
{
    return std::format("matrix({} {} {} {} {} {})", num(t.m11), num(t.m12), num(t.m21), num(t.m22), num(t.dx), num(t.dy));
}

// Dash pattern of a pen style in pen widths, as Qt defines it
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

class SvgWriter
{
public:
    explicit SvgWriter(const SvgOptions& options)
    : m_options(options)
    , m_metrics(options.metrics ? *options.metrics : m_fallbackMetrics)
    {
    }

    std::string write(const DisplayList& list)
    {
        m_out += std::format("<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
                             "width=\"{0}\" height=\"{1}\" viewBox=\"0 0 {0} {1}\">\n",
                             num(list.width), num(list.height));
        if (!m_options.title.empty()) {
            m_out += std::format("<title>{}</title>\n", escape(m_options.title));
        }
        if (list.background) {
            m_out += std::format("<rect x=\"0\" y=\"0\" width=\"{}\" height=\"{}\"{}/>\n", num(list.width), num(list.height),
                                 fill(Brush::solid(*list.background)));
        }
        for (const Command& command : list.commands) {
            setClips(command.clips);
            std::visit([&](const auto& shape) { draw(shape, command); }, command.shape);
        }
        setClips({});
        m_out += "</svg>\n";
        return std::move(m_out);
    }

private:
    // Clip groups stay open while consecutive commands share their clips
    void setClips(const std::vector<Clip>& clips)
    {
        if (clips == m_openClips) {
            return;
        }
        for (std::size_t i = 0; i < m_openClips.size(); ++i) {
            m_out += "</g>\n";
        }
        m_openClips = clips;
        for (const Clip& clip : clips) {
            auto it = std::find(m_clipDefs.begin(), m_clipDefs.end(), clip);
            if (it == m_clipDefs.end()) {
                m_clipDefs.push_back(clip);
                it = m_clipDefs.end() - 1;
                m_out += std::format("<defs><clipPath id=\"c{}\"><rect x=\"{}\" y=\"{}\" width=\"{}\" height=\"{}\"{}/></clipPath></defs>\n",
                                     it - m_clipDefs.begin(), num(clip.rect.x), num(clip.rect.y), num(clip.rect.width),
                                     num(clip.rect.height), transformAttribute(clip.transform));
            }
            m_out += std::format("<g clip-path=\"url(#c{})\">\n", it - m_clipDefs.begin());
        }
    }

    // The transform and opacity of a command
    static std::string transformAttribute(const Command& command)
    {
        std::string out = transformAttribute(command.transform);
        if (command.opacity < 1.0) {
            out += std::format(" opacity=\"{}\"", num(command.opacity));
        }
        return out;
    }

    static std::string transformAttribute(const Transform& transform)
    {
        return transform.identity() ? std::string() : std::format(" transform=\"{}\"", matrix(transform));
    }

    std::string stroke(const Pen& pen) const
    {
        if (!pen.visible()) {
            return " stroke=\"none\"";
        }
        const double width = pen.width > 0.0 ? pen.width : 1.0;
        std::string out = std::format(" stroke=\"{}\"", hex(pen.color));
        if (pen.color.a < 255) {
            out += std::format(" stroke-opacity=\"{}\"", num(pen.color.a / 255.0));
        }
        out += std::format(" stroke-width=\"{}\"", num(width));
        if (pen.width <= 0.0) {
            out += " vector-effect=\"non-scaling-stroke\"";
        }
        out += pen.cap == LineCap::Flat ? " stroke-linecap=\"butt\"" : pen.cap == LineCap::Round ? " stroke-linecap=\"round\""
                                                                                                 : " stroke-linecap=\"square\"";
        out += pen.join == LineJoin::Miter ? " stroke-linejoin=\"miter\"" : pen.join == LineJoin::Round ? " stroke-linejoin=\"round\""
                                                                                                        : " stroke-linejoin=\"bevel\"";
        const std::vector<double> dashes = dashPattern(pen.style);
        if (!dashes.empty()) {
            out += " stroke-dasharray=\"";
            for (std::size_t i = 0; i < dashes.size(); ++i) {
                out += (i ? " " : "") + num(dashes[i] * width);
            }
            out += "\"";
        }
        return out;
    }

    std::string fill(const Brush& brush)
    {
        if (!brush.visible()) {
            return " fill=\"none\"";
        }
        if (brush.style == Brush::Style::Solid) {
            std::string out = std::format(" fill=\"{}\"", hex(brush.color));
            if (brush.color.a < 255) {
                out += std::format(" fill-opacity=\"{}\"", num(brush.color.a / 255.0));
            }
            return out;
        }
        const int id = m_gradients++;
        m_out += std::format("<defs><linearGradient id=\"g{}\" gradientUnits=\"userSpaceOnUse\" x1=\"{}\" y1=\"{}\" x2=\"{}\" y2=\"{}\">", id,
                             num(brush.start.x), num(brush.start.y), num(brush.end.x), num(brush.end.y));
        for (const GradientStop& stop : brush.stops) {
            m_out += std::format("<stop offset=\"{}\" stop-color=\"{}\"", num(std::clamp(stop.position, 0.0, 1.0)), hex(stop.color));
            if (stop.color.a < 255) {
                m_out += std::format(" stop-opacity=\"{}\"", num(stop.color.a / 255.0));
            }
            m_out += "/>";
        }
        m_out += "</linearGradient></defs>\n";
        return std::format(" fill=\"url(#g{})\"", id);
    }

    void draw(const LineShape& line, const Command& command)
    {
        m_out += std::format("<line x1=\"{}\" y1=\"{}\" x2=\"{}\" y2=\"{}\"{}{}/>\n", num(line.from.x), num(line.from.y), num(line.to.x),
                             num(line.to.y), stroke(line.pen), transformAttribute(command));
    }

    void draw(const PathShape& path, const Command& command)
    {
        if (path.cubic) {
            std::string d = "M" + num(path.points[0].x) + "," + num(path.points[0].y);
            for (std::size_t i = 1; i + 2 < path.points.size(); i += 3) {
                d += " C";
                for (std::size_t k = i; k < i + 3; ++k) {
                    d += (k == i ? "" : " ") + num(path.points[k].x) + "," + num(path.points[k].y);
                }
            }
            if (path.closed) {
                d += " Z";
            }
            const std::string paint = path.closed ? fill(path.brush) : std::string(" fill=\"none\"");
            m_out += std::format("<path d=\"{}\"{}{}{}/>\n", d, paint, stroke(path.pen), transformAttribute(command));
            return;
        }
        std::string points;
        for (const Point& p : path.points) {
            points += (points.empty() ? "" : " ") + num(p.x) + "," + num(p.y);
        }
        const std::string paint = path.closed ? fill(path.brush) : std::string(" fill=\"none\"");
        m_out += std::format("<{} points=\"{}\"{}{}{}/>\n", path.closed ? "polygon" : "polyline", points, paint, stroke(path.pen),
                             transformAttribute(command));
    }

    void draw(const RectShape& rect, const Command& command)
    {
        std::string corners;
        if (rect.rx > 0.0 || rect.ry > 0.0) {
            corners = std::format(" rx=\"{}\" ry=\"{}\"", num(rect.rx), num(rect.ry));
        }
        const std::string paint = fill(rect.brush);
        m_out += std::format("<rect x=\"{}\" y=\"{}\" width=\"{}\" height=\"{}\"{}{}{}{}/>\n", num(rect.rect.x), num(rect.rect.y),
                             num(rect.rect.width), num(rect.rect.height), corners, paint, stroke(rect.pen),
                             transformAttribute(command));
    }

    void draw(const EllipseShape& ellipse, const Command& command)
    {
        const std::string paint = fill(ellipse.brush);
        m_out += std::format("<ellipse cx=\"{}\" cy=\"{}\" rx=\"{}\" ry=\"{}\"{}{}{}/>\n", num(ellipse.center.x), num(ellipse.center.y),
                             num(ellipse.rx), num(ellipse.ry), paint, stroke(ellipse.pen), transformAttribute(command));
    }

    void draw(const TextShape& text, const Command& command)
    {
        const Font& font = text.font;
        // The x position comes from the anchor, so only the vertical placement uses the metrics
        const Point origin = detail::text_origin(text, 0.0, m_metrics.ascent(font), m_metrics.descent(font));
        std::string_view anchor = "start";
        double x = text.box.x;
        if (!has_flag(text.align, Align::Baseline)) {
            if (has_flag(text.align, Align::Right)) {
                anchor = "end";
                x = text.box.right();
            } else if (has_flag(text.align, Align::HCenter)) {
                anchor = "middle";
                x = text.box.center().x;
            }
        }
        std::string attributes = std::format(" font-family=\"{}\" font-size=\"{}\"", escape(font.family.empty() ? m_options.font_family : font.family),
                                             num(m_metrics.pixel_size(font)));
        if (font.bold) {
            attributes += " font-weight=\"bold\"";
        }
        if (font.italic) {
            attributes += " font-style=\"italic\"";
        }
        if (anchor != "start") {
            attributes += std::format(" text-anchor=\"{}\"", anchor);
        }
        attributes += std::format(" fill=\"{}\"", hex(text.color));
        if (text.color.a < 255) {
            attributes += std::format(" fill-opacity=\"{}\"", num(text.color.a / 255.0));
        }
        m_out += std::format("<text x=\"{}\" y=\"{}\"{}{} xml:space=\"preserve\">{}</text>\n", num(x), num(origin.y), attributes,
                             transformAttribute(command), escape(text.text));
    }

    void draw(const ImageShape& image, const Command& command)
    {
        RasterImage raster(image.image->width, image.image->height);
        for (int y = 0; y < raster.height(); ++y) {
            for (int x = 0; x < raster.width(); ++x) {
                const Color c = image.image->pixel(x, y);
                raster.set_pixel(x, y, {c.r, c.g, c.b});
            }
        }
        const std::vector<std::uint8_t> png = raster.encode_png();
        const std::string data = to_base64(std::as_bytes(std::span(png)));
        m_out += std::format("<image x=\"{}\" y=\"{}\" width=\"{}\" height=\"{}\" preserveAspectRatio=\"none\"{}{} xlink:href=\"data:image/png;base64,{}\"/>\n",
                             num(image.target.x), num(image.target.y), num(image.target.width), num(image.target.height),
                             image.smooth ? "" : " style=\"image-rendering:pixelated\"", transformAttribute(command), data);
    }

    const SvgOptions& m_options;
    ApproximateTextMetrics m_fallbackMetrics;
    const TextMetrics& m_metrics;
    std::string m_out;
    std::vector<Clip> m_openClips;
    std::vector<Clip> m_clipDefs;
    int m_gradients = 0;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::string mne::viz::to_svg(const DisplayList& list, const SvgOptions& options)
{
    return SvgWriter(options).write(list);
}

//=============================================================================================================

Expected<void> mne::viz::write_svg(const DisplayList& list, const std::filesystem::path& path, const SvgOptions& options)
{
    const std::string svg = to_svg(list, options);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(svg.data(), static_cast<std::streamsize>(svg.size()));
    if (!out.flush()) {
        return fail(ErrorCode::Io, "cannot write {}", utf8_from_path(path));
    }
    return {};
}
