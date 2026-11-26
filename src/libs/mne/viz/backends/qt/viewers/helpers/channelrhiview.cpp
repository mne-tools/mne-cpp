//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channelrhiview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of the ChannelRhiView QRhiWidget host of the ChannelRhiRenderer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channelrhiview.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

namespace
{

//=============================================================================================================
// Widget services for the renderer: frames are scheduled with QWidget::update(), sizes are the widget's.
//=============================================================================================================

class WidgetHost : public ChannelRhiHost
{
public:
    explicit WidgetHost(ChannelRhiView* view)
    : m_view(view)
    {
    }

    void requestFrame() override
    {
        m_view->update();
    }
    QSize logicalSize() const override
    {
        return m_view->size();
    }
    qreal surfaceDevicePixelRatio() const override
    {
        return m_view->devicePixelRatioF();
    }
    void setCursorShape(Qt::CursorShape shape) override
    {
        m_view->setCursor(shape);
    }
    void unsetCursorShape() override
    {
        m_view->unsetCursor();
    }
    void setMouseTracking(bool enabled) override
    {
        m_view->setMouseTracking(enabled);
    }
    QFont font() const override
    {
        return m_view->font();
    }

private:
    ChannelRhiView* m_view;
};

} // namespace

//=============================================================================================================

ChannelRhiView::ChannelRhiView(QWidget* parent)
: QRhiWidget(parent)
, m_pHost(std::make_unique<WidgetHost>(this))
, m_pRenderer(std::make_unique<ChannelRhiRenderer>(m_pHost.get()))
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setContextMenuPolicy(Qt::PreventContextMenu); // prevent right-click context menu

    // Platform-specific backend selection
#if defined(WASMBUILD) || defined(__EMSCRIPTEN__)
    setApi(QRhiWidget::Api::OpenGL); // WebGL 2
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    setApi(QRhiWidget::Api::Metal);
#elif defined(Q_OS_WIN)
    setApi(QRhiWidget::Api::Direct3D11);
#else
    setApi(QRhiWidget::Api::OpenGL);
#endif
    setSampleCount(1);
    // Force a native window so Metal/OpenGL can create their backing surface.
    // Without this, QRhiWidget may fail to obtain an NSView handle on macOS.
    setAttribute(Qt::WA_NativeWindow);

    ChannelRhiRenderer* r = m_pRenderer.get();
    connect(r, &ChannelRhiRenderer::scrollSampleChanged, this, &ChannelRhiView::scrollSampleChanged);
    connect(r, &ChannelRhiRenderer::samplesPerPixelChanged, this, &ChannelRhiView::samplesPerPixelChanged);
    connect(r, &ChannelRhiRenderer::viewResized, this, &ChannelRhiView::viewResized);
    connect(r, &ChannelRhiRenderer::channelOffsetChanged, this, &ChannelRhiView::channelOffsetChanged);
    connect(r, &ChannelRhiRenderer::sampleClicked, this, &ChannelRhiView::sampleClicked);
    connect(r, &ChannelRhiRenderer::sampleRangeSelected, this, &ChannelRhiView::sampleRangeSelected);
    connect(r, &ChannelRhiRenderer::annotationBoundaryMoved, this, &ChannelRhiView::annotationBoundaryMoved);
    connect(r, &ChannelRhiRenderer::cursorDataChanged, this, &ChannelRhiView::cursorDataChanged);
}

//=============================================================================================================

ChannelRhiView::~ChannelRhiView() = default;

//=============================================================================================================

void ChannelRhiView::initialize(QRhiCommandBuffer* cb)
{
    Q_UNUSED(cb);
    m_pRenderer->initializeResources();
}

//=============================================================================================================

void ChannelRhiView::render(QRhiCommandBuffer* cb)
{
    renderFrame(rhi(), renderTarget(), cb);
}

//=============================================================================================================

void ChannelRhiView::releaseResources()
{
    m_pRenderer->releaseResources();
}

//=============================================================================================================

void ChannelRhiView::renderFrame(QRhi* rhi, QRhiRenderTarget* target, QRhiCommandBuffer* cb)
{
    m_pRenderer->renderFrame(rhi, target, cb);
}

//=============================================================================================================

void ChannelRhiView::paintEvent(QPaintEvent* event)
{
    QRhiWidget::paintEvent(event);
}

//=============================================================================================================

void ChannelRhiView::resizeEvent(QResizeEvent* event)
{
    QRhiWidget::resizeEvent(event);
    m_pRenderer->handleResize();
}

//=============================================================================================================

void ChannelRhiView::wheelEvent(QWheelEvent* event)
{
    if (!m_pRenderer->handleWheel(event))
        QRhiWidget::wheelEvent(event);
}

//=============================================================================================================

void ChannelRhiView::mousePressEvent(QMouseEvent* event)
{
    if (!m_pRenderer->handleMousePress(event))
        QRhiWidget::mousePressEvent(event);
}

//=============================================================================================================

void ChannelRhiView::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_pRenderer->handleMouseMove(event))
        QRhiWidget::mouseMoveEvent(event);
}

//=============================================================================================================

void ChannelRhiView::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_pRenderer->handleMouseRelease(event))
        QRhiWidget::mouseReleaseEvent(event);
}

//=============================================================================================================

void ChannelRhiView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_pRenderer->handleMouseDoubleClick(event))
        QRhiWidget::mouseDoubleClickEvent(event);
}
