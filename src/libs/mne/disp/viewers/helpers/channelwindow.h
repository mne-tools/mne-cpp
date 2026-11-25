//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channelwindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Widgets-free QWindow host of the GPU raw-data channel trace view.
 *
 * ChannelWindow creates its own QRhi, swap chain and render pass descriptor, renders a
 * @ref mne::disp::ChannelRhiRenderer frame on every expose and update request and forwards its input
 * events to the renderer. It only needs QtGui. A Qt Widgets application embeds it with
 * @c createWindowContainer(channelWindow, parent).
 */

#ifndef DISP_CHANNELWINDOW_H
#define DISP_CHANNELWINDOW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include "channelrhirenderer.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWindow>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

class QOffscreenSurface;
class QRhi;
class QRhiRenderPassDescriptor;
class QRhiSwapChain;

namespace mne::disp
{

//=============================================================================================================
/**
 * @brief Widgets-free QWindow host of the GPU raw-data channel trace view.
 *
 * The view state and API live in renderer(); the window re-emits the renderer's signals. Embed it in a
 * widget layout with @c createWindowContainer() of Qt Widgets.
 */
class DISPSHARED_EXPORT ChannelWindow : public QWindow
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * @brief Graphics API used for the window's QRhi.
     */
    enum class Api
    {
        Default,    ///< Metal on macOS/iOS, Direct3D 11 on Windows, OpenGL elsewhere.
        Null,       ///< QRhi::Null: no output, for tests and headless runs.
        OpenGL,     ///< OpenGL (ES) 2+.
        Vulkan,     ///< Vulkan (only if Qt was built with Vulkan support).
        Metal,      ///< Metal (macOS/iOS only).
        Direct3D11, ///< Direct3D 11 (Windows only).
        Direct3D12  ///< Direct3D 12 (Windows only).
    };

    //=========================================================================================================
    /**
     * Constructs the window.
     *
     * @param[in] api     Graphics API of the QRhi created on first expose.
     * @param[in] parent  Optional parent window.
     */
    explicit ChannelWindow(Api api = Api::Default, QWindow* parent = nullptr);

    //=========================================================================================================
    /**
     * Releases the renderer's GPU resources, the swap chain and the QRhi.
     */
    ~ChannelWindow() override;

    //=========================================================================================================
    /**
     * Selects the graphics API. Only effective before the window is created (shown) for the first time.
     *
     * @param[in] api  Graphics API.
     */
    void setApi(Api api);

    //=========================================================================================================
    /**
     * Returns the selected graphics API (Default resolved to the platform API).
     *
     * @return The graphics API.
     */
    Api api() const
    {
        return m_api;
    }

    //=========================================================================================================
    /**
     * Returns the renderer that holds the view state and API (model, scroll, zoom, overlays, ...).
     *
     * @return The renderer owned by this window.
     */
    ChannelRhiRenderer* renderer() const
    {
        return m_pRenderer.get();
    }

    //=========================================================================================================
    /**
     * Returns the window's QRhi.
     *
     * @return The QRhi, or nullptr before the first expose or if its creation failed.
     */
    QRhi* rhi() const
    {
        return m_pRhi.get();
    }

    //=========================================================================================================
    /**
     * Returns the window's swap chain.
     *
     * @return The swap chain, or nullptr before the first expose.
     */
    QRhiSwapChain* swapChain() const
    {
        return m_pSwapChain.get();
    }

    //=========================================================================================================
    /**
     * Returns the number of frames rendered so far.
     *
     * @return Frame count.
     */
    int frameCount() const
    {
        return m_frameCount;
    }

signals:
    //=========================================================================================================
    /**
     * Re-emitted from ChannelRhiRenderer::scrollSampleChanged().
     *
     * @param[in] sample  New scroll position in samples.
     */
    void scrollSampleChanged(float sample);

    //=========================================================================================================
    /**
     * Re-emitted from ChannelRhiRenderer::samplesPerPixelChanged().
     *
     * @param[in] spp  New samples per pixel.
     */
    void samplesPerPixelChanged(float spp);

    //=========================================================================================================
    /**
     * Re-emitted from ChannelRhiRenderer::viewResized().
     *
     * @param[in] newWidth   New logical width.
     * @param[in] newHeight  New logical height.
     */
    void viewResized(int newWidth, int newHeight);

    //=========================================================================================================
    /**
     * Re-emitted from ChannelRhiRenderer::channelOffsetChanged().
     *
     * @param[in] firstChannel  New first visible logical channel.
     */
    void channelOffsetChanged(int firstChannel);

    //=========================================================================================================
    /**
     * Re-emitted from ChannelRhiRenderer::sampleClicked().
     *
     * @param[in] sample  Absolute sample index.
     */
    void sampleClicked(int sample);

    //=========================================================================================================
    /**
     * Re-emitted from ChannelRhiRenderer::sampleRangeSelected().
     *
     * @param[in] startSample  First selected sample.
     * @param[in] endSample    Last selected sample.
     */
    void sampleRangeSelected(int startSample, int endSample);

    //=========================================================================================================
    /**
     * Re-emitted from ChannelRhiRenderer::annotationBoundaryMoved().
     *
     * @param[in] annotationIndex  Index in the annotation span list.
     * @param[in] isStartBoundary  True if the start boundary was dragged.
     * @param[in] newSample        New absolute sample position of the boundary.
     */
    void annotationBoundaryMoved(int annotationIndex, bool isStartBoundary, int newSample);

    //=========================================================================================================
    /**
     * Re-emitted from ChannelRhiRenderer::cursorDataChanged().
     *
     * @param[in] timeSec     Time at cursor in seconds relative to file start.
     * @param[in] amplitude   Raw amplitude value under the cursor in physical units.
     * @param[in] channelName Name of the channel under the cursor.
     * @param[in] unitLabel   Short unit label.
     */
    void cursorDataChanged(float timeSec, float amplitude, const QString& channelName, const QString& unitLabel);

protected:
    //=========================================================================================================
    /**
     * Creates the QRhi and swap chain on the first expose and renders a frame.
     *
     * @param[in] event  The expose event.
     */
    void exposeEvent(QExposeEvent* event) override;

    //=========================================================================================================
    /**
     * Renders on update requests and releases the swap chain before the surface is destroyed.
     *
     * @param[in] event  The event.
     *
     * @return True if the event was recognized.
     */
    bool event(QEvent* event) override;

    //=========================================================================================================
    /**
     * Forwards the resize to the renderer.
     *
     * @param[in] event  The resize event.
     */
    void resizeEvent(QResizeEvent* event) override;

    //=========================================================================================================
    /**
     * Forwards the wheel event to the renderer.
     *
     * @param[in] event  The wheel event.
     */
    void wheelEvent(QWheelEvent* event) override;

    //=========================================================================================================
    /**
     * Forwards the mouse press to the renderer.
     *
     * @param[in] event  The mouse event.
     */
    void mousePressEvent(QMouseEvent* event) override;

    //=========================================================================================================
    /**
     * Forwards the mouse move to the renderer (moves without buttons only while mouse tracking is on).
     *
     * @param[in] event  The mouse event.
     */
    void mouseMoveEvent(QMouseEvent* event) override;

    //=========================================================================================================
    /**
     * Forwards the mouse release to the renderer.
     *
     * @param[in] event  The mouse event.
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

    //=========================================================================================================
    /**
     * Forwards the double click to the renderer.
     *
     * @param[in] event  The mouse event.
     */
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    class Host; // ChannelRhiHost implementation (channelwindow.cpp)

    void initRhi();
    void resizeSwapChain();
    void releaseSwapChain();
    void renderNow();

    Api m_api = Api::Default;
    bool m_mouseTracking = true;
    bool m_initialized = false;
    bool m_hasSwapChain = false;
    bool m_notExposed = false;
    bool m_newlyExposed = false;
    int m_frameCount = 0;

    std::unique_ptr<ChannelRhiHost> m_pHost;
    std::unique_ptr<ChannelRhiRenderer> m_pRenderer;
    std::unique_ptr<QOffscreenSurface> m_pFallbackSurface;
    std::unique_ptr<QRhi> m_pRhi;
    std::unique_ptr<QRhiSwapChain> m_pSwapChain;
    std::unique_ptr<QRhiRenderPassDescriptor> m_pRenderPass;
};

} // namespace mne::disp

#endif // DISP_CHANNELWINDOW_H
