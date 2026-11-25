//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channelrhirenderer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Implementation of the Widgets-free ChannelRhiRenderer (pipelines, buffer streaming, overlays, input).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channelrhirenderer.h"
#include "../../plots/helpers/scenepainter.h"
#include <mne/scene/trace_geometry.h>
#include <mne/base/log.h>

#include <rhi/qrhi.h>
#include <rhi/qshader.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QTimerEvent>
#include <QWheelEvent>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;
namespace scene = mne::scene;

//=============================================================================================================
// Overlay quad and its shader parameters
//=============================================================================================================

namespace
{
// Full-screen quad: (pos.x, pos.y, uv.x, uv.y) per vertex, TriangleStrip. NDC Y+ = top, image UV Y=0 = top.
constexpr float kQuadVerts[] = {
    -1.f,
    -1.f,
    0.f,
    1.f, // bottom-left
    -1.f,
    1.f,
    0.f,
    0.f, // top-left
    1.f,
    -1.f,
    1.f,
    1.f, // bottom-right
    1.f,
    1.f,
    1.f,
    0.f, // top-right
};

// Per-frame parameters of overlay.frag (binding 2)
struct OverlayParams
{
    float scrollSample;
    float samplesPerPixel;
    float viewWidth;
    float sfreq;
    float firstFileSample;
    float gridEnabled;
    float overlayFirstSample;
    float overlayTotalSamples;
};

constexpr int kOverlayUboSize = 256; // 8 floats, aligned to 256 for std140 on all backends

} // namespace

//=============================================================================================================
// HELPERS
//=============================================================================================================

static QShader loadShader(const QString& filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        mne::Log::warning("ChannelRhiView: cannot open shader {}", filename.toStdString());
        return {};
    }
    return QShader::fromSerialized(f.readAll());
}

static scene::Modifier toSceneModifiers(Qt::KeyboardModifiers modifiers)
{
    scene::Modifier result = scene::Modifier::None;
    if (modifiers & Qt::ShiftModifier)
        result = result | scene::Modifier::Shift;
    if (modifiers & Qt::ControlModifier)
        result = result | scene::Modifier::Ctrl;
    if (modifiers & Qt::AltModifier)
        result = result | scene::Modifier::Alt;
    if (modifiers & Qt::MetaModifier)
        result = result | scene::Modifier::Meta;
    return result;
}

static scene::MouseButton toSceneButton(Qt::MouseButton button)
{
    switch (button) {
        case Qt::LeftButton:
            return scene::MouseButton::Left;
        case Qt::RightButton:
            return scene::MouseButton::Right;
        case Qt::MiddleButton:
            return scene::MouseButton::Middle;
        default:
            return scene::MouseButton::None;
    }
}

// Monotonic time of an input event; the drag velocity of TraceView is measured with it
static std::int64_t monotonicMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

//=============================================================================================================
// TRACE SOURCE
//=============================================================================================================

/**
 * The channels of the renderer's model as the TraceView sees them: logical (filtered) channel indices.
 */
class ChannelRhiRenderer::TraceSourceAdapter : public scene::TraceSource
{
public:
    explicit TraceSourceAdapter(const ChannelRhiRenderer& renderer)
    : m_renderer(renderer)
    {
    }

    int channel_count() const override
    {
        return m_renderer.totalLogicalChannels();
    }

    std::optional<Channel> channel(int index) const override
    {
        const int ch = m_renderer.actualChannelAt(index);
        if (!m_renderer.m_model || ch < 0)
            return std::nullopt;
        ChannelDisplayInfo info = m_renderer.m_model->channelInfo(ch);
        return Channel{std::move(info.name), std::move(info.typeLabel), info.amplitudeMax, toSceneColor(info.color), info.bad};
    }

    float value_at(int index, int sample) const override
    {
        const int ch = m_renderer.actualChannelAt(index);
        return (m_renderer.m_model && ch >= 0) ? m_renderer.m_model->sampleValueAt(ch, sample) : 0.f;
    }

    int first_sample() const override
    {
        const ChannelDataModel* model = m_renderer.m_model.data();
        return (model && model->totalSamples() > 0) ? model->firstSample() : 0;
    }

    int sample_count() const override
    {
        return m_renderer.m_model ? m_renderer.m_model->totalSamples() : 0;
    }

    scene::TraceVertices vertices(int index, int first, int last, int pixelWidth) const override
    {
        scene::TraceVertices result{first, {}};
        const int ch = m_renderer.actualChannelAt(index);
        if (m_renderer.m_model && ch >= 0)
            result.xy = m_renderer.m_model->decimatedVertices(ch, first, last, pixelWidth, result.first_sample);
        return result;
    }

private:
    const ChannelRhiRenderer& m_renderer;
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelRhiRenderer::ChannelRhiRenderer(ChannelRhiHost* host, QObject* parent)
: QObject(parent)
, m_pHost(host)
, m_traceSource(std::make_unique<TraceSourceAdapter>(*this))
{
    m_view.set_source(m_traceSource.get());
    connectView();

    // Repaint when the app regains focus
    if (qGuiApp) {
        connect(qGuiApp, &QGuiApplication::applicationStateChanged,
                this, [this](Qt::ApplicationState s) {
                    if (s == Qt::ApplicationActive)
                        requestUpdate();
                });
    }
}

//=============================================================================================================

void ChannelRhiRenderer::connectView()
{
    scene::TraceView::Effects effects;
    effects.scroll_changed = [this](double) {
        const float sample = scrollSample();
        const float visible = static_cast<float>(viewWidth()) * samplesPerPixel();

        // Overlay prefetch: only rebuild when scroll exceeds the cached sample range.
        // The shader handles bands via uniforms, so we only need to rebuild when
        // annotations/events would be outside the cached texture.
        if (m_overlayTotalSamples <= 0.f || sample < m_overlayFirstSample ||
            sample + visible > m_overlayFirstSample + m_overlayTotalSamples) {
            m_overlayDirty = true;
        }

        emit scrollSampleChanged(sample);
        requestUpdate();
    };
    effects.zoom_changed = [this](double) {
        m_geometry.invalidate(); // zoom change → decimation changes
        m_overlayDirty = true;
        emit samplesPerPixelChanged(samplesPerPixel());
        requestUpdate();
    };
    effects.channel_offset_changed = [this](int firstChannel) {
        m_geometry.invalidate();
        m_pipelineDirty = true;
        emit channelOffsetChanged(firstChannel);
        requestUpdate();
    };
    effects.animate_scroll = [this](const scene::TraceView::ScrollAnimation& animation) {
        m_animator.start_scroll(animation, monotonicMs());
        startAnimationTimer();
    };
    effects.animate_zoom = [this](const scene::TraceView::ZoomAnimation& animation) {
        m_animator.start_zoom(animation, monotonicMs());
        startAnimationTimer();
    };
    effects.stop_animations = [this] {
        m_animator.stop_scroll();
    };
    effects.sample_clicked = [this](int sample) {
        emit sampleClicked(sample);
    };
    effects.range_selected = [this](int startSample, int endSample) {
        emit sampleRangeSelected(startSample, endSample);
    };
    effects.annotation_boundary_moved = [this](int index, bool isStart, int sample) {
        emit annotationBoundaryMoved(index, isStart, sample);
    };
    effects.annotations_changed = [this] {
        m_overlayDirty = true;
        requestUpdate();
    };
    effects.channel_double_clicked = [this](int logicalChannel) {
        const int ch = actualChannelAt(logicalChannel);
        if (m_model && ch >= 0) {
            m_model->setChannelBad(ch, !m_model->channelInfo(ch).bad);
        }
    };
    effects.cursor_changed = [this](scene::CursorShape shape) {
        if (!m_pHost)
            return;
        if (shape == scene::CursorShape::SizeHorizontal)
            m_pHost->setCursorShape(Qt::SizeHorCursor);
        else
            m_pHost->unsetCursorShape();
    };
    effects.cursor_data_changed = [this](const scene::TraceView::CursorData& data) {
        emit cursorDataChanged(data.time_sec, data.amplitude, QString::fromStdString(data.channel_name),
                               QString::fromStdString(data.unit_label));
    };
    effects.redraw = [this] {
        requestUpdate();
    };
    m_view.set_effects(std::move(effects));
}

//=============================================================================================================

scene::TraceView& ChannelRhiRenderer::view() const
{
    m_view.set_view_size(viewWidth(), viewHeight());
    return m_view;
}

//=============================================================================================================

void ChannelRhiRenderer::tickAnimations()
{
    if ((!m_animator.running() || !m_animator.tick(monotonicMs(), view())) && m_animationTimerId != 0) {
        killTimer(m_animationTimerId);
        m_animationTimerId = 0;
    }
}

//=============================================================================================================

void ChannelRhiRenderer::startAnimationTimer()
{
    // Animations advance on every frame and, for hosts that render no frames (e.g. hidden ones), on this timer
    if (m_animationTimerId == 0)
        m_animationTimerId = startTimer(std::chrono::milliseconds(16), Qt::PreciseTimer);
}

//=============================================================================================================

void ChannelRhiRenderer::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == m_animationTimerId)
        tickAnimations();
    else
        QObject::timerEvent(event);
}

//=============================================================================================================

scene::TraceStyle ChannelRhiRenderer::traceStyle() const
{
    scene::TraceStyle style;
    style.background = toSceneColor(m_bgColor);
    style.z_score = m_bZScoreMode;
    style.show_clipping = m_bShowClipping;
    style.hide_bad_channels = m_hideBadChannels;
    style.grid = m_gridVisible;
    return style;
}

//=============================================================================================================

ChannelRhiRenderer::~ChannelRhiRenderer() = default;

//=============================================================================================================

void ChannelRhiRenderer::setHost(ChannelRhiHost* host)
{
    m_pHost = host;
    m_geometry.invalidate();
    m_overlayDirty = true;
    m_hudDirty = true;
}

//=============================================================================================================

int ChannelRhiRenderer::viewWidth() const
{
    return m_pHost ? m_pHost->logicalSize().width() : 0;
}

//=============================================================================================================

int ChannelRhiRenderer::viewHeight() const
{
    return m_pHost ? m_pHost->logicalSize().height() : 0;
}

//=============================================================================================================

QFont ChannelRhiRenderer::hostFont() const
{
    return m_pHost ? m_pHost->font() : QGuiApplication::font();
}

//=============================================================================================================

void ChannelRhiRenderer::requestUpdate()
{
    // Crosshair, scalebar and ruler values may depend on any state change
    m_hudDirty = true;
    if (m_pHost)
        m_pHost->requestFrame();
}

//=============================================================================================================

void ChannelRhiRenderer::setModel(ChannelDataModel* model)
{
    if (m_model == model)
        return;
    if (m_model) {
        disconnect(m_model, &ChannelDataModel::dataChanged, this, nullptr);
        disconnect(m_model, &ChannelDataModel::metaChanged, this, nullptr);
    }
    m_model = model;
    if (m_model) {
        connect(m_model, &ChannelDataModel::dataChanged, this, [this] {
            m_geometry.invalidate();
            requestUpdate();
        });
        connect(m_model, &ChannelDataModel::metaChanged, this, [this] {
            m_geometry.invalidate();
            m_pipelineDirty = true;
            requestUpdate();
        });
    }
    m_geometry.invalidate();
    m_pipelineDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setScrollSample(float sample)
{
    view().set_scroll_sample(sample);
}

//=============================================================================================================

void ChannelRhiRenderer::setSamplesPerPixel(float spp)
{
    view().set_samples_per_pixel(spp);
}

//=============================================================================================================

void ChannelRhiRenderer::scrollTo(float targetSample, int durationMs)
{
    view().scroll_to(targetSample, durationMs);
}

//=============================================================================================================

void ChannelRhiRenderer::scrollBy(float deltaSamples, int durationMs)
{
    view().scroll_by(deltaSamples, durationMs);
}

//=============================================================================================================

void ChannelRhiRenderer::zoomTo(float targetSpp, int durationMs)
{
    view().zoom_to(targetSpp, durationMs);
}

//=============================================================================================================

void ChannelRhiRenderer::setBackgroundColor(const QColor& color)
{
    m_bgColor = color;
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setPrefetchFactor(float factor)
{
    m_geometry.set_prefetch_factor(factor);
}

//=============================================================================================================

int ChannelRhiRenderer::visibleFirstSample() const
{
    return m_view.visible_first_sample();
}

//=============================================================================================================

int ChannelRhiRenderer::visibleSampleCount() const
{
    return view().visible_sample_count();
}

//=============================================================================================================

void ChannelRhiRenderer::setFirstVisibleChannel(int ch)
{
    m_view.set_first_visible_channel(ch);
}

//=============================================================================================================

void ChannelRhiRenderer::setVisibleChannelCount(int count)
{
    if (!m_view.set_visible_channel_count(count))
        return;
    m_geometry.invalidate();
    m_pipelineDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setFrozen(bool frozen)
{
    m_view.set_frozen(frozen);
}

//=============================================================================================================

void ChannelRhiRenderer::setGridVisible(bool visible)
{
    if (visible == m_gridVisible)
        return;
    m_gridVisible = visible;
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setSfreq(float sfreq)
{
    m_view.set_sfreq(sfreq);
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setFirstFileSample(int first)
{
    if (!m_view.set_first_file_sample(first))
        return;
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setLastFileSample(int last)
{
    m_view.set_last_file_sample(last);
}

//=============================================================================================================

void ChannelRhiRenderer::setHideBadChannels(bool hide)
{
    if (m_hideBadChannels == hide)
        return;

    m_hideBadChannels = hide;
    // Re-clamp the channel window to the new channel count
    m_view.set_first_visible_channel(m_view.first_visible_channel());
    m_geometry.invalidate();
    m_pipelineDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setWheelScrollsChannels(bool channelsMode)
{
    m_view.set_wheel_scrolls_channels(channelsMode);
}

//=============================================================================================================

void ChannelRhiRenderer::setScrollSpeedFactor(float factor)
{
    m_view.set_scroll_speed_factor(factor);
}

//=============================================================================================================

void ChannelRhiRenderer::setCrosshairEnabled(bool enabled)
{
    if (!m_view.set_crosshair_enabled(enabled))
        return;
    if (m_pHost)
        m_pHost->setMouseTracking(enabled);
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setClockTimeFormat(bool useClock)
{
    m_view.set_clock_time_format(useClock);
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setScalebarsVisible(bool visible)
{
    if (!m_view.set_scalebars_visible(visible))
        return;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setButterflyMode(bool enabled)
{
    if (!m_view.set_butterfly_mode(enabled))
        return;
    m_geometry.invalidate();
    m_pipelineDirty = true;
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setChannelIndices(const std::vector<int>& indices)
{
    m_filteredChannels = indices;
    // Clamp the channel window to the new range
    m_view.set_first_visible_channel(m_view.first_visible_channel());
    m_geometry.invalidate();
    m_pipelineDirty = true;
    requestUpdate();
}

//=============================================================================================================

int ChannelRhiRenderer::totalLogicalChannels() const
{
    return static_cast<int>(effectiveChannelIndices().size());
}

//=============================================================================================================

int ChannelRhiRenderer::actualChannelAt(int logicalIdx) const
{
    const std::vector<int> indices = effectiveChannelIndices();
    if (logicalIdx < 0 || logicalIdx >= std::ssize(indices))
        return -1;
    return indices.at(logicalIdx);
}

//=============================================================================================================

std::vector<int> ChannelRhiRenderer::effectiveChannelIndices() const
{
    if (!m_model)
        return {};
    const ChannelDataModel* model = m_model.data();
    return scene::visible_channels(model->channelCount(), m_filteredChannels, m_hideBadChannels,
                                   [model](int channel) { return model->channelInfo(channel).bad; });
}

//=============================================================================================================

void ChannelRhiRenderer::setEvents(const std::vector<EventMarker>& events)
{
    std::vector<scene::TraceMarkers::Event> sceneEvents;
    sceneEvents.reserve(events.size());
    for (const EventMarker& event : events)
        sceneEvents.push_back({event.sample, event.type, toSceneColor(event.color), event.label});
    m_markers.set_events(std::move(sceneEvents));
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setEpochMarkers(const std::vector<int>& triggerSamples)
{
    m_markers.set_epoch_markers(triggerSamples);
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setEpochMarkersVisible(bool visible)
{
    if (!m_markers.set_epoch_markers_visible(visible))
        return;
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setClippingVisible(bool visible)
{
    if (m_bShowClipping == visible)
        return;
    m_bShowClipping = visible;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setZScoreMode(bool enabled)
{
    if (m_bZScoreMode == enabled)
        return;
    m_bZScoreMode = enabled;
    m_geometry.invalidate(); // VBO data changes (z-score normalization)
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setAnnotations(const std::vector<AnnotationSpan>& annotations)
{
    std::vector<scene::TraceView::Annotation> spans;
    spans.reserve(annotations.size());
    for (const AnnotationSpan& annotation : annotations) {
        spans.push_back({annotation.startSample, annotation.endSample, toSceneColor(annotation.color), annotation.label});
    }
    m_view.set_annotations(std::move(spans));
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setAnnotationSelectionEnabled(bool enabled)
{
    m_view.set_annotation_selection_enabled(enabled);
}

//=============================================================================================================

void ChannelRhiRenderer::setEventsVisible(bool visible)
{
    if (!m_markers.set_events_visible(visible))
        return;
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setAnnotationsVisible(bool visible)
{
    if (!m_markers.set_annotations_visible(visible))
        return;
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::initializeResources()
{
    m_pipelineDirty = true;
    m_geometry.invalidate();
}

//=============================================================================================================

void ChannelRhiRenderer::releaseResources()
{
    m_pipeline.reset();
    m_srb.reset();
    m_ubo.reset();
    m_gpuChannels.clear();
    m_pipelineDirty = true;
    m_geometry.invalidate();

    m_overlayPipeline.reset();
    m_overlaySrb.reset();
    m_overlaySampler.reset();
    m_overlayTex.reset();
    m_overlayVbo.reset();
    m_overlayUbo.reset();
    m_overlayTexSize = QSize();
    m_overlayDirty = true;

    m_hudSrb.reset();
    m_hudTex.reset();
    m_hudUbo.reset();
    m_hudTexSize = QSize();
    m_hudDirty = true;
}

//=============================================================================================================

void ChannelRhiRenderer::ensurePipeline(QRhi* rhi, QRhiRenderTarget* target)
{
    if (!m_pipelineDirty)
        return;

    m_uboStride = static_cast<int>(
        (static_cast<int>(sizeof(scene::TraceUniforms)) + rhi->ubufAlignment() - 1) & ~(rhi->ubufAlignment() - 1));

    // One UBO slot per drawn trace: the visible rows, or every channel in butterfly mode
    const int nCh = scene::TraceGeometry::slot_count(view(), *m_traceSource);

    // ── Uniform buffer ──────────────────────────────────────────────────
    bool uboRecreated = false;
    if (!m_ubo || m_ubo->size() < static_cast<quint32>(nCh * m_uboStride)) {
        m_ubo.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                   QRhiBuffer::UniformBuffer,
                                   nCh * m_uboStride));
        m_ubo->create();
        uboRecreated = true;
    }

    // ── Shader resource bindings ────────────────────────────────────────
    // Recreate if the UBO pointer changed — SRB holds a raw pointer to the UBO.
    if (!m_srb || uboRecreated) {
        m_srb.reset(rhi->newShaderResourceBindings());
        m_srb->setBindings({QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
            0,
            QRhiShaderResourceBinding::VertexStage |
                QRhiShaderResourceBinding::FragmentStage,
            m_ubo.get(),
            sizeof(scene::TraceUniforms) // visible block size for the shader
            )});
        m_srb->create();
    }

    // ── Shaders ─────────────────────────────────────────────────────────
    // Resource path matches qt_add_shaders PREFIX + file path (including subdirectory).
    QShader vs = loadShader(QStringLiteral(":/disp/shaders/viewers/helpers/shaders/channeldata.vert.qsb"));
    QShader fs = loadShader(QStringLiteral(":/disp/shaders/viewers/helpers/shaders/channeldata.frag.qsb"));

    if (!vs.isValid() || !fs.isValid()) {
        mne::Log::warning("ChannelRhiView: shaders not found. Ensure qt_add_shaders is configured in CMakeLists.");
        return;
    }

    // ── Graphics pipeline ───────────────────────────────────────────────
    // Destroy any existing pipeline before creating a new one.
    m_pipeline.reset();
    m_pipeline.reset(rhi->newGraphicsPipeline());
    m_pipeline->setShaderStages({{QRhiShaderStage::Vertex, vs},
                                 {QRhiShaderStage::Fragment, fs}});

    QRhiVertexInputLayout il;
    il.setBindings({{2 * sizeof(float)}});                           // stride = vec2
    il.setAttributes({{0, 0, QRhiVertexInputAttribute::Float2, 0}}); // location 0 = vec2

    m_pipeline->setVertexInputLayout(il);
    m_pipeline->setShaderResourceBindings(m_srb.get());
    m_pipeline->setRenderPassDescriptor(target->renderPassDescriptor());
    m_pipeline->setTopology(QRhiGraphicsPipeline::LineStrip);
    m_pipeline->setDepthTest(false);
    m_pipeline->setDepthWrite(false);

    // Alpha blending for anti-aliased lines (if multisampling is disabled)
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_pipeline->setTargetBlends({blend});

    if (!m_pipeline->create()) {
        mne::Log::warning("ChannelRhiView: failed to create graphics pipeline");
        m_pipeline.reset();
        return;
    }

    m_pipelineDirty = false;
}

//=============================================================================================================

bool ChannelRhiRenderer::isVboDirty() const
{
    return m_model && m_geometry.needs_stream(view());
}

//=============================================================================================================

void ChannelRhiRenderer::rebuildVBOs(QRhi* rhi, QRhiResourceUpdateBatch* batch)
{
    if (!m_model || !m_geometry.stream(view(), *m_traceSource, m_bZScoreMode))
        return;

    // VBOs are indexed by logical (filtered) channel index, not model channel index
    const std::vector<scene::TraceVertices>& channels = m_geometry.channels();
    m_gpuChannels.resize(channels.size());
    for (std::size_t logCh = 0; logCh < channels.size(); ++logCh) {
        const std::vector<float>& verts = channels[logCh].xy;
        ChannelGpuData& gd = m_gpuChannels[logCh];
        gd.vertexCount = 0;
        if (verts.empty())
            continue;

        const quint32 byteSize = static_cast<quint32>(verts.size() * sizeof(float));
        if (!gd.vbo || static_cast<quint32>(gd.vbo->size()) < byteSize) {
            gd.vbo.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, byteSize));
            if (!gd.vbo->create()) {
                mne::Log::warning("ChannelRhiView: VBO create failed for channel {}", logCh);
                continue;
            }
        }
        batch->updateDynamicBuffer(gd.vbo.get(), 0, byteSize, verts.data());
        gd.vertexCount = channels[logCh].vertex_count();
    }
}

//=============================================================================================================

void ChannelRhiRenderer::updateUBO(QRhiResourceUpdateBatch* batch)
{
    m_traceDraws.clear();
    if (!m_model || !m_ubo)
        return;

    m_traceDraws = m_geometry.draws(view(), *m_traceSource, traceStyle());
    // Never more slots than the UBO holds (it grows in ensurePipeline())
    const int slotCount = static_cast<int>(m_ubo->size()) / m_uboStride;
    if (std::ssize(m_traceDraws) > slotCount)
        m_traceDraws.resize(static_cast<std::size_t>(slotCount));

    QVarLengthArray<quint8> buf(m_uboStride, 0);
    for (int slot = 0; slot < std::ssize(m_traceDraws); ++slot) {
        memcpy(buf.data(), &m_traceDraws[static_cast<std::size_t>(slot)].uniforms, sizeof(scene::TraceUniforms));
        batch->updateDynamicBuffer(m_ubo.get(), slot * m_uboStride, m_uboStride, buf.constData());
    }
}

//=============================================================================================================
// Overlay texture — annotations, events, and epoch markers baked into a QImage.
// Alternating per-second bands are now computed in the fragment shader, so this
// image only needs rebuilding when annotations/events change or when the scroll
// exceeds the prefetch window.
//
// The overlay covers a wider sample range than the viewport (controlled by
// kOverlayPrefetchFactor).  The fragment shader maps screen UVs into this
// wider texture via the OverlayParams UBO.
//=============================================================================================================

void ChannelRhiRenderer::rebuildOverlayImage(int logicalWidth, int logicalHeight, qreal devicePixelRatio)
{
    const qreal dpr = qMax(devicePixelRatio, 1.0);
    const int pixelWidth = qMax(1, qRound(logicalWidth * dpr));
    const int pixelHeight = qMax(1, qRound(logicalHeight * dpr));

    m_overlayImage = QImage(pixelWidth, pixelHeight, QImage::Format_RGBA8888);
    m_overlayImage.setDevicePixelRatio(dpr);
    m_overlayImage.fill(Qt::transparent);
    m_overlayDirty = false;

    if (logicalWidth <= 0 || logicalHeight <= 0 || m_view.sfreq() <= 0.0 || m_view.samples_per_pixel() <= 0.0 || m_overlayTotalSamples <= 0.f)
        return;

    // The overlay covers m_overlayFirstSample .. m_overlayFirstSample + m_overlayTotalSamples; per-second bands
    // are computed in the fragment shader
    const QFont font = QGuiApplication::font();
    const QtTextMetrics metrics(font);
    scene::Canvas canvas(logicalWidth, logicalHeight);
    m_markers.draw(canvas, metrics, scene::Font{}, m_view.annotations(), m_overlayFirstSample,
                   static_cast<double>(logicalWidth) / static_cast<double>(m_overlayTotalSamples));
    QPainter p(&m_overlayImage);
    paintScene(p, canvas.display_list(), font);
}

//=============================================================================================================

void ChannelRhiRenderer::ensureOverlayPipeline(QRhi* rhi, QRhiRenderTarget* target)
{
    if (m_overlayPipeline)
        return;

    // Static full-screen quad (screen-space NDC coordinates); uploaded once in ensureOverlayResources()
    static constexpr int kQuadBytes = sizeof(kQuadVerts);
    m_overlayVbo.reset(rhi->newBuffer(QRhiBuffer::Immutable,
                                      QRhiBuffer::VertexBuffer,
                                      kQuadBytes));
    if (!m_overlayVbo->create()) {
        m_overlayVbo.reset();
        return;
    }

    // Texture — placeholder 1×1; resized lazily in renderFrame() when pw/ph are known.
    m_overlayTex.reset(rhi->newTexture(QRhiTexture::RGBA8, QSize(1, 1)));
    m_overlayTex->create();
    m_overlayTexSize = QSize();

    m_overlaySampler.reset(rhi->newSampler(
        QRhiSampler::Linear, QRhiSampler::Linear,
        QRhiSampler::None,
        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    m_overlaySampler->create();

    // UBO for per-frame overlay parameters (binding 2)
    m_overlayUbo.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                      QRhiBuffer::UniformBuffer,
                                      kOverlayUboSize));
    if (!m_overlayUbo->create()) {
        m_overlayVbo.reset();
        m_overlayUbo.reset();
        return;
    }

    // SRB: binding 1 = combined image sampler, binding 2 = overlay UBO
    m_overlaySrb.reset(rhi->newShaderResourceBindings());
    m_overlaySrb->setBindings({QRhiShaderResourceBinding::sampledTexture(
                                   1, QRhiShaderResourceBinding::FragmentStage,
                                   m_overlayTex.get(), m_overlaySampler.get()),
                               QRhiShaderResourceBinding::uniformBuffer(
                                   2, QRhiShaderResourceBinding::FragmentStage,
                                   m_overlayUbo.get())});
    m_overlaySrb->create();

    QShader vs = loadShader(QStringLiteral(":/disp/shaders/viewers/helpers/shaders/overlay.vert.qsb"));
    QShader fs = loadShader(QStringLiteral(":/disp/shaders/viewers/helpers/shaders/overlay.frag.qsb"));
    if (!vs.isValid() || !fs.isValid()) {
        m_overlayVbo.reset();
        m_overlayUbo.reset();
        m_overlaySrb.reset();
        m_overlaySampler.reset();
        m_overlayTex.reset();
        return;
    }

    m_overlayPipeline.reset(rhi->newGraphicsPipeline());
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_overlayPipeline->setTargetBlends({blend});
    m_overlayPipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    m_overlayPipeline->setDepthTest(false);
    m_overlayPipeline->setDepthWrite(false);
    m_overlayPipeline->setShaderStages({
        {QRhiShaderStage::Vertex, vs},
        {QRhiShaderStage::Fragment, fs},
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding(4 * sizeof(float))});
    inputLayout.setAttributes({QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0),
                               QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float))});
    m_overlayPipeline->setVertexInputLayout(inputLayout);
    m_overlayPipeline->setShaderResourceBindings(m_overlaySrb.get());
    m_overlayPipeline->setRenderPassDescriptor(target->renderPassDescriptor());

    if (!m_overlayPipeline->create()) {
        mne::Log::warning("ChannelRhiView: overlay pipeline create failed");
        m_overlayPipeline.reset();
        return;
    }

    // The static VBO needs an initial upload in the frame's resource batch.
    m_overlayVboNeedsUpload = true;
}

//=============================================================================================================

bool ChannelRhiRenderer::ensureOverlayResources(QRhi* rhi, QRhiRenderTarget* target, QRhiResourceUpdateBatch* batch)
{
    ensureOverlayPipeline(rhi, target);
    if (!m_overlayPipeline || !m_overlayVbo || !m_overlayUbo)
        return false;

    // Upload the static quad VBO if it was just created
    if (m_overlayVboNeedsUpload) {
        batch->uploadStaticBuffer(m_overlayVbo.get(), 0,
                                  static_cast<quint32>(sizeof(kQuadVerts)), kQuadVerts);
        m_overlayVboNeedsUpload = false;
    }
    return true;
}

//=============================================================================================================

bool ChannelRhiRenderer::hudVisible() const
{
    return m_view.overlays_visible();
}

//=============================================================================================================

void ChannelRhiRenderer::paintHud(QPainter& painter) const
{
    painter.setRenderHint(QPainter::Antialiasing, false);
    const QFont font = hostFont();
    const QtTextMetrics metrics(font);
    paintScene(painter, view().render_overlays(metrics), font);
}

//=============================================================================================================
// HUD quad — crosshair, scalebars, ruler and annotation selection painted into a screen-sized QImage and
// drawn with the overlay pipeline after the traces. The shader parameters map the screen 1:1 to the texture
// (no bands: gridEnabled = 0, sfreq = 0).
//=============================================================================================================

bool ChannelRhiRenderer::prepareHud(QRhi* rhi, QRhiResourceUpdateBatch* batch, int logicalW, int logicalH, qreal dpr)
{
    if (!hudVisible() || logicalW <= 0 || logicalH <= 0)
        return false;

    if (!m_hudUbo) {
        m_hudUbo.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, kOverlayUboSize));
        if (!m_hudUbo->create()) {
            m_hudUbo.reset();
            return false;
        }
    }

    const qreal hudDpr = qMax(dpr, 1.0);
    const QSize requiredTexSize(qMax(1, qRound(logicalW * hudDpr)), qMax(1, qRound(logicalH * hudDpr)));
    if (!m_hudTex || !m_hudSrb || requiredTexSize != m_hudTexSize) {
        m_hudTex.reset(rhi->newTexture(QRhiTexture::RGBA8, requiredTexSize));
        if (!m_hudTex->create()) {
            m_hudTex.reset();
            return false;
        }
        // Same layout as m_overlaySrb, so the overlay pipeline can draw with it
        m_hudSrb.reset(rhi->newShaderResourceBindings());
        m_hudSrb->setBindings({QRhiShaderResourceBinding::sampledTexture(
                                   1, QRhiShaderResourceBinding::FragmentStage,
                                   m_hudTex.get(), m_overlaySampler.get()),
                               QRhiShaderResourceBinding::uniformBuffer(
                                   2, QRhiShaderResourceBinding::FragmentStage,
                                   m_hudUbo.get())});
        m_hudSrb->create();
        m_hudTexSize = requiredTexSize;
        m_hudDirty = true;
    }

    if (m_hudDirty) {
        m_hudImage = QImage(requiredTexSize, QImage::Format_RGBA8888);
        m_hudImage.setDevicePixelRatio(hudDpr);
        m_hudImage.fill(Qt::transparent);
        {
            QPainter p(&m_hudImage);
            paintHud(p);
        }
        QRhiTextureUploadEntry entry(0, 0, QRhiTextureSubresourceUploadDescription(m_hudImage));
        batch->uploadTexture(m_hudTex.get(), entry);
        m_hudDirty = false;
    }

    OverlayParams params;
    params.scrollSample = 0.f;
    params.samplesPerPixel = 1.f;
    params.viewWidth = static_cast<float>(logicalW);
    params.sfreq = 0.f;
    params.firstFileSample = 0.f;
    params.gridEnabled = 0.f;
    params.overlayFirstSample = 0.f;
    params.overlayTotalSamples = static_cast<float>(logicalW);
    batch->updateDynamicBuffer(m_hudUbo.get(), 0, static_cast<quint32>(sizeof(params)), &params);
    return true;
}

//=============================================================================================================

void ChannelRhiRenderer::renderFrame(QRhi* rhi, QRhiRenderTarget* target, QRhiCommandBuffer* cb)
{
    tickAnimations();
    const bool hasTraces = m_model && totalLogicalChannels() > 0;

    // ── Ensure GPU resources ─────────────────────────────────────────────
    if (hasTraces) {
        ensurePipeline(rhi, target);
        if (!m_pipeline) {
            // Pipeline not ready: show RED background so the failure is visible
            QRhiResourceUpdateBatch* u = rhi->nextResourceUpdateBatch();
            cb->beginPass(target, QColor(220, 0, 0), {1.f, 0}, u);
            cb->endPass();
            return;
        }
    }

    QSize ps = target->pixelSize();
    const int pw = ps.width();
    const int ph = ps.height();
    const int logicalW = viewWidth();
    const int logicalH = viewHeight();
    const qreal overlayDpr = (logicalW > 0) ? (static_cast<qreal>(pw) / static_cast<qreal>(logicalW))
                                            : (m_pHost ? m_pHost->surfaceDevicePixelRatio() : 1.0);

    QRhiResourceUpdateBatch* batch = rhi->nextResourceUpdateBatch();

    bool overlayReady = false;
    if (hasTraces) {
        if (isVboDirty())
            rebuildVBOs(rhi, batch);

        updateUBO(batch);

        // ── Overlay image (annotations + events; bands computed in shader) ──
        if (ensureOverlayResources(rhi, target, batch) && pw > 0 && ph > 0 && logicalW > 0 && logicalH > 0) {
            // Compute the overlay prefetch window (wider than the viewport)
            const float visibleSamples = static_cast<float>(logicalW) * samplesPerPixel();
            const float extraSamples = kOverlayPrefetchFactor * visibleSamples;
            const float overlayFirstSample = scrollSample() - extraSamples;
            const float overlayTotalSamples = visibleSamples + 2.0f * extraSamples;
            // Width of overlay texture in logical pixels = 1 + 2*prefetch factor times viewport
            const int overlayLogicalW = static_cast<int>(std::ceil(
                (1.0f + 2.0f * kOverlayPrefetchFactor) * static_cast<float>(logicalW)));

            // Rebuild the overlay QImage if dirty or resized
            const QSize requiredTexSize(qRound(overlayLogicalW * overlayDpr),
                                        qRound(logicalH * overlayDpr));
            if (m_overlayDirty || requiredTexSize != m_overlayTexSize) {
                // Store the sample range covered by this overlay build
                m_overlayFirstSample = overlayFirstSample;
                m_overlayTotalSamples = overlayTotalSamples;

                rebuildOverlayImage(overlayLogicalW, logicalH, overlayDpr);

                // (Re)create texture at the correct pixel size
                if (requiredTexSize != m_overlayTexSize) {
                    m_overlayTex.reset(rhi->newTexture(QRhiTexture::RGBA8, requiredTexSize));
                    m_overlayTex->create();
                    // Re-create SRB because it references the texture
                    m_overlaySrb->setBindings({QRhiShaderResourceBinding::sampledTexture(
                                                   1, QRhiShaderResourceBinding::FragmentStage,
                                                   m_overlayTex.get(), m_overlaySampler.get()),
                                               QRhiShaderResourceBinding::uniformBuffer(
                                                   2, QRhiShaderResourceBinding::FragmentStage,
                                                   m_overlayUbo.get())});
                    m_overlaySrb->create();
                    m_overlayTexSize = requiredTexSize;
                }
                QRhiTextureUploadEntry entry(0, 0, QRhiTextureSubresourceUploadDescription(m_overlayImage));
                batch->uploadTexture(m_overlayTex.get(), entry);
            }

            // Upload per-frame overlay UBO (scroll params for shader-computed bands + UV mapping)
            OverlayParams params;
            params.scrollSample = scrollSample();
            params.samplesPerPixel = samplesPerPixel();
            params.viewWidth = static_cast<float>(logicalW);
            params.sfreq = static_cast<float>(m_view.sfreq());
            params.firstFileSample = static_cast<float>(m_view.first_file_sample());
            params.gridEnabled = m_gridVisible ? 1.0f : 0.0f;
            params.overlayFirstSample = m_overlayFirstSample;
            params.overlayTotalSamples = m_overlayTotalSamples;
            batch->updateDynamicBuffer(m_overlayUbo.get(), 0,
                                       static_cast<quint32>(sizeof(params)), &params);

            overlayReady = true;
        }
    }

    // ── HUD image (crosshair, scalebars, ruler, annotation selection) ──
    const bool hudReady = hudVisible() && pw > 0 && ph > 0 && ensureOverlayResources(rhi, target, batch) && prepareHud(rhi, batch, logicalW, logicalH, overlayDpr);

    // ── Render pass ──────────────────────────────────────────────────────
    QColor bg = m_bgColor;
    cb->beginPass(target, bg, {1.f, 0}, batch);

    cb->setViewport(QRhiViewport(0.f, 0.f, static_cast<float>(pw),
                                 static_cast<float>(ph)));

    if (hasTraces) {
        // ── Waveform traces ──────────────────────────────────────────────────
        cb->setGraphicsPipeline(m_pipeline.get());

        // One draw per trace; its uniforms are in the UBO slot of the same index
        for (int slot = 0; slot < std::ssize(m_traceDraws); ++slot) {
            const int logCh = m_traceDraws[static_cast<std::size_t>(slot)].channel;
            if (logCh >= std::ssize(m_gpuChannels))
                continue;
            const ChannelGpuData& gd = m_gpuChannels[static_cast<std::size_t>(logCh)];
            if (!gd.vbo || gd.vertexCount < 2)
                continue;

            QRhiCommandBuffer::DynamicOffset dynOff{0, static_cast<quint32>(slot * m_uboStride)};
            cb->setShaderResources(m_srb.get(), 1, &dynOff);
            QRhiCommandBuffer::VertexInput vi(gd.vbo.get(), 0);
            cb->setVertexInput(0, 1, &vi);
            cb->draw(static_cast<quint32>(gd.vertexCount));
        }
    }

    QRhiCommandBuffer::VertexInput overlayVi(m_overlayVbo.get(), 0);

    // ── Overlay blit (bands + event lines) — drawn after waveforms ───────
    if (overlayReady) {
        cb->setGraphicsPipeline(m_overlayPipeline.get());
        cb->setShaderResources(m_overlaySrb.get());
        cb->setVertexInput(0, 1, &overlayVi);
        cb->draw(4); // TriangleStrip: 4 vertices = 2 triangles = full-screen quad
    }

    // ── HUD blit (crosshair, scalebars, ruler, annotation selection) — on top of everything ──
    if (hudReady) {
        cb->setGraphicsPipeline(m_overlayPipeline.get());
        cb->setShaderResources(m_hudSrb.get());
        cb->setVertexInput(0, 1, &overlayVi);
        cb->draw(4);
    }

    cb->endPass();
}

//=============================================================================================================

QImage ChannelRhiRenderer::renderToImage(const QSize& size) const
{
    const int pw = size.isValid() ? size.width() : viewWidth();
    const int ph = size.isValid() ? size.height() : viewHeight();
    if (!m_model || pw <= 0 || ph <= 0 || samplesPerPixel() <= 0.f) {
        return QImage();
    }

    const QFont font = QGuiApplication::font();
    const QtTextMetrics metrics(font);
    scene::Canvas canvas(pw, ph);
    scene::draw_trace_figure(canvas, metrics, scene::Font{}, view(), *m_traceSource, m_markers, traceStyle());

    QImage img(pw, ph, QImage::Format_RGB32);
    img.fill(m_bgColor.rgb());
    QPainter p(&img);
    paintScene(p, canvas.display_list(), font);
    return img;
}


//=============================================================================================================
// Shared event handlers
//=============================================================================================================

void ChannelRhiRenderer::handleResize()
{
    m_geometry.invalidate();
    m_overlayDirty = true;
    emit viewResized(viewWidth(), viewHeight());
    requestUpdate();
}

//=============================================================================================================

bool ChannelRhiRenderer::handleWheel(QWheelEvent* event)
{
    scene::WheelEvent wheel;
    wheel.x = event->position().x();
    wheel.y = event->position().y();
    wheel.angle_delta_x = event->angleDelta().x();
    wheel.angle_delta_y = event->angleDelta().y();
    wheel.modifiers = toSceneModifiers(event->modifiers());
    wheel.timestamp_ms = monotonicMs();
    view().handle_wheel(wheel);
    event->accept();
    return true;
}

//=============================================================================================================

bool ChannelRhiRenderer::handleMousePress(QMouseEvent* event)
{
    return forwardPointer(event, scene::PointerEvent::Type::Press);
}

//=============================================================================================================

bool ChannelRhiRenderer::handleMouseMove(QMouseEvent* event)
{
    return forwardPointer(event, scene::PointerEvent::Type::Move);
}

//=============================================================================================================

bool ChannelRhiRenderer::handleMouseRelease(QMouseEvent* event)
{
    return forwardPointer(event, scene::PointerEvent::Type::Release);
}

//=============================================================================================================

bool ChannelRhiRenderer::handleMouseDoubleClick(QMouseEvent* event)
{
    // Double clicks toggle the bad state of a model channel
    if (!m_model)
        return false;
    return forwardPointer(event, scene::PointerEvent::Type::DoubleClick);
}

//=============================================================================================================

bool ChannelRhiRenderer::forwardPointer(QMouseEvent* event, scene::PointerEvent::Type type)
{
    scene::PointerEvent pointer;
    pointer.type = type;
    pointer.x = event->position().x();
    pointer.y = event->position().y();
    pointer.button = toSceneButton(event->button());
    pointer.modifiers = toSceneModifiers(event->modifiers());
    pointer.timestamp_ms = monotonicMs();
    const bool used = view().handle_pointer(pointer);
    if (used)
        event->accept();
    return used;
}
