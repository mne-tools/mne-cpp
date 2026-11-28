//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_viz_qt_channel_window.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests the Widgets-free ChannelRhiRenderer and its QWindow host ChannelWindow.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/backends/qt/plots/helpers/rhiframerenderer.h>
#include <mne/viz/backends/qt/viewers/helpers/channeldatamodel.h>
#include <mne/viz/backends/qt/viewers/helpers/channelrhirenderer.h>
#include <mne/viz/backends/qt/viewers/helpers/channelwindow.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_info.h>
#include <mne/viz/figures/text_metrics.h>
#include <mne/viz/gpu/trace_frame.h>

#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QSignalSpy>
#include <QWheelEvent>
#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;
using namespace mne::viz::qt;

namespace
{

/** Fixed-size host for driving a renderer without any surface. */
class TestHost : public ChannelRhiHost
{
public:
    QSize size{400, 200};
    int frameRequests = 0;
    bool tracking = false;

    void requestFrame() override
    {
        ++frameRequests;
    }
    QSize logicalSize() const override
    {
        return size;
    }
    qreal surfaceDevicePixelRatio() const override
    {
        return 1.0;
    }
    void setCursorShape(Qt::CursorShape) override
    {
    }
    void unsetCursorShape() override
    {
    }
    void setMouseTracking(bool enabled) override
    {
        tracking = enabled;
    }
    QFont font() const override
    {
        return QGuiApplication::font();
    }
};

std::shared_ptr<FiffInfo> createTestInfo()
{
    auto info = std::make_shared<FiffInfo>();
    info->sfreq = 1000.0f;
    info->nchan = 4;
    info->ch_names = {"MEG 0111", "MEG 0112", "MEG 0113", "STI 014"};
    info->chs.resize(info->nchan);
    for (int i = 0; i < info->nchan; ++i) {
        FiffChInfo channelInfo;
        channelInfo.ch_name = info->ch_names.at(i);
        channelInfo.scanNo = i + 1;
        channelInfo.logNo = i + 1;
        channelInfo.chpos.coil_type = FIFFV_COIL_VV_MAG_T3;
        channelInfo.coord_frame = FIFFV_COORD_DEVICE;
        channelInfo.cal = 1.0f;
        channelInfo.range = 1.0f;
        channelInfo.unit_mul = 0;
        channelInfo.kind = (i == info->nchan - 1) ? FIFFV_STIM_CH : FIFFV_MEG_CH;
        channelInfo.unit = (i == info->nchan - 1) ? FIFF_UNIT_NONE : FIFF_UNIT_T;
        info->chs[i] = channelInfo;
    }
    return info;
}

Eigen::MatrixXd createTestData()
{
    Eigen::MatrixXd data = Eigen::MatrixXd::Zero(4, 2000);
    for (int sample = 0; sample < data.cols(); ++sample) {
        data(0, sample) = std::sin(static_cast<double>(sample) * 0.05);
        data(1, sample) = std::cos(static_cast<double>(sample) * 0.08);
        data(3, sample) = (sample % 50 == 0) ? 3.0 : 0.0;
    }
    return data;
}

/** Offscreen QRhi::Null target the renderer records frames into. */
struct NullTarget
{
    std::unique_ptr<QRhi> rhi;
    std::unique_ptr<QRhiTexture> color;
    std::unique_ptr<QRhiTextureRenderTarget> target;
    std::unique_ptr<QRhiRenderPassDescriptor> pass;

    explicit NullTarget(const QSize& size)
    {
        QRhiNullInitParams params;
        rhi.reset(QRhi::create(QRhi::Null, &params));
        color.reset(rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget));
        color->create();
        target.reset(rhi->newTextureRenderTarget({QRhiColorAttachment(color.get())}));
        pass.reset(target->newCompatibleRenderPassDescriptor());
        target->setRenderPassDescriptor(pass.get());
        target->create();
    }

    bool frame(ChannelRhiRenderer& renderer)
    {
        QRhiCommandBuffer* cb = nullptr;
        if (rhi->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess) {
            return false;
        }
        renderer.renderFrame(rhi.get(), target.get(), cb);
        return rhi->endOffscreenFrame() == QRhi::FrameOpSuccess;
    }
};

bool columnHasInk(const QImage& image, int x)
{
    for (int y = 0; y < image.height(); ++y) {
        if (image.pixelColor(x, y).alpha() > 0) {
            return true;
        }
    }
    return false;
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestVizQtChannelWindow
 *
 * @brief Tests ChannelRhiRenderer frames on the QRhi Null backend and the ChannelWindow host.
 */
class TestVizQtChannelWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void renderer_rendersOffscreenFrame();
    void renderer_drawsHudOverlay();
    void renderer_animatesWithoutFrames();
    void frames_matchTheirShaders();
    void window_rendersWithNullBackend();
    void window_forwardsInput();
    void cleanupTestCase();

private:
    ChannelDataModel m_model;
};

//=============================================================================================================

void TestVizQtChannelWindow::initTestCase()
{
    m_model.init(createTestInfo());
    m_model.setData(createTestData(), 100);
}

//=============================================================================================================

void TestVizQtChannelWindow::renderer_rendersOffscreenFrame()
{
    TestHost host;
    ChannelRhiRenderer renderer(&host);
    QTest::failOnWarning(QRegularExpression(QStringLiteral("ChannelRhiView")));

    // Without a model only the background is cleared
    NullTarget target(host.size);
    QVERIFY(target.frame(renderer));

    renderer.setModel(&m_model);
    renderer.setSfreq(1000.f);
    renderer.setFirstFileSample(100);
    renderer.setLastFileSample(2099);
    renderer.setVisibleChannelCount(4);
    renderer.setSamplesPerPixel(0.5f);
    renderer.setScrollSample(150.f);
    renderer.setEvents({ChannelRhiRenderer::EventMarker{180, 1, Qt::red, "event"}});
    renderer.setAnnotations({ChannelRhiRenderer::AnnotationSpan{200, 220, Qt::blue, "blink"}});
    QCOMPARE(renderer.visibleSampleCount(), 200);
    QVERIFY(host.frameRequests > 0);

    for (const bool butterfly : {false, true}) {
        renderer.setButterflyMode(butterfly);
        QVERIFY(target.frame(renderer));
    }
    // No crosshair, scalebar or ruler: no overlay quad
    QVERIFY(!renderer.hudVisible());
    QVERIFY(renderer.hudImage().isNull());

    renderer.releaseResources();
    QVERIFY(target.frame(renderer));
    renderer.releaseResources();
}

//=============================================================================================================

void TestVizQtChannelWindow::renderer_drawsHudOverlay()
{
    TestHost host;
    ChannelRhiRenderer renderer(&host);
    renderer.setModel(&m_model);
    renderer.setSfreq(1000.f);
    renderer.setFirstFileSample(100);
    renderer.setVisibleChannelCount(4);
    renderer.setSamplesPerPixel(0.5f);
    renderer.setScrollSample(100.f);
    NullTarget target(host.size);

    // Scalebars are part of the GPU frame now: drawn into the overlay image in the bottom-right corner
    renderer.setScalebarsVisible(true);
    QVERIFY(renderer.hudVisible());
    QVERIFY(target.frame(renderer));
    QImage hud = renderer.hudImage();
    QCOMPARE(hud.size(), host.size);
    QVERIFY(hud.pixelColor(host.size.width() - 14, host.size.height() - 14).alpha() > 0);
    QCOMPARE(hud.pixelColor(5, 5).alpha(), 0);
    renderer.setScalebarsVisible(false);

    // Crosshair: enabling it turns on mouse tracking; a hover move draws the cross and reports the cursor data
    QSignalSpy cursorSpy(&renderer, &ChannelRhiRenderer::cursorDataChanged);
    renderer.setCrosshairEnabled(true);
    QVERIFY(host.tracking);
    QVERIFY(!renderer.hudVisible());
    QMouseEvent hover(QEvent::MouseMove, QPointF(40, 25), QPointF(40, 25), Qt::NoButton, Qt::NoButton,
                      Qt::NoModifier);
    QVERIFY(!renderer.handleMouseMove(&hover));
    QCOMPARE(cursorSpy.size(), 1);
    QCOMPARE(cursorSpy.at(0).at(2).toString(), QStringLiteral("MEG 0111"));
    QCOMPARE(cursorSpy.at(0).at(0).toFloat(), 0.02f);
    QVERIFY(renderer.hudVisible());
    QVERIFY(target.frame(renderer));
    hud = renderer.hudImage();
    QVERIFY(columnHasInk(hud, 40));
    QCOMPARE(hud.pixelColor(300, 150).alpha(), 0);

    // paintHud() paints the same overlay on any device
    QImage painted(host.size, QImage::Format_RGBA8888);
    painted.fill(Qt::transparent);
    {
        QPainter painter(&painted);
        renderer.paintHud(painter);
    }
    QCOMPARE(painted, hud);
    renderer.setCrosshairEnabled(false);
    QVERIFY(!host.tracking);

    // The ruler overlay is drawn while the right button is held
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(100, 30), QPointF(100, 30), Qt::RightButton,
                      Qt::RightButton, Qt::NoModifier);
    QMouseEvent move(QEvent::MouseMove, QPointF(180, 30), QPointF(180, 30), Qt::NoButton, Qt::RightButton,
                     Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(180, 30), QPointF(180, 30), Qt::RightButton,
                        Qt::NoButton, Qt::NoModifier);
    QVERIFY(renderer.handleMousePress(&press));
    QVERIFY(renderer.handleMouseMove(&move));
    QVERIFY(target.frame(renderer));
    QVERIFY(columnHasInk(renderer.hudImage(), 100));
    QVERIFY(columnHasInk(renderer.hudImage(), 180));
    QVERIFY(renderer.handleMouseRelease(&release));
    QVERIFY(!renderer.hudVisible());

    renderer.releaseResources();
}

//=============================================================================================================

void TestVizQtChannelWindow::renderer_animatesWithoutFrames()
{
    // A hidden host renders no frames; the animations still reach their targets and report each step
    TestHost host;
    ChannelRhiRenderer renderer(&host);
    renderer.setModel(&m_model);
    renderer.setSfreq(1000.f);
    renderer.setFirstFileSample(100);
    renderer.setLastFileSample(2099);
    renderer.setSamplesPerPixel(1.f);
    renderer.setScrollSample(100.f);
    QSignalSpy scrolled(&renderer, &ChannelRhiRenderer::scrollSampleChanged);
    QSignalSpy zoomed(&renderer, &ChannelRhiRenderer::samplesPerPixelChanged);
    const int requests = host.frameRequests;

    // Long enough for several timer steps on a busy machine, where an 80 ms animation may finish in one step
    renderer.scrollTo(600.f, 500);
    QTRY_COMPARE_WITH_TIMEOUT(renderer.scrollSample(), 600.f, 5000);
    QVERIFY(scrolled.count() > 1);
    QVERIFY(host.frameRequests > requests + 1);

    renderer.zoomTo(2.f, 500);
    QTRY_COMPARE_WITH_TIMEOUT(renderer.samplesPerPixel(), 2.f, 5000);
    QVERIFY(zoomed.count() > 1);
}

//=============================================================================================================

void TestVizQtChannelWindow::frames_matchTheirShaders()
{
    // The trace view's frames feed every uniform block, sampler and vertex input of the .qsb shaders
    viz::TraceView view;
    view.set_source(&m_model.buffer());
    view.set_view_size(320, 200);
    view.set_sfreq(1000.0);
    view.set_samples_per_pixel(0.5);
    view.set_scalebars_visible(true);
    viz::TraceFrameBuilder builder;
    viz::GpuFrameTarget target;
    target.pixel_width = 320;
    target.pixel_height = 200;
    const viz::ApproximateTextMetrics metrics;
    viz::GpuFrame frame = builder.build(view, m_model.buffer(), viz::TraceStyle{}, viz::TraceMarkers{}, metrics, target);
    QVERIFY(frame.passes.at(0).draws.size() >= 3);
    RhiFrameRenderer renderer;
    renderer.addShaderPrefix(":/disp/shaders/viewers/helpers/shaders/");
    QVERIFY(renderer.checkShaders(frame, builder.resources()).empty());

    // A uniform block of the wrong size, a missing sampler and a missing shader are reported
    frame.passes[0].draws[0].uniforms[0].data.resize(48);
    frame.passes[0].draws.back().textures.clear();
    const std::vector<std::string> expected = {"channeldata: uniform block 0 has 48 bytes, the shader 52", "overlay: sampler 1 not set"};
    QVERIFY(renderer.checkShaders(frame, builder.resources()) == expected);
    RhiFrameRenderer unconfigured;
    const std::vector<std::string> missing = unconfigured.checkShaders(frame, builder.resources());
    QVERIFY(std::find(missing.begin(), missing.end(), "channeldata: no shaders") != missing.end());
}

//=============================================================================================================

void TestVizQtChannelWindow::window_rendersWithNullBackend()
{
    ChannelWindow window(ChannelWindow::Api::Null);
    QCOMPARE(window.api(), ChannelWindow::Api::Null);
    window.renderer()->setModel(&m_model);
    window.renderer()->setVisibleChannelCount(4);
    window.renderer()->setScalebarsVisible(true);
    window.resize(320, 240);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QVERIFY(window.rhi() != nullptr);
    QCOMPARE(window.rhi()->backend(), QRhi::Null);
    QVERIFY(window.swapChain() != nullptr);
    QTRY_VERIFY(window.frameCount() > 0);
    // The Null swap chain has a fixed pixel size; the overlay image is built at the target's pixel scale
    QVERIFY(!window.swapChain()->currentPixelSize().isEmpty());
    QVERIFY(!window.renderer()->hudImage().isNull());

    // Renderer changes schedule a new frame through QWindow::requestUpdate()
    const int frames = window.frameCount();
    window.renderer()->setScrollSample(300.f);
    QTRY_VERIFY(window.frameCount() > frames);

    // Resizing resizes the swap chain and reports the new size
    QSignalSpy resizeSpy(&window, &ChannelWindow::viewResized);
    window.resize(400, 200);
    QTRY_VERIFY(!resizeSpy.isEmpty());
    QCOMPARE(resizeSpy.last().at(0).toInt(), 400);
    QCOMPARE(resizeSpy.last().at(1).toInt(), 200);
    QCOMPARE(window.renderer()->visibleSampleCount(), 400);
    const int resizedFrames = window.frameCount();
    QTRY_VERIFY(window.frameCount() > resizedFrames);

    // The API is fixed once the window exists
    window.setApi(ChannelWindow::Api::OpenGL);
    QCOMPARE(window.api(), ChannelWindow::Api::Null);
}

//=============================================================================================================

void TestVizQtChannelWindow::window_forwardsInput()
{
    ChannelWindow window(ChannelWindow::Api::Null);
    ChannelRhiRenderer* renderer = window.renderer();
    renderer->setModel(&m_model);
    renderer->setSfreq(1000.f);
    renderer->setFirstFileSample(100);
    renderer->setLastFileSample(2099);
    renderer->setVisibleChannelCount(2);
    renderer->setSamplesPerPixel(0.5f);
    renderer->setScrollSample(500.f);
    window.resize(400, 200);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const auto wheel = [&window](const QPoint& delta, Qt::KeyboardModifiers modifiers) {
        QWheelEvent event(QPointF(50, 40), QPointF(50, 40), QPoint(), delta, Qt::NoButton, modifiers,
                          Qt::NoScrollPhase, false);
        QGuiApplication::sendEvent(&window, &event);
    };

    // Vertical wheel scrolls channels by default
    QSignalSpy channelSpy(&window, &ChannelWindow::channelOffsetChanged);
    wheel(QPoint(0, -120), Qt::NoModifier);
    QCOMPARE(channelSpy.size(), 1);
    QCOMPARE(renderer->firstVisibleChannel(), 1);

    // In time mode it scrolls 15 % of the view, animated
    QSignalSpy scrollSpy(&window, &ChannelWindow::scrollSampleChanged);
    renderer->setWheelScrollsChannels(false);
    wheel(QPoint(0, -120), Qt::NoModifier);
    QTRY_COMPARE(renderer->scrollSample(), 500.f + 0.15f * 400.f * 0.5f);
    QVERIFY(!scrollSpy.isEmpty());

    // Ctrl + wheel zooms
    QSignalSpy zoomSpy(&window, &ChannelWindow::samplesPerPixelChanged);
    wheel(QPoint(0, 120), Qt::ControlModifier);
    QTRY_VERIFY(qFuzzyCompare(renderer->samplesPerPixel(), 0.4f));
    QVERIFY(!zoomSpy.isEmpty());

    // A left click reports the sample under the cursor
    QSignalSpy clickSpy(&window, &ChannelWindow::sampleClicked);
    const float scroll = renderer->scrollSample();
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(40, 50));
    QCOMPARE(clickSpy.size(), 1);
    QCOMPARE(clickSpy.at(0).at(0).toInt(), static_cast<int>(scroll + 40.f * renderer->samplesPerPixel()));

    // Hover moves report cursor data only while the crosshair is on
    QSignalSpy cursorSpy(&window, &ChannelWindow::cursorDataChanged);
    QTest::mouseMove(&window, QPoint(60, 50));
    QCOMPARE(cursorSpy.size(), 0);
    renderer->setCrosshairEnabled(true);
    QTest::mouseMove(&window, QPoint(80, 50));
    QTRY_COMPARE(cursorSpy.size(), 1);
    QVERIFY(renderer->hudVisible());
}

//=============================================================================================================

void TestVizQtChannelWindow::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    // A QGuiApplication: neither the renderer nor the window host needs Qt Widgets
    QGuiApplication app(argc, argv);
    TestVizQtChannelWindow test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "test_viz_qt_channel_window.moc"
