//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bundled_fonts.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    The font files compiled into mne_viz (resources/fonts, SIL Open Font License 1.1, unmodified).
 *
 * The arrays are generated at build time by cmake/MneEmbedFile.cmake.
 */

#ifndef VIZ_DETAIL_BUNDLED_FONTS_H
#define VIZ_DETAIL_BUNDLED_FONTS_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::detail
//=============================================================================================================

namespace mne::viz::detail
{

extern const unsigned char kSourceSans3Regular[];   /**< SourceSans3-Regular.ttf. */
extern const std::size_t kSourceSans3RegularSize;   /**< Its size in bytes. */
extern const unsigned char kSourceSans3Semibold[];  /**< SourceSans3-Semibold.ttf. */
extern const std::size_t kSourceSans3SemiboldSize;  /**< Its size in bytes. */
extern const unsigned char kSourceCodeProRegular[]; /**< SourceCodePro-Regular.ttf. */
extern const std::size_t kSourceCodeProRegularSize; /**< Its size in bytes. */

} // namespace mne::viz::detail

#endif // VIZ_DETAIL_BUNDLED_FONTS_H
