//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file multiviewlayout.h
 * @since March 2026
 * @brief Viewport geometry, splitter logic and per-pane preset assignment for the disp3D multi-view grid.
 *
 * MultiViewLayout computes the four sub-viewports (Top-left
 * perspective, Top-right orthographic top, Bottom-left
 * orthographic left, Bottom-right orthographic front) inside the
 * owning QRhiWidget, honours user-dragged splitter ratios, and
 * publishes the per-pane @ref SubView so the camera and renderer
 * can set viewport / scissor and pick the right matrix preset.
 */

#ifndef MULTIVIEWLAYOUT_H
#define MULTIVIEWLAYOUT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <QRect>
#include <QSize>
#include <QPoint>
#include <QVector>
#include <Qt>

//=============================================================================================================
/**
 * Identifies which splitter(s) are hit by a cursor position.
 */
enum class SplitterHit {
    None,
    Vertical,
    Horizontal,
    Both
};

//=============================================================================================================
/**
 * Pure-geometry class for multi-view viewport layout.
 *
 * This class has no widget dependencies — it works with sizes, rectangles,
 * and fractional split positions. The hosting widget queries this class
 * for rectangles and hit-test results, and updates separator widgets
 * accordingly.
 *
 * Layout rules:
 *   - 1 pane  → full area
 *   - 2 panes → side-by-side, split at m_splitX
 *   - 3 panes → full-width top row + 2 bottom panes split at m_splitX
 *   - 4 panes → 2×2 grid split at (m_splitX, m_splitY)
 *
 * @brief    Multi-view geometry computations.
 */
class DISP3DSHARED_EXPORT MultiViewLayout
{
public:
    //=========================================================================================================
    /**
     * Constructor.
     */
    MultiViewLayout() = default;

    // ── Split fractions ────────────────────────────────────────────────

    float splitX() const              { return m_splitX; }
    float splitY() const              { return m_splitY; }
    void  setSplitX(float x)          { m_splitX = clampSplit(x); }
    void  setSplitY(float y)          { m_splitY = clampSplit(y); }
    void  resetSplits()               { m_splitX = 0.5f; m_splitY = 0.5f; }

    // ── Tolerance / sizing constants ───────────────────────────────────

    int  hitTolerancePx()   const     { return m_hitTolerancePx; }
    int  minPanePx()        const     { return m_minPanePx; }
    int  separatorLinePx()  const     { return m_separatorLinePx; }

    //=========================================================================================================
    /**
     * Compute the pixel rectangle for a layout slot.
     *
     * @param[in] slot          Slot index (0-based among enabled viewports).
     * @param[in] numEnabled    Total number of enabled panes (1–4).
     * @param[in] outputSize    Total output pixel size.
     * @return                  Pixel rectangle for the slot.
     */
    QRect slotRect(int slot, int numEnabled, const QSize &outputSize) const;

    //=========================================================================================================
    /**
     * Hit-test the splitter bars.
     *
     * @param[in] pos           Cursor position (widget coords).
     * @param[in] numEnabled    Total number of enabled panes.
     * @param[in] outputSize    Total output pixel size.
     * @return                  Which splitter(s) are under the cursor.
     */
    SplitterHit hitTestSplitter(const QPoint &pos,
                                int numEnabled,
                                const QSize &outputSize) const;

    //=========================================================================================================
    /**
     * Return the Qt::CursorShape appropriate for a splitter hit.
     *
     * @param[in] hit           Hit-test result.
     * @return                  Cursor shape (ArrowCursor if None).
     */
    static Qt::CursorShape cursorForHit(SplitterHit hit);

    //=========================================================================================================
    /**
     * Given a cursor position and output size, determine which viewport
     * index the cursor is in.
     *
     * @param[in] pos               Cursor position (widget coords).
     * @param[in] enabledViewports  Ordered list of enabled viewport indices.
     * @param[in] outputSize        Total output pixel size.
     * @return                      Viewport index, or -1 if outside.
     */
    int viewportIndexAt(const QPoint &pos,
                        const QVector<int> &enabledViewports,
                        const QSize &outputSize) const;

    //=========================================================================================================
    /**
     * Apply a separator-pixel inset to a pane rect so adjacent panes
     * don't overlap the separator bar.  This is used in the render loop
     * to trim viewports.
     *
     * @param[in] paneRect      Original pane rectangle.
     * @param[in] slot          Slot index (0-based).
     * @param[in] numEnabled    Total enabled count.
     * @return                  Inset rectangle.
     */
    QRect insetForSeparator(const QRect &paneRect,
                            int slot,
                            int numEnabled) const;

    //=========================================================================================================
    /**
     * Compute the separator widget geometries.
     *
     * @param[in]  numEnabled      Number of enabled panes.
     * @param[in]  widgetSize      Widget size in pixels.
     * @param[out] verticalRect    Geometry for the vertical separator (empty if hidden).
     * @param[out] horizontalRect  Geometry for the horizontal separator (empty if hidden).
     */
    void separatorGeometries(int numEnabled,
                             const QSize &widgetSize,
                             QRect &verticalRect,
                             QRect &horizontalRect) const;

    //=========================================================================================================
    /**
     * Update split fractions from a mouse drag position.
     *
     * @param[in] pos            Current cursor position.
     * @param[in] activeSplitter Which splitter is being dragged.
     * @param[in] widgetSize     Widget size in pixels.
     */
    void dragSplitter(const QPoint &pos,
                      SplitterHit activeSplitter,
                      const QSize &widgetSize);

private:
    static float clampSplit(float v) { return std::clamp(v, 0.15f, 0.85f); }

    float m_splitX = 0.5f;
    float m_splitY = 0.5f;
    int   m_hitTolerancePx = 6;
    int   m_minPanePx      = 80;
    int   m_separatorLinePx = 2;
};

#endif // MULTIVIEWLAYOUT_H
