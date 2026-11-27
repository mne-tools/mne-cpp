//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     system_fonts.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of SystemFonts.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "system_fonts.h"
#include "detail/font_names.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <set>
#include <system_error>
#include <tuple>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
namespace fs = std::filesystem;

namespace
{

constexpr std::size_t kMaxNameTable = 1 << 20;
constexpr int kMaxFallbackLoads = 32;

std::string lower(std::string_view text)
{
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(), [](char c) {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    });
    return out;
}

std::uint32_t be32(const std::vector<std::uint8_t>& b, std::size_t offset)
{
    if (offset + 4 > b.size()) {
        return 0;
    }
    return (static_cast<std::uint32_t>(b[offset]) << 24) | (static_cast<std::uint32_t>(b[offset + 1]) << 16) |
        (static_cast<std::uint32_t>(b[offset + 2]) << 8) | b[offset + 3];
}

std::uint16_t be16(const std::vector<std::uint8_t>& b, std::size_t offset)
{
    return offset + 2 <= b.size() ? static_cast<std::uint16_t>((b[offset] << 8) | b[offset + 1]) : 0;
}

std::vector<std::uint8_t> readAt(std::ifstream& file, std::uint64_t offset, std::size_t size)
{
    std::vector<std::uint8_t> bytes(size);
    file.clear();
    file.seekg(static_cast<std::streamoff>(offset));
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
    bytes.resize(file ? size : static_cast<std::size_t>(std::max<std::streamsize>(file.gcount(), 0)));
    return bytes;
}

// Appends the TrueType faces of a font file, reading only the table directory, name and OS/2
void indexFile(const fs::path& path, std::vector<SystemFontInfo>& out)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return;
    }
    const std::vector<std::uint8_t> header = readAt(file, 0, 12);
    std::vector<std::uint32_t> directories;
    if (be32(header, 0) == 0x74746366u) { // ttcf
        const std::uint32_t count = std::min<std::uint32_t>(be32(header, 8), 256);
        const std::vector<std::uint8_t> offsets = readAt(file, 12, 4 * static_cast<std::size_t>(count));
        for (std::size_t i = 0; i + 4 <= offsets.size(); i += 4) {
            directories.push_back(be32(offsets, i));
        }
    } else {
        directories.push_back(0);
    }
    for (std::size_t faceIndex = 0; faceIndex < directories.size(); ++faceIndex) {
        const std::vector<std::uint8_t> directory = readAt(file, directories[faceIndex], 12);
        const std::uint32_t version = be32(directory, 0);
        if (version != 0x00010000u && version != 0x74727565u) { // TrueType or 'true'; CFF fonts are skipped
            continue;
        }
        const std::size_t tableCount = be16(directory, 4);
        const std::vector<std::uint8_t> records = readAt(file, directories[faceIndex] + 12ull, 16 * tableCount);
        std::uint32_t nameOffset = 0;
        std::uint32_t nameLength = 0;
        std::uint32_t os2Offset = 0;
        std::uint32_t os2Length = 0;
        bool glyf = false;
        for (std::size_t r = 0; r + 16 <= records.size(); r += 16) {
            const std::uint32_t tableTag = be32(records, r);
            if (tableTag == 0x6E616D65u) { // name
                nameOffset = be32(records, r + 8);
                nameLength = be32(records, r + 12);
            } else if (tableTag == 0x4F532F32u) { // OS/2
                os2Offset = be32(records, r + 8);
                os2Length = be32(records, r + 12);
            } else if (tableTag == 0x676C7966u && be32(records, r + 12) > 0) { // glyf
                glyf = true;
            }
        }
        if (!glyf || nameLength == 0) {
            continue;
        }
        SystemFontInfo info;
        info.path = path;
        info.face_index = static_cast<int>(faceIndex);
        const std::vector<std::uint8_t> name = readAt(file, nameOffset, std::min<std::size_t>(nameLength, kMaxNameTable));
        std::tie(info.family, info.style) = detail::font_names(name);
        if (info.family.empty()) {
            continue;
        }
        if (os2Length >= 64) {
            const std::vector<std::uint8_t> os2 = readAt(file, os2Offset, 64);
            if (os2.size() == 64) {
                info.weight = std::clamp<int>(be16(os2, 4), 1, 1000);
                info.italic = (be16(os2, 62) & 0x1) != 0;
                for (std::size_t k = 0; k < 4; ++k) {
                    info.unicode_ranges[k] = be32(os2, 42 + 4 * k);
                }
            }
        } else {
            const std::string style = lower(info.style);
            info.weight = style.find("bold") != std::string::npos ? 700 : 400;
            info.italic = style.find("italic") != std::string::npos || style.find("oblique") != std::string::npos;
        }
        out.push_back(std::move(info));
    }
}

// The OS/2 Unicode range bit of a code point, -1 if not in the table
int unicodeRangeBit(char32_t c)
{
    struct Range
    {
        char32_t first;
        char32_t last;
        int bit;
    };
    static constexpr Range kRanges[] = {
        {0x0000, 0x007F, 0},
        {0x0080, 0x00FF, 1},
        {0x0100, 0x017F, 2},
        {0x0180, 0x024F, 3},
        {0x0250, 0x02AF, 4},
        {0x02B0, 0x02FF, 5},
        {0x0300, 0x036F, 6},
        {0x0370, 0x03FF, 7},
        {0x0400, 0x052F, 9},
        {0x0530, 0x058F, 10},
        {0x0590, 0x05FF, 11},
        {0x0600, 0x06FF, 13},
        {0x0900, 0x097F, 15},
        {0x0E00, 0x0E7F, 24},
        {0x10A0, 0x10FF, 26},
        {0x1100, 0x11FF, 28},
        {0x1E00, 0x1EFF, 29},
        {0x1F00, 0x1FFF, 30},
        {0x2000, 0x206F, 31},
        {0x2070, 0x209F, 32},
        {0x20A0, 0x20CF, 33},
        {0x2100, 0x214F, 35},
        {0x2150, 0x218F, 36},
        {0x2190, 0x21FF, 37},
        {0x2200, 0x22FF, 38},
        {0x2300, 0x23FF, 39},
        {0x2500, 0x257F, 43},
        {0x2580, 0x259F, 44},
        {0x25A0, 0x25FF, 45},
        {0x2600, 0x26FF, 46},
        {0x2700, 0x27BF, 47},
        {0x3000, 0x303F, 48},
        {0x3040, 0x309F, 49},
        {0x30A0, 0x30FF, 50},
        {0x3100, 0x312F, 51},
        {0x3400, 0x4DBF, 59},
        {0x4E00, 0x9FFF, 59},
        {0xAC00, 0xD7AF, 56},
        {0xFF00, 0xFFEF, 68},
    };
    if (c >= 0x10000) {
        return 57;
    }
    for (const Range& range : kRanges) {
        if (c >= range.first && c <= range.last) {
            return range.bit;
        }
    }
    return -1;
}

bool hasRangeBit(const SystemFontInfo& info, int bit)
{
    return ((info.unicode_ranges[static_cast<std::size_t>(bit / 32)] >> (bit % 32)) & 1u) != 0;
}

bool rangesUnknown(const SystemFontInfo& info)
{
    return std::all_of(info.unicode_ranges.begin(), info.unicode_ranges.end(), [](std::uint32_t r) { return r == 0; });
}

// Lower is better: italic as requested, then the weight
int matchScore(const SystemFontInfo& info, bool bold, bool italic, bool hasHeavy)
{
    int score = info.italic == italic ? 0 : 10000;
    if (bold && hasHeavy) {
        score += info.weight >= 600 ? info.weight - 600 : 5000;
    } else {
        score += 2 * std::abs(info.weight - 400) + (info.weight < 400 ? 1 : 0);
    }
    return score;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SystemFonts::SystemFonts(std::vector<fs::path> directories, std::vector<std::string> preferred_families)
: m_directories(std::move(directories))
, m_preferredFamilies(std::move(preferred_families))
{
}

//=============================================================================================================

std::shared_ptr<const SystemFonts> SystemFonts::shared()
{
    static const std::shared_ptr<const SystemFonts> instance = std::make_shared<SystemFonts>(default_directories());
    return instance;
}

//=============================================================================================================

std::vector<fs::path> SystemFonts::default_directories()
{
    std::vector<fs::path> directories;
    const auto env = [](const char* name) -> std::string {
        const char* value = std::getenv(name); // NOLINT(concurrency-mt-unsafe): read-only
        return value ? std::string(value) : std::string();
    };
#if defined(_WIN32)
    if (const std::string windir = env("WINDIR"); !windir.empty()) {
        directories.push_back(fs::path(windir) / "Fonts");
    }
    if (const std::string local = env("LOCALAPPDATA"); !local.empty()) {
        directories.push_back(fs::path(local) / "Microsoft" / "Windows" / "Fonts");
    }
    constexpr char kSeparator = ';';
#elif defined(__APPLE__)
    directories.push_back("/System/Library/Fonts");
    directories.push_back("/Library/Fonts");
    if (const std::string home = env("HOME"); !home.empty()) {
        directories.push_back(fs::path(home) / "Library" / "Fonts");
    }
    constexpr char kSeparator = ':';
#else
    directories.push_back("/usr/share/fonts");
    directories.push_back("/usr/local/share/fonts");
    if (const std::string home = env("HOME"); !home.empty()) {
        directories.push_back(fs::path(home) / ".local" / "share" / "fonts");
        directories.push_back(fs::path(home) / ".fonts");
    }
    constexpr char kSeparator = ':';
#endif
    const std::string extra = env("MNE_FONT_PATH");
    std::size_t start = 0;
    while (start <= extra.size()) {
        const std::size_t end = std::min(extra.find(kSeparator, start), extra.size());
        if (end > start) {
            directories.emplace_back(extra.substr(start, end - start));
        }
        start = end + 1;
    }
    return directories;
}

//=============================================================================================================

std::vector<std::string> SystemFonts::default_families()
{
    return {"Source Sans 3", "Source Sans Pro", "Helvetica Neue", "Segoe UI", "Inter", "Noto Sans", "DejaVu Sans", "Liberation Sans", "Arial"};
}

//=============================================================================================================

const std::vector<SystemFontInfo>& SystemFonts::faces() const
{
    std::call_once(m_indexed, [this] {
        std::set<fs::path> files;
        for (const fs::path& directory : m_directories) {
            std::error_code error;
            fs::recursive_directory_iterator it(directory, fs::directory_options::skip_permission_denied, error);
            for (; !error && it != fs::recursive_directory_iterator(); it.increment(error)) {
                std::error_code statError;
                if (!it->is_regular_file(statError)) {
                    continue;
                }
                const std::string extension = lower(it->path().extension().string());
                if (extension == ".ttf" || extension == ".ttc" || extension == ".otf") {
                    std::error_code canonicalError;
                    const fs::path canonical = fs::weakly_canonical(it->path(), canonicalError);
                    files.insert(canonicalError ? it->path() : canonical);
                }
            }
        }
        for (const fs::path& file : files) {
            indexFile(file, m_faces);
        }
    });
    return m_faces;
}

//=============================================================================================================

std::shared_ptr<const FontFace> SystemFonts::load(const SystemFontInfo& info) const
{
    // Called with m_mutex held
    const std::pair key{info.path.string(), info.face_index};
    if (const auto it = m_loaded.find(key); it != m_loaded.end()) {
        return it->second;
    }
    auto face = FontFace::load(info.path, info.face_index);
    std::shared_ptr<const FontFace> loaded = face ? *face : nullptr;
    m_loaded.emplace(key, loaded);
    return loaded;
}

//=============================================================================================================

std::shared_ptr<const FontFace> SystemFonts::find(std::string_view family, bool bold, bool italic) const
{
    const std::string wanted = lower(family);
    std::vector<const SystemFontInfo*> candidates;
    for (const SystemFontInfo& info : faces()) {
        if (lower(info.family) == wanted) {
            candidates.push_back(&info);
        }
    }
    const bool hasHeavy = std::any_of(candidates.begin(), candidates.end(), [](const SystemFontInfo* info) { return info->weight >= 600; });
    std::stable_sort(candidates.begin(), candidates.end(), [&](const SystemFontInfo* a, const SystemFontInfo* b) {
        return matchScore(*a, bold, italic, hasHeavy) < matchScore(*b, bold, italic, hasHeavy);
    });
    const std::lock_guard lock(m_mutex);
    for (const SystemFontInfo* info : candidates) {
        if (auto face = load(*info)) {
            return face;
        }
    }
    return nullptr;
}

//=============================================================================================================

std::shared_ptr<const FontFace> SystemFonts::face(const Font& font) const
{
    const std::string key = lower(font.family) + (font.bold ? "|b" : "|") + (font.italic ? "i" : "");
    {
        const std::lock_guard lock(m_mutex);
        if (const auto it = m_resolved.find(key); it != m_resolved.end()) {
            return it->second;
        }
    }
    std::shared_ptr<const FontFace> resolved;
    if (!font.family.empty()) {
        resolved = find(font.family, font.bold, font.italic);
    }
    for (std::size_t i = 0; !resolved && i < m_preferredFamilies.size(); ++i) {
        resolved = find(m_preferredFamilies[i], font.bold, font.italic);
    }
    if (!resolved) {
        // Any regular face with Latin letters
        std::vector<const SystemFontInfo*> latin;
        for (const SystemFontInfo& info : faces()) {
            if (hasRangeBit(info, 0) && !info.italic) {
                latin.push_back(&info);
            }
        }
        std::stable_sort(latin.begin(), latin.end(), [](const SystemFontInfo* a, const SystemFontInfo* b) {
            return std::abs(a->weight - 400) < std::abs(b->weight - 400);
        });
        const std::lock_guard lock(m_mutex);
        for (std::size_t i = 0; !resolved && i < latin.size(); ++i) {
            if (auto candidate = load(*latin[i]); candidate && candidate->glyph_index(U'A') != 0) {
                resolved = std::move(candidate);
            }
        }
    }
    const std::lock_guard lock(m_mutex);
    m_resolved.emplace(key, resolved);
    return resolved;
}

//=============================================================================================================

std::shared_ptr<const FontFace> SystemFonts::fallback_face(const Font& /*font*/, char32_t code_point) const
{
    {
        const std::lock_guard lock(m_mutex);
        if (const auto it = m_fallbacks.find(code_point); it != m_fallbacks.end()) {
            return it->second;
        }
    }
    // One regular face per family, preferred families first, that claims the code point's Unicode range
    const int bit = unicodeRangeBit(code_point);
    std::vector<const SystemFontInfo*> candidates;
    for (const SystemFontInfo& info : faces()) {
        if (info.italic || lower(info.family) == "lastresort" || (bit >= 0 && !hasRangeBit(info, bit) && !rangesUnknown(info))) {
            continue;
        }
        const auto same = std::find_if(candidates.begin(), candidates.end(), [&](const SystemFontInfo* c) { return c->family == info.family; });
        if (same == candidates.end()) {
            candidates.push_back(&info);
        } else if (std::abs(info.weight - 400) < std::abs((*same)->weight - 400)) {
            *same = &info;
        }
    }
    const auto rank = [&](const SystemFontInfo* info) {
        const std::string family = lower(info->family);
        for (std::size_t i = 0; i < m_preferredFamilies.size(); ++i) {
            if (lower(m_preferredFamilies[i]) == family) {
                return static_cast<int>(i);
            }
        }
        return static_cast<int>(m_preferredFamilies.size()) + (rangesUnknown(*info) ? 1 : 0);
    };
    std::stable_sort(candidates.begin(), candidates.end(), [&](const SystemFontInfo* a, const SystemFontInfo* b) { return rank(a) < rank(b); });
    const std::lock_guard lock(m_mutex);
    std::shared_ptr<const FontFace> found;
    int loads = 0;
    for (const SystemFontInfo* info : candidates) {
        if (loads++ >= kMaxFallbackLoads) {
            break;
        }
        if (auto face = load(*info); face && face->glyph_index(code_point) != 0) {
            found = std::move(face);
            break;
        }
    }
    m_fallbacks.emplace(code_point, found);
    return found;
}
