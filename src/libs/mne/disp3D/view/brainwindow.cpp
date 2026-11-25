//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainwindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    BrainWindow definition: QRhi and swap-chain management, expose-driven rendering and input forwarding.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainwindow.h"

#include <mne/base/log.h>

#include <rhi/qrhi.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCursor>
#include <QExposeEvent>
#include <QMouseEvent>
#include <QOffscreenSurface>
#include <QPlatformSurfaceEvent>

namespace mne::disp
{

namespace
{

BrainWindow::GraphicsApi resolvedApi(BrainWindow::GraphicsApi api)
{
    if (api != BrainWindow::GraphicsApi::Platform) {
        return api;
    }
#if defined(WASMBUILD) || defined(__EMSCRIPTEN__)
    return BrainWindow::GraphicsApi::OpenGL; // WebGL 2 (OpenGL ES 3.0) on WASM
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    return BrainWindow::GraphicsApi::Metal;
#elif defined(Q_OS_WIN)
    return BrainWindow::GraphicsApi::Direct3D11;
#else
    return BrainWindow::GraphicsApi::OpenGL;
#endif
}

QSurface::SurfaceType surfaceTypeFor(BrainWindow::GraphicsApi api)
{
    switch (api) {
        case BrainWindow::GraphicsApi::OpenGL:
            return QSurface::OpenGLSurface;
        case BrainWindow::GraphicsApi::Metal:
            return QSurface::MetalSurface;
        case BrainWindow::GraphicsApi::Direct3D11:
        case BrainWindow::GraphicsApi::Direct3D12:
            return QSurface::Direct3DSurface;
        case BrainWindow::GraphicsApi::Platform:
        case BrainWindow::GraphicsApi::Null:
            break;
    }
    return QSurface::RasterSurface;
}

} // namespace

//=============================================================================================================
/**
 * @brief GPU objects of a BrainWindow, destroyed in reverse order of creation.
 */
struct BrainWindow::Gpu
{
    std::unique_ptr<QOffscreenSurface> fallbackSurface;
    std::unique_ptr<QRhi> rhi;
    std::unique_ptr<QRhiSwapChain> swapChain;
    std::unique_ptr<QRhiRenderBuffer> depthStencil;
    std::unique_ptr<QRhiRenderPassDescriptor> renderPass;
};

//=============================================================================================================
/**
 * @brief Connects a BrainViewCore to its BrainWindow.
 */
class BrainWindow::Host : public BrainViewHost
{
public:
    explicit Host(BrainWindow& window)
    : m_window(window)
    {
    }

    void requestUpdate() override
    {
        m_window.requestUpdate();
    }

    QSize logicalSize() const override
    {
        return m_window.size();
    }

    qreal devicePixelRatio() const override
    {
        return m_window.devicePixelRatio();
    }

    bool isVisible() const override
    {
        return m_window.isVisible();
    }

    void setCursorShape(Qt::CursorShape shape) override
    {
#ifndef QT_NO_CURSOR
        if (shape == Qt::ArrowCursor) {
            m_window.unsetCursor();
        } else {
            m_window.setCursor(QCursor(shape));
        }
#else
        Q_UNUSED(shape);
#endif
    }

    QImage grabFrame() override
    {
        return m_window.grabFrame();
    }

    void requestViewportPresetMenu(int viewport, const QPoint& pos) override
    {
        emit m_window.viewportPresetMenuRequested(viewport, m_window.mapToGlobal(pos));
    }

    bool beginOffscreenCapture(const QSize& size) override
    {
        // A QWindow has no "do not show on screen" mode: show it (if hidden) at the capture size
        m_window.m_captureWasVisible = m_window.isVisible();
        m_window.resize(size);
        if (!m_window.m_captureWasVisible) {
            m_window.show();
        }
        return true;
    }

    void endOffscreenCapture() override
    {
        if (!m_window.m_captureWasVisible) {
            m_window.hide();
        }
    }

private:
    BrainWindow& m_window;
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainWindow::BrainWindow(GraphicsApi api, QWindow* parent)
: QWindow(parent)
, m_api(resolvedApi(api))
, m_pHost(std::make_unique<Host>(*this))
, m_pGpu(std::make_unique<Gpu>())
{
    setSurfaceType(surfaceTypeFor(m_api));
    m_pCore = std::make_unique<BrainViewCore>(m_pHost.get());
}

//=============================================================================================================

BrainWindow::~BrainWindow()
{
    // Scene resources first, then the swap chain, then the QRhi that created them
    m_pCore->releaseResources();
    m_pCore.reset();
    releaseSwapChain();
    m_pGpu.reset();
}

//=============================================================================================================

BrainViewCore* BrainWindow::core() const
{
    return m_pCore.get();
}

//=============================================================================================================

BrainWindow::GraphicsApi BrainWindow::graphicsApi() const
{
    return m_api;
}

//=============================================================================================================

QRhi* BrainWindow::rhi() const
{
    return m_pGpu->rhi.get();
}

//=============================================================================================================

QRhiSwapChain* BrainWindow::swapChain() const
{
    return m_pGpu->swapChain.get();
}

//=============================================================================================================

bool BrainWindow::hasSwapChain() const
{
    return m_hasSwapChain;
}

//=============================================================================================================

int BrainWindow::frameCount() const
{
    return m_frameCount;
}

//=============================================================================================================

QImage BrainWindow::grabFrame()
{
    QImage frame;
    if (!renderNow(&frame)) {
        return {};
    }
    return frame;
}

//=============================================================================================================

void BrainWindow::initRhi()
{
    Gpu& gpu = *m_pGpu;
    switch (m_api) {
        case GraphicsApi::Null: {
            QRhiNullInitParams params;
            gpu.rhi.reset(QRhi::create(QRhi::Null, &params));
            break;
        }
        case GraphicsApi::OpenGL: {
#if QT_CONFIG(opengl)
            gpu.fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface());
            QRhiGles2InitParams params;
            params.fallbackSurface = gpu.fallbackSurface.get();
            params.window = this;
            gpu.rhi.reset(QRhi::create(QRhi::OpenGLES2, &params));
#endif
            break;
        }
        case GraphicsApi::Metal: {
#if QT_CONFIG(metal)
            QRhiMetalInitParams params;
            gpu.rhi.reset(QRhi::create(QRhi::Metal, &params));
#endif
            break;
        }
        case GraphicsApi::Direct3D11: {
#ifdef Q_OS_WIN
            QRhiD3D11InitParams params;
            gpu.rhi.reset(QRhi::create(QRhi::D3D11, &params));
#endif
            break;
        }
        case GraphicsApi::Direct3D12: {
#ifdef Q_OS_WIN
            QRhiD3D12InitParams params;
            gpu.rhi.reset(QRhi::create(QRhi::D3D12, &params));
#endif
            break;
        }
        case GraphicsApi::Platform:
            break;
    }

    if (!gpu.rhi) {
        mne::Log::warning("BrainWindow: could not create a QRhi for graphics API {}", static_cast<int>(m_api));
        m_rhiFailed = true;
        return;
    }

    gpu.swapChain.reset(gpu.rhi->newSwapChain());
    gpu.depthStencil.reset(gpu.rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, QSize(), 1,
                                                    QRhiRenderBuffer::UsedWithSwapChainOnly));
    gpu.swapChain->setWindow(this);
    gpu.swapChain->setDepthStencil(gpu.depthStencil.get());
    // Allows reading the back buffer for grabFrame()
    gpu.swapChain->setFlags(QRhiSwapChain::UsedAsTransferSource);
    gpu.renderPass.reset(gpu.swapChain->newCompatibleRenderPassDescriptor());
    gpu.swapChain->setRenderPassDescriptor(gpu.renderPass.get());
}

//=============================================================================================================

void BrainWindow::resizeSwapChain()
{
    // Also resizes the depth-stencil buffer
    m_hasSwapChain = m_pGpu->swapChain && m_pGpu->swapChain->createOrResize();
}

//=============================================================================================================

void BrainWindow::releaseSwapChain()
{
    if (m_hasSwapChain) {
        m_hasSwapChain = false;
        m_pGpu->swapChain->destroy();
    }
}

//=============================================================================================================

bool BrainWindow::renderNow(QImage* readback)
{
    Gpu& gpu = *m_pGpu;
    if (!isExposed() || !gpu.rhi || !gpu.swapChain) {
        return false;
    }

    if (!m_hasSwapChain || gpu.swapChain->currentPixelSize() != gpu.swapChain->surfacePixelSize() || m_newlyExposed) {
        resizeSwapChain();
        if (!m_hasSwapChain) {
            return false;
        }
        m_newlyExposed = false;
    }

    QRhi::FrameOpResult result = gpu.rhi->beginFrame(gpu.swapChain.get());
    if (result == QRhi::FrameOpSwapChainOutOfDate) {
        resizeSwapChain();
        if (!m_hasSwapChain) {
            return false;
        }
        result = gpu.rhi->beginFrame(gpu.swapChain.get());
    }
    if (result != QRhi::FrameOpSuccess) {
        requestUpdate();
        return false;
    }

    QRhiCommandBuffer* cb = gpu.swapChain->currentFrameCommandBuffer();
    m_pCore->render(gpu.rhi.get(), gpu.swapChain->currentFrameRenderTarget(), cb);

    QRhiReadbackResult readbackResult;
    if (readback) {
        // An empty description reads the current back buffer
        QRhiResourceUpdateBatch* updates = gpu.rhi->nextResourceUpdateBatch();
        updates->readBackTexture(QRhiReadbackDescription(), &readbackResult);
        cb->resourceUpdate(updates);
    }

    gpu.rhi->endFrame(gpu.swapChain.get());
    ++m_frameCount;

    if (readback) {
        // Completes the readback
        gpu.rhi->finish();
        const QSize size = readbackResult.pixelSize;
        if (readbackResult.data.isEmpty() || size.isEmpty()) {
            *readback = QImage();
            return false;
        }
        const QImage::Format format = (readbackResult.format == QRhiTexture::BGRA8) ? QImage::Format_ARGB32_Premultiplied
                                                                                    : QImage::Format_RGBA8888_Premultiplied;
        const QImage wrapped(reinterpret_cast<const uchar*>(readbackResult.data.constData()), size.width(), size.height(), format);
        *readback = gpu.rhi->isYUpInFramebuffer() ? wrapped.flipped(Qt::Vertical) : wrapped.copy();
        readback->setDevicePixelRatio(devicePixelRatio());
    }
    return true;
}

//=============================================================================================================

void BrainWindow::exposeEvent(QExposeEvent* event)
{
    Q_UNUSED(event);
    if (isExposed() && !m_pGpu->rhi && !m_rhiFailed) {
        initRhi();
        m_newlyExposed = true;
    }
    if (!isExposed()) {
        return;
    }
    // Re-exposed windows (e.g. un-minimised) get a fresh swap chain size
    if (!m_hasSwapChain) {
        m_newlyExposed = true;
    }
    renderNow();
}

//=============================================================================================================

bool BrainWindow::event(QEvent* event)
{
    switch (event->type()) {
        case QEvent::UpdateRequest:
            renderNow();
            break;
        case QEvent::PlatformSurface:
            // The native window is about to go away; the swap chain must be released before
            if (static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
                releaseSwapChain();
            }
            break;
        default:
            break;
    }
    return QWindow::event(event);
}

//=============================================================================================================

void BrainWindow::resizeEvent(QResizeEvent* event)
{
    QWindow::resizeEvent(event);
    m_pCore->handleResize();
    requestUpdate();
}

//=============================================================================================================

void BrainWindow::mousePressEvent(QMouseEvent* event)
{
    m_pCore->handleMousePress(event);
}

//=============================================================================================================

void BrainWindow::mouseMoveEvent(QMouseEvent* event)
{
    m_pCore->handleMouseMove(event);
}

//=============================================================================================================

void BrainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    m_pCore->handleMouseRelease(event);
}

//=============================================================================================================

void BrainWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_pCore->handleMouseDoubleClick(event)) {
        QWindow::mouseDoubleClickEvent(event);
    }
}

//=============================================================================================================

void BrainWindow::wheelEvent(QWheelEvent* event)
{
    m_pCore->handleWheel(event);
}

//=============================================================================================================

void BrainWindow::keyPressEvent(QKeyEvent* event)
{
    m_pCore->handleKeyPress(event);
}

} // namespace mne::disp
