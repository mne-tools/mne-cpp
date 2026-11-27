//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_disp3d_brain_window.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests for the Widgets-free BrainViewCore and its QWindow host BrainWindow on the QRhi Null backend.
 *           Covers: frame recording into an offscreen texture and into an external render target, the 2-D overlay
 *           that replaces BrainView's child labels and separators, BrainWindow swap-chain rendering and input forwarding.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/backends/qt/plots/helpers/rhiframerenderer.h>
#include <mne/viz/backends/qt/3d/view/brainviewcore.h>
#include <mne/viz/backends/qt/3d/view/brainwindow.h>
#include <mne/viz/backends/qt/3d/model/braintreemodel.h>
#include <mne/fs/fs_surface.h>
#include <mne/viz/gpu/brain_frame.h>
#include <mne/viz/figures/canvas.h>
#include <rhi/qrhi.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QMouseEvent>
#include <QSettings>
#include <QWheelEvent>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;
using namespace mne::viz::qt;

namespace
{

//=============================================================================================================
/**
 * @brief Host double that records what the core asks of its surface.
 */
class TestHost : public BrainViewHost
{
public:
    void requestUpdate() override
    {
        ++updates;
    }

    QSize logicalSize() const override
    {
        return size;
    }

    qreal devicePixelRatio() const override
    {
        return dpr;
    }

    bool isVisible() const override
    {
        return visible;
    }

    void setCursorShape(Qt::CursorShape shape) override
    {
        cursor = shape;
    }

    QImage grabFrame() override
    {
        return {};
    }

    void requestViewportPresetMenu(int viewport, const QPoint& pos) override
    {
        Q_UNUSED(pos);
        menuViewport = viewport;
    }

    QSize size = QSize(800, 600);
    qreal dpr = 1.0;
    bool visible = true;
    int updates = 0;
    int menuViewport = -1;
    Qt::CursorShape cursor = Qt::ArrowCursor;
};

// Number of pixels in a rectangle of the image that are not fully transparent
int paintedPixels(const QImage& image, const QRect& rect)
{
    int count = 0;
    const QRect clipped = rect.intersected(image.rect());
    for (int y = clipped.top(); y <= clipped.bottom(); ++y) {
        for (int x = clipped.left(); x <= clipped.right(); ++x) {
            count += qAlpha(image.pixel(x, y)) > 0 ? 1 : 0;
        }
    }
    return count;
}

void sendMouse(QObject* target, QEvent::Type type, const QPoint& pos, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    QMouseEvent event(type, QPointF(pos), QPointF(pos), button, buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(target, &event);
}

void sendWheel(QObject* target, const QPoint& pos)
{
    QWheelEvent event(QPointF(pos), QPointF(pos), QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(target, &event);
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestDisp3dBrainWindow
 *
 * @brief The TestDisp3dBrainWindow class tests BrainViewCore and BrainWindow headlessly on the QRhi Null backend.
 */
class TestDisp3dBrainWindow : public QObject
{
    Q_OBJECT

private slots:

    //=========================================================================================================
    /**
     * Called once before any test function: isolates the persisted view layout.
     */
    void initTestCase();

    //=========================================================================================================
    /**
     * Called once after all test functions have run.
     */
    void cleanupTestCase();

    //=========================================================================================================
    /**
     * Verifies that the core records a frame with a surface into an offscreen texture and into an external
     * render target, and fills the pane info label while doing so.
     */
    void core_rendersFrameWithNullRhi();

    //=========================================================================================================
    /**
     * Verifies that the overlay image carries the FPS, info and pane-name labels and the pane separators
     * where BrainView's child widgets used to be, with the same visibility rules.
     */
    void core_overlayReplacesChildLabels();

    //=========================================================================================================
    /**
     * Verifies that BrainWindow creates its swap chain on the Null backend, renders on expose and on update
     * requests, follows resizes and reads frames back.
     */
    void window_swapChainOnNullRhi();

    //=========================================================================================================
    /**
     * Verifies that wheel and drag events sent to BrainWindow reach the core's camera.
     */
    void window_forwardsInput();

    //=========================================================================================================
    /**
     * The brain view's frames feed every uniform block, sampler and vertex input of the mne_viz_qt shaders.
     */
    void frames_matchTheirShaders();

private:
    QString m_dataDir;
};

//=============================================================================================================

void TestDisp3dBrainWindow::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("MNE-CPP-Tests"));
    QCoreApplication::setApplicationName(QStringLiteral("test_disp3d_brain_window"));
    QSettings().clear();
    m_dataDir = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data/");
}

//=============================================================================================================

void TestDisp3dBrainWindow::cleanupTestCase()
{
    QSettings().clear();
}

//=============================================================================================================

void TestDisp3dBrainWindow::core_rendersFrameWithNullRhi()
{
    const QString surfPath = m_dataDir + QStringLiteral("subjects/sample/surf/lh.white");
    if (!QFileInfo::exists(surfPath)) {
        QSKIP("mne-cpp-test-data is not available");
    }

    QRhiNullInitParams params;
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::Null, &params));
    QVERIFY(rhi);

    TestHost host;
    host.size = QSize(320, 240);
    BrainViewCore core(&host);
    core.resetAllSubViewState();
    core.setInfoPanelVisible(true);
    core.setViewCount(1);
    QVERIFY(!core.hasRenderer());

    BrainTreeModel model;
    core.setModel(&model);
    const mne::FsSurface lh(surfPath.toStdU16String());
    model.addSurface(QStringLiteral("sample"), QStringLiteral("lh"), QStringLiteral("white"), lh);
    core.setActiveSurface("white");

    std::unique_ptr<QRhiTexture> color(rhi->newTexture(QRhiTexture::RGBA8, host.size, 1,
                                                       QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(color->create());

    QRhiCommandBuffer* cb = nullptr;
    QCOMPARE(rhi->beginOffscreenFrame(&cb), QRhi::FrameOpSuccess);
    core.render(rhi.get(), color.get(), 1, cb);
    QRhiReadbackResult readback;
    QRhiResourceUpdateBatch* updates = rhi->nextResourceUpdateBatch();
    updates->readBackTexture(QRhiReadbackDescription(color.get()), &readback);
    cb->resourceUpdate(updates);
    QCOMPARE(rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);

    QVERIFY(core.hasRenderer());
    QCOMPARE(readback.pixelSize, host.size);
    // The single-view info label is filled while the frame is recorded
    const QString info = core.overlayText(BrainViewCore::SingleViewInfoLabel);
    QVERIFY2(info.contains(QStringLiteral("Surface: white")), qPrintable(info));
    QVERIFY2(!info.contains(QStringLiteral("Drawn: none")), qPrintable(info));
    QVERIFY(core.isOverlayItemVisible(BrainViewCore::SingleViewInfoLabel));

    // The same core records into a render target owned by the caller, as BrainWindow does with its swap chain
    std::unique_ptr<QRhiRenderBuffer> depth(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, host.size));
    QVERIFY(depth->create());
    QRhiTextureRenderTargetDescription desc{QRhiColorAttachment(color.get())};
    desc.setDepthStencilBuffer(depth.get());
    std::unique_ptr<QRhiTextureRenderTarget> target(rhi->newTextureRenderTarget(desc));
    std::unique_ptr<QRhiRenderPassDescriptor> pass(target->newCompatibleRenderPassDescriptor());
    target->setRenderPassDescriptor(pass.get());
    QVERIFY(target->create());
    QCOMPARE(rhi->beginOffscreenFrame(&cb), QRhi::FrameOpSuccess);
    core.render(rhi.get(), target.get(), cb);
    QCOMPARE(rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);

    core.releaseResources();
    QVERIFY(!core.hasRenderer());
    core.setModel(nullptr);
}

//=============================================================================================================

void TestDisp3dBrainWindow::core_overlayReplacesChildLabels()
{
    TestHost host;
    BrainViewCore core(&host);
    core.resetAllSubViewState();
    core.resetMultiViewLayout();
    core.setInfoPanelVisible(true);
    core.setViewCount(4);
    QCOMPARE(core.viewMode(), BrainViewCore::MultiView);

    // Pane names sit 8 px into each pane of the 2x2 layout, the info labels top right of the panes
    QCOMPARE(core.overlayText(BrainViewCore::ViewportNameLabel, 0), QStringLiteral("Top"));
    QCOMPARE(core.overlayGeometry(BrainViewCore::ViewportNameLabel, 0).topLeft(), QPoint(8, 8));
    for (int vp = 0; vp < 4; ++vp) {
        QVERIFY(core.isOverlayItemVisible(BrainViewCore::ViewportNameLabel, vp));
        QVERIFY(core.isOverlayItemVisible(BrainViewCore::ViewportInfoLabel, vp));
    }
    const QRect name3 = core.overlayGeometry(BrainViewCore::ViewportNameLabel, 3);
    QVERIFY2(name3.left() > 400 && name3.top() > 300, qPrintable(QStringLiteral("%1,%2").arg(name3.left()).arg(name3.top())));
    QCOMPARE(core.overlayGeometry(BrainViewCore::ViewportInfoLabel, 0).top(), 8);
    QVERIFY(core.overlayGeometry(BrainViewCore::ViewportInfoLabel, 0).right() < 400 - 8);
    QVERIFY(!core.isOverlayItemVisible(BrainViewCore::SingleViewInfoLabel));
    QVERIFY(!core.isOverlayItemVisible(BrainViewCore::RegionLabel));

    // FPS label bottom right of the whole surface
    const QRect fps = core.overlayGeometry(BrainViewCore::FpsLabel);
    QVERIFY(core.isOverlayItemVisible(BrainViewCore::FpsLabel));
    QVERIFY(core.overlayText(BrainViewCore::FpsLabel).startsWith(QStringLiteral("FPS: ")));
    QCOMPARE(fps.right(), 800 - 10 - 1);
    QCOMPARE(fps.bottom(), 600 - 2 - 1);

    // Separators between the panes
    QVERIFY(core.isOverlayItemVisible(BrainViewCore::VerticalSeparator));
    QVERIFY(core.isOverlayItemVisible(BrainViewCore::HorizontalSeparator));
    const QRect vSep = core.overlayGeometry(BrainViewCore::VerticalSeparator);
    const QRect hSep = core.overlayGeometry(BrainViewCore::HorizontalSeparator);
    QVERIFY(vSep.contains(QPoint(vSep.center().x(), 100)));
    QVERIFY(qAbs(vSep.center().x() - 400) <= 2);
    QVERIFY(qAbs(hSep.center().y() - 300) <= 2);

    // The overlay image carries all of them, and nothing elsewhere
    const QImage overlay = core.overlayImage(QSize(800, 600), 1.0);
    QCOMPARE(overlay.size(), QSize(800, 600));
    QVERIFY(qAlpha(overlay.pixel(vSep.center().x(), 100)) > 0);
    QVERIFY(qAlpha(overlay.pixel(100, hSep.center().y())) > 0);
    QCOMPARE(qAlpha(overlay.pixel(200, 200)), 0);
    QVERIFY(paintedPixels(overlay, core.overlayGeometry(BrainViewCore::ViewportNameLabel, 0)) > 0);
    QVERIFY(paintedPixels(overlay, core.overlayGeometry(BrainViewCore::ViewportInfoLabel, 1)) > 0);
    QVERIFY(paintedPixels(overlay, fps) > 0);

    // On a high-DPI surface the overlay is painted in logical coordinates at twice the resolution
    const QImage hiDpi = core.overlayImage(QSize(1600, 1200), 2.0);
    QVERIFY(qAlpha(hiDpi.pixel(2 * vSep.center().x(), 200)) > 0);
    QCOMPARE(qAlpha(hiDpi.pixel(400, 400)), 0);

    // Hiding the info panel hides FPS and pane info but keeps the pane names
    const int updatesBefore = host.updates;
    core.setInfoPanelVisible(false);
    QVERIFY(host.updates > updatesBefore);
    QVERIFY(!core.isOverlayItemVisible(BrainViewCore::FpsLabel));
    QVERIFY(!core.isOverlayItemVisible(BrainViewCore::ViewportInfoLabel, 0));
    QVERIFY(core.isOverlayItemVisible(BrainViewCore::ViewportNameLabel, 0));
    const QImage noInfo = core.overlayImage(QSize(800, 600), 1.0);
    QCOMPARE(paintedPixels(noInfo, fps), 0);
    core.setInfoPanelVisible(true);

    // The selected pane's name is highlighted with a larger label
    core.setVisualizationEditTarget(0);
    const int plainWidth = core.overlayGeometry(BrainViewCore::ViewportNameLabel, 2).width();
    core.setVisualizationEditTarget(2);
    QVERIFY(core.overlayGeometry(BrainViewCore::ViewportNameLabel, 2).width() > plainWidth);

    // Clicking a pane name selects the pane and asks the host for the preset menu, only while shown
    const QPoint nameCentre = core.overlayGeometry(BrainViewCore::ViewportNameLabel, 1).center();
    host.visible = false;
    QMouseEvent hiddenPress(QEvent::MouseButtonPress, QPointF(nameCentre), QPointF(nameCentre), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    core.handleMousePress(&hiddenPress);
    QCOMPARE(host.menuViewport, -1);
    host.visible = true;
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(nameCentre), QPointF(nameCentre), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    core.handleMousePress(&press);
    QCOMPARE(host.menuViewport, 1);
    QCOMPARE(core.visualizationEditTarget(), 1);

    // The splitter cursor follows the pointer over a separator
    QMouseEvent hover(QEvent::MouseMove, QPointF(vSep.center().x(), 100), QPointF(vSep.center().x(), 100), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    core.handleMouseMove(&hover);
    QVERIFY(host.cursor != Qt::ArrowCursor);

    // A resized host moves the labels with it
    host.size = QSize(1000, 700);
    core.handleResize();
    QCOMPARE(core.overlayGeometry(BrainViewCore::FpsLabel).right(), 1000 - 10 - 1);

    // Single view: no pane names or separators, the single-view info instead
    core.setViewCount(1);
    QVERIFY(!core.isOverlayItemVisible(BrainViewCore::ViewportNameLabel, 0));
    QVERIFY(!core.isOverlayItemVisible(BrainViewCore::VerticalSeparator));
    QVERIFY(!core.isOverlayItemVisible(BrainViewCore::HorizontalSeparator));
    QVERIFY(core.isOverlayItemVisible(BrainViewCore::SingleViewInfoLabel));
    QCOMPARE(core.overlayGeometry(BrainViewCore::SingleViewInfoLabel).top(), 8);
    QCOMPARE(core.overlayGeometry(BrainViewCore::RegionLabel), QRect(10, 10, 300, 30));
}

//=============================================================================================================

void TestDisp3dBrainWindow::window_swapChainOnNullRhi()
{
    BrainWindow window(BrainWindow::GraphicsApi::Null);
    QCOMPARE(window.graphicsApi(), BrainWindow::GraphicsApi::Null);
    QVERIFY(window.core());
    QVERIFY(!window.rhi());
    QVERIFY(window.grabFrame().isNull());

    window.resize(320, 240);
    window.show();
    QTRY_VERIFY(window.hasSwapChain());
    QVERIFY(window.rhi());
    QCOMPARE(window.rhi()->backend(), QRhi::Null);
    QVERIFY(window.swapChain());
    QTRY_VERIFY(window.frameCount() > 0);
    QVERIFY(window.core()->hasRenderer());
    // The Null backend reports a fixed surface size instead of the window's
    const QSize pixelSize = window.swapChain()->currentPixelSize();
    QVERIFY(!pixelSize.isEmpty());

    // Update requests from the core render a new frame
    int frames = window.frameCount();
    window.core()->setInfoPanelVisible(false);
    QTRY_VERIFY(window.frameCount() > frames);
    window.core()->setInfoPanelVisible(true);

    // A grab renders synchronously and reads the back buffer of the window's size
    frames = window.frameCount();
    const QImage frame = window.grabFrame();
    QCOMPARE(window.frameCount(), frames + 1);
    if (!frame.isNull()) {
        QCOMPARE(frame.size(), pixelSize);
    }

    // The overlay layout follows the window size, and the resized window renders again
    frames = window.frameCount();
    window.resize(400, 300);
    QTRY_COMPARE(window.core()->overlayGeometry(BrainViewCore::FpsLabel).right(), 400 - 10 - 1);
    QTRY_VERIFY(window.frameCount() > frames);
    QVERIFY(window.hasSwapChain());

    window.hide();
}

//=============================================================================================================

void TestDisp3dBrainWindow::window_forwardsInput()
{
    BrainWindow window(BrainWindow::GraphicsApi::Null);
    window.resize(400, 300);
    window.show();
    QTRY_VERIFY(window.hasSwapChain());
    BrainViewCore* core = window.core();
    core->resetAllSubViewState();
    core->setViewCount(1);

    // Wheel zooms the single view
    const float zoom = core->zoomForTarget(-1);
    sendWheel(&window, QPoint(200, 150));
    QCOMPARE(core->zoomForTarget(-1), zoom + 1.0f);

    // A left-button drag rotates the camera and renders again
    const QQuaternion rotation = core->cameraRotation();
    const int frames = window.frameCount();
    sendMouse(&window, QEvent::MouseButtonPress, QPoint(200, 150), Qt::LeftButton, Qt::LeftButton);
    sendMouse(&window, QEvent::MouseMove, QPoint(240, 150), Qt::NoButton, Qt::LeftButton);
    sendMouse(&window, QEvent::MouseButtonRelease, QPoint(240, 150), Qt::LeftButton, Qt::NoButton);
    QVERIFY(core->cameraRotation() != rotation);
    QTRY_VERIFY(window.frameCount() > frames);

    // R resets the rotation
    QKeyEvent key(QEvent::KeyPress, Qt::Key_R, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &key);
    QCOMPARE(core->cameraRotation(), QQuaternion());

    // In the 2x2 layout the wheel zooms the pane under the pointer only
    core->setViewCount(4);
    const float paneZoom = core->zoomForTarget(3);
    const float otherZoom = core->zoomForTarget(0);
    sendWheel(&window, QPoint(300, 225));
    QCOMPARE(core->zoomForTarget(3), paneZoom + 1.0f);
    QCOMPARE(core->zoomForTarget(0), otherZoom);

    // A click on a pane name asks the application for the preset menu
    QSignalSpy menu(&window, &BrainWindow::viewportPresetMenuRequested);
    const QPoint nameCentre = core->overlayGeometry(BrainViewCore::ViewportNameLabel, 2).center();
    sendMouse(&window, QEvent::MouseButtonPress, nameCentre, Qt::LeftButton, Qt::LeftButton);
    sendMouse(&window, QEvent::MouseButtonRelease, nameCentre, Qt::LeftButton, Qt::NoButton);
    QCOMPARE(menu.size(), 1);
    QCOMPARE(menu.first().at(0).toInt(), 2);

    core->setViewCount(1);
    window.hide();
}

//=============================================================================================================

void TestDisp3dBrainWindow::frames_matchTheirShaders()
{
    // One triangle as a surface and as an instanced glyph shape, one glyph instance
    const std::vector<float> geometry = {0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f};
    const std::vector<std::uint32_t> colors = {0xff808080u, 0xff808080u, 0xff808080u};
    const std::vector<std::uint32_t> indices = {0, 1, 2};
    const std::vector<float> instance = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f};
    viz::BrainSurfaceStreams surface;
    surface.geometry = {std::as_bytes(std::span(geometry)), 1};
    surface.colors = {std::as_bytes(std::span(colors)), 1};
    surface.annotations = {std::as_bytes(std::span(colors)), 1};
    surface.indices = {std::as_bytes(std::span(indices)), 1};
    surface.index_count = 3;
    viz::BrainInstancedStreams glyphs;
    glyphs.vertices = surface.geometry;
    glyphs.instances = {std::as_bytes(std::span(instance)), 1};
    glyphs.indices = surface.indices;
    glyphs.index_count = 3;
    glyphs.instance_count = 1;

    viz::BrainFrameBuilder builder;
    viz::GpuFrameTarget target;
    target.pixel_width = 64;
    target.pixel_height = 48;
    const viz::GpuRect pane{0, 0, 64, 48};
    builder.begin(target);
    for (const viz::BrainShader shader : {viz::BrainShader::Standard, viz::BrainShader::Holographic, viz::BrainShader::Anatomical,
                                          viz::BrainShader::XRay, viz::BrainShader::ShowNormals}) {
        QVERIFY(builder.draw_surface(1, surface, shader, viz::BrainSurfaceUniforms{}, pane));
    }
    QVERIFY(builder.draw_instanced(2, glyphs, viz::BrainShader::Dipole, viz::BrainInstanceUniforms{}, pane));
    QVERIFY(builder.draw_instanced(2, glyphs, viz::BrainShader::Electrode, viz::BrainInstanceUniforms{}, pane));
    // Textured draws: an MRI slice, the video disk and the video decal, flat and following a depth frame
    viz::Image pixels = viz::Image::transparent(4, 4);
    const std::array<float, 20> quad = {0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f};
    QVERIFY(builder.draw_slice(3, viz::BrainTextureView{&pixels, 1}, quad, viz::BrainSliceUniforms{}, pane));
    QVERIFY(builder.draw_video_billboard(4, viz::BrainTextureView{&pixels, 1}, quad, viz::BrainVideoBillboardUniforms{}, pane));
    QVERIFY(builder.draw_video_decal(1, surface, 5, viz::BrainTextureView{&pixels, 1}, viz::BrainTextureView{}, viz::BrainVideoDecalUniforms{}, pane));
    QVERIFY(builder.draw_video_decal(1, surface, 6, viz::BrainTextureView{&pixels, 1}, viz::BrainTextureView{&pixels, 1}, viz::BrainVideoDecalUniforms{},
                                     pane));
    builder.draw_overlay(viz::Canvas(64, 48).display_list());
    const viz::GpuFrame frame = builder.finish();

    RhiFrameRenderer renderer;
    renderer.addShaderPrefix(":/");
    const std::vector<std::string> problems = renderer.checkShaders(frame, builder.resources());
    for (const std::string& problem : problems) {
        qWarning("%s", problem.c_str());
    }
    QVERIFY(problems.empty());
}

//=============================================================================================================

QTEST_MAIN(TestDisp3dBrainWindow)
#include "test_disp3d_brain_window.moc"
