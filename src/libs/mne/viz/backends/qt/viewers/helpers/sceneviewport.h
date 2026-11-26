//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sceneviewport.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SceneViewport: a zoomable, pannable widget that draws an mne::viz model in scene coordinates.
 *
 * Replaces LayoutScene and the QGraphicsView it drove: the wheel and pinch zoom around the pointer, the right
 * mouse button and pan gestures move the view, a double click fits the contents, and a left drag outside an item
 * spans a rubber band. Until the user zooms or pans, the view keeps its contents fitted as it resizes.
 */

#ifndef SCENEVIEWPORT_H
#define SCENEVIEWPORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"

#include <mne/viz/figures/canvas.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QWidget>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
/**
 * @brief Base of the widgets that show a scene model, such as a sensor layout, with zoom, pan and rubber band.
 *
 * Subclasses report the area of their contents in sceneBounds(), draw them in drawScene() and react to clicks
 * and bands in scenePressed() and sceneBandSelected().
 *
 * @since 3.0.0
 */
class VIZQTSHARED_EXPORT SceneViewport : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs an empty viewport with a white background.
     *
     * @param[in] parent   Parent widget.
     */
    explicit SceneViewport(QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * @param[in] color   Colour behind the contents.
     */
    void setBackgroundColor(const QColor& color);

    //=========================================================================================================
    /**
     * @return The colour behind the contents.
     */
    QColor backgroundColor() const;

    //=========================================================================================================
    /**
     * Fits the contents into the widget, keeping their aspect ratio, and keeps them fitted as the widget resizes
     * until the user zooms or pans.
     */
    void fitToContents();

    //=========================================================================================================
    /**
     * @return Whether the contents are kept fitted.
     */
    bool isFitted() const;

    //=========================================================================================================
    /**
     * @return Pixels per scene unit.
     */
    double zoom() const;

    //=========================================================================================================
    /**
     * Zooms by @p factor, keeping the scene point under @p anchor in place.
     *
     * @param[in] factor   Factor, above 1 to zoom in.
     * @param[in] anchor   Fixed point in widget coordinates.
     */
    void zoomBy(double factor, const QPointF& anchor);

    //=========================================================================================================
    /**
     * Moves the contents.
     *
     * @param[in] pixels   Offset in widget pixels.
     */
    void panBy(const QPointF& pixels);

    //=========================================================================================================
    /**
     * @param[in] point   Point in widget coordinates.
     *
     * @return The point in scene coordinates.
     */
    viz::Point mapToScene(const QPointF& point) const;

    //=========================================================================================================
    /**
     * @param[in] point   Point in scene coordinates.
     *
     * @return The point in widget coordinates.
     */
    QPointF mapFromScene(viz::Point point) const;

    //=========================================================================================================
    /**
     * The contents fitted into a figure, e.g. to export them with viz::write_svg().
     *
     * @param[in] size   Size of the figure in pixels.
     *
     * @return The figure on the background colour.
     */
    viz::DisplayList figure(const QSize& size) const;

protected:
    //=========================================================================================================
    /**
     * @return The area of the contents in scene coordinates; empty without contents.
     */
    virtual viz::Rect sceneBounds() const = 0;

    //=========================================================================================================
    /**
     * Draws the contents in scene coordinates; @p canvas already maps them to the widget.
     *
     * @param[in, out] canvas   Canvas to draw on.
     */
    virtual void drawScene(viz::Canvas& canvas) const = 0;

    //=========================================================================================================
    /**
     * Called when the left button goes down.
     *
     * @param[in] point       Scene coordinates of the press.
     * @param[in] modifiers   Keyboard modifiers.
     *
     * @return Whether an item took the press; otherwise a rubber band starts if bandSelectionEnabled().
     */
    virtual bool scenePressed(viz::Point point, Qt::KeyboardModifiers modifiers);

    //=========================================================================================================
    /**
     * Called when the left button goes up after scenePressed() took the press.
     *
     * @param[in] point   Scene coordinates of the release.
     */
    virtual void sceneReleased(viz::Point point);

    //=========================================================================================================
    /**
     * Called when a rubber band is released.
     *
     * @param[in] band        The band in scene coordinates.
     * @param[in] modifiers   Keyboard modifiers at the start of the band.
     */
    virtual void sceneBandSelected(const viz::Rect& band, Qt::KeyboardModifiers modifiers);

    //=========================================================================================================
    /**
     * @return Whether a left drag outside an item spans a rubber band.
     */
    virtual bool bandSelectionEnabled() const;

    //=========================================================================================================
    /**
     * Refits the contents if they are kept fitted, and repaints. Subclasses call it when their contents change.
     */
    void contentsChanged();

    //=========================================================================================================
    /**
     * Draws the background, the contents and the rubber band.
     *
     * @param[in] event   The paint event.
     */
    void paintEvent(QPaintEvent* event) override;

    //=========================================================================================================
    /**
     * Keeps fitted contents fitted.
     *
     * @param[in] event   The resize event.
     */
    void resizeEvent(QResizeEvent* event) override;

    //=========================================================================================================
    /**
     * Zooms by 1.15 per notch around the pointer.
     *
     * @param[in] event   The wheel event.
     */
    void wheelEvent(QWheelEvent* event) override;

    //=========================================================================================================
    /**
     * Starts a pan (right button), an item press or a rubber band (left button).
     *
     * @param[in] event   The mouse event.
     */
    void mousePressEvent(QMouseEvent* event) override;

    //=========================================================================================================
    /**
     * Pans or stretches the rubber band.
     *
     * @param[in] event   The mouse event.
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    //=========================================================================================================
    /**
     * Ends a pan, an item press or a rubber band.
     *
     * @param[in] event   The mouse event.
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

    //=========================================================================================================
    /**
     * Fits the contents on a left double click.
     *
     * @param[in] event   The mouse event.
     */
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    //=========================================================================================================
    /**
     * Handles pinch (zoom) and pan gestures.
     *
     * @param[in] event   The event.
     *
     * @return Whether the event was handled.
     */
    bool event(QEvent* event) override;

private:
    enum class Drag
    {
        None,
        Pan,
        Item,
        Band
    };

    void applyFit();

    double m_zoom = 1.0;                   /**< Pixels per scene unit. */
    QPointF m_offset;                      /**< Widget position of the scene origin. */
    bool m_fitted = true;                  /**< Whether the contents are kept fitted. */
    QColor m_background = Qt::white;       /**< Colour behind the contents. */
    Drag m_drag = Drag::None;              /**< Current mouse drag. */
    QPointF m_lastPosition;                /**< Last pointer position of a pan. */
    QPointF m_bandStart;                   /**< Widget position where the rubber band started. */
    QPointF m_bandEnd;                     /**< Current corner of the rubber band. */
    Qt::KeyboardModifiers m_bandModifiers; /**< Modifiers at the start of the band. */
};

} // namespace mne::viz::qt

#endif // SCENEVIEWPORT_H
