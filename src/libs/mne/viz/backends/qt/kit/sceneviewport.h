//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sceneviewport.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SceneViewport: a widget that shows any toolkit-free viz::SceneFigure with zoom, pan and rubber band.
 *
 * Replaces LayoutScene and the QGraphicsView it drove. The navigation is viz::FigureView's, shared with hosts
 * without a toolkit: the wheel and pinch zoom around the pointer, the right mouse button and pan gestures move the
 * view, a double click fits the contents, and a left drag outside an item spans a rubber band. Until the user zooms
 * or pans, the view keeps its contents fitted as it resizes.
 */

#ifndef SCENEVIEWPORT_H
#define SCENEVIEWPORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_qt_global.h"

#include <mne/viz/figures/display_list.h>
#include <mne/viz/figures/geometry.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QWidget>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne::viz
{
class FigureView;
class SceneFigure;
struct Theme;
} // namespace mne::viz

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

class QtTextMetrics;

//=============================================================================================================
/**
 * @brief Shows a viz::SceneFigure, such as a sensor or average layout, through a viz::FigureView.
 *
 * @since 3.0.0
 */
class VIZQTSHARED_EXPORT SceneViewport : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a viewport without a figure on a white background.
     *
     * @param[in] parent   Parent widget.
     */
    explicit SceneViewport(QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Constructs a viewport that shows @p figure.
     *
     * @param[in] figure   The figure; the viewport shares its ownership.
     * @param[in] parent   Parent widget.
     */
    explicit SceneViewport(std::shared_ptr<viz::SceneFigure> figure, QWidget* parent = nullptr);

    ~SceneViewport() override;

    //=========================================================================================================
    /**
     * Shows @p figure, fitted.
     *
     * @param[in] figure   The figure, or null to show only the background; the viewport shares its ownership.
     */
    void setFigure(std::shared_ptr<viz::SceneFigure> figure);

    //=========================================================================================================
    /**
     * @return The figure shown, or null.
     */
    viz::SceneFigure* figure() const;

    //=========================================================================================================
    /**
     * @return The navigation of the figure, or null without a figure.
     */
    viz::FigureView* figureView() const;

    //=========================================================================================================
    /**
     * @param[in] theme   Colours of the rubber band.
     */
    void setTheme(const viz::Theme& theme);

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
     * The contents fitted into a picture, e.g. to export them with viz::write_svg().
     *
     * @param[in] size   Size of the picture in pixels.
     *
     * @return The figure on the background colour.
     */
    viz::DisplayList picture(const QSize& size) const;

protected:
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
     * Zooms by 1.15 per notch around the pointer, or scrolls a figure that is not zoomable.
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
     * Pans, stretches the rubber band or hovers.
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
     * Passes a left double click to the figure, or fits the contents.
     *
     * @param[in] event   The mouse event.
     */
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    //=========================================================================================================
    /**
     * Tells the figure that the pointer left.
     *
     * @param[in] event   The event.
     */
    void leaveEvent(QEvent* event) override;

    //=========================================================================================================
    /**
     * Handles pinch (zoom) and pan gestures, and font changes.
     *
     * @param[in] event   The event.
     *
     * @return Whether the event was handled.
     */
    bool event(QEvent* event) override;

private:
    class Surface;

    void fontChanged();

    std::shared_ptr<viz::SceneFigure> m_figure; /**< The figure shown. */
    std::unique_ptr<viz::FigureView> m_view;    /**< Navigation and drawing of the figure. */
    std::unique_ptr<Surface> m_surface;         /**< Repaints the widget when the view asks to. */
    std::unique_ptr<QtTextMetrics> m_metrics;   /**< Metrics of the widget's font. */
    std::unique_ptr<viz::Theme> m_theme;        /**< Colours of the rubber band. */
    QColor m_background = Qt::white;            /**< Colour behind the contents. */
};

} // namespace mne::viz::qt

#endif // SCENEVIEWPORT_H
