//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channelwindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Implementation of the ChannelWindow QWindow host (QRhi, swap chain, frame loop, input forwarding).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channelwindow.h"
#include <mne/base/log.h>

#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QExposeEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QOffscreenSurface>
#include <QPlatformSurfaceEvent>
#include <QResizeEvent>
#include <QWheelEvent>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

namespace
{

ChannelWindow::Api resolveApi(ChannelWindow::Api api)
{
    if (api != ChannelWindow::Api::Default)
        return api;
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    return ChannelWindow::Api::Metal;
#elif defined(Q_OS_WIN)
    return ChannelWindow::Api::Direct3D11;
#else
    return ChannelWindow::Api::OpenGL;
#endif
}

QSurface::SurfaceType surfaceTypeFor(ChannelWindow::Api api)
{
    switch (api) {
        case ChannelWindow::Api::OpenGL:
            return QSurface::OpenGLSurface;
        case ChannelWindow::Api::Vulkan:
            return QSurface::VulkanSurface;
        case ChannelWindow::Api::Metal:
            return QSurface::MetalSurface;
        case ChannelWindow::Api::Direct3D11:
        case ChannelWindow::Api::Direct3D12:
            return QSurface::Direct3DSurface;
        default:
            return QSurface::RasterSurface;
    }
}

} // namespace

//=============================================================================================================
// Window services for the renderer: frames are scheduled with QWindow::requestUpdate().
//=============================================================================================================

class ChannelWindow::Host : public ChannelRhiHost
{
public:
    explicit Host(ChannelWindow* window)
    : m_window(window)
    {
    }

    void requestFrame() override
    {
        m_window->requestUpdate();
    }
    QSize logicalSize() const override
    {
        return m_window->size();
    }
    qreal surfaceDevicePixelRatio() const override
    {
        return m_window->devicePixelRatio();
    }
    void setCursorShape(Qt::CursorShape shape) override
    {
#ifndef QT_NO_CURSOR
        m_window->setCursor(shape);
#else
        Q_UNUSED(shape);
#endif
    }
    void unsetCursorShape() override
    {
#ifndef QT_NO_CURSOR
        m_window->unsetCursor();
#endif
    }
    void setMouseTracking(bool enabled) override
    {
        m_window->m_mouseTracking = enabled;
    }
    QFont font() const override
    {
        return QGuiApplication::font();
    }

private:
    ChannelWindow* m_window;
};

//=============================================================================================================

ChannelWindow::ChannelWindow(Api api, QWindow* parent)
: QWindow(parent)
, m_pHost(std::make_unique<Host>(this))
, m_pRenderer(std::make_unique<ChannelRhiRenderer>(m_pHost.get()))
{
    setApi(api);

    ChannelRhiRenderer* r = m_pRenderer.get();
    connect(r, &ChannelRhiRenderer::scrollSampleChanged, this, &ChannelWindow::scrollSampleChanged);
    connect(r, &ChannelRhiRenderer::samplesPerPixelChanged, this, &ChannelWindow::samplesPerPixelChanged);
    connect(r, &ChannelRhiRenderer::viewResized, this, &ChannelWindow::viewResized);
    connect(r, &ChannelRhiRenderer::channelOffsetChanged, this, &ChannelWindow::channelOffsetChanged);
    connect(r, &ChannelRhiRenderer::sampleClicked, this, &ChannelWindow::sampleClicked);
    connect(r, &ChannelRhiRenderer::sampleRangeSelected, this, &ChannelWindow::sampleRangeSelected);
    connect(r, &ChannelRhiRenderer::annotationBoundaryMoved, this, &ChannelWindow::annotationBoundaryMoved);
    connect(r, &ChannelRhiRenderer::cursorDataChanged, this, &ChannelWindow::cursorDataChanged);
}

//=============================================================================================================

ChannelWindow::~ChannelWindow()
{
    // GPU resources must go before the QRhi that created them
    m_pRenderer->releaseResources();
    releaseSwapChain();
    m_pRenderPass.reset();
    m_pSwapChain.reset();
    m_pRhi.reset();
    m_pFallbackSurface.reset();
}

//=============================================================================================================

void ChannelWindow::setApi(Api api)
{
    if (handle()) {
        mne::Log::warning("ChannelWindow: the graphics API cannot be changed after the window was created");
        return;
    }
    m_api = resolveApi(api);
    setSurfaceType(surfaceTypeFor(m_api));
}

//=============================================================================================================

void ChannelWindow::initRhi()
{
    switch (m_api) {
        case Api::Null: {
            QRhiNullInitParams params;
            m_pRhi.reset(QRhi::create(QRhi::Null, &params));
            break;
        }
#if QT_CONFIG(opengl)
        case Api::OpenGL: {
            m_pFallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface());
            QRhiGles2InitParams params;
            params.fallbackSurface = m_pFallbackSurface.get();
            params.window = this;
            m_pRhi.reset(QRhi::create(QRhi::OpenGLES2, &params));
            break;
        }
#endif
#if QT_CONFIG(vulkan) && __has_include(<vulkan/vulkan.h>)
        case Api::Vulkan: {
            if (!vulkanInstance()) {
                mne::Log::warning("ChannelWindow: Vulkan needs a QVulkanInstance set with QWindow::setVulkanInstance()");
                break;
            }
            QRhiVulkanInitParams params;
            params.inst = vulkanInstance();
            params.window = this;
            m_pRhi.reset(QRhi::create(QRhi::Vulkan, &params));
            break;
        }
#endif
#if QT_CONFIG(metal)
        case Api::Metal: {
            QRhiMetalInitParams params;
            m_pRhi.reset(QRhi::create(QRhi::Metal, &params));
            break;
        }
#endif
#if defined(Q_OS_WIN)
        case Api::Direct3D11: {
            QRhiD3D11InitParams params;
            m_pRhi.reset(QRhi::create(QRhi::D3D11, &params));
            break;
        }
        case Api::Direct3D12: {
            QRhiD3D12InitParams params;
            m_pRhi.reset(QRhi::create(QRhi::D3D12, &params));
            break;
        }
#endif
        default:
            break;
    }

    if (!m_pRhi) {
        mne::Log::warning("ChannelWindow: failed to create a QRhi for the selected graphics API");
        return;
    }

    m_pSwapChain.reset(m_pRhi->newSwapChain());
    m_pSwapChain->setWindow(this);
    m_pRenderPass.reset(m_pSwapChain->newCompatibleRenderPassDescriptor());
    m_pSwapChain->setRenderPassDescriptor(m_pRenderPass.get());
    m_pRenderer->initializeResources();
}

//=============================================================================================================

void ChannelWindow::resizeSwapChain()
{
    if (!m_pSwapChain)
        return;
    m_hasSwapChain = m_pSwapChain->createOrResize();
}

//=============================================================================================================

void ChannelWindow::releaseSwapChain()
{
    if (m_hasSwapChain) {
        m_hasSwapChain = false;
        m_pSwapChain->destroy();
    }
}

//=============================================================================================================

void ChannelWindow::renderNow()
{
    if (!m_hasSwapChain || m_notExposed)
        return;

    if (m_pSwapChain->currentPixelSize() != m_pSwapChain->surfacePixelSize() || m_newlyExposed) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        m_newlyExposed = false;
    }

    QRhi::FrameOpResult result = m_pRhi->beginFrame(m_pSwapChain.get());
    if (result == QRhi::FrameOpSwapChainOutOfDate) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        result = m_pRhi->beginFrame(m_pSwapChain.get());
    }
    if (result != QRhi::FrameOpSuccess) {
        requestUpdate();
        return;
    }

    m_pRenderer->renderFrame(m_pRhi.get(), m_pSwapChain->currentFrameRenderTarget(),
                             m_pSwapChain->currentFrameCommandBuffer());
    m_pRhi->endFrame(m_pSwapChain.get());
    ++m_frameCount;
}

//=============================================================================================================

void ChannelWindow::exposeEvent(QExposeEvent* event)
{
    Q_UNUSED(event);

    if (isExposed() && !m_initialized) {
        initRhi();
        resizeSwapChain();
        m_initialized = true;
    }

    const QSize surfaceSize = m_hasSwapChain ? m_pSwapChain->surfacePixelSize() : QSize();

    // Stop rendering while the window is hidden or minimised
    if ((!isExposed() || (m_hasSwapChain && surfaceSize.isEmpty())) && m_initialized && !m_notExposed)
        m_notExposed = true;

    // Resize the swap chain on the next frame after the window became visible again
    if (isExposed() && m_initialized && m_notExposed && !surfaceSize.isEmpty()) {
        m_notExposed = false;
        m_newlyExposed = true;
    }

    if (isExposed() && !surfaceSize.isEmpty())
        renderNow();
}

//=============================================================================================================

bool ChannelWindow::event(QEvent* event)
{
    switch (event->type()) {
        case QEvent::UpdateRequest:
            renderNow();
            break;
        case QEvent::PlatformSurface:
            if (static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
                releaseSwapChain();
            break;
        default:
            break;
    }
    return QWindow::event(event);
}

//=============================================================================================================

void ChannelWindow::resizeEvent(QResizeEvent* event)
{
    QWindow::resizeEvent(event);
    m_pRenderer->handleResize();
}

//=============================================================================================================

void ChannelWindow::wheelEvent(QWheelEvent* event)
{
    if (!m_pRenderer->handleWheel(event))
        QWindow::wheelEvent(event);
}

//=============================================================================================================

void ChannelWindow::mousePressEvent(QMouseEvent* event)
{
    if (!m_pRenderer->handleMousePress(event))
        QWindow::mousePressEvent(event);
}

//=============================================================================================================

void ChannelWindow::mouseMoveEvent(QMouseEvent* event)
{
    // QWindow always delivers hover moves; emulate the mouse tracking of a widget
    if (!m_mouseTracking && event->buttons() == Qt::NoButton) {
        QWindow::mouseMoveEvent(event);
        return;
    }
    if (!m_pRenderer->handleMouseMove(event))
        QWindow::mouseMoveEvent(event);
}

//=============================================================================================================

void ChannelWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_pRenderer->handleMouseRelease(event))
        QWindow::mouseReleaseEvent(event);
}

//=============================================================================================================

void ChannelWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_pRenderer->handleMouseDoubleClick(event))
        QWindow::mouseDoubleClickEvent(event);
}
