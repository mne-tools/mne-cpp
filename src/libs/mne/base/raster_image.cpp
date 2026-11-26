//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     raster_image.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    RasterImage class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "raster_image.h"
#include "string_utils.h"

#include <zlib.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <string_view>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

namespace
{

// Supersampling per pixel and axis, so a pixel has kSamples * kSamples coverage levels
constexpr int kSamples = 4;
constexpr int kFullCoverage = kSamples * kSamples;

using Glyph = std::array<std::uint8_t, 7>;

// 5 x 7 glyphs, one byte per row from the top, bit 4 is the leftmost column
Glyph glyph(char c)
{
    if (c >= 'A' && c <= 'Z') {
        c = static_cast<char>(c - 'A' + 'a');
    }
    switch (c) {
        case ' ':
            return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        case '0':
            return {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
        case '1':
            return {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
        case '2':
            return {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
        case '3':
            return {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E};
        case '4':
            return {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
        case '5':
            return {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E};
        case '6':
            return {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
        case '7':
            return {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
        case '8':
            return {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
        case '9':
            return {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C};
        case 'a':
            return {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F};
        case 'b':
            return {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x1E};
        case 'c':
            return {0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E};
        case 'd':
            return {0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F};
        case 'e':
            return {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E};
        case 'f':
            return {0x06, 0x09, 0x08, 0x1C, 0x08, 0x08, 0x08};
        case 'g':
            return {0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01, 0x0E};
        case 'h':
            return {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11};
        case 'i':
            return {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E};
        case 'j':
            return {0x02, 0x00, 0x06, 0x02, 0x02, 0x12, 0x0C};
        case 'k':
            return {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12};
        case 'l':
            return {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
        case 'm':
            return {0x00, 0x00, 0x1A, 0x15, 0x15, 0x11, 0x11};
        case 'n':
            return {0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11};
        case 'o':
            return {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E};
        case 'p':
            return {0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10};
        case 'q':
            return {0x00, 0x00, 0x0D, 0x13, 0x0F, 0x01, 0x01};
        case 'r':
            return {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10};
        case 's':
            return {0x00, 0x00, 0x0E, 0x10, 0x0E, 0x01, 0x1E};
        case 't':
            return {0x08, 0x08, 0x1C, 0x08, 0x08, 0x09, 0x06};
        case 'u':
            return {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D};
        case 'v':
            return {0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04};
        case 'w':
            return {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A};
        case 'x':
            return {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11};
        case 'y':
            return {0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E};
        case 'z':
            return {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F};
        case '.':
            return {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C};
        case ',':
            return {0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08};
        case ':':
            return {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00};
        case ';':
            return {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x04, 0x08};
        case '+':
            return {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00};
        case '-':
            return {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};
        case '=':
            return {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00};
        case '/':
            return {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00};
        case '%':
            return {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03};
        case '(':
            return {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02};
        case ')':
            return {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08};
        case '_':
            return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F};
        default:
            return {0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F};
    }
}

void appendBig32(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    out.push_back(static_cast<std::uint8_t>(value >> 24));
    out.push_back(static_cast<std::uint8_t>(value >> 16));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
    out.push_back(static_cast<std::uint8_t>(value));
}

// A PNG chunk: length, type, data and the CRC of type and data
void appendChunk(std::vector<std::uint8_t>& out, const char (&type)[5], std::span<const std::uint8_t> data)
{
    appendBig32(out, static_cast<std::uint32_t>(data.size()));
    const std::size_t typeStart = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, out.data() + typeStart, static_cast<uInt>(out.size() - typeStart));
    appendBig32(out, static_cast<std::uint32_t>(crc));
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RasterImage::RasterImage(int width, int height, Rgb background)
: m_width(std::max(width, 0))
, m_height(std::max(height, 0))
, m_rgb(static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height) * 3)
{
    fill(background);
}

//=============================================================================================================

Rgb RasterImage::pixel(int x, int y) const
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) {
        return {};
    }
    const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) + static_cast<std::size_t>(x)) * 3;
    return {m_rgb[i], m_rgb[i + 1], m_rgb[i + 2]};
}

//=============================================================================================================

void RasterImage::set_pixel(int x, int y, Rgb color)
{
    blend(x, y, color, kFullCoverage);
}

//=============================================================================================================

void RasterImage::fill(Rgb color)
{
    for (std::size_t i = 0; i < m_rgb.size(); i += 3) {
        m_rgb[i] = color.r;
        m_rgb[i + 1] = color.g;
        m_rgb[i + 2] = color.b;
    }
}

//=============================================================================================================

void RasterImage::blend(int x, int y, Rgb color, int coverage)
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height || coverage <= 0) {
        return;
    }
    const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) + static_cast<std::size_t>(x)) * 3;
    const auto mix = [coverage](std::uint8_t old, std::uint8_t value) {
        return static_cast<std::uint8_t>((old * (kFullCoverage - coverage) + value * coverage + kFullCoverage / 2) / kFullCoverage);
    };
    m_rgb[i] = mix(m_rgb[i], color.r);
    m_rgb[i + 1] = mix(m_rgb[i + 1], color.g);
    m_rgb[i + 2] = mix(m_rgb[i + 2], color.b);
}

//=============================================================================================================

void RasterImage::blend_pixel(int x, int y, Rgb color, int alpha)
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height || alpha <= 0) {
        return;
    }
    alpha = std::min(alpha, 255);
    const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) + static_cast<std::size_t>(x)) * 3;
    const auto mix = [alpha](std::uint8_t old, std::uint8_t value) {
        return static_cast<std::uint8_t>((old * (255 - alpha) + value * alpha + 127) / 255);
    };
    m_rgb[i] = mix(m_rgb[i], color.r);
    m_rgb[i + 1] = mix(m_rgb[i + 1], color.g);
    m_rgb[i + 2] = mix(m_rgb[i + 2], color.b);
}

//=============================================================================================================

void RasterImage::fill_polygon(std::span<const PointF> points, Rgb color, int opacity)
{
    const std::span<const PointF> contours[1] = {points};
    fill_contours(contours, color, opacity);
}

//=============================================================================================================

void RasterImage::fill_polygons(std::span<const std::vector<PointF>> polygons, Rgb color, int opacity)
{
    const std::vector<std::span<const PointF>> contours(polygons.begin(), polygons.end());
    fill_contours(contours, color, opacity);
}

//=============================================================================================================

void RasterImage::fill_contours(std::span<const std::span<const PointF>> polygons, Rgb color, int opacity)
{
    if (m_width == 0 || m_height == 0 || opacity <= 0) {
        return;
    }
    double minX = std::numeric_limits<double>::infinity();
    double maxX = -minX;
    double minY = minX;
    double maxY = -minX;
    std::size_t edges = 0;
    for (const std::span<const PointF> points : polygons) {
        if (points.size() < 3) {
            continue;
        }
        for (const PointF& point : points) {
            if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
                return;
            }
            minX = std::min(minX, point.x);
            maxX = std::max(maxX, point.x);
            minY = std::min(minY, point.y);
            maxY = std::max(maxY, point.y);
        }
        edges += points.size();
    }
    if (edges == 0) {
        return;
    }
    const int x0 = std::max(0, static_cast<int>(std::floor(minX)));
    const int x1 = std::min(m_width - 1, static_cast<int>(std::floor(maxX)));
    const int y0 = std::max(0, static_cast<int>(std::floor(minY)));
    const int y1 = std::min(m_height - 1, static_cast<int>(std::floor(maxY)));
    if (x0 > x1 || y0 > y1) {
        return;
    }

    // Coverage per pixel of a row, counted in sub-pixel samples
    std::vector<int> coverage(static_cast<std::size_t>(x1 - x0 + 1));
    std::vector<std::pair<double, int>> crossings;
    crossings.reserve(edges);
    for (int y = y0; y <= y1; ++y) {
        std::fill(coverage.begin(), coverage.end(), 0);
        for (int sub = 0; sub < kSamples; ++sub) {
            const double sampleY = y + (sub + 0.5) / kSamples;
            crossings.clear();
            for (const std::span<const PointF> points : polygons) {
                if (points.size() < 3) {
                    continue;
                }
                for (std::size_t i = 0; i < points.size(); ++i) {
                    const PointF& a = points[i];
                    const PointF& b = points[(i + 1) % points.size()];
                    if ((a.y <= sampleY && sampleY < b.y) || (b.y <= sampleY && sampleY < a.y)) {
                        const double x = a.x + (sampleY - a.y) * (b.x - a.x) / (b.y - a.y);
                        crossings.emplace_back(x, a.y < b.y ? 1 : -1);
                    }
                }
            }
            std::sort(crossings.begin(), crossings.end());
            int winding = 0;
            for (std::size_t i = 0; i + 1 < crossings.size(); ++i) {
                winding += crossings[i].second;
                if (winding == 0) {
                    continue;
                }
                // Sample columns k sit at (k + 0.5) / kSamples; take those in [start, end)
                const double start = crossings[i].first * kSamples - 0.5;
                const double end = crossings[i + 1].first * kSamples - 0.5;
                const long first = std::max(static_cast<long>(std::ceil(start)), static_cast<long>(x0) * kSamples);
                const long last = std::min(static_cast<long>(std::ceil(end)) - 1, static_cast<long>(x1 + 1) * kSamples - 1);
                for (long k = first; k <= last; ++k) {
                    ++coverage[static_cast<std::size_t>(k / kSamples - x0)];
                }
            }
        }
        for (int x = x0; x <= x1; ++x) {
            const int samples = coverage[static_cast<std::size_t>(x - x0)];
            if (opacity >= 255) {
                blend(x, y, color, samples);
            } else {
                blend_pixel(x, y, color, (samples * opacity + kFullCoverage / 2) / kFullCoverage);
            }
        }
    }
}

//=============================================================================================================

void RasterImage::draw_text(int x, int baseline, std::string_view text, Rgb color, int scale)
{
    scale = std::max(scale, 1);
    const int top = baseline - 7 * scale;
    for (const char c : text) {
        const Glyph rows = glyph(c);
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((rows[static_cast<std::size_t>(row)] >> (4 - column)) & 1) {
                    for (int dy = 0; dy < scale; ++dy) {
                        for (int dx = 0; dx < scale; ++dx) {
                            set_pixel(x + column * scale + dx, top + row * scale + dy, color);
                        }
                    }
                }
            }
        }
        x += 6 * scale;
    }
}

//=============================================================================================================

std::vector<std::uint8_t> RasterImage::encode_png() const
{
    std::vector<std::uint8_t> png = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};

    std::vector<std::uint8_t> header;
    appendBig32(header, static_cast<std::uint32_t>(m_width));
    appendBig32(header, static_cast<std::uint32_t>(m_height));
    // 8 bits per sample, colour type 2 (RGB), deflate, adaptive filtering, no interlace
    header.insert(header.end(), {8, 2, 0, 0, 0});
    appendChunk(png, "IHDR", header);

    // Every row starts with its filter type; "Sub" (1) stores the difference to the pixel on the left
    const std::size_t stride = static_cast<std::size_t>(m_width) * 3;
    std::vector<std::uint8_t> filtered((stride + 1) * static_cast<std::size_t>(m_height));
    for (std::size_t y = 0; y < static_cast<std::size_t>(m_height); ++y) {
        std::uint8_t* out = filtered.data() + y * (stride + 1);
        const std::uint8_t* in = m_rgb.data() + y * stride;
        out[0] = 1;
        for (std::size_t i = 0; i < stride; ++i) {
            out[i + 1] = static_cast<std::uint8_t>(in[i] - (i >= 3 ? in[i - 3] : 0));
        }
    }
    uLongf compressedSize = compressBound(static_cast<uLong>(filtered.size()));
    std::vector<std::uint8_t> compressed(compressedSize);
    compress2(compressed.data(), &compressedSize, filtered.data(), static_cast<uLong>(filtered.size()), Z_DEFAULT_COMPRESSION);
    compressed.resize(compressedSize);
    appendChunk(png, "IDAT", compressed);
    appendChunk(png, "IEND", {});
    return png;
}

//=============================================================================================================

Expected<RasterImage> RasterImage::decode_png(std::span<const std::uint8_t> png)
{
    static constexpr std::array<std::uint8_t, 8> kSignature = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    if (png.size() < kSignature.size() || !std::equal(kSignature.begin(), kSignature.end(), png.begin())) {
        return fail(ErrorCode::Format, "not a PNG file");
    }
    const auto big32 = [&png](std::size_t at) {
        return (std::uint32_t{png[at]} << 24) | (std::uint32_t{png[at + 1]} << 16) | (std::uint32_t{png[at + 2]} << 8) | std::uint32_t{png[at + 3]};
    };

    std::uint32_t width = 0;
    std::uint32_t height = 0;
    int channels = 0;
    std::vector<std::uint8_t> compressed;
    bool ended = false;
    for (std::size_t at = kSignature.size(); at + 12 <= png.size() && !ended;) {
        const std::size_t length = big32(at);
        const std::string_view type(reinterpret_cast<const char*>(png.data() + at + 4), 4);
        if (at + 12 + length > png.size()) {
            return fail(ErrorCode::Format, "truncated PNG chunk {}", type);
        }
        const std::span<const std::uint8_t> data = png.subspan(at + 8, length);
        if (type == "IHDR") {
            if (length < 13) {
                return fail(ErrorCode::Format, "short PNG header");
            }
            width = big32(at + 8);
            height = big32(at + 12);
            const std::uint8_t depth = data[8];
            const std::uint8_t colorType = data[9];
            const std::uint8_t interlace = data[12];
            channels = colorType == 2 ? 3 : colorType == 6 ? 4
                                                           : 0;
            if (depth != 8 || channels == 0 || interlace != 0) {
                return fail(ErrorCode::Format, "unsupported PNG (bit depth {}, colour type {}, interlace {}); 8-bit RGB or RGBA without interlace is read",
                            depth, colorType, interlace);
            }
            if (width > 65535 || height > 65535) {
                return fail(ErrorCode::Format, "PNG of {} x {} pixels is too large", width, height);
            }
        } else if (type == "IDAT") {
            compressed.insert(compressed.end(), data.begin(), data.end());
        } else if (type == "IEND") {
            ended = true;
        }
        at += 12 + length;
    }
    if (channels == 0 || !ended) {
        return fail(ErrorCode::Format, "PNG without header or end");
    }

    // Undo the per-row filters (None, Sub, Up, Average, Paeth)
    const std::size_t stride = std::size_t{width} * static_cast<std::size_t>(channels);
    std::vector<std::uint8_t> filtered((stride + 1) * height);
    uLongf size = static_cast<uLongf>(filtered.size());
    if (uncompress(filtered.data(), &size, compressed.data(), static_cast<uLong>(compressed.size())) != Z_OK || size != filtered.size()) {
        return fail(ErrorCode::Format, "damaged PNG image data");
    }
    const auto bpp = static_cast<std::size_t>(channels);
    std::vector<std::uint8_t> rows(stride * height);
    for (std::size_t y = 0; y < height; ++y) {
        const std::uint8_t filter = filtered[y * (stride + 1)];
        const std::uint8_t* in = filtered.data() + y * (stride + 1) + 1;
        std::uint8_t* out = rows.data() + y * stride;
        const std::uint8_t* up = y > 0 ? out - stride : nullptr;
        for (std::size_t i = 0; i < stride; ++i) {
            const int a = i >= bpp ? out[i - bpp] : 0;
            const int b = up ? up[i] : 0;
            const int c = up && i >= bpp ? up[i - bpp] : 0;
            int predictor = 0;
            switch (filter) {
                case 0:
                    break;
                case 1:
                    predictor = a;
                    break;
                case 2:
                    predictor = b;
                    break;
                case 3:
                    predictor = (a + b) / 2;
                    break;
                case 4: {
                    const int p = a + b - c;
                    const int pa = std::abs(p - a);
                    const int pb = std::abs(p - b);
                    const int pc = std::abs(p - c);
                    predictor = pa <= pb && pa <= pc ? a : pb <= pc ? b
                                                                    : c;
                    break;
                }
                default:
                    return fail(ErrorCode::Format, "unknown PNG filter {}", filter);
            }
            out[i] = static_cast<std::uint8_t>(in[i] + predictor);
        }
    }

    RasterImage image(static_cast<int>(width), static_cast<int>(height));
    for (std::size_t pixel = 0; pixel < std::size_t{width} * height; ++pixel) {
        for (std::size_t k = 0; k < 3; ++k) {
            image.m_rgb[pixel * 3 + k] = rows[pixel * bpp + k];
        }
    }
    return image;
}

//=============================================================================================================

Expected<RasterImage> RasterImage::load_png(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return fail(ErrorCode::Io, "cannot read {}", utf8_from_path(path));
    }
    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    Expected<RasterImage> image = decode_png(bytes);
    if (!image) {
        return fail(image.error().code, "{}: {}", utf8_from_path(path), image.error().what());
    }
    return image;
}

//=============================================================================================================

Expected<void> RasterImage::save_png(const std::filesystem::path& path) const
{
    const std::vector<std::uint8_t> png = encode_png();
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    if (!out.flush()) {
        return fail(ErrorCode::Io, "cannot write {}", utf8_from_path(path));
    }
    return {};
}
