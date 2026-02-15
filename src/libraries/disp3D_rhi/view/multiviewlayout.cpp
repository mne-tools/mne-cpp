//=============================================================================================================
/**
 * @file     multiviewlayout.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MultiViewLayout class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "multiviewlayout.h"

#include <algorithm>
#include <cmath>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QRect MultiViewLayout::slotRect(int slot, int numEnabled, const QSize &outputSize) const
{
    if (numEnabled <= 1) {
        return QRect(0, 0, outputSize.width(), outputSize.height());
    }

    const int w = outputSize.width();
    const int h = outputSize.height();

    // ── 2 panes: side-by-side ─────────────────────────────────────────
    if (numEnabled == 2) {
        const int leftW = std::clamp(static_cast<int>(std::lround(w * m_splitX)),
                                     m_minPanePx,
                                     std::max(m_minPanePx, w - m_minPanePx));
        const int rightW = std::max(1, w - leftW);
        return (slot == 0) ? QRect(0, 0, leftW, h)
                           : QRect(leftW, 0, rightW, h);
    }

    // ── 3 panes: full-width top, split bottom ─────────────────────────
    if (numEnabled == 3) {
        const int topH = std::clamp(static_cast<int>(std::lround(h * m_splitY)),
                                    m_minPanePx,
                                    std::max(m_minPanePx, h - m_minPanePx));
        const int bottomH = std::max(1, h - topH);
        if (slot == 0) {
            return QRect(0, 0, w, topH);
        }
        const int leftW = std::clamp(static_cast<int>(std::lround(w * m_splitX)),
                                     m_minPanePx,
                                     std::max(m_minPanePx, w - m_minPanePx));
        const int rightW = std::max(1, w - leftW);
        return (slot == 1) ? QRect(0, topH, leftW, bottomH)
                           : QRect(leftW, topH, rightW, bottomH);
    }

    // ── 4 panes: 2×2 grid ─────────────────────────────────────────────
    const int leftW = std::clamp(static_cast<int>(std::lround(w * m_splitX)),
                                 m_minPanePx,
                                 std::max(m_minPanePx, w - m_minPanePx));
    const int rightW = std::max(1, w - leftW);
    const int topH  = std::clamp(static_cast<int>(std::lround(h * m_splitY)),
                                 m_minPanePx,
                                 std::max(m_minPanePx, h - m_minPanePx));
    const int bottomH = std::max(1, h - topH);

    const int col = slot % 2;
    const int row = slot / 2;
    return QRect((col == 0) ? 0 : leftW,
                 (row == 0) ? 0 : topH,
                 (col == 0) ? leftW : rightW,
                 (row == 0) ? topH  : bottomH);
}

//=============================================================================================================

SplitterHit MultiViewLayout::hitTestSplitter(const QPoint &pos,
                                              int numEnabled,
                                              const QSize &outputSize) const
{
    if (numEnabled <= 1) {
        return SplitterHit::None;
    }

    const int w = outputSize.width();
    const int h = outputSize.height();

    // 2-pane: only vertical splitter
    if (numEnabled == 2) {
        const int splitX = std::clamp(static_cast<int>(std::lround(w * m_splitX)),
                                      m_minPanePx,
                                      std::max(m_minPanePx, w - m_minPanePx));
        return (std::abs(pos.x() - splitX) <= m_hitTolerancePx)
            ? SplitterHit::Vertical
            : SplitterHit::None;
    }

    // 3- or 4-pane
    const int splitX = std::clamp(static_cast<int>(std::lround(w * m_splitX)),
                                  m_minPanePx,
                                  std::max(m_minPanePx, w - m_minPanePx));
    const int splitY = std::clamp(static_cast<int>(std::lround(h * m_splitY)),
                                  m_minPanePx,
                                  std::max(m_minPanePx, h - m_minPanePx));

    const bool nearHorizontal = (std::abs(pos.y() - splitY) <= m_hitTolerancePx);

    // For 3-view: vertical only exists in bottom half
    const bool inBottomHalf = (pos.y() > splitY);
    const bool nearVertical = (std::abs(pos.x() - splitX) <= m_hitTolerancePx);
    const bool verticalActive = nearVertical && (numEnabled != 3 || inBottomHalf);

    if (verticalActive && nearHorizontal) return SplitterHit::Both;
    if (verticalActive)                   return SplitterHit::Vertical;
    if (nearHorizontal)                   return SplitterHit::Horizontal;

    return SplitterHit::None;
}

//=============================================================================================================

Qt::CursorShape MultiViewLayout::cursorForHit(SplitterHit hit)
{
    switch (hit) {
    case SplitterHit::Vertical:    return Qt::SizeHorCursor;
    case SplitterHit::Horizontal:  return Qt::SizeVerCursor;
    case SplitterHit::Both:        return Qt::SizeAllCursor;
    default:                       return Qt::ArrowCursor;
    }
}

//=============================================================================================================

int MultiViewLayout::viewportIndexAt(const QPoint &pos,
                                      const QVector<int> &enabledViewports,
                                      const QSize &outputSize) const
{
    const int numEnabled = enabledViewports.size();
    for (int slot = 0; slot < numEnabled; ++slot) {
        if (slotRect(slot, numEnabled, outputSize).contains(pos)) {
            return enabledViewports[slot];
        }
    }
    return -1;
}

//=============================================================================================================

QRect MultiViewLayout::insetForSeparator(const QRect &paneRect,
                                          int slot,
                                          int numEnabled) const
{
    if (numEnabled <= 1) {
        return paneRect;
    }

    QRect r = paneRect;

    if (numEnabled == 2) {
        if (slot == 0)
            r.setWidth(std::max(1, r.width() - m_separatorLinePx));
        return r;
    }

    if (numEnabled == 3) {
        if (slot == 0) {
            r.setHeight(std::max(1, r.height() - m_separatorLinePx));
        } else if (slot == 1) {
            r.setWidth(std::max(1, r.width() - m_separatorLinePx));
        }
        return r;
    }

    // 4 panes
    const int col = slot % 2;
    const int row = slot / 2;
    const bool hasRightNeighbor  = (col == 0) && (slot + 1 < numEnabled)
                                   && ((slot / 2) == ((slot + 1) / 2));
    const bool hasBottomNeighbor = (row == 0) && (slot + 2 < numEnabled);

    if (hasRightNeighbor)
        r.setWidth(std::max(1, r.width() - m_separatorLinePx));
    if (hasBottomNeighbor)
        r.setHeight(std::max(1, r.height() - m_separatorLinePx));

    return r;
}

//=============================================================================================================

void MultiViewLayout::separatorGeometries(int numEnabled,
                                           const QSize &widgetSize,
                                           QRect &verticalRect,
                                           QRect &horizontalRect) const
{
    verticalRect   = QRect();
    horizontalRect = QRect();

    if (numEnabled <= 1) return;

    const int w = std::max(1, widgetSize.width());
    const int h = std::max(1, widgetSize.height());

    const int splitX = std::clamp(static_cast<int>(std::lround(w * m_splitX)),
                                  m_minPanePx,
                                  std::max(m_minPanePx, w - m_minPanePx));

    if (numEnabled >= 3) {
        const int splitY = std::clamp(static_cast<int>(std::lround(h * m_splitY)),
                                      m_minPanePx,
                                      std::max(m_minPanePx, h - m_minPanePx));

        horizontalRect = QRect(0,
                               splitY - m_separatorLinePx / 2,
                               w,
                               m_separatorLinePx);

        if (numEnabled == 3) {
            // Vertical only in bottom half
            verticalRect = QRect(splitX - m_separatorLinePx / 2,
                                 splitY,
                                 m_separatorLinePx,
                                 h - splitY);
        } else {
            // Full-height vertical
            verticalRect = QRect(splitX - m_separatorLinePx / 2,
                                 0,
                                 m_separatorLinePx,
                                 h);
        }
    } else {
        // 2 panes: full-height vertical
        verticalRect = QRect(splitX - m_separatorLinePx / 2,
                             0,
                             m_separatorLinePx,
                             h);
    }
}

//=============================================================================================================

void MultiViewLayout::dragSplitter(const QPoint &pos,
                                    SplitterHit activeSplitter,
                                    const QSize &widgetSize)
{
    const int w = std::max(1, widgetSize.width());
    const int h = std::max(1, widgetSize.height());

    if (activeSplitter == SplitterHit::Vertical || activeSplitter == SplitterHit::Both) {
        const int clampedX = std::clamp(pos.x(), m_minPanePx, std::max(m_minPanePx, w - m_minPanePx));
        m_splitX = clampSplit(static_cast<float>(clampedX) / static_cast<float>(w));
    }

    if (activeSplitter == SplitterHit::Horizontal || activeSplitter == SplitterHit::Both) {
        const int clampedY = std::clamp(pos.y(), m_minPanePx, std::max(m_minPanePx, h - m_minPanePx));
        m_splitY = clampSplit(static_cast<float>(clampedY) / static_cast<float>(h));
    }
}
