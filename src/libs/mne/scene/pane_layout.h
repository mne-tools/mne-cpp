//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     pane_layout.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    PaneLayout: pane rectangles, separators and splitter dragging of a view split into 1 to 4 panes.
 */

#ifndef SCENE_PANE_LAYOUT_H
#define SCENE_PANE_LAYOUT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "geometry.h"
#include "input_event.h"
#include "scene_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <span>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

/**
 * @brief Size in whole pixels.
 *
 * @since 3.0.0
 */
struct PixelSize
{
    int width = 0;  /**< Width. */
    int height = 0; /**< Height. */

    friend bool operator==(const PixelSize&, const PixelSize&) = default;
};

/**
 * @brief Rectangle in whole pixels; it covers the pixels x .. x + width - 1 and y .. y + height - 1.
 *
 * @since 3.0.0
 */
struct PixelRect
{
    int x = 0;      /**< Left edge. */
    int y = 0;      /**< Top edge. */
    int width = 0;  /**< Width. */
    int height = 0; /**< Height. */

    friend bool operator==(const PixelRect&, const PixelRect&) = default;

    /** @return Whether the rectangle covers no pixel. */
    bool empty() const
    {
        return width <= 0 || height <= 0;
    }

    /**
     * @param[in] p   Point.
     *
     * @return Whether @p p lies in one of the pixels of the rectangle (right and bottom edges excluded).
     */
    bool contains(Point p) const
    {
        return p.x >= x && p.x < x + width && p.y >= y && p.y < y + height;
    }

    /** @return The rectangle in floating-point coordinates. */
    Rect to_rect() const
    {
        return {static_cast<double>(x), static_cast<double>(y), static_cast<double>(width), static_cast<double>(height)};
    }
};

/**
 * @brief Splitter of a multi-pane layout under the pointer.
 *
 * @since 3.0.0
 */
enum class SplitterHit
{
    None,       /**< No splitter. */
    Vertical,   /**< The vertical splitter between left and right panes. */
    Horizontal, /**< The horizontal splitter between top and bottom panes. */
    Both        /**< The crossing of both splitters. */
};

/**
 * @brief Splits a view into 1 to 4 panes with draggable splitters.
 *
 * Two panes sit side by side, three panes are one full-width pane above two, four panes form a 2 x 2 grid. The
 * splitters sit at fractions of the width and height, clamped to 0.15 .. 0.85, and no pane gets narrower than
 * the minimum pane size.
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT PaneLayout
{
public:
    /** Separator rectangles; empty ones are not shown. */
    struct Separators
    {
        PixelRect vertical;   /**< Between left and right panes. */
        PixelRect horizontal; /**< Between top and bottom panes. */
    };

    /** @return The horizontal splitter fraction. */
    float split_x() const
    {
        return m_split_x;
    }

    /** @return The vertical splitter fraction. */
    float split_y() const
    {
        return m_split_y;
    }

    /** @param[in] x   Horizontal splitter fraction, clamped to 0.15 .. 0.85. */
    void set_split_x(float x)
    {
        m_split_x = clamp_split(x);
    }

    /** @param[in] y   Vertical splitter fraction, clamped to 0.15 .. 0.85. */
    void set_split_y(float y)
    {
        m_split_y = clamp_split(y);
    }

    /** Centres both splitters. */
    void reset_splits()
    {
        m_split_x = 0.5f;
        m_split_y = 0.5f;
    }

    /** @return How far from a splitter the pointer still grabs it, in pixels. */
    int hit_tolerance_px() const
    {
        return m_hit_tolerance_px;
    }

    /** @return The minimum pane extent in pixels. */
    int min_pane_px() const
    {
        return m_min_pane_px;
    }

    /** @return The separator thickness in pixels. */
    int separator_line_px() const
    {
        return m_separator_line_px;
    }

    //=========================================================================================================
    /**
     * @param[in] slot          Pane slot, 0 .. pane_count - 1.
     * @param[in] pane_count    Number of panes.
     * @param[in] size          Size of the view.
     *
     * @return The rectangle of the pane.
     */
    PixelRect slot_rect(int slot, int pane_count, PixelSize size) const;

    //=========================================================================================================
    /**
     * @param[in] pos           Pointer position.
     * @param[in] pane_count    Number of panes.
     * @param[in] size          Size of the view.
     *
     * @return The splitter under the pointer; with three panes the vertical splitter exists below the
     *         horizontal one only.
     */
    SplitterHit hit_test_splitter(Point pos, int pane_count, PixelSize size) const;

    //=========================================================================================================
    /**
     * @param[in] hit   Splitter under the pointer.
     *
     * @return The pointer shape that shows the splitter.
     */
    static CursorShape cursor_for_hit(SplitterHit hit);

    //=========================================================================================================
    /**
     * @param[in] pos         Pointer position.
     * @param[in] viewports   Viewport index of each slot, in slot order.
     * @param[in] size        Size of the view.
     *
     * @return The viewport index of the pane under the pointer, or -1.
     */
    int viewport_index_at(Point pos, std::span<const int> viewports, PixelSize size) const;

    //=========================================================================================================
    /**
     * @param[in] pane         Pane rectangle.
     * @param[in] slot         Pane slot.
     * @param[in] pane_count   Number of panes.
     *
     * @return The pane rectangle without the separators on its right and bottom edges, where the 3-D scene is
     *         drawn.
     */
    PixelRect inset_for_separator(const PixelRect& pane, int slot, int pane_count) const;

    //=========================================================================================================
    /**
     * @param[in] pane_count   Number of panes.
     * @param[in] size         Size of the view.
     *
     * @return The separator rectangles.
     */
    Separators separator_geometries(int pane_count, PixelSize size) const;

    //=========================================================================================================
    /**
     * Moves the dragged splitter (or both) to the pointer.
     *
     * @param[in] pos        Pointer position.
     * @param[in] splitter   Dragged splitter.
     * @param[in] size       Size of the view.
     */
    void drag_splitter(Point pos, SplitterHit splitter, PixelSize size);

private:
    static float clamp_split(float v)
    {
        return std::clamp(v, 0.15f, 0.85f);
    }

    int split_pos(float fraction, int extent) const;

    float m_split_x = 0.5f;
    float m_split_y = 0.5f;
    int m_hit_tolerance_px = 6;
    int m_min_pane_px = 80;
    int m_separator_line_px = 2;
};

} // namespace mne::scene

#endif // SCENE_PANE_LAYOUT_H
