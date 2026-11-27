//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     font_names.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Reads the family and style names of a TrueType name table, for FontFace and SystemFonts.
 */

#ifndef VIZ_DETAIL_FONT_NAMES_H
#define VIZ_DETAIL_FONT_NAMES_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <span>
#include <string>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::detail
//=============================================================================================================

namespace mne::viz::detail
{

/**
 * @param[in] table   The bytes of a name table.
 *
 * @return The family and style, typographic names (ids 16 and 17) before the legacy ones (1 and 2), in UTF-8.
 */
std::pair<std::string, std::string> font_names(std::span<const std::uint8_t> table);

} // namespace mne::viz::detail

#endif // VIZ_DETAIL_FONT_NAMES_H
