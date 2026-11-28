//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     figure_view.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FigureView shows any SceneFigure with zoom, pan and rubber band, without a toolkit.
 */

#ifndef VIZ_FIGURE_VIEW_H
#define VIZ_FIGURE_VIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../figures/display_list.h"
#include "../figures/scene_figure.h"
#include "../figures/text_metrics.h"
#include "../figures/theme.h"
#include "../host/surface.h"
#include "../viz_global.h"

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Navigation and drawing of a SceneFigure for any host: a window without a toolkit or a Qt widget.
 *
 * A zoomable figure is fitted into the view, keeping its aspect ratio, and stays fitted as the view resizes until
 * the user zooms or pans: the wheel zooms by 1.15 per notch around the pointer, the right button pans, a double
 * click fits again and a left drag outside an item spans a rubber band if the figure wants one. A figure that is
 * not zoomable lays itself out to the view size and the wheel scrolls it vertically. Hovers, clicks and bands
 * reach the figure in scene coordinates.
 *
 * @snippet ex_scene_sdl/main.cpp average_layout_panel_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT FigureView : public SurfaceClient
{
public:
    //=========================================================================================================
    /**
     * Shows @p figure, which must outlive the view, on a white background.
     *
     * @param[in] figure   The figure.
     * @param[in] theme    Colours of the rubber band.
     */
    explicit FigureView(SceneFigure& figure, Theme theme = Theme::light());

    ~FigureView() override;

    FigureView(const FigureView&) = delete;
    FigureView& operator=(const FigureView&) = delete;

    //=========================================================================================================
    /**
     * @return The figure.
     */
    SceneFigure& figure() const
    {
        return m_figure;
    }

    //=========================================================================================================
    /**
     * @param[in] theme   Colours of the rubber band.
     */
    void set_theme(Theme theme);

    //=========================================================================================================
    /**
     * @param[in] color   Colour behind the figure.
     */
    void set_background(Color color);

    //=========================================================================================================
    /**
     * @return The colour behind the figure.
     */
    Color background() const
    {
        return m_background;
    }

    //=========================================================================================================
    /**
     * @param[in] font   Base font the figure draws its text with.
     */
    void set_font(Font font);

    //=========================================================================================================
    /**
     * @return The base font the figure draws its text with.
     */
    const Font& font() const
    {
        return m_font;
    }

    //=========================================================================================================
    /**
     * @param[in] metrics   Metrics of the fonts the host draws with; must outlive the view. Null restores the
     *                      approximate metrics.
     */
    void set_text_metrics(const TextMetrics* metrics);

    //=========================================================================================================
    /**
     * @param[in] width    Width of the view.
     * @param[in] height   Height of the view.
     */
    void set_view_size(double width, double height);

    //=========================================================================================================
    /**
     * @return Width of the view.
     */
    double view_width() const
    {
        return m_width;
    }

    //=========================================================================================================
    /**
     * @return Height of the view.
     */
    double view_height() const
    {
        return m_height;
    }

    //=========================================================================================================
    /**
     * Fits a zoomable figure into the view and keeps it fitted as the view resizes until the user zooms or pans;
     * scrolls other figures to the top.
     */
    void fit_to_contents();

    //=========================================================================================================
    /**
     * @return Whether the figure is kept fitted.
     */
    bool fitted() const
    {
        return m_fitted;
    }

    //=========================================================================================================
    /**
     * @return View units per scene unit.
     */
    double zoom() const
    {
        return m_zoom;
    }

    //=========================================================================================================
    /**
     * Zooms a zoomable figure by @p factor, keeping the scene point under @p anchor in place.
     *
     * @param[in] factor   Factor, above 1 to zoom in.
     * @param[in] anchor   Fixed point in view coordinates.
     */
    void zoom_by(double factor, Point anchor);

    //=========================================================================================================
    /**
     * Moves a zoomable figure, or scrolls another figure vertically.
     *
     * @param[in] dx   Horizontal offset in view units.
     * @param[in] dy   Vertical offset in view units.
     */
    void pan_by(double dx, double dy);

    //=========================================================================================================
    /**
     * @param[in] point   Point in view coordinates.
     *
     * @return The point in scene coordinates.
     */
    Point map_to_scene(Point point) const;

    //=========================================================================================================
    /**
     * @param[in] point   Point in scene coordinates.
     *
     * @return The point in view coordinates.
     */
    Point map_from_scene(Point point) const;

    //=========================================================================================================
    /**
     * @return The rubber band in view coordinates while one is spanned.
     */
    std::optional<Rect> band() const;

    //=========================================================================================================
    /**
     * Tells the figure that the pointer left the view.
     */
    void leave();

    //=========================================================================================================
    /**
     * @return The view: background, figure and rubber band, in view coordinates.
     */
    DisplayList draw() const;

    //=========================================================================================================
    /**
     * The figure fitted into a picture of its own, e.g. to export it with write_svg().
     *
     * @param[in] width    Width of the picture.
     * @param[in] height   Height of the picture.
     *
     * @return The figure on the background colour; a figure that is not zoomable is laid out to the picture.
     */
    DisplayList picture(double width, double height) const;

    void resize(const SurfaceSize& size) override;
    bool handle_pointer(const PointerEvent& event) override;
    bool handle_wheel(const WheelEvent& event) override;
    GpuFrame frame(const GpuFrameTarget& target) override;
    const GpuResources& resources() const override;

private:
    enum class Drag
    {
        None,
        Pan,
        Item,
        Band
    };

    void apply_fit();
    void clamp_scroll();
    void figure_changed(SceneFigure::Change change);
    void update() const;

    SceneFigure& m_figure;
    Theme m_theme;
    Color m_background{255, 255, 255};
    Font m_font;
    ApproximateTextMetrics m_approximateMetrics;
    const TextMetrics* m_metrics = &m_approximateMetrics;
    int m_listener = 0;
    double m_width = 0.0;
    double m_height = 0.0;
    double m_zoom = 1.0;
    Point m_offset;
    bool m_fitted = true;
    Drag m_drag = Drag::None;
    Point m_last;
    Point m_bandStart;
    Point m_bandEnd;
    Modifier m_bandModifiers = Modifier::None;
    GpuResources m_resources;
    GpuHandle m_pipeline = 0;
    GpuHandle m_quad = 0;
    GpuHandle m_texture = 0;
};

} // namespace mne::viz

#endif // VIZ_FIGURE_VIEW_H
