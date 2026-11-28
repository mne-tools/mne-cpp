//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     font_face.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of FontFace.
 *
 * Tables are read in place from the font data; every access goes through bounds-checked readers that return
 * zero past the end, and offsets are validated before they are followed.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "font_face.h"
#include "detail/font_names.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <tuple>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;

namespace
{

constexpr int kMaxCompositeDepth = 8;
constexpr std::size_t kMaxGlyphPoints = 1 << 16;

std::uint32_t tag(const char (&name)[5])
{
    return (static_cast<std::uint32_t>(static_cast<unsigned char>(name[0])) << 24) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(name[1])) << 16) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(name[2])) << 8) | static_cast<std::uint32_t>(static_cast<unsigned char>(name[3]));
}

// Big-endian reads that give 0 past the end of the data
struct Bytes
{
    std::span<const std::uint8_t> data;

    bool has(std::size_t offset, std::size_t size) const
    {
        return offset <= data.size() && size <= data.size() - offset;
    }

    std::uint8_t u8(std::size_t offset) const
    {
        return has(offset, 1) ? data[offset] : 0;
    }

    std::uint16_t u16(std::size_t offset) const
    {
        return has(offset, 2) ? static_cast<std::uint16_t>((data[offset] << 8) | data[offset + 1]) : 0;
    }

    std::int16_t i16(std::size_t offset) const
    {
        return static_cast<std::int16_t>(u16(offset));
    }

    std::uint32_t u32(std::size_t offset) const
    {
        return has(offset, 4) ? (static_cast<std::uint32_t>(u16(offset)) << 16) | u16(offset + 2) : 0;
    }
};

void appendUtf8(std::string& out, char32_t c)
{
    if (c < 0x80) {
        out += static_cast<char>(c);
    } else if (c < 0x800) {
        out += static_cast<char>(0xC0 | (c >> 6));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        out += static_cast<char>(0xE0 | (c >> 12));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (c >> 18));
        out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    }
}

std::string utf16beToUtf8(const Bytes& bytes, std::size_t offset, std::size_t length)
{
    std::string out;
    for (std::size_t i = 0; i + 1 < length; i += 2) {
        char32_t c = bytes.u16(offset + i);
        if (c >= 0xD800 && c < 0xDC00 && i + 3 < length) {
            const char32_t low = bytes.u16(offset + i + 2);
            if (low >= 0xDC00 && low < 0xE000) {
                c = 0x10000 + ((c - 0xD800) << 10) + (low - 0xDC00);
                i += 2;
            }
        }
        appendUtf8(out, c);
    }
    return out;
}

// Size in bytes of a GPOS value record of the given format
std::size_t valueRecordSize(std::uint16_t format)
{
    std::size_t size = 0;
    for (int bit = 0; bit < 8; ++bit) {
        size += (format >> bit) & 1u ? 2 : 0;
    }
    return size;
}

// The XAdvance of a value record, 0 if the format has none
int xAdvance(const Bytes& b, std::size_t record, std::uint16_t format)
{
    if ((format & 0x4) == 0) {
        return 0;
    }
    return b.i16(record + ((format & 0x1) ? 2 : 0) + ((format & 0x2) ? 2 : 0));
}

// Coverage index of a glyph, -1 if not covered
int coverageIndex(const Bytes& b, std::size_t coverage, std::uint32_t glyph)
{
    const std::uint16_t format = b.u16(coverage);
    if (format == 1) {
        int lo = 0;
        int hi = static_cast<int>(b.u16(coverage + 2)) - 1;
        while (lo <= hi) {
            const int mid = (lo + hi) / 2;
            const std::uint16_t g = b.u16(coverage + 4 + 2 * static_cast<std::size_t>(mid));
            if (g == glyph) {
                return mid;
            }
            if (g < glyph) {
                lo = mid + 1;
            } else {
                hi = mid - 1;
            }
        }
    } else if (format == 2) {
        int lo = 0;
        int hi = static_cast<int>(b.u16(coverage + 2)) - 1;
        while (lo <= hi) {
            const int mid = (lo + hi) / 2;
            const std::size_t range = coverage + 4 + 6 * static_cast<std::size_t>(mid);
            if (glyph < b.u16(range)) {
                hi = mid - 1;
            } else if (glyph > b.u16(range + 2)) {
                lo = mid + 1;
            } else {
                return static_cast<int>(b.u16(range + 4) + (glyph - b.u16(range)));
            }
        }
    }
    return -1;
}

// Class of a glyph in a class definition table, 0 if unlisted
int glyphClass(const Bytes& b, std::size_t classDef, std::uint32_t glyph)
{
    const std::uint16_t format = b.u16(classDef);
    if (format == 1) {
        const std::uint32_t start = b.u16(classDef + 2);
        const std::uint32_t count = b.u16(classDef + 4);
        if (glyph >= start && glyph - start < count) {
            return b.u16(classDef + 6 + 2 * static_cast<std::size_t>(glyph - start));
        }
    } else if (format == 2) {
        int lo = 0;
        int hi = static_cast<int>(b.u16(classDef + 2)) - 1;
        while (lo <= hi) {
            const int mid = (lo + hi) / 2;
            const std::size_t range = classDef + 4 + 6 * static_cast<std::size_t>(mid);
            if (glyph < b.u16(range)) {
                hi = mid - 1;
            } else if (glyph > b.u16(range + 2)) {
                lo = mid + 1;
            } else {
                return b.u16(range + 4);
            }
        }
    }
    return 0;
}

// Quadratic segments of a contour, flattened so they deviate at most `tolerance` from the curve
FontFace::Polygon flatten(const FontFace::Contour& contour, double tolerance)
{
    FontFace::Polygon out;
    const std::size_t n = contour.size();
    if (n < 2) {
        return out;
    }
    // Implied on-curve points between consecutive off-curve ones, so off-curve points stand alone
    std::vector<FontFace::OutlinePoint> points;
    points.reserve(2 * n);
    for (std::size_t i = 0; i < n; ++i) {
        const FontFace::OutlinePoint& point = contour[i];
        const FontFace::OutlinePoint& next = contour[(i + 1) % n];
        points.push_back(point);
        if (!point.on_curve && !next.on_curve) {
            points.push_back({(point.x + next.x) / 2.0, (point.y + next.y) / 2.0, true});
        }
    }
    const std::size_t m = points.size();
    std::size_t start = 0;
    while (start < m && !points[start].on_curve) {
        ++start;
    }
    if (start == m) {
        return out;
    }
    Point current{points[start].x, points[start].y};
    out.push_back(current);
    for (std::size_t k = 1; k <= m; ++k) {
        const FontFace::OutlinePoint& p = points[(start + k) % m];
        if (p.on_curve) {
            current = {p.x, p.y};
            out.push_back(current);
            continue;
        }
        const FontFace::OutlinePoint& next = points[(start + k + 1) % m];
        const Point to{next.x, next.y};
        const double dx = current.x - 2.0 * p.x + to.x;
        const double dy = current.y - 2.0 * p.y + to.y;
        const double deviation = std::hypot(dx, dy) / 4.0;
        const int steps = std::clamp(static_cast<int>(std::ceil(std::sqrt(deviation / tolerance))), 1, 64);
        for (int s = 1; s <= steps; ++s) {
            const double t = static_cast<double>(s) / steps;
            const double u = 1.0 - t;
            out.push_back({u * u * current.x + 2.0 * u * t * p.x + t * t * to.x, u * u * current.y + 2.0 * u * t * p.y + t * t * to.y});
        }
        current = to;
        ++k;
    }
    if (out.size() > 1 && out.back() == out.front()) {
        out.pop_back();
    }
    return out;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Expected<std::shared_ptr<const FontFace>> FontFace::load(const std::filesystem::path& path, int face_index)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return fail(ErrorCode::Io, "Cannot open font {}", path.string());
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (file.bad()) {
        return fail(ErrorCode::Io, "Cannot read font {}", path.string());
    }
    auto face = from_bytes(std::move(bytes), face_index);
    if (!face) {
        return fail(face.error().code, "{}: {}", path.string(), face.error().what());
    }
    return face;
}

//=============================================================================================================

Expected<std::shared_ptr<const FontFace>> FontFace::from_bytes(std::vector<std::uint8_t> bytes, int face_index)
{
    std::shared_ptr<FontFace> face(new FontFace());
    face->m_owned = std::move(bytes);
    face->m_data = face->m_owned;
    return parse(std::move(face), face_index);
}

//=============================================================================================================

Expected<std::shared_ptr<const FontFace>> FontFace::from_static_bytes(std::span<const std::uint8_t> bytes, int face_index)
{
    std::shared_ptr<FontFace> face(new FontFace());
    face->m_data = bytes;
    return parse(std::move(face), face_index);
}

//=============================================================================================================

int FontFace::face_count(std::span<const std::uint8_t> bytes)
{
    const Bytes b{bytes};
    const std::uint32_t version = b.u32(0);
    if (version == tag("ttcf")) {
        const std::uint32_t count = b.u32(8);
        return b.has(12, 4 * static_cast<std::size_t>(count)) ? static_cast<int>(std::min<std::uint32_t>(count, 4096)) : 0;
    }
    return version == 0x00010000u || version == tag("true") || version == tag("OTTO") ? 1 : 0;
}

//=============================================================================================================

Expected<std::shared_ptr<const FontFace>> FontFace::parse(std::shared_ptr<FontFace> face, int face_index)
{
    const Bytes b{face->m_data};
    std::size_t directory = 0;
    if (b.u32(0) == tag("ttcf")) {
        const int count = face_count(face->m_data);
        if (face_index < 0 || face_index >= count) {
            return fail(ErrorCode::NotFound, "The collection has no face {}", face_index);
        }
        directory = b.u32(12 + 4 * static_cast<std::size_t>(face_index));
    } else if (face_index != 0) {
        return fail(ErrorCode::NotFound, "The font has no face {}", face_index);
    }
    const std::uint32_t version = b.u32(directory);
    if (version == tag("OTTO")) {
        return fail(ErrorCode::Format, "CFF outlines are not supported");
    }
    if (version != 0x00010000u && version != tag("true")) {
        return fail(ErrorCode::Format, "Not a TrueType font");
    }
    const std::size_t tableCount = b.u16(directory + 4);
    if (!b.has(directory + 12, 16 * tableCount)) {
        return fail(ErrorCode::Format, "Truncated table directory");
    }
    bool hasCff = false;
    Table maxp;
    for (std::size_t i = 0; i < tableCount; ++i) {
        const std::size_t record = directory + 12 + 16 * i;
        const Table table{b.u32(record + 8), b.u32(record + 12)};
        if (!b.has(table.offset, table.length)) {
            continue;
        }
        const std::uint32_t name = b.u32(record);
        if (name == tag("head")) {
            face->m_head = table;
        } else if (name == tag("hhea")) {
            face->m_hhea = table;
        } else if (name == tag("hmtx")) {
            face->m_hmtx = table;
        } else if (name == tag("loca")) {
            face->m_loca = table;
        } else if (name == tag("glyf")) {
            face->m_glyf = table;
        } else if (name == tag("cmap")) {
            face->m_cmap = table;
        } else if (name == tag("name")) {
            face->m_name = table;
        } else if (name == tag("OS/2")) {
            face->m_os2 = table;
        } else if (name == tag("kern")) {
            face->m_kern = table;
        } else if (name == tag("GPOS")) {
            face->m_gpos = table;
        } else if (name == tag("maxp")) {
            maxp = table;
        } else if (name == tag("CFF ") || name == tag("CFF2")) {
            hasCff = true;
        }
    }
    if (face->m_glyf.length == 0 && hasCff) {
        return fail(ErrorCode::Format, "CFF outlines are not supported");
    }
    if (face->m_head.length < 54 || face->m_hhea.length < 36 || maxp.length < 6 || face->m_loca.length == 0 || face->m_glyf.length == 0 ||
        face->m_cmap.length < 4 || face->m_hmtx.length < 4) {
        return fail(ErrorCode::Format, "Missing or truncated TrueType tables");
    }

    const std::size_t head = face->m_head.offset;
    face->m_unitsPerEm = b.u16(head + 18);
    if (face->m_unitsPerEm < 16 || face->m_unitsPerEm > 16384) {
        return fail(ErrorCode::Format, "Invalid units per em");
    }
    face->m_longLoca = b.i16(head + 50) != 0;
    face->m_glyphCount = b.u16(maxp.offset + 4);
    const std::size_t locaEntries = face->m_loca.length / (face->m_longLoca ? 4 : 2);
    face->m_glyphCount = static_cast<int>(std::min<std::size_t>(static_cast<std::size_t>(face->m_glyphCount), locaEntries > 0 ? locaEntries - 1 : 0));
    if (face->m_glyphCount == 0) {
        return fail(ErrorCode::Format, "The font has no glyphs");
    }
    const std::size_t hhea = face->m_hhea.offset;
    face->m_ascender = b.i16(hhea + 4);
    face->m_descender = b.i16(hhea + 6);
    face->m_lineGap = b.i16(hhea + 8);
    face->m_hMetricCount = static_cast<int>(std::min<std::size_t>(b.u16(hhea + 34), face->m_hmtx.length / 4));
    if (face->m_hMetricCount == 0) {
        return fail(ErrorCode::Format, "The font has no horizontal metrics");
    }

    // The Unicode character map: full repertoire (format 12) before the basic plane (format 4)
    const std::size_t cmap = face->m_cmap.offset;
    const std::size_t subtables = b.u16(cmap + 2);
    int bestRank = 0;
    for (std::size_t i = 0; i < subtables; ++i) {
        const std::size_t record = cmap + 4 + 8 * i;
        if (!b.has(record, 8)) {
            break;
        }
        const std::uint16_t platform = b.u16(record);
        const std::uint16_t encoding = b.u16(record + 2);
        const std::size_t offset = cmap + b.u32(record + 4);
        if (offset >= cmap + face->m_cmap.length || !b.has(offset, 4)) {
            continue;
        }
        const std::uint16_t format = b.u16(offset);
        const bool unicode = platform == 0 || (platform == 3 && (encoding == 1 || encoding == 10));
        int rank = 0;
        if (unicode && format == 12) {
            rank = 2;
        } else if (unicode && format == 4) {
            rank = 1;
        }
        if (rank > bestRank) {
            bestRank = rank;
            face->m_cmapSubtable = offset;
            face->m_cmapFormat = format;
        }
    }
    if (bestRank == 0) {
        return fail(ErrorCode::Format, "The font has no Unicode character map");
    }

    if (face->m_os2.length >= 64) {
        face->m_weight = std::clamp<int>(b.u16(face->m_os2.offset + 4), 1, 1000);
        face->m_italic = (b.u16(face->m_os2.offset + 62) & 0x1) != 0;
    } else {
        const std::uint16_t macStyle = b.u16(head + 44);
        face->m_weight = (macStyle & 0x1) ? 700 : 400;
        face->m_italic = (macStyle & 0x2) != 0;
    }
    if (face->m_ascender == 0 && face->m_descender == 0) {
        face->m_ascender = face->m_unitsPerEm * 4 / 5;
        face->m_descender = -face->m_unitsPerEm / 5;
    }
    // sCapHeight exists from version 2 of the OS/2 table on
    if (face->m_os2.length >= 90 && b.u16(face->m_os2.offset) >= 2) {
        face->m_capHeight = static_cast<std::int16_t>(b.u16(face->m_os2.offset + 88));
    }
    if (face->m_capHeight <= 0) {
        face->m_capHeight = face->m_unitsPerEm * 7 / 10;
    }
    face->read_names();
    face->read_gpos_kerning();
    return std::shared_ptr<const FontFace>(std::move(face));
}

//=============================================================================================================

void FontFace::read_names()
{
    std::tie(m_family, m_style) = detail::font_names(m_data.subspan(m_name.offset, m_name.length));
}

//=============================================================================================================

void FontFace::read_gpos_kerning()
{
    if (m_gpos.length < 10) {
        return;
    }
    const Bytes b{m_data.subspan(m_gpos.offset, m_gpos.length)};
    const std::size_t scripts = b.u16(4);
    const std::size_t features = b.u16(6);
    const std::size_t lookups = b.u16(8);
    if (scripts == 0 || features == 0 || lookups == 0) {
        return;
    }
    // Feature indices of the default language systems of the DFLT and latn scripts
    std::vector<std::uint16_t> featureIndices;
    const std::size_t scriptCount = b.u16(scripts);
    for (std::size_t i = 0; i < scriptCount && i < 512; ++i) {
        const std::size_t record = scripts + 2 + 6 * i;
        const std::uint32_t scriptTag = b.u32(record);
        if (scriptTag != tag("DFLT") && scriptTag != tag("latn")) {
            continue;
        }
        const std::size_t script = scripts + b.u16(record + 4);
        const std::uint16_t langSysOffset = b.u16(script);
        if (langSysOffset == 0) {
            continue;
        }
        const std::size_t langSys = script + langSysOffset;
        const std::size_t indexCount = b.u16(langSys + 4);
        for (std::size_t k = 0; k < indexCount && k < 1024; ++k) {
            featureIndices.push_back(b.u16(langSys + 6 + 2 * k));
        }
    }
    // Lookup indices of their kern features
    std::vector<std::uint16_t> lookupIndices;
    const std::size_t featureCount = b.u16(features);
    for (const std::uint16_t index : featureIndices) {
        if (index >= featureCount) {
            continue;
        }
        const std::size_t record = features + 2 + 6 * static_cast<std::size_t>(index);
        if (b.u32(record) != tag("kern")) {
            continue;
        }
        const std::size_t feature = features + b.u16(record + 4);
        const std::size_t count = b.u16(feature + 2);
        for (std::size_t k = 0; k < count && k < 1024; ++k) {
            lookupIndices.push_back(b.u16(feature + 4 + 2 * k));
        }
    }
    std::sort(lookupIndices.begin(), lookupIndices.end());
    lookupIndices.erase(std::unique(lookupIndices.begin(), lookupIndices.end()), lookupIndices.end());
    // Their pair adjustment subtables, extensions unwrapped
    const std::size_t lookupCount = b.u16(lookups);
    for (const std::uint16_t index : lookupIndices) {
        if (index >= lookupCount) {
            continue;
        }
        const std::size_t lookup = lookups + b.u16(lookups + 2 + 2 * static_cast<std::size_t>(index));
        const std::uint16_t type = b.u16(lookup);
        const std::size_t subtableCount = b.u16(lookup + 4);
        for (std::size_t k = 0; k < subtableCount && k < 4096; ++k) {
            std::size_t subtable = lookup + b.u16(lookup + 6 + 2 * k);
            if (type == 9) {
                if (b.u16(subtable) != 1 || b.u16(subtable + 2) != 2) {
                    continue;
                }
                subtable += b.u32(subtable + 4);
            } else if (type != 2) {
                continue;
            }
            if (b.has(subtable, 10)) {
                m_pairSubtables.push_back(m_gpos.offset + subtable);
            }
        }
        m_pairLookupEnds.push_back(m_pairSubtables.size());
    }
}

//=============================================================================================================

std::uint32_t FontFace::glyph_index(char32_t code_point) const
{
    const Bytes b{m_data};
    const std::size_t t = m_cmapSubtable;
    std::uint32_t glyph = 0;
    if (m_cmapFormat == 4) {
        if (code_point > 0xFFFF) {
            return 0;
        }
        const std::size_t segX2 = b.u16(t + 6) & ~1u;
        const std::size_t ends = t + 14;
        const std::size_t starts = ends + segX2 + 2;
        const std::size_t deltas = starts + segX2;
        const std::size_t ranges = deltas + segX2;
        std::size_t lo = 0;
        std::size_t hi = segX2 / 2;
        while (lo < hi) {
            const std::size_t mid = (lo + hi) / 2;
            if (b.u16(ends + 2 * mid) < code_point) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo >= segX2 / 2) {
            return 0;
        }
        const std::uint16_t start = b.u16(starts + 2 * lo);
        if (code_point < start) {
            return 0;
        }
        const std::uint16_t delta = b.u16(deltas + 2 * lo);
        const std::uint16_t rangeOffset = b.u16(ranges + 2 * lo);
        if (rangeOffset == 0) {
            glyph = (code_point + delta) & 0xFFFFu;
        } else {
            const std::size_t address = ranges + 2 * lo + rangeOffset + 2 * (code_point - start);
            glyph = b.u16(address);
            if (glyph != 0) {
                glyph = (glyph + delta) & 0xFFFFu;
            }
        }
    } else if (m_cmapFormat == 12) {
        const std::size_t groups = std::min<std::size_t>(b.u32(t + 12), (m_data.size() - std::min(m_data.size(), t + 16)) / 12);
        std::size_t lo = 0;
        std::size_t hi = groups;
        while (lo < hi) {
            const std::size_t mid = (lo + hi) / 2;
            const std::size_t group = t + 16 + 12 * mid;
            if (b.u32(group + 4) < code_point) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo < groups) {
            const std::size_t group = t + 16 + 12 * lo;
            const std::uint32_t start = b.u32(group);
            if (code_point >= start) {
                glyph = b.u32(group + 8) + (code_point - start);
            }
        }
    }
    return glyph < static_cast<std::uint32_t>(m_glyphCount) ? glyph : 0;
}

//=============================================================================================================

int FontFace::advance(std::uint32_t glyph) const
{
    const Bytes b{m_data};
    const std::size_t index = std::min<std::size_t>(glyph, static_cast<std::size_t>(m_hMetricCount - 1));
    return b.u16(m_hmtx.offset + 4 * index);
}

//=============================================================================================================

int FontFace::kerning(std::uint32_t left, std::uint32_t right) const
{
    if (left > 0xFFFF || right > 0xFFFF || (m_pairSubtables.empty() && m_kern.length == 0)) {
        return 0;
    }
    const std::uint32_t key = (left << 16) | right;
    {
        const std::lock_guard lock(m_cacheMutex);
        if (const auto it = m_kerning.find(key); it != m_kerning.end()) {
            return it->second;
        }
    }
    const int value = !m_pairSubtables.empty() ? gpos_kerning(left, right) : kern_table_kerning(left, right);
    const std::lock_guard lock(m_cacheMutex);
    if (m_kerning.size() > 65536) {
        m_kerning.clear();
    }
    m_kerning.emplace(key, value);
    return value;
}

//=============================================================================================================

int FontFace::gpos_kerning(std::uint32_t left, std::uint32_t right) const
{
    const Bytes b{m_data};
    int total = 0;
    std::size_t begin = 0;
    for (const std::size_t end : m_pairLookupEnds) {
        // The first subtable of a lookup that covers the pair applies
        for (std::size_t k = begin; k < end; ++k) {
            const std::size_t s = m_pairSubtables[k];
            const int coverage = coverageIndex(b, s + b.u16(s + 2), left);
            if (coverage < 0) {
                continue;
            }
            const std::uint16_t format1 = b.u16(s + 4);
            const std::uint16_t format2 = b.u16(s + 6);
            const std::size_t size1 = valueRecordSize(format1);
            const std::size_t size2 = valueRecordSize(format2);
            const std::uint16_t format = b.u16(s);
            if (format == 1) {
                if (static_cast<std::size_t>(coverage) >= b.u16(s + 8)) {
                    continue;
                }
                const std::size_t set = s + b.u16(s + 10 + 2 * static_cast<std::size_t>(coverage));
                const std::size_t recordSize = 2 + size1 + size2;
                int lo = 0;
                int hi = static_cast<int>(b.u16(set)) - 1;
                bool found = false;
                while (lo <= hi) {
                    const int mid = (lo + hi) / 2;
                    const std::size_t record = set + 2 + recordSize * static_cast<std::size_t>(mid);
                    const std::uint16_t second = b.u16(record);
                    if (second == right) {
                        total += xAdvance(b, record + 2, format1);
                        found = true;
                        break;
                    }
                    if (second < right) {
                        lo = mid + 1;
                    } else {
                        hi = mid - 1;
                    }
                }
                if (!found) {
                    continue;
                }
                break;
            }
            if (format == 2) {
                const std::size_t class1Count = b.u16(s + 12);
                const std::size_t class2Count = b.u16(s + 14);
                const std::size_t class1 = static_cast<std::size_t>(glyphClass(b, s + b.u16(s + 8), left));
                const std::size_t class2 = static_cast<std::size_t>(glyphClass(b, s + b.u16(s + 10), right));
                if (class1 < class1Count && class2 < class2Count) {
                    total += xAdvance(b, s + 16 + (class1 * class2Count + class2) * (size1 + size2), format1);
                }
                break;
            }
        }
        begin = end;
    }
    return total;
}

//=============================================================================================================

int FontFace::kern_table_kerning(std::uint32_t left, std::uint32_t right) const
{
    const Bytes b{m_data.subspan(m_kern.offset, m_kern.length)};
    if (b.u16(0) != 0) {
        return 0; // Apple's kern table version 1
    }
    const std::size_t tables = b.u16(2);
    std::size_t offset = 4;
    int total = 0;
    for (std::size_t i = 0; i < tables && b.has(offset, 6); ++i) {
        const std::size_t length = b.u16(offset + 2);
        const std::uint16_t coverage = b.u16(offset + 4);
        // Format 0, horizontal, kerning values, not cross-stream
        if ((coverage >> 8) == 0 && (coverage & 0x7) == 0x1) {
            const std::size_t pairs = b.u16(offset + 6);
            const std::uint32_t key = (left << 16) | right;
            std::size_t lo = 0;
            std::size_t hi = pairs;
            while (lo < hi) {
                const std::size_t mid = (lo + hi) / 2;
                const std::uint32_t k = b.u32(offset + 14 + 6 * mid);
                if (k < key) {
                    lo = mid + 1;
                } else {
                    hi = mid;
                }
            }
            if (lo < pairs && b.u32(offset + 14 + 6 * lo) == key) {
                total += b.i16(offset + 18 + 6 * lo);
            }
        }
        if (length < 6) {
            break;
        }
        offset += length;
    }
    return total;
}

//=============================================================================================================

std::vector<FontFace::Contour> FontFace::outline(std::uint32_t glyph) const
{
    std::vector<Contour> contours;
    append_outline(glyph, 0, contours);
    return contours;
}

//=============================================================================================================

void FontFace::append_outline(std::uint32_t glyph, int depth, std::vector<Contour>& contours) const
{
    if (glyph >= static_cast<std::uint32_t>(m_glyphCount) || depth > kMaxCompositeDepth) {
        return;
    }
    const Bytes b{m_data};
    const std::size_t loca = m_loca.offset;
    const std::size_t start = m_longLoca ? b.u32(loca + 4 * glyph) : 2 * static_cast<std::size_t>(b.u16(loca + 2 * glyph));
    const std::size_t end = m_longLoca ? b.u32(loca + 4 * (glyph + 1)) : 2 * static_cast<std::size_t>(b.u16(loca + 2 * (glyph + 1)));
    if (end <= start || end > m_glyf.length || end - start < 10) {
        return;
    }
    // Reads are confined to the glyph's own data
    const Bytes g{m_data.subspan(m_glyf.offset + start, end - start)};
    const int contourCount = g.i16(0);
    if (contourCount >= 0) {
        const std::size_t n = static_cast<std::size_t>(contourCount);
        std::vector<std::size_t> ends(n);
        std::size_t previous = 0;
        for (std::size_t i = 0; i < n; ++i) {
            ends[i] = g.u16(10 + 2 * i);
            if ((i > 0 && ends[i] < previous) || !g.has(10 + 2 * i, 2)) {
                return;
            }
            previous = ends[i];
        }
        const std::size_t pointCount = n > 0 ? ends.back() + 1 : 0;
        if (pointCount == 0 || pointCount > kMaxGlyphPoints) {
            return;
        }
        std::size_t p = 10 + 2 * n;
        p += 2 + g.u16(p);
        std::vector<std::uint8_t> flags;
        flags.reserve(pointCount);
        while (flags.size() < pointCount) {
            if (!g.has(p, 1)) {
                return;
            }
            const std::uint8_t flag = g.u8(p++);
            flags.push_back(flag);
            if (flag & 0x08) {
                if (!g.has(p, 1)) {
                    return;
                }
                const std::size_t repeat = g.u8(p++);
                for (std::size_t r = 0; r < repeat && flags.size() < pointCount; ++r) {
                    flags.push_back(flag);
                }
            }
        }
        std::vector<OutlinePoint> points(pointCount);
        const auto readCoordinates = [&](std::uint8_t shortBit, std::uint8_t sameBit, bool isX) {
            int value = 0;
            for (std::size_t i = 0; i < pointCount; ++i) {
                const std::uint8_t flag = flags[i];
                if (flag & shortBit) {
                    if (!g.has(p, 1)) {
                        return false;
                    }
                    const int delta = g.u8(p++);
                    value += (flag & sameBit) ? delta : -delta;
                } else if (!(flag & sameBit)) {
                    if (!g.has(p, 2)) {
                        return false;
                    }
                    value += g.i16(p);
                    p += 2;
                }
                (isX ? points[i].x : points[i].y) = value;
            }
            return true;
        };
        if (!readCoordinates(0x02, 0x10, true) || !readCoordinates(0x04, 0x20, false)) {
            return;
        }
        std::size_t first = 0;
        for (std::size_t i = 0; i < n; ++i) {
            Contour contour;
            for (std::size_t k = first; k <= ends[i]; ++k) {
                contour.push_back({points[k].x, points[k].y, (flags[k] & 0x01) != 0});
            }
            first = ends[i] + 1;
            if (contour.size() >= 2) {
                contours.push_back(std::move(contour));
            }
        }
        return;
    }

    // A composite: transformed components
    std::size_t p = 10;
    for (int component = 0; component < 256; ++component) {
        if (!g.has(p, 4)) {
            return;
        }
        const std::uint16_t flags = g.u16(p);
        const std::uint16_t child = g.u16(p + 2);
        p += 4;
        double arg1 = 0.0;
        double arg2 = 0.0;
        if (flags & 0x0001) {
            if (!g.has(p, 4)) {
                return;
            }
            arg1 = (flags & 0x0002) ? g.i16(p) : g.u16(p);
            arg2 = (flags & 0x0002) ? g.i16(p + 2) : g.u16(p + 2);
            p += 4;
        } else {
            if (!g.has(p, 2)) {
                return;
            }
            arg1 = (flags & 0x0002) ? static_cast<std::int8_t>(g.u8(p)) : g.u8(p);
            arg2 = (flags & 0x0002) ? static_cast<std::int8_t>(g.u8(p + 1)) : g.u8(p + 1);
            p += 2;
        }
        double a = 1.0;
        double bb = 0.0;
        double c = 0.0;
        double d = 1.0;
        const auto f2dot14 = [&](std::size_t at) {
            return g.i16(at) / 16384.0;
        };
        if (flags & 0x0008) {
            a = d = f2dot14(p);
            p += 2;
        } else if (flags & 0x0040) {
            a = f2dot14(p);
            d = f2dot14(p + 2);
            p += 4;
        } else if (flags & 0x0080) {
            a = f2dot14(p);
            bb = f2dot14(p + 2);
            c = f2dot14(p + 4);
            d = f2dot14(p + 6);
            p += 8;
        }
        if (p > g.data.size()) {
            return;
        }
        std::vector<Contour> parts;
        append_outline(child, depth + 1, parts);
        for (Contour& contour : parts) {
            for (OutlinePoint& point : contour) {
                const double x = point.x;
                point.x = a * x + c * point.y;
                point.y = bb * x + d * point.y;
            }
        }
        double dx = 0.0;
        double dy = 0.0;
        if (flags & 0x0002) {
            dx = arg1;
            dy = arg2;
            if (flags & 0x0800) {
                dx = a * arg1 + c * arg2;
                dy = bb * arg1 + d * arg2;
            }
        } else {
            // Point matching: the child's point arg2 lands on the parent's point arg1
            const auto pointAt = [](const std::vector<Contour>& list, std::size_t index) -> const OutlinePoint* {
                for (const Contour& contour : list) {
                    if (index < contour.size()) {
                        return &contour[index];
                    }
                    index -= contour.size();
                }
                return nullptr;
            };
            const OutlinePoint* parent = pointAt(contours, static_cast<std::size_t>(arg1));
            const OutlinePoint* own = pointAt(parts, static_cast<std::size_t>(arg2));
            if (parent && own) {
                dx = parent->x - own->x;
                dy = parent->y - own->y;
            }
        }
        for (Contour& contour : parts) {
            for (OutlinePoint& point : contour) {
                point.x += dx;
                point.y += dy;
            }
            contours.push_back(std::move(contour));
        }
        if (!(flags & 0x0020)) {
            return;
        }
    }
}

//=============================================================================================================

std::shared_ptr<const std::vector<FontFace::Polygon>> FontFace::flattened_outline(std::uint32_t glyph, double pixels_per_em) const
{
    // Sizes share a cache entry per power of two; the tolerance fits the largest size of the bucket
    const int level = std::clamp(static_cast<int>(std::ceil(std::log2(std::max(pixels_per_em, 1.0)))), 0, 14);
    const std::uint64_t key = (static_cast<std::uint64_t>(glyph) << 8) | static_cast<std::uint64_t>(level);
    {
        const std::lock_guard lock(m_cacheMutex);
        if (const auto it = m_outlines.find(key); it != m_outlines.end()) {
            return it->second;
        }
    }
    const double tolerance = 0.1 * m_unitsPerEm / std::ldexp(1.0, level);
    auto polygons = std::make_shared<std::vector<Polygon>>();
    for (const Contour& contour : outline(glyph)) {
        Polygon polygon = flatten(contour, tolerance);
        if (polygon.size() >= 3) {
            polygons->push_back(std::move(polygon));
        }
    }
    const std::lock_guard lock(m_cacheMutex);
    return m_outlines.emplace(key, std::move(polygons)).first->second;
}

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::u32string mne::viz::decode_utf8(std::string_view text)
{
    std::u32string out;
    out.reserve(text.size());
    std::size_t i = 0;
    while (i < text.size()) {
        const unsigned char lead = static_cast<unsigned char>(text[i]);
        int length = 0;
        char32_t c = 0;
        char32_t minimum = 0;
        if (lead < 0x80) {
            out += lead;
            ++i;
            continue;
        }
        if ((lead & 0xE0) == 0xC0) {
            length = 2;
            c = lead & 0x1F;
            minimum = 0x80;
        } else if ((lead & 0xF0) == 0xE0) {
            length = 3;
            c = lead & 0x0F;
            minimum = 0x800;
        } else if ((lead & 0xF8) == 0xF0) {
            length = 4;
            c = lead & 0x07;
            minimum = 0x10000;
        }
        bool valid = length > 0 && i + static_cast<std::size_t>(length) <= text.size();
        for (int k = 1; valid && k < length; ++k) {
            const unsigned char next = static_cast<unsigned char>(text[i + static_cast<std::size_t>(k)]);
            valid = (next & 0xC0) == 0x80;
            c = (c << 6) | (next & 0x3F);
        }
        if (valid && c >= minimum && c <= 0x10FFFF && (c < 0xD800 || c > 0xDFFF)) {
            out += c;
            i += static_cast<std::size_t>(length);
        } else {
            out += U'\uFFFD';
            ++i;
            while (i < text.size() && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80) {
                ++i;
            }
        }
    }
    return out;
}

//=============================================================================================================

std::pair<std::string, std::string> mne::viz::detail::font_names(std::span<const std::uint8_t> table)
{
    const Bytes b{table};
    if (table.size() < 6) {
        return {};
    }
    const std::size_t count = b.u16(2);
    const std::size_t storage = b.u16(4);
    // Best record per name id: Windows English before other Unicode before Mac Roman
    std::string found[18];
    int rank[18] = {};
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t record = 6 + 12 * i;
        if (!b.has(record, 12)) {
            break;
        }
        const std::uint16_t platform = b.u16(record);
        const std::uint16_t language = b.u16(record + 4);
        const std::uint16_t id = b.u16(record + 6);
        const std::size_t length = b.u16(record + 8);
        const std::size_t offset = storage + b.u16(record + 10);
        if ((id != 1 && id != 2 && id != 16 && id != 17) || !b.has(offset, length)) {
            continue;
        }
        int r = 0;
        if (platform == 3 && language == 0x409) {
            r = 4;
        } else if (platform == 3 || platform == 0) {
            r = 3;
        } else if (platform == 1 && language == 0) {
            r = 2;
        }
        if (r <= rank[id]) {
            continue;
        }
        if (platform == 1) {
            std::string text;
            for (std::size_t k = 0; k < length; ++k) {
                const std::uint8_t c = b.u8(offset + k);
                text += c < 0x80 ? static_cast<char>(c) : '?';
            }
            found[id] = std::move(text);
        } else {
            found[id] = utf16beToUtf8(b, offset, length);
        }
        rank[id] = r;
    }
    return {!found[16].empty() ? found[16] : found[1], !found[17].empty() ? found[17] : found[2]};
}
