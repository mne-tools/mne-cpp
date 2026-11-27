//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     font_face.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FontFace reads one face of a TrueType font for the software text renderer.
 *
 * The reader covers what drawing Latin, Greek and Cyrillic text needs: metrics, the character map, glyph
 * outlines (simple and composite) and pair kerning from GPOS or the kern table. Hinting, shaping and CFF
 * outlines are out of scope. Every read is bounds-checked, so damaged files fail to load or yield empty
 * glyphs instead of crashing.
 */

#ifndef VIZ_FONT_FACE_H
#define VIZ_FONT_FACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "geometry.h"
#include <mne/base/error.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief One face of a TrueType (.ttf) or TrueType collection (.ttc) font.
 *
 * Fonts with CFF outlines (.otf without a glyf table) are rejected. Of a variable font the default instance
 * is read. Glyph coordinates are in font units with y growing upwards; units_per_em() of them make the font
 * size.
 *
 * @snippet ex_scene/main.cpp font_face_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT FontFace
{
public:
    /** A point of a glyph outline in font units, y up. */
    struct OutlinePoint
    {
        double x = 0.0;       /**< Horizontal position. */
        double y = 0.0;       /**< Vertical position, growing upwards. */
        bool on_curve = true; /**< Whether the point lies on the outline; off-curve points are quadratic controls. */
    };

    /** A closed glyph contour of on- and off-curve points. */
    using Contour = std::vector<OutlinePoint>;

    /** A closed glyph contour flattened to a polygon, in font units, y up. */
    using Polygon = std::vector<Point>;

    //=========================================================================================================
    /**
     * Reads a face from a font file.
     *
     * @param[in] path         .ttf or .ttc file.
     * @param[in] face_index   Face of a collection; 0 for a single font.
     *
     * @return The face, or why it cannot be read.
     *
     * @since 3.0.0
     */
    static Expected<std::shared_ptr<const FontFace>> load(const std::filesystem::path& path, int face_index = 0);

    //=========================================================================================================
    /**
     * Reads a face from a copy of font data in memory.
     *
     * @param[in] bytes        Contents of a .ttf or .ttc file.
     * @param[in] face_index   Face of a collection; 0 for a single font.
     *
     * @return The face, or why it cannot be read.
     *
     * @since 3.0.0
     */
    static Expected<std::shared_ptr<const FontFace>> from_bytes(std::vector<std::uint8_t> bytes, int face_index = 0);

    //=========================================================================================================
    /**
     * Reads a face from font data that stays valid and unchanged while the face exists, without copying it.
     *
     * @param[in] bytes        Contents of a .ttf or .ttc file, e.g. a font compiled into the program.
     * @param[in] face_index   Face of a collection; 0 for a single font.
     *
     * @return The face, or why it cannot be read.
     *
     * @since 3.0.0
     */
    static Expected<std::shared_ptr<const FontFace>> from_static_bytes(std::span<const std::uint8_t> bytes, int face_index = 0);

    //=========================================================================================================
    /**
     * @param[in] bytes   Contents of a font file.
     *
     * @return The number of faces: the count of a collection, 1 for a single font, 0 for anything else.
     *
     * @since 3.0.0
     */
    static int face_count(std::span<const std::uint8_t> bytes);

    /** @return The family name, e.g. "Source Sans 3", preferring the typographic family. */
    const std::string& family() const
    {
        return m_family;
    }

    /** @return The style name, e.g. "Semibold Italic", preferring the typographic subfamily. */
    const std::string& style() const
    {
        return m_style;
    }

    /** @return The weight from 1 to 1000, 400 regular and 700 bold. */
    int weight() const
    {
        return m_weight;
    }

    /** @return Whether the face is italic or oblique. */
    bool italic() const
    {
        return m_italic;
    }

    /** @return Font units per em. */
    int units_per_em() const
    {
        return m_unitsPerEm;
    }

    /** @return Distance from the baseline to the top of the line, in font units, positive. */
    int ascender() const
    {
        return m_ascender;
    }

    /** @return Distance from the baseline to the bottom of the line, in font units, negative. */
    int descender() const
    {
        return m_descender;
    }

    /** @return Extra space between lines, in font units. */
    int line_gap() const
    {
        return m_lineGap;
    }

    /** @return The number of glyphs. */
    int glyph_count() const
    {
        return m_glyphCount;
    }

    //=========================================================================================================
    /**
     * @param[in] code_point   Unicode code point.
     *
     * @return Its glyph, or 0 (the missing-glyph box) if the face does not cover it.
     *
     * @since 3.0.0
     */
    std::uint32_t glyph_index(char32_t code_point) const;

    //=========================================================================================================
    /**
     * @param[in] glyph   Glyph index.
     *
     * @return Its horizontal advance in font units.
     *
     * @since 3.0.0
     */
    int advance(std::uint32_t glyph) const;

    //=========================================================================================================
    /**
     * Pair kerning from the kern feature of GPOS, else from the kern table.
     *
     * @param[in] left    Glyph before.
     * @param[in] right   Glyph after.
     *
     * @return The adjustment of the advance of @p left in font units, negative to tighten.
     *
     * @since 3.0.0
     */
    int kerning(std::uint32_t left, std::uint32_t right) const;

    //=========================================================================================================
    /**
     * @param[in] glyph   Glyph index.
     *
     * @return The contours of the glyph, composites resolved; empty for blank or damaged glyphs.
     *
     * @since 3.0.0
     */
    std::vector<Contour> outline(std::uint32_t glyph) const;

    //=========================================================================================================
    /**
     * The outline flattened to polygons fine enough to be drawn at @p pixels_per_em, cached per glyph and size.
     *
     * @param[in] glyph           Glyph index.
     * @param[in] pixels_per_em   Size it is drawn at, in device pixels.
     *
     * @return Closed polygons in font units, y up, with their original orientation (fill with non-zero winding).
     *
     * @since 3.0.0
     */
    std::shared_ptr<const std::vector<Polygon>> flattened_outline(std::uint32_t glyph, double pixels_per_em) const;

private:
    FontFace() = default;

    static Expected<std::shared_ptr<const FontFace>> parse(std::shared_ptr<FontFace> face, int face_index);

    void read_names();
    void read_gpos_kerning();
    void append_outline(std::uint32_t glyph, int depth, std::vector<Contour>& contours) const;
    int gpos_kerning(std::uint32_t left, std::uint32_t right) const;
    int kern_table_kerning(std::uint32_t left, std::uint32_t right) const;

    struct Table
    {
        std::size_t offset = 0;
        std::size_t length = 0;
    };

    std::vector<std::uint8_t> m_owned;
    std::span<const std::uint8_t> m_data;

    Table m_head;
    Table m_hhea;
    Table m_hmtx;
    Table m_loca;
    Table m_glyf;
    Table m_cmap;
    Table m_name;
    Table m_os2;
    Table m_kern;
    Table m_gpos;

    std::size_t m_cmapSubtable = 0;
    int m_cmapFormat = 0;
    bool m_longLoca = false;
    int m_hMetricCount = 0;
    std::vector<std::size_t> m_pairSubtables; // GPOS PairPos subtables, absolute offsets, grouped per lookup
    std::vector<std::size_t> m_pairLookupEnds;

    std::string m_family;
    std::string m_style;
    int m_weight = 400;
    bool m_italic = false;
    int m_unitsPerEm = 1000;
    int m_ascender = 0;
    int m_descender = 0;
    int m_lineGap = 0;
    int m_glyphCount = 0;

    mutable std::mutex m_cacheMutex;
    mutable std::unordered_map<std::uint64_t, std::shared_ptr<const std::vector<Polygon>>> m_outlines;
    mutable std::unordered_map<std::uint32_t, int> m_kerning;
};

//=============================================================================================================
/**
 * Decodes UTF-8; malformed sequences become U+FFFD.
 *
 * @param[in] text   UTF-8 text.
 *
 * @return Its code points.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT std::u32string decode_utf8(std::string_view text);

} // namespace mne::viz

#endif // VIZ_FONT_FACE_H
