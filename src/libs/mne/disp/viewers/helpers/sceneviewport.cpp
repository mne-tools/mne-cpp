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
#include "../../plots/helpers/scenepainter.h"

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

#include <algorithm>
#include <cmath>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::disp;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

// Zoom per wheel notch
constexpr double kWheelZoom = 1.15;
// Gap around fitted contents, as QGraphicsView::fitInView leaves
constexpr double kFitMargin = 2.0;

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SceneViewport::SceneViewport(QWidget* parent)
: QWidget(parent)
{
    setMouseTracking(false);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::PanGesture);
}

//=============================================================================================================

void SceneViewport::setBackgroundColor(const QColor& color)
{
    m_background = color;
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
    m_fitted = true;
    applyFit();
    update();
}

//=============================================================================================================

bool SceneViewport::isFitted() const
{
    return m_fitted;
}

//=============================================================================================================

double SceneViewport::zoom() const
{
    return m_zoom;
}

//=============================================================================================================

void SceneViewport::zoomBy(double factor, const QPointF& anchor)
{
    if (!(factor > 0.0)) {
        return;
    }
    const scene::Point fixed = mapToScene(anchor);
    m_zoom *= factor;
    m_offset = anchor - QPointF(fixed.x, fixed.y) * m_zoom;
    m_fitted = false;
    update();
}

//=============================================================================================================

void SceneViewport::panBy(const QPointF& pixels)
{
    m_offset += pixels;
    m_fitted = false;
    update();
}

//=============================================================================================================

scene::Point SceneViewport::mapToScene(const QPointF& point) const
{
    return {(point.x() - m_offset.x()) / m_zoom, (point.y() - m_offset.y()) / m_zoom};
}

//=============================================================================================================

QPointF SceneViewport::mapFromScene(scene::Point point) const
{
    return QPointF(point.x, point.y) * m_zoom + m_offset;
}

//=============================================================================================================

scene::DisplayList SceneViewport::figure(const QSize& size) const
{
    scene::Canvas canvas(size.width(), size.height(), toSceneColor(m_background));
    const scene::Rect bounds = sceneBounds();
    if (!bounds.empty()) {
        const double zoom = std::min((size.width() - 2 * kFitMargin) / bounds.width,
                                     (size.height() - 2 * kFitMargin) / bounds.height);
        canvas.translate(size.width() / 2.0 - bounds.center().x * zoom, size.height() / 2.0 - bounds.center().y * zoom);
        canvas.scale(zoom, zoom);
        drawScene(canvas);
    }
    return canvas.take();
}

//=============================================================================================================

bool SceneViewport::scenePressed(scene::Point /*point*/, Qt::KeyboardModifiers /*modifiers*/)
{
    return false;
}

//=============================================================================================================

void SceneViewport::sceneReleased(scene::Point /*point*/)
{
}

//=============================================================================================================

void SceneViewport::sceneBandSelected(const scene::Rect& /*band*/, Qt::KeyboardModifiers /*modifiers*/)
{
}

//=============================================================================================================

bool SceneViewport::bandSelectionEnabled() const
{
    return false;
}

//=============================================================================================================

void SceneViewport::contentsChanged()
{
    if (m_fitted) {
        applyFit();
    }
    update();
}

//=============================================================================================================

void SceneViewport::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    scene::Canvas canvas(width(), height(), toSceneColor(m_background));
    canvas.translate(m_offset.x(), m_offset.y());
    canvas.scale(m_zoom, m_zoom);
    drawScene(canvas);
    paintScene(painter, canvas.display_list(), font());

    if (m_drag == Drag::Band) {
        QColor fill = palette().highlight().color();
        fill.setAlpha(60);
        painter.setPen(QPen(palette().highlight().color(), 1));
        painter.setBrush(fill);
        painter.drawRect(QRectF(m_bandStart, m_bandEnd).normalized());
    }
}

//=============================================================================================================

void SceneViewport::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_fitted) {
        applyFit();
    }
}

//=============================================================================================================

void SceneViewport::wheelEvent(QWheelEvent* event)
{
    const int delta = event->angleDelta().y();
    if (delta != 0) {
        zoomBy(delta > 0 ? kWheelZoom : 1.0 / kWheelZoom, event->position());
    }
    event->accept();
}

//=============================================================================================================

void SceneViewport::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        m_drag = Drag::Pan;
        m_lastPosition = event->position();
    } else if (event->button() == Qt::LeftButton) {
        if (scenePressed(mapToScene(event->position()), event->modifiers())) {
            m_drag = Drag::Item;
        } else if (bandSelectionEnabled()) {
            m_drag = Drag::Band;
            m_bandStart = m_bandEnd = event->position();
            m_bandModifiers = event->modifiers();
        }
    }
    event->accept();
}

//=============================================================================================================

void SceneViewport::mouseMoveEvent(QMouseEvent* event)
{
    if (m_drag == Drag::Pan) {
        panBy(event->position() - m_lastPosition);
        m_lastPosition = event->position();
    } else if (m_drag == Drag::Band) {
        m_bandEnd = event->position();
        update();
    }
    event->accept();
}

//=============================================================================================================

void SceneViewport::mouseReleaseEvent(QMouseEvent* event)
{
    const Drag drag = std::exchange(m_drag, Drag::None);
    if (drag == Drag::Item) {
        sceneReleased(mapToScene(event->position()));
    } else if (drag == Drag::Band) {
        m_bandEnd = event->position();
        const scene::Point a = mapToScene(m_bandStart);
        const scene::Point b = mapToScene(m_bandEnd);
        sceneBandSelected(scene::Rect{std::min(a.x, b.x), std::min(a.y, b.y), std::abs(b.x - a.x), std::abs(b.y - a.y)},
                          m_bandModifiers);
        update();
    }
    event->accept();
}

//=============================================================================================================

void SceneViewport::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        fitToContents();
    }
    event->accept();
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
    return QWidget::event(event);
}

//=============================================================================================================

void SceneViewport::applyFit()
{
    const scene::Rect bounds = sceneBounds();
    if (bounds.empty() || width() <= 2 * kFitMargin || height() <= 2 * kFitMargin) {
        return;
    }
    m_zoom = std::min((width() - 2 * kFitMargin) / bounds.width, (height() - 2 * kFitMargin) / bounds.height);
    m_offset = QPointF(width() / 2.0 - bounds.center().x * m_zoom, height() / 2.0 - bounds.center().y * m_zoom);
}
