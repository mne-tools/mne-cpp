//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     system_fonts.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SystemFonts finds the TrueType fonts installed on the system, without a font service.
 *
 * The platform font directories are scanned once, on first use; of every face only the table directory, the
 * name table and the OS/2 table are read. Faces are loaded when they are first drawn.
 */

#ifndef VIZ_SYSTEM_FONTS_H
#define VIZ_SYSTEM_FONTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "font_source.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief An installed font face as the index of SystemFonts lists it.
 *
 * @since 3.0.0
 */
struct SystemFontInfo
{
    std::filesystem::path path;                    /**< Font file. */
    int face_index = 0;                            /**< Face in a collection. */
    std::string family;                            /**< Family name, typographic family preferred. */
    std::string style;                             /**< Style name. */
    int weight = 400;                              /**< Weight from 1 to 1000. */
    bool italic = false;                           /**< Italic or oblique. */
    std::array<std::uint32_t, 4> unicode_ranges{}; /**< OS/2 ulUnicodeRange1 to 4; all zero if unknown. */
};

/**
 * @brief The TrueType fonts installed on the system, as a FontSource.
 *
 * Without an explicit family, face() takes the first installed family of the preferred families; a Font's
 * family is honoured when it is installed. Bold takes the lightest weight of at least 600 (SemiBold before
 * Bold), else the regular face.
 *
 * @snippet ex_scene/main.cpp system_fonts_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT SystemFonts : public FontSource
{
public:
    //=========================================================================================================
    /**
     * @param[in] directories          Directories scanned recursively for .ttf, .ttc and .otf files.
     * @param[in] preferred_families   Families tried in order for fonts without family.
     *
     * @since 3.0.0
     */
    explicit SystemFonts(std::vector<std::filesystem::path> directories, std::vector<std::string> preferred_families = default_families());

    //=========================================================================================================
    /**
     * @return The fonts of default_directories(), shared by the whole process.
     *
     * @since 3.0.0
     */
    static std::shared_ptr<const SystemFonts> shared();

    //=========================================================================================================
    /**
     * @return The font directories of the platform and of the user, then those listed in the environment
     *         variable MNE_FONT_PATH (separated by ':' or, on Windows, ';').
     *
     * @since 3.0.0
     */
    static std::vector<std::filesystem::path> default_directories();

    //=========================================================================================================
    /**
     * @return The user-interface families tried for fonts without family, Source Sans first.
     *
     * @since 3.0.0
     */
    static std::vector<std::string> default_families();

    //=========================================================================================================
    /**
     * @return Every installed TrueType face, sorted by path; scanned on the first call.
     *
     * @since 3.0.0
     */
    const std::vector<SystemFontInfo>& faces() const;

    //=========================================================================================================
    /**
     * @param[in] family   Family name, compared case-insensitively.
     * @param[in] bold     Whether to prefer a weight of at least 600.
     * @param[in] italic   Whether to prefer an italic face.
     *
     * @return The best face of the family, or nullptr if it is not installed.
     *
     * @since 3.0.0
     */
    std::shared_ptr<const FontFace> find(std::string_view family, bool bold, bool italic) const;

    std::shared_ptr<const FontFace> face(const Font& font) const override;
    std::shared_ptr<const FontFace> fallback_face(const Font& font, char32_t code_point) const override;

private:
    std::shared_ptr<const FontFace> load(const SystemFontInfo& info) const;

    std::vector<std::filesystem::path> m_directories;
    std::vector<std::string> m_preferredFamilies;

    mutable std::once_flag m_indexed;
    mutable std::vector<SystemFontInfo> m_faces;
    mutable std::mutex m_mutex;
    mutable std::map<std::pair<std::string, int>, std::shared_ptr<const FontFace>> m_loaded;
    mutable std::map<std::string, std::shared_ptr<const FontFace>> m_resolved;
    mutable std::map<char32_t, std::shared_ptr<const FontFace>> m_fallbacks;
};

} // namespace mne::viz

#endif // VIZ_SYSTEM_FONTS_H
