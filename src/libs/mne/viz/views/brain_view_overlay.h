//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brain_view_overlay.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    BrainViewOverlay: the labels and pane separators drawn over the 3-D brain view, as a display list.
 */

#ifndef VIZ_BRAIN_VIEW_OVERLAY_H
#define VIZ_BRAIN_VIEW_OVERLAY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../figures/display_list.h"
#include "pane_layout.h"
#include "../viz_global.h"
#include "../figures/style.h"
#include "../figures/text_metrics.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class Canvas;

/**
 * @brief Head-up display of the 3-D brain view: FPS, view info, hovered region, pane names and separators.
 *
 * The overlay keeps the text and geometry of its items in logical pixels; update_layout() places them for the
 * current view size and pane layout, and draw() renders the visible ones (separators below labels) as a
 * display list that a backend paints over the 3-D scene.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT BrainViewOverlay
{
public:
    /** Items of the overlay. */
    enum class Item
    {
        FpsLabel,            /**< FPS and vertex count, bottom right. */
        SingleViewInfoLabel, /**< Shader, surface and overlay of the single view, top right. */
        RegionLabel,         /**< Name of the hovered region or object, top left. */
        ViewportNameLabel,   /**< Preset name of a pane, top left of the pane; clicking it opens the preset menu. */
        ViewportInfoLabel,   /**< Shader, surface and overlay of a pane, top right of the pane. */
        VerticalSeparator,   /**< Separator between left and right panes. */
        HorizontalSeparator  /**< Separator between top and bottom panes. */
    };

    /** One label or separator. */
    struct Label
    {
        std::string text;      /**< Text; lines are separated by '\n'. */
        PixelRect geometry;    /**< Box in logical pixels. */
        bool visible = false;  /**< Whether the item is drawn. */
        bool selected = false; /**< Pane name of the pane being edited. */
    };

    /** Look of a label. */
    struct LabelStyle
    {
        Font font;                                  /**< Text font. */
        Color color = Color::white();               /**< Text colour. */
        Color background = Color::transparent();    /**< Box fill. */
        Color border = Color::transparent();        /**< Box outline. */
        int border_px = 0;                          /**< Outline width. */
        int radius = 0;                             /**< Corner radius of the box. */
        int pad_x = 0;                              /**< Horizontal padding inside the outline. */
        int pad_y = 0;                              /**< Vertical padding inside the outline. */
        Align align = Align::Left | Align::VCenter; /**< Text alignment in the padded box. */
    };

    /** Font families of the labels; the defaults are the generic families. */
    struct Fonts
    {
        std::string monospace = "monospace";   /**< Family of the FPS and info labels. */
        std::string sans_serif = "sans-serif"; /**< Family of the region and pane name labels. */
    };

    /** What the layout depends on. */
    struct LayoutInput
    {
        PixelSize size;                     /**< Size of the view in logical pixels. */
        bool multi_view = false;            /**< Whether the view is split into panes. */
        std::span<const int> viewports;     /**< Viewport index of each pane, in slot order (multi view). */
        std::span<const int> presets;       /**< View preset of each viewport. */
        int edit_target = -1;               /**< Viewport being edited, -1 for the single view. */
        bool info_visible = true;           /**< Whether the FPS and info labels are shown. */
        const PaneLayout* layout = nullptr; /**< Pane layout; nullptr for a default layout. */
    };

    //=========================================================================================================
    /**
     * @param[in] viewport_count   Number of viewports that have name and info labels.
     */
    explicit BrainViewOverlay(int viewport_count = 0);

    /** @param[in] count   Number of viewports that have name and info labels. */
    void set_viewport_count(int count);

    /** @return The number of viewports. */
    int viewport_count() const
    {
        return static_cast<int>(m_names.size());
    }

    /** @param[in] fonts   Font families. */
    void set_fonts(Fonts fonts)
    {
        m_fonts = std::move(fonts);
    }

    /** @param[in] color   Fill of the pane separators. */
    void set_separator_color(Color color)
    {
        m_separator_color = color;
    }

    //=========================================================================================================
    /**
     * @param[in] item       Item.
     * @param[in] viewport   Viewport for the pane labels.
     *
     * @return The item, or nullptr for a viewport out of range.
     */
    const Label* item(Item item, int viewport = 0) const;

    //=========================================================================================================
    /**
     * Sets the text of a label; the next update_layout() sizes it.
     *
     * @param[in] item       Label.
     * @param[in] text       Text.
     * @param[in] viewport   Viewport for the pane labels.
     *
     * @return Whether the text changed.
     */
    bool set_text(Item item, std::string text, int viewport = 0);

    //=========================================================================================================
    /**
     * Shows the hovered region, or hides the region label for an empty name (keeping the last text).
     *
     * @param[in] name   Region name.
     */
    void set_region(const std::string& name);

    //=========================================================================================================
    /**
     * @param[in] item       Label.
     * @param[in] selected   Whether a pane name is selected.
     *
     * @return The look of the label.
     */
    LabelStyle style(Item item, bool selected = false) const;

    //=========================================================================================================
    /**
     * @param[in] style     Look.
     * @param[in] text      Text.
     * @param[in] metrics   Text measurement.
     *
     * @return The size of the label box: the text with padding and outline, rounded up to whole pixels.
     */
    static PixelSize label_size(const LabelStyle& style, std::string_view text, const TextMetrics& metrics);

    //=========================================================================================================
    /**
     * Places and shows or hides the items.
     *
     * @param[in] input     View size and pane layout.
     * @param[in] metrics   Text measurement.
     */
    void update_layout(const LayoutInput& input, const TextMetrics& metrics);

    //=========================================================================================================
    /**
     * @param[in] width     Width of the view in logical pixels.
     * @param[in] height    Height of the view in logical pixels.
     * @param[in] metrics   Text measurement for the line spacing.
     *
     * @return The visible items as a display list on a transparent background.
     */
    DisplayList draw(int width, int height, const TextMetrics& metrics) const;

private:
    Label* item_mut(Item item, int viewport);
    void draw_label(Canvas& canvas, const LabelStyle& style, const Label& label, const TextMetrics& metrics) const;

    Fonts m_fonts;
    Color m_separator_color{160, 160, 160, 180};
    Label m_fps;
    Label m_single_info;
    Label m_region;
    std::vector<Label> m_names;
    std::vector<Label> m_infos;
    Label m_vertical;
    Label m_horizontal;
};

} // namespace mne::viz

#endif // VIZ_BRAIN_VIEW_OVERLAY_H
