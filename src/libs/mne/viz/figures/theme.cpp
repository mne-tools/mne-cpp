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
    theme.background = Color{24, 26, 30};
    theme.surface = Color{34, 37, 43};
    theme.field = Color{44, 48, 55};
    theme.text = Color{231, 233, 236};
    theme.muted_text = Color{150, 156, 166};
    theme.accent = Color{92, 142, 245};
    theme.accent_text = Color{255, 255, 255};
    theme.border = Color{58, 63, 71};
    theme.positive = Color{86, 196, 128};
    theme.warning = Color{232, 172, 72};
    theme.negative = Color{240, 112, 102};
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
