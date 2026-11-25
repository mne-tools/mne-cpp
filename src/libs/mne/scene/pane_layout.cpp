//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     pane_layout.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of PaneLayout.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pane_layout.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

int PaneLayout::split_pos(float fraction, int extent) const
{
    return std::clamp(static_cast<int>(std::lround(static_cast<float>(extent) * fraction)), m_min_pane_px,
                      std::max(m_min_pane_px, extent - m_min_pane_px));
}

//=============================================================================================================

PixelRect PaneLayout::slot_rect(int slot, int pane_count, PixelSize size) const
{
    const int w = size.width;
    const int h = size.height;
    if (pane_count <= 1) {
        return {0, 0, w, h};
    }

    const int left_w = split_pos(m_split_x, w);
    const int right_w = std::max(1, w - left_w);

    if (pane_count == 2) {
        return (slot == 0) ? PixelRect{0, 0, left_w, h} : PixelRect{left_w, 0, right_w, h};
    }

    const int top_h = split_pos(m_split_y, h);
    const int bottom_h = std::max(1, h - top_h);

    if (pane_count == 3) {
        if (slot == 0) {
            return {0, 0, w, top_h};
        }
        return (slot == 1) ? PixelRect{0, top_h, left_w, bottom_h} : PixelRect{left_w, top_h, right_w, bottom_h};
    }

    const int col = slot % 2;
    const int row = slot / 2;
    return {(col == 0) ? 0 : left_w, (row == 0) ? 0 : top_h, (col == 0) ? left_w : right_w, (row == 0) ? top_h : bottom_h};
}

//=============================================================================================================

SplitterHit PaneLayout::hit_test_splitter(Point pos, int pane_count, PixelSize size) const
{
    if (pane_count <= 1) {
        return SplitterHit::None;
    }

    const int split_x = split_pos(m_split_x, size.width);
    const bool near_vertical = std::abs(pos.x - split_x) <= m_hit_tolerance_px;
    if (pane_count == 2) {
        return near_vertical ? SplitterHit::Vertical : SplitterHit::None;
    }

    const int split_y = split_pos(m_split_y, size.height);
    const bool near_horizontal = std::abs(pos.y - split_y) <= m_hit_tolerance_px;
    // With three panes the vertical splitter only divides the bottom row
    const bool vertical_active = near_vertical && (pane_count != 3 || pos.y > split_y);

    if (vertical_active && near_horizontal) {
        return SplitterHit::Both;
    }
    if (vertical_active) {
        return SplitterHit::Vertical;
    }
    if (near_horizontal) {
        return SplitterHit::Horizontal;
    }
    return SplitterHit::None;
}

//=============================================================================================================

CursorShape PaneLayout::cursor_for_hit(SplitterHit hit)
{
    switch (hit) {
        case SplitterHit::Vertical:
            return CursorShape::SizeHorizontal;
        case SplitterHit::Horizontal:
            return CursorShape::SizeVertical;
        case SplitterHit::Both:
            return CursorShape::SizeAll;
        case SplitterHit::None:
            break;
    }
    return CursorShape::Default;
}

//=============================================================================================================

int PaneLayout::viewport_index_at(Point pos, std::span<const int> viewports, PixelSize size) const
{
    const int pane_count = static_cast<int>(viewports.size());
    for (int slot = 0; slot < pane_count; ++slot) {
        if (slot_rect(slot, pane_count, size).contains(pos)) {
            return viewports[static_cast<std::size_t>(slot)];
        }
    }
    return -1;
}

//=============================================================================================================

PixelRect PaneLayout::inset_for_separator(const PixelRect& pane, int slot, int pane_count) const
{
    PixelRect r = pane;
    bool right_neighbour = false;
    bool bottom_neighbour = false;
    if (pane_count == 2) {
        right_neighbour = slot == 0;
    } else if (pane_count == 3) {
        bottom_neighbour = slot == 0;
        right_neighbour = slot == 1;
    } else if (pane_count >= 4) {
        right_neighbour = (slot % 2 == 0) && (slot + 1 < pane_count);
        bottom_neighbour = (slot / 2 == 0) && (slot + 2 < pane_count);
    }
    if (right_neighbour) {
        r.width = std::max(1, r.width - m_separator_line_px);
    }
    if (bottom_neighbour) {
        r.height = std::max(1, r.height - m_separator_line_px);
    }
    return r;
}

//=============================================================================================================

PaneLayout::Separators PaneLayout::separator_geometries(int pane_count, PixelSize size) const
{
    Separators separators;
    if (pane_count <= 1) {
        return separators;
    }

    const int w = std::max(1, size.width);
    const int h = std::max(1, size.height);
    const int split_x = split_pos(m_split_x, w);
    const int half_line = m_separator_line_px / 2;

    if (pane_count == 2) {
        separators.vertical = {split_x - half_line, 0, m_separator_line_px, h};
        return separators;
    }

    const int split_y = split_pos(m_split_y, h);
    separators.horizontal = {0, split_y - half_line, w, m_separator_line_px};
    // With three panes the vertical separator divides the bottom row only
    separators.vertical = (pane_count == 3) ? PixelRect{split_x - half_line, split_y, m_separator_line_px, h - split_y}
                                            : PixelRect{split_x - half_line, 0, m_separator_line_px, h};
    return separators;
}

//=============================================================================================================

void PaneLayout::drag_splitter(Point pos, SplitterHit splitter, PixelSize size)
{
    const int w = std::max(1, size.width);
    const int h = std::max(1, size.height);

    if (splitter == SplitterHit::Vertical || splitter == SplitterHit::Both) {
        const int x = std::clamp(static_cast<int>(std::lround(pos.x)), m_min_pane_px, std::max(m_min_pane_px, w - m_min_pane_px));
        m_split_x = clamp_split(static_cast<float>(x) / static_cast<float>(w));
    }
    if (splitter == SplitterHit::Horizontal || splitter == SplitterHit::Both) {
        const int y = std::clamp(static_cast<int>(std::lround(pos.y)), m_min_pane_px, std::max(m_min_pane_px, h - m_min_pane_px));
        m_split_y = clamp_split(static_cast<float>(y) / static_cast<float>(h));
    }
}

} // namespace mne::scene
