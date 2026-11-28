//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sceneviewport.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the SceneViewport class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sceneviewport.h"
#include "../plots/helpers/sceneinput.h"
#include "../plots/helpers/scenepainter.h"
#include <mne/viz/figures/canvas.h>
#include <mne/viz/figures/scene_figure.h>
#include <mne/viz/figures/theme.h>
#include <mne/viz/host/surface.h>
#include <mne/viz/panels/figure_view.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGestureEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPanGesture>
#include <QPinchGesture>
#include <QWheelEvent>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

/** Repaints the widget when the figure view asks for it. */
class SceneViewport::Surface : public viz::Surface
{
public:
    explicit Surface(QWidget& widget)
    : m_widget(widget)
    {
    }

    void request_update() override
    {
        m_widget.update();
    }

    std::int64_t now_ms() const override
    {
        return sceneTimestampMs();
    }

private:
    QWidget& m_widget;
};

//=============================================================================================================

SceneViewport::SceneViewport(QWidget* parent)
: QWidget(parent)
, m_surface(std::make_unique<Surface>(*this))
, m_metrics(std::make_unique<QtTextMetrics>(font()))
, m_theme(std::make_unique<viz::Theme>(viz::Theme::light()))
{
    setMouseTracking(true);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::PanGesture);
}

//=============================================================================================================

SceneViewport::SceneViewport(std::shared_ptr<viz::SceneFigure> figure, QWidget* parent)
: SceneViewport(parent)
{
    setFigure(std::move(figure));
}

//=============================================================================================================

SceneViewport::~SceneViewport()
{
    // The view stops listening to the figure before the figure may go
    m_view.reset();
}

//=============================================================================================================

void SceneViewport::setFigure(std::shared_ptr<viz::SceneFigure> figure)
{
    m_view.reset();
    m_figure = std::move(figure);
    if (m_figure) {
        m_view = std::make_unique<viz::FigureView>(*m_figure, *m_theme);
        m_view->set_background(toSceneColor(m_background));
        m_view->set_font(toSceneFont(font()));
        m_view->set_text_metrics(m_metrics.get());
        m_view->attach(m_surface.get());
        m_view->set_view_size(width(), height());
    }
    update();
}

//=============================================================================================================

viz::SceneFigure* SceneViewport::figure() const
{
    return m_figure.get();
}

//=============================================================================================================

viz::FigureView* SceneViewport::figureView() const
{
    return m_view.get();
}

//=============================================================================================================

void SceneViewport::setTheme(const viz::Theme& theme)
{
    *m_theme = theme;
    if (m_view) {
        m_view->set_theme(theme);
    }
}

//=============================================================================================================

void SceneViewport::setBackgroundColor(const QColor& color)
{
    m_background = color;
    if (m_view) {
        m_view->set_background(toSceneColor(color));
    }
    update();
}

//=============================================================================================================

QColor SceneViewport::backgroundColor() const
{
    return m_background;
}

//=============================================================================================================

void SceneViewport::fitToContents()
{
    if (m_view) {
        m_view->fit_to_contents();
    }
}

//=============================================================================================================

bool SceneViewport::isFitted() const
{
    return !m_view || m_view->fitted();
}

//=============================================================================================================

double SceneViewport::zoom() const
{
    return m_view ? m_view->zoom() : 1.0;
}

//=============================================================================================================

void SceneViewport::zoomBy(double factor, const QPointF& anchor)
{
    if (m_view) {
        m_view->zoom_by(factor, {anchor.x(), anchor.y()});
    }
}

//=============================================================================================================

void SceneViewport::panBy(const QPointF& pixels)
{
    if (m_view) {
        m_view->pan_by(pixels.x(), pixels.y());
    }
}

//=============================================================================================================

mne::viz::Point SceneViewport::mapToScene(const QPointF& point) const
{
    return m_view ? m_view->map_to_scene({point.x(), point.y()}) : viz::Point{point.x(), point.y()};
}

//=============================================================================================================

QPointF SceneViewport::mapFromScene(viz::Point point) const
{
    const viz::Point mapped = m_view ? m_view->map_from_scene(point) : point;
    return {mapped.x, mapped.y};
}

//=============================================================================================================

mne::viz::DisplayList SceneViewport::picture(const QSize& size) const
{
    if (!m_view) {
        return viz::Canvas(size.width(), size.height(), toSceneColor(m_background)).take();
    }
    return m_view->picture(size.width(), size.height());
}

//=============================================================================================================

void SceneViewport::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    if (!m_view) {
        painter.fillRect(rect(), m_background);
        return;
    }
    paintScene(painter, m_view->draw(), font());
}

//=============================================================================================================

void SceneViewport::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_view) {
        m_view->set_view_size(width(), height());
    }
}

//=============================================================================================================

void SceneViewport::wheelEvent(QWheelEvent* event)
{
    if (m_view) {
        m_view->handle_wheel(toSceneWheelEvent(*event));
    }
    event->accept();
}

//=============================================================================================================

void SceneViewport::mousePressEvent(QMouseEvent* event)
{
    if (m_view) {
        m_view->handle_pointer(toScenePointerEvent(*event, viz::PointerEvent::Type::Press));
    }
    event->accept();
}

//=============================================================================================================

void SceneViewport::mouseMoveEvent(QMouseEvent* event)
{
    if (m_view) {
        m_view->handle_pointer(toScenePointerEvent(*event, viz::PointerEvent::Type::Move));
    }
    event->accept();
}

//=============================================================================================================

void SceneViewport::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_view) {
        m_view->handle_pointer(toScenePointerEvent(*event, viz::PointerEvent::Type::Release));
    }
    event->accept();
}

//=============================================================================================================

void SceneViewport::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (m_view) {
        m_view->handle_pointer(toScenePointerEvent(*event, viz::PointerEvent::Type::DoubleClick));
    }
    event->accept();
}

//=============================================================================================================

void SceneViewport::leaveEvent(QEvent* event)
{
    if (m_view) {
        m_view->leave();
    }
    QWidget::leaveEvent(event);
}

//=============================================================================================================

bool SceneViewport::event(QEvent* event)
{
    if (event->type() == QEvent::Gesture) {
        auto* gestures = static_cast<QGestureEvent*>(event);
        if (auto* pinch = static_cast<QPinchGesture*>(gestures->gesture(Qt::PinchGesture))) {
            zoomBy(pinch->scaleFactor(), mapFromGlobal(pinch->centerPoint()));
        }
        if (auto* pan = static_cast<QPanGesture*>(gestures->gesture(Qt::PanGesture))) {
            panBy(-pan->delta());
        }
        return true;
    }
    if (event->type() == QEvent::FontChange) {
        fontChanged();
    }
    return QWidget::event(event);
}

//=============================================================================================================

void SceneViewport::fontChanged()
{
    m_metrics = std::make_unique<QtTextMetrics>(font());
    if (m_view) {
        m_view->set_text_metrics(m_metrics.get());
        m_view->set_font(toSceneFont(font()));
    }
}
