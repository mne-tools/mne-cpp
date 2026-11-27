//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     font_library.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FontLibrary: the fonts compiled into mne_viz, with the installed fonts as fallback.
 *
 * Source Sans 3 (Regular and Semibold) and Source Code Pro (Regular), under the SIL Open Font License 1.1, are
 * compiled into the library unmodified, so text looks and measures the same on every platform.
 */

#ifndef VIZ_FONT_LIBRARY_H
#define VIZ_FONT_LIBRARY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "font_source.h"
#include "system_fonts.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <span>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief The bundled fonts as a FontSource, with installed fonts for other families and missing characters.
 *
 * A Font without family, or asking for "Source Sans 3", "sans-serif" or another generic sans family, is drawn
 * in the bundled Source Sans 3, bold in its Semibold; "monospace" and "Source Code Pro" in the bundled Source
 * Code Pro. Another family is taken from the system fonts when it is installed, else the bundled family that
 * fits its name. A character the face lacks is taken from the other bundled faces, then from any installed
 * face that has it, else the missing-glyph box is drawn.
 *
 * @snippet ex_scene/main.cpp font_library_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT FontLibrary : public FontSource
{
public:
    //=========================================================================================================
    /**
     * @param[in] system   Installed fonts for other families and missing characters; nullptr for the bundled
     *                     fonts alone, which renders identically everywhere.
     *
     * @since 3.0.0
     */
    explicit FontLibrary(std::shared_ptr<const SystemFonts> system = nullptr);

    //=========================================================================================================
    /**
     * @return The bundled fonts with SystemFonts::shared() as fallback, shared by the whole process.
     *
     * @since 3.0.0
     */
    static std::shared_ptr<const FontLibrary> shared();

    //=========================================================================================================
    /**
     * @param[in] semibold   Whether to return the Semibold face.
     *
     * @return The bundled Source Sans 3 Regular or Semibold.
     *
     * @since 3.0.0
     */
    static std::shared_ptr<const FontFace> sans(bool semibold = false);

    //=========================================================================================================
    /**
     * @return The bundled Source Code Pro Regular.
     *
     * @since 3.0.0
     */
    static std::shared_ptr<const FontFace> mono();

    //=========================================================================================================
    /**
     * The files of the bundled fonts, e.g. for a toolkit that renders text itself (Qt registers them so its
     * widgets use the same typefaces).
     *
     * @return The unmodified .ttf files: Source Sans 3 Regular and Semibold, Source Code Pro Regular.
     *
     * @since 3.0.0
     */
    static std::vector<std::span<const unsigned char>> bundled_files();

    std::shared_ptr<const FontFace> face(const Font& font) const override;
    std::shared_ptr<const FontFace> fallback_face(const Font& font, char32_t code_point) const override;

private:
    std::shared_ptr<const SystemFonts> m_system;
};

} // namespace mne::viz

#endif // VIZ_FONT_LIBRARY_H
