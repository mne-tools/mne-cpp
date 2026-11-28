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
 * @param[in] text         Text shape.
 * @param[in] width        Width of the text.
 * @param[in] ascent       Ascent of its font.
 * @param[in] descent      Descent of its font.
 * @param[in] cap_height   Height of its capitals; when known (positive), vertically centred text has its capitals
 *                         centred in the box, so faces with different ascents and descents line up.
 *
 * @return The start of the baseline that places the text in its box as QPainter::drawText does, but for the
 *         centring on the capitals.
 */
inline Point text_origin(const TextShape& text, double width, double ascent, double descent, double cap_height = 0.0)
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
        y = cap_height > 0.0 ? box.center().y + cap_height / 2.0 : box.y + (box.height - ascent - descent) / 2.0 + ascent;
    }
    return {x, y};
}

} // namespace mne::viz::detail

#endif // VIZ_DETAIL_TEXT_LAYOUT_H
