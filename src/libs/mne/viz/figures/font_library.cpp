//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     font_library.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of FontLibrary.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "font_library.h"
#include "detail/bundled_fonts.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

namespace
{

std::shared_ptr<const FontFace> bundled(const unsigned char* data, std::size_t size)
{
    auto face = FontFace::from_static_bytes(std::span<const std::uint8_t>(data, size));
    return face ? *face : nullptr;
}

std::string normalized(std::string_view family)
{
    std::string out;
    for (const char c : family) {
        if (c != ' ' || (!out.empty() && out.back() != ' ')) {
            out += (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
        }
    }
    while (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }
    return out;
}

bool isOneOf(std::string_view family, std::initializer_list<std::string_view> names)
{
    return std::find(names.begin(), names.end(), family) != names.end();
}

bool isMonoName(std::string_view family)
{
    return isOneOf(family, {"monospace", "mono", "ui-monospace", "source code pro"}) || family.find("mono") != std::string_view::npos ||
        family.find("code") != std::string_view::npos || family.find("courier") != std::string_view::npos ||
        family.find("consol") != std::string_view::npos;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FontLibrary::FontLibrary(std::shared_ptr<const SystemFonts> system)
: m_system(std::move(system))
{
}

//=============================================================================================================

std::shared_ptr<const FontLibrary> FontLibrary::shared()
{
    static const std::shared_ptr<const FontLibrary> instance = std::make_shared<FontLibrary>(SystemFonts::shared());
    return instance;
}

//=============================================================================================================

std::shared_ptr<const FontFace> FontLibrary::sans(bool semibold)
{
    static const std::shared_ptr<const FontFace> regular = bundled(detail::kSourceSans3Regular, detail::kSourceSans3RegularSize);
    static const std::shared_ptr<const FontFace> heavy = bundled(detail::kSourceSans3Semibold, detail::kSourceSans3SemiboldSize);
    return semibold ? heavy : regular;
}

//=============================================================================================================

std::vector<std::span<const unsigned char>> FontLibrary::bundled_files()
{
    return {std::span(detail::kSourceSans3Regular, detail::kSourceSans3RegularSize), std::span(detail::kSourceSans3Semibold, detail::kSourceSans3SemiboldSize),
            std::span(detail::kSourceCodeProRegular, detail::kSourceCodeProRegularSize)};
}

//=============================================================================================================

std::shared_ptr<const FontFace> FontLibrary::mono()
{
    static const std::shared_ptr<const FontFace> face = bundled(detail::kSourceCodeProRegular, detail::kSourceCodeProRegularSize);
    return face;
}

//=============================================================================================================

std::shared_ptr<const FontFace> FontLibrary::face(const Font& font) const
{
    const std::string family = normalized(font.family);
    if (family.empty() || isOneOf(family, {"source sans 3", "source sans pro", "sans", "sans-serif", "sans serif", "system-ui", "ui-sans-serif"})) {
        return sans(font.bold);
    }
    if (isOneOf(family, {"monospace", "mono", "ui-monospace", "source code pro"})) {
        return mono();
    }
    if (m_system) {
        if (auto installed = m_system->find(font.family, font.bold, font.italic)) {
            return installed;
        }
    }
    return isMonoName(family) ? mono() : sans(font.bold);
}

//=============================================================================================================

std::shared_ptr<const FontFace> FontLibrary::fallback_face(const Font& font, char32_t code_point) const
{
    for (const std::shared_ptr<const FontFace>& candidate : {sans(font.bold), mono()}) {
        if (candidate && candidate->glyph_index(code_point) != 0) {
            return candidate;
        }
    }
    return m_system ? m_system->fallback_face(font, code_point) : nullptr;
}
