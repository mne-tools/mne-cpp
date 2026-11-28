//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     scene_figure.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SceneFigure: the contract of an interactive figure that any host can show, with or without a toolkit.
 */

#ifndef VIZ_SCENE_FIGURE_H
#define VIZ_SCENE_FIGURE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "../views/input_event.h"
#include "geometry.h"
#include "style.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <optional>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class Canvas;
class TextMetrics;

//=============================================================================================================
/**
 * @brief An interactive figure in scene coordinates: its bounds, how it draws and how it answers the pointer.
 *
 * A host (@ref FigureView without a toolkit, mne::viz::qt::SceneViewport in Qt) maps the scene to its window and
 * passes clicks, hovers and rubber bands in scene coordinates. A zoomable figure is fitted, zoomed and panned by
 * the host; a figure that is not zoomable lays itself out to the view size given in resize() and draws in view
 * pixels, the host only scrolling it vertically. The figure tells its hosts to repaint through changed().
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT SceneFigure
{
public:
    /** What a change touched. */
    enum class Change
    {
        Appearance, /**< Only the drawing; the host repaints. */
        Bounds      /**< The bounds; the host refits fitted figures and repaints. */
    };

    SceneFigure() = default;

    virtual ~SceneFigure();

    SceneFigure(const SceneFigure&) = delete;
    SceneFigure& operator=(const SceneFigure&) = delete;

    //=========================================================================================================
    /**
     * @return The area of the contents in scene coordinates; empty without contents.
     */
    virtual Rect bounds() const = 0;

    //=========================================================================================================
    /**
     * Draws the contents in scene coordinates; the host has already mapped them to its window.
     *
     * @param[in, out] canvas    Canvas to draw on.
     * @param[in]      metrics   Metrics of the host's fonts, to lay out text.
     * @param[in]      font      The host's base font.
     */
    virtual void draw(Canvas& canvas, const TextMetrics& metrics, const Font& font) const = 0;

    //=========================================================================================================
    /**
     * @return Whether the host fits, zooms and pans the figure; otherwise it lays itself out to the view size.
     */
    virtual bool zoomable() const;

    //=========================================================================================================
    /**
     * The view size changed; figures that are not zoomable lay themselves out to it.
     *
     * @param[in] width    Width of the view in pixels.
     * @param[in] height   Height of the view in pixels.
     */
    virtual void resize(double width, double height);

    //=========================================================================================================
    /**
     * The left button went down.
     *
     * @param[in] point       Scene coordinates of the press.
     * @param[in] modifiers   Keyboard modifiers.
     *
     * @return Whether an item took the press; otherwise the host starts a rubber band if band_selection_enabled().
     */
    virtual bool press(Point point, Modifier modifiers);

    //=========================================================================================================
    /**
     * The left button went up after press() took the press.
     *
     * @param[in] point   Scene coordinates of the release.
     */
    virtual void release(Point point);

    //=========================================================================================================
    /**
     * The pointer moved over the figure without a button, or left it.
     *
     * @param[in] point   Scene coordinates of the pointer, or no value when it left.
     */
    virtual void hover(std::optional<Point> point);

    //=========================================================================================================
    /**
     * The left button was clicked twice.
     *
     * @param[in] point   Scene coordinates of the click.
     *
     * @return Whether the figure used the click; otherwise the host fits a zoomable figure.
     */
    virtual bool double_click(Point point);

    //=========================================================================================================
    /**
     * @return Whether a left drag outside an item spans a rubber band.
     */
    virtual bool band_selection_enabled() const;

    //=========================================================================================================
    /**
     * A rubber band was released.
     *
     * @param[in] band        The band in scene coordinates.
     * @param[in] modifiers   Keyboard modifiers at the start of the band.
     */
    virtual void band_selected(const Rect& band, Modifier modifiers);

    //=========================================================================================================
    /**
     * @param[in] listener   Called on every change.
     *
     * @return An id for remove_listener().
     */
    int add_listener(std::function<void(Change)> listener);

    //=========================================================================================================
    /**
     * @param[in] id   Id from add_listener(); unknown ids are ignored.
     */
    void remove_listener(int id);

protected:
    //=========================================================================================================
    /**
     * Tells the hosts to repaint, and to refit for Change::Bounds.
     *
     * @param[in] change   What changed.
     */
    void changed(Change change = Change::Appearance) const;

private:
    std::map<int, std::function<void(Change)>> m_listeners; /**< Listeners by id. */
    int m_nextListener = 1;                                 /**< Id of the next listener. */
};

} // namespace mne::viz

#endif // VIZ_SCENE_FIGURE_H
