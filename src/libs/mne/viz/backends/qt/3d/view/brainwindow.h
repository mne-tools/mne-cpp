//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainwindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    QWindow host of the 3-D brain view that owns its QRhi and swap chain and needs no Qt Widgets.
 *
 * BrainWindow shows a @ref mne::viz::qt::BrainViewCore in a plain QWindow: it
 * creates a QRhi for the platform (Metal on macOS, Direct3D 11 on Windows,
 * OpenGL elsewhere, or the Null backend for tests), a swap chain with a
 * depth-stencil buffer, renders on expose and on update requests, resizes
 * the swap chain with the window and forwards mouse, wheel and key events
 * to the core.
 *
 * Widget applications embed it in their widget tree through a window
 * container created with createWindowContainer() of the widget module;
 * the container takes ownership of the window. mne_viz_qt itself does not
 * depend on that module for this class.
 */

#ifndef DISP3D_BRAINWINDOW_H
#define DISP3D_BRAINWINDOW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"
#include "brainviewcore.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWindow>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QRhi;
class QRhiSwapChain;

namespace mne::viz::qt
{

//=============================================================================================================
/**
 * BrainWindow is a QWindow that renders a @ref BrainViewCore through its own QRhi and swap chain.
 * It needs only QtGui, so it also serves applications without Qt Widgets; widget applications
 * embed it with a window container.
 *
 * Clicking the name label of a multi-view pane emits viewportPresetMenuRequested(); the application
 * may offer the camera presets and apply the choice with @ref BrainViewCore::setViewportCameraPreset.
 *
 * @brief Widgets-free QWindow host of the 3-D brain view with its own QRhi, swap chain and input forwarding.
 */
class VIZQTSHARED_EXPORT BrainWindow : public QWindow
{
    Q_OBJECT

public:
    /**
     * Graphics API of the window's QRhi.
     */
    enum class GraphicsApi
    {
        Platform,   /**< Metal on macOS / iOS, Direct3D 11 on Windows, OpenGL (ES) elsewhere. */
        Null,       /**< QRhi::Null: records frames without a GPU, e.g. for tests. */
        OpenGL,     /**< OpenGL or OpenGL ES. */
        Metal,      /**< Metal (macOS / iOS). */
        Direct3D11, /**< Direct3D 11 (Windows). */
        Direct3D12  /**< Direct3D 12 (Windows). */
    };

    //=========================================================================================================
    /**
     * Constructor. The QRhi and the swap chain are created when the window is first exposed.
     *
     * @param[in] api        Graphics API of the window's QRhi.
     * @param[in] parent     Parent window.
     */
    explicit BrainWindow(GraphicsApi api = GraphicsApi::Platform, QWindow* parent = nullptr);

    //=========================================================================================================
    /**
     * Destructor. Frees the scene's GPU resources before the QRhi.
     */
    ~BrainWindow() override;

    //=========================================================================================================
    /**
     * @return The core holding scene, camera and overlay state; use it to load data and drive the view.
     */
    BrainViewCore* core() const;

    //=========================================================================================================
    /**
     * @return The graphics API resolved for this window (never GraphicsApi::Platform).
     */
    GraphicsApi graphicsApi() const;

    //=========================================================================================================
    /**
     * @return The window's QRhi, or nullptr before the first expose or if it could not be created.
     */
    QRhi* rhi() const;

    //=========================================================================================================
    /**
     * @return The window's swap chain, or nullptr before the first expose.
     */
    QRhiSwapChain* swapChain() const;

    //=========================================================================================================
    /**
     * @return Whether the swap chain has been created with a non-empty size.
     */
    bool hasSwapChain() const;

    //=========================================================================================================
    /**
     * @return Number of frames recorded since the window was created.
     */
    int frameCount() const;

    //=========================================================================================================
    /**
     * Render a frame now and read the back buffer back, including the 2-D overlay.
     * BrainViewCore::takeScreenshot() and BrainViewCore::savePng() leave the overlay out.
     *
     * @return The frame, or a null image when the window is not exposed or cannot render.
     */
    QImage grabFrame();

signals:
    //=========================================================================================================
    /**
     * Emitted when the user clicks the name label of a multi-view pane.
     *
     * @param[in] viewport   Index of the pane (0..3).
     * @param[in] globalPos  Click position in global screen coordinates, e.g. for a popup menu.
     */
    void viewportPresetMenuRequested(int viewport, const QPoint& globalPos);

protected:
    void exposeEvent(QExposeEvent* event) override;
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    class Host;
    struct Gpu;

    void initRhi();
    void releaseSwapChain();
    void resizeSwapChain();
    bool renderNow(QImage* readback = nullptr);

    GraphicsApi m_api;                      /**< Resolved graphics API. */
    std::unique_ptr<Host> m_pHost;          /**< Adapter through which the core reaches this window. */
    std::unique_ptr<Gpu> m_pGpu;            /**< QRhi, swap chain, depth-stencil and render pass descriptor. */
    std::unique_ptr<BrainViewCore> m_pCore; /**< Scene, camera, overlay and frame recording. */
    bool m_hasSwapChain = false;            /**< Swap chain created with a non-empty size. */
    bool m_rhiFailed = false;               /**< QRhi creation failed; do not retry. */
    bool m_newlyExposed = false;            /**< Resize the swap chain before the next frame. */
    bool m_captureWasVisible = false;       /**< Visibility before BrainViewHost::beginOffscreenCapture(). */
    int m_frameCount = 0;                   /**< Frames recorded. */
};

} // namespace mne::viz::qt

#endif // DISP3D_BRAINWINDOW_H
