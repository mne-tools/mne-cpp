//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     multiviewlayout.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    MultiViewLayout definition: converts the pane geometry of viz::PaneLayout to Qt types.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "multiviewlayout.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

namespace mne::viz::qt
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

namespace
{

QRect toQRect(const viz::PixelRect& r)
{
    return {r.x, r.y, r.width, r.height};
}

mne::viz::PixelRect toPixelRect(const QRect& r)
{
    return {r.x(), r.y(), r.width(), r.height()};
}

mne::viz::PixelSize toPixelSize(const QSize& size)
{
    return {size.width(), size.height()};
}

mne::viz::Point toPoint(const QPoint& pos)
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
    switch (viz::PaneLayout::cursor_for_hit(hit)) {
        case viz::CursorShape::SizeHorizontal:
            return Qt::SizeHorCursor;
        case viz::CursorShape::SizeVertical:
            return Qt::SizeVerCursor;
        case viz::CursorShape::SizeAll:
            return Qt::SizeAllCursor;
        case viz::CursorShape::Default:
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
    const viz::PaneLayout::Separators separators = m_layout.separator_geometries(numEnabled, toPixelSize(widgetSize));
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

} // namespace mne::viz::qt
