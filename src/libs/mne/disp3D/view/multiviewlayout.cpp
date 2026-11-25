//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     multiviewlayout.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    MultiViewLayout definition: converts the pane geometry of scene::PaneLayout to Qt types.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "multiviewlayout.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

namespace mne::disp
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

namespace
{

QRect toQRect(const scene::PixelRect& r)
{
    return {r.x, r.y, r.width, r.height};
}

scene::PixelRect toPixelRect(const QRect& r)
{
    return {r.x(), r.y(), r.width(), r.height()};
}

scene::PixelSize toPixelSize(const QSize& size)
{
    return {size.width(), size.height()};
}

scene::Point toPoint(const QPoint& pos)
{
    return {static_cast<double>(pos.x()), static_cast<double>(pos.y())};
}

} // namespace

//=============================================================================================================

QRect MultiViewLayout::slotRect(int slot, int numEnabled, const QSize& outputSize) const
{
    return toQRect(m_layout.slot_rect(slot, numEnabled, toPixelSize(outputSize)));
}

//=============================================================================================================

SplitterHit MultiViewLayout::hitTestSplitter(const QPoint& pos,
                                             int numEnabled,
                                             const QSize& outputSize) const
{
    return m_layout.hit_test_splitter(toPoint(pos), numEnabled, toPixelSize(outputSize));
}

//=============================================================================================================

Qt::CursorShape MultiViewLayout::cursorForHit(SplitterHit hit)
{
    switch (scene::PaneLayout::cursor_for_hit(hit)) {
        case scene::CursorShape::SizeHorizontal:
            return Qt::SizeHorCursor;
        case scene::CursorShape::SizeVertical:
            return Qt::SizeVerCursor;
        case scene::CursorShape::SizeAll:
            return Qt::SizeAllCursor;
        case scene::CursorShape::Default:
            break;
    }
    return Qt::ArrowCursor;
}

//=============================================================================================================

int MultiViewLayout::viewportIndexAt(const QPoint& pos,
                                     const std::vector<int>& enabledViewports,
                                     const QSize& outputSize) const
{
    return m_layout.viewport_index_at(toPoint(pos), enabledViewports, toPixelSize(outputSize));
}

//=============================================================================================================

QRect MultiViewLayout::insetForSeparator(const QRect& paneRect,
                                         int slot,
                                         int numEnabled) const
{
    if (numEnabled <= 1) {
        return paneRect;
    }
    return toQRect(m_layout.inset_for_separator(toPixelRect(paneRect), slot, numEnabled));
}

//=============================================================================================================

void MultiViewLayout::separatorGeometries(int numEnabled,
                                          const QSize& widgetSize,
                                          QRect& verticalRect,
                                          QRect& horizontalRect) const
{
    const scene::PaneLayout::Separators separators = m_layout.separator_geometries(numEnabled, toPixelSize(widgetSize));
    verticalRect = separators.vertical.empty() ? QRect() : toQRect(separators.vertical);
    horizontalRect = separators.horizontal.empty() ? QRect() : toQRect(separators.horizontal);
}

//=============================================================================================================

void MultiViewLayout::dragSplitter(const QPoint& pos,
                                   SplitterHit activeSplitter,
                                   const QSize& widgetSize)
{
    m_layout.drag_splitter(toPoint(pos), activeSplitter, toPixelSize(widgetSize));
}

} // namespace mne::disp
