//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     svg_renderer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Writes a DisplayList as an SVG document.
 *
 * The output is deterministic: the same figure gives the same bytes on every platform (numbers are written
 * with at most three decimals), so SVG files serve as golden references in tests.
 */

#ifndef VIZ_SVG_RENDERER_H
#define VIZ_SVG_RENDERER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "display_list.h"
#include "../viz_global.h"
#include <mne/base/error.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class TextMetrics;

/**
 * Options of to_svg() and write_svg().
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @since 3.0.0
 */
struct SvgOptions
{
    std::string title;                      /**< Text of the title element, none if empty. */
    std::string font_family = "sans-serif"; /**< Family of fonts without one. */
    const TextMetrics* metrics = nullptr;   /**< Places text vertically; ApproximateTextMetrics if null. */
};

/**
 * Writes @p list as an SVG document.
 *
 * Images are embedded as PNG files and drawn opaque. Text is anchored at its start, middle or end, so it stays
 * aligned in whichever font the viewer uses.
 *
 * @snippet ex_scene/main.cpp scene_usage
 *
 * @param[in] list      Figure.
 * @param[in] options   Title, default font and metrics.
 *
 * @return The SVG document, UTF-8.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT std::string to_svg(const DisplayList& list, const SvgOptions& options = {});

/**
 * Writes @p list as an SVG file, see to_svg().
 *
 * @param[in] list      Figure.
 * @param[in] path      Output file.
 * @param[in] options   Title, default font and metrics.
 *
 * @return Nothing, or an Io error if the file cannot be written.
 *
 * @since 3.0.0
 */
[[nodiscard]] VIZSHARED_EXPORT Expected<void> write_svg(const DisplayList& list, const std::filesystem::path& path,
                                                        const SvgOptions& options = {});

} // namespace mne::viz

#endif // VIZ_SVG_RENDERER_H
