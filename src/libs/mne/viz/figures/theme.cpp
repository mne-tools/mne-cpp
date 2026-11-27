//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     theme.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the Theme.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "theme.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Theme Theme::light()
{
    return Theme{};
}

//=============================================================================================================

Theme Theme::dark()
{
    Theme theme;
    theme.background = Color{21, 24, 29};
    theme.surface = Color{30, 34, 40};
    theme.field = Color{24, 27, 33};
    theme.field_border = Color{66, 72, 82};
    theme.text = Color{230, 233, 237};
    theme.muted_text = Color{148, 156, 168};
    theme.accent = Color{88, 150, 255};
    theme.accent_text = Color{12, 18, 28};
    theme.border = Color{48, 53, 61};
    theme.separator = Color{40, 45, 52};
    theme.positive = Color{92, 196, 136};
    theme.warning = Color{232, 168, 76};
    theme.negative = Color{240, 113, 103};
    return theme;
}

//=============================================================================================================

Font Theme::font(double pixel_size, bool bold) const
{
    Font result;
    result.family = font_family;
    result.pixel_size = pixel_size;
    result.bold = bold;
    return result;
}

//=============================================================================================================

Font Theme::mono_font(double pixel_size) const
{
    Font result;
    result.family = mono_font_family.empty() ? std::string("monospace") : mono_font_family;
    result.pixel_size = pixel_size;
    return result;
}
