//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     text_layout.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Baseline position of aligned text, shared by the SVG and raster renderers (internal).
 */

#ifndef VIZ_DETAIL_TEXT_LAYOUT_H
#define VIZ_DETAIL_TEXT_LAYOUT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../display_list.h"

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::detail
//=============================================================================================================

namespace mne::viz::detail
{

/**
 * @param[in] text      Text shape.
 * @param[in] width     Width of the text.
 * @param[in] ascent    Ascent of its font.
 * @param[in] descent   Descent of its font.
 *
 * @return The start of the baseline that places the text in its box as QPainter::drawText does.
 */
inline Point text_origin(const TextShape& text, double width, double ascent, double descent)
{
    const Rect& box = text.box;
    double x = box.x;
    if (has_flag(text.align, Align::Right)) {
        x = box.right() - width;
    } else if (has_flag(text.align, Align::HCenter)) {
        x = box.x + (box.width - width) / 2.0;
    }
    double y = box.y + ascent;
    if (has_flag(text.align, Align::Baseline)) {
        y = box.y;
    } else if (has_flag(text.align, Align::Bottom)) {
        y = box.bottom() - descent;
    } else if (has_flag(text.align, Align::VCenter)) {
        y = box.y + (box.height - ascent - descent) / 2.0 + ascent;
    }
    return {x, y};
}

} // namespace mne::viz::detail

#endif // VIZ_DETAIL_TEXT_LAYOUT_H
