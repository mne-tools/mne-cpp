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
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

#include <rhi/qrhi.h>
#include <rhi/qshader.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <QPropertyAnimation>
#include <QWheelEvent>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cmath>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;
namespace scene = mne::scene;

//=============================================================================================================
// Uniform block layout — must match channeldata.vert / channeldata.frag
//=============================================================================================================

namespace
{
// Byte offsets inside one aligned UBO slot
constexpr int kUboOffsetColor = 0;           // vec4  (16 bytes)
constexpr int kUboOffsetFirstSample = 16;    // float
constexpr int kUboOffsetScrollSample = 20;   // float
constexpr int kUboOffsetSampPerPixel = 24;   // float
constexpr int kUboOffsetViewWidth = 28;      // float
constexpr int kUboOffsetViewHeight = 32;     // float
constexpr int kUboOffsetChannelYCenter = 36; // float
constexpr int kUboOffsetChannelYRange = 40;  // float
constexpr int kUboOffsetAmplitudeMax = 44;   // float
constexpr int kUboOffsetShowClipping = 48;   // float
// Total used: 52 bytes  — padded to m_uboStride (≥ 256) per dynamic offset rules.

constexpr int kMaxChannels = 1024; // Upper hard limit for UBO pre-allocation

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

static void writeFloat(quint8* base, int byteOffset, float v)
{
    memcpy(base + byteOffset, &v, sizeof(float));
}

static void writeFloats(quint8* base, int byteOffset, const float* data, int count)
{
    memcpy(base + byteOffset, data, count * sizeof(float));
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
        return Channel{std::move(info.name), std::move(info.typeLabel), info.amplitudeMax};
    }

    float value_at(int index, int sample) const override
    {
        const int ch = m_renderer.actualChannelAt(index);
        return (m_renderer.m_model && ch >= 0) ? m_renderer.m_model->sampleValueAt(ch, sample) : 0.f;
    }

    std::vector<Group> groups() const override
    {
        std::vector<Group> groups;
        for (const ButterflyTypeGroup& group : m_renderer.butterflyTypeGroups())
            groups.push_back({group.typeLabel, group.amplitudeMax});
        return groups;
    }

    int first_sample() const override
    {
        const ChannelDataModel* model = m_renderer.m_model.data();
        return (model && model->totalSamples() > 0) ? model->firstSample() : 0;
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

        // Check whether the prefetch window is still valid
        const float visible = static_cast<float>(viewWidth()) * samplesPerPixel();
        const float margin = m_prefetchFactor * visible;
        if (sample < m_vboWindowFirst + margin || sample + visible > m_vboWindowLast - margin) {
            m_vboDirty = true;
        }

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
        m_vboDirty = true; // zoom change → decimation changes
        m_overlayDirty = true;
        emit samplesPerPixelChanged(samplesPerPixel());
        requestUpdate();
    };
    effects.channel_offset_changed = [this](int firstChannel) {
        m_vboDirty = true;
        m_pipelineDirty = true;
        emit channelOffsetChanged(firstChannel);
        requestUpdate();
    };
    effects.animate_scroll = [this](const scene::TraceView::ScrollAnimation& animation) {
        auto* anim = new QPropertyAnimation(this, "scrollSample", this);
        anim->setDuration(animation.duration_ms);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->setStartValue(static_cast<float>(animation.from));
        anim->setEndValue(static_cast<float>(animation.to));
        if (animation.inertial) {
            m_pInertialAnim = anim;
            connect(anim, &QPropertyAnimation::finished, this, [this]() {
                m_pInertialAnim = nullptr;
            });
        } else {
            m_pScrollAnim = anim;
            connect(anim, &QPropertyAnimation::finished, this, [this]() {
                m_view.scroll_animation_finished();
            });
        }
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    };
    effects.animate_zoom = [this](const scene::TraceView::ZoomAnimation& animation) {
        auto* anim = new QPropertyAnimation(this, "samplesPerPixel", this);
        anim->setDuration(animation.duration_ms);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->setStartValue(static_cast<float>(animation.from));
        anim->setEndValue(static_cast<float>(animation.to));
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    };
    effects.stop_animations = [this] {
        stopScrollAnimations();
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

ChannelRhiRenderer::~ChannelRhiRenderer() = default;

//=============================================================================================================

void ChannelRhiRenderer::setHost(ChannelRhiHost* host)
{
    m_pHost = host;
    m_vboDirty = true;
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
            m_vboDirty = true;
            requestUpdate();
        });
        connect(m_model, &ChannelDataModel::metaChanged, this, [this] {
            m_vboDirty = true;
            m_pipelineDirty = true;
            requestUpdate();
        });
    }
    m_vboDirty = true;
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

void ChannelRhiRenderer::stopScrollAnimations()
{
    if (m_pScrollAnim) {
        m_pScrollAnim->stop();
    }
    if (m_pInertialAnim) {
        m_pInertialAnim->stop();
        m_pInertialAnim = nullptr;
    }
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
    m_prefetchFactor = qMax(factor, 0.1f);
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
    m_vboDirty = true;
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
    m_vboDirty = true;
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
    m_vboDirty = true;
    m_pipelineDirty = true;
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

std::vector<ChannelRhiRenderer::ButterflyTypeGroup> ChannelRhiRenderer::butterflyTypeGroups() const
{
    std::vector<ButterflyTypeGroup> groups;
    if (!m_model)
        return groups;

    const std::vector<int> allCh = effectiveChannelIndices();
    std::map<std::string, int> typeToGroup; // typeLabel → index in groups

    for (int ch : allCh) {
        auto info = m_model->channelInfo(ch);
        if (m_hideBadChannels && info.bad)
            continue;
        int gIdx;
        if (typeToGroup.contains(info.typeLabel)) {
            gIdx = typeToGroup[info.typeLabel];
        } else {
            gIdx = static_cast<int>(groups.size());
            typeToGroup[info.typeLabel] = gIdx;
            ButterflyTypeGroup g;
            g.typeLabel = info.typeLabel;
            g.color = info.color;
            g.amplitudeMax = info.amplitudeMax;
            groups.push_back(g);
        }
        groups[gIdx].channelIndices.push_back(ch);
    }
    return groups;
}

//=============================================================================================================

int ChannelRhiRenderer::butterflyLaneCount() const
{
    if (!m_model)
        return 0;
    const std::vector<int> allCh = effectiveChannelIndices();
    std::set<std::string> types;
    for (int ch : allCh) {
        auto info = m_model->channelInfo(ch);
        if (m_hideBadChannels && info.bad)
            continue;
        types.insert(info.typeLabel);
    }
    return static_cast<int>(types.size());
}

//=============================================================================================================

void ChannelRhiRenderer::setChannelIndices(const std::vector<int>& indices)
{
    m_filteredChannels = indices;
    // Clamp the channel window to the new range
    m_view.set_first_visible_channel(m_view.first_visible_channel());
    m_vboDirty = true;
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
    std::vector<int> indices;

    if (!m_model) {
        return indices;
    }

    if (m_filteredChannels.empty()) {
        indices.reserve(m_model->channelCount());
        for (int channelIndex = 0; channelIndex < m_model->channelCount(); ++channelIndex) {
            indices.push_back(channelIndex);
        }
    } else {
        indices = m_filteredChannels;
    }

    if (!m_hideBadChannels) {
        return indices;
    }

    std::vector<int> visibleIndices;
    visibleIndices.reserve(indices.size());
    for (int channelIndex : std::as_const(indices)) {
        if (channelIndex < 0) {
            continue;
        }

        const ChannelDisplayInfo info = m_model->channelInfo(channelIndex);
        if (!info.bad) {
            visibleIndices.push_back(channelIndex);
        }
    }

    return visibleIndices;
}

//=============================================================================================================

void ChannelRhiRenderer::setEvents(const std::vector<EventMarker>& events)
{
    m_events = events;
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setEpochMarkers(const std::vector<int>& triggerSamples)
{
    m_epochTriggerSamples = triggerSamples;
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setEpochMarkersVisible(bool visible)
{
    if (m_bShowEpochMarkers == visible)
        return;
    m_bShowEpochMarkers = visible;
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
    m_vboDirty = true; // VBO data changes (z-score normalization)
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

std::vector<ChannelRhiRenderer::AnnotationSpan> ChannelRhiRenderer::annotationSpans() const
{
    std::vector<AnnotationSpan> spans;
    spans.reserve(m_view.annotations().size());
    for (const scene::TraceView::Annotation& annotation : m_view.annotations()) {
        const scene::Color& c = annotation.color;
        spans.push_back({annotation.start_sample, annotation.end_sample, QColor(c.r, c.g, c.b, c.a), annotation.label});
    }
    return spans;
}

//=============================================================================================================

void ChannelRhiRenderer::setAnnotationSelectionEnabled(bool enabled)
{
    m_view.set_annotation_selection_enabled(enabled);
}

//=============================================================================================================

void ChannelRhiRenderer::setEventsVisible(bool visible)
{
    if (m_bShowEvents == visible)
        return;
    m_bShowEvents = visible;
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setAnnotationsVisible(bool visible)
{
    if (m_bShowAnnotations == visible)
        return;
    m_bShowAnnotations = visible;
    m_overlayDirty = true;
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::initializeResources()
{
    m_pipelineDirty = true;
    m_vboDirty = true;
}

//=============================================================================================================

void ChannelRhiRenderer::releaseResources()
{
    m_pipeline.reset();
    m_srb.reset();
    m_ubo.reset();
    m_gpuChannels.clear();
    m_pipelineDirty = true;
    m_vboDirty = true;

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
        (52 + rhi->ubufAlignment() - 1) & ~(rhi->ubufAlignment() - 1));

    // UBO has one slot per *visible* channel row, not all channels
    // In butterfly mode, we need a slot for EVERY channel (all overlaid)
    int totalCh = totalLogicalChannels();
    int nCh;
    if (m_view.butterfly_mode()) {
        nCh = qMin(totalCh, kMaxChannels);
    } else {
        nCh = qMin(m_view.visible_channel_count(), totalCh - m_view.first_visible_channel());
    }
    nCh = qMax(nCh, 1);
    nCh = qMin(nCh, kMaxChannels);

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
            52 // visible block size for the shader
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
    if (!m_model)
        return false;
    float visible = viewWidth() * samplesPerPixel();
    float margin = m_prefetchFactor * visible;
    return m_vboDirty || scrollSample() < m_vboWindowFirst + margin || (scrollSample() + visible) > m_vboWindowLast - margin;
}

//=============================================================================================================

void ChannelRhiRenderer::rebuildVBOs(QRhi* rhi, QRhiResourceUpdateBatch* batch)
{
    if (!m_model)
        return;

    int nCh = totalLogicalChannels();
    int px = viewWidth();
    float visible = px * samplesPerPixel();

    // Prefetch window: [scroll - prefetch*visible, scroll + (1+prefetch)*visible]
    float windowFirst = scrollSample() - m_prefetchFactor * visible;
    float windowLast = scrollSample() + (1.f + m_prefetchFactor) * visible;

    int iFirst = qMax(static_cast<int>(windowFirst), m_model->firstSample());
    int iLast = qMin(static_cast<int>(windowLast),
                     m_model->firstSample() + m_model->totalSamples());
    if (iFirst >= iLast) {
        m_vboDirty = false;
        return;
    }

    m_vboWindowFirst = iFirst;
    m_vboWindowLast = iLast;

    // VBOs are indexed by logical (filtered) channel index, not model channel index
    m_gpuChannels.resize(nCh);

    // Compute the max vertex count across channels to right-size allocations
    int prefetchedSamples = iLast - iFirst;
    // With decimation, vertices ≤ 2 * px * prefetchFactor * (1 + prefetchFactor)
    // Use a conservative upper bound
    int maxVertices = qMax(prefetchedSamples * 2, 2 * px * 4);
    Q_UNUSED(maxVertices)

    for (int logCh = 0; logCh < nCh; ++logCh) {
        int ch = actualChannelAt(logCh); // actual model channel index
        if (ch < 0) {
            m_gpuChannels[logCh].vertexCount = 0;
            continue;
        }
        int vboFirst = 0;
        std::vector<float> verts = m_model->decimatedVertices(
            ch, iFirst, iLast, static_cast<int>(prefetchedSamples / samplesPerPixel()), vboFirst);

        if (verts.empty()) {
            m_gpuChannels[logCh].vertexCount = 0;
            continue;
        }

        // Z-score normalization: replace raw amplitudes with (y - mean) / std
        if (m_bZScoreMode) {
            int nv = static_cast<int>(verts.size()) / 2;
            if (nv > 1) {
                double sum = 0.0, sumSq = 0.0;
                for (int v = 0; v < nv; ++v) {
                    double a = static_cast<double>(verts[v * 2 + 1]);
                    sum += a;
                    sumSq += a * a;
                }
                float mean = static_cast<float>(sum / nv);
                double var = sumSq / nv - static_cast<double>(mean) * mean;
                float sd = var > 0.0 ? static_cast<float>(std::sqrt(var)) : 1.f;
                for (int v = 0; v < nv; ++v)
                    verts[v * 2 + 1] = (verts[v * 2 + 1] - mean) / sd;
            }
        }

        int vertexCount = static_cast<int>(verts.size()) / 2; // each vertex is (x, y) = 2 floats
        quint32 byteSize = static_cast<quint32>(verts.size() * sizeof(float));

        auto& gd = m_gpuChannels[logCh];

        // Re-create buffer if size changed significantly
        if (!gd.vbo || static_cast<quint32>(gd.vbo->size()) < byteSize) {
            gd.vbo.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                        QRhiBuffer::VertexBuffer,
                                        byteSize));
            if (!gd.vbo->create()) {
                mne::Log::warning("ChannelRhiView: VBO create failed for channel {}", logCh);
                gd.vertexCount = 0;
                continue;
            }
        }

        batch->updateDynamicBuffer(gd.vbo.get(), 0, byteSize,
                                   verts.data());
        gd.vertexCount = vertexCount;
        gd.vboFirstSample = vboFirst;
    }

    m_vboDirty = false;
}

//=============================================================================================================

void ChannelRhiRenderer::updateUBO(QRhiResourceUpdateBatch* batch)
{
    if (!m_model || !m_ubo)
        return;

    int totalCh = totalLogicalChannels();

    // ── Butterfly mode: one UBO slot per channel, type-based lane positions ──
    if (m_view.butterfly_mode()) {
        const auto groups = butterflyTypeGroups();
        int nLanes = static_cast<int>(groups.size());
        if (nLanes <= 0)
            return;

        // Build channel→lane map
        std::unordered_map<int, int> chToLane; // model channel idx → lane index
        for (int g = 0; g < std::ssize(groups); ++g)
            for (int ch : groups[g].channelIndices)
                chToLane[ch] = g;

        float vw = static_cast<float>(viewWidth());
        float vh = static_cast<float>(viewHeight());
        float laneRange = 2.f / nLanes; // NDC height per lane

        QVarLengthArray<quint8> buf(m_uboStride, 0);
        int nToUpload = qMin(totalCh, kMaxChannels);

        for (int logCh = 0; logCh < nToUpload; ++logCh) {
            int ch = actualChannelAt(logCh);
            memset(buf.data(), 0, m_uboStride);

            auto info = (ch >= 0) ? m_model->channelInfo(ch) : ChannelDisplayInfo{};
            bool hideThis = (ch < 0) || (m_hideBadChannels && info.bad);

            const auto laneIt = chToLane.find(ch);
            int lane = laneIt != chToLane.end() ? laneIt->second : -1;
            if (lane < 0)
                hideThis = true;

            QColor col = hideThis ? m_bgColor
                                  : (info.bad ? QColor(200, 60, 60, 180) : info.color);
            float yRng = hideThis ? 0.f : laneRange;

            float yCenter = (lane >= 0)
                ? (1.f - laneRange * (lane + 0.5f))
                : 0.f;

            float rgba[4] = {
                static_cast<float>(col.redF()),
                static_cast<float>(col.greenF()),
                static_cast<float>(col.blueF()),
                static_cast<float>(col.alphaF())};

            auto* d = buf.data();
            writeFloats(d, kUboOffsetColor, rgba, 4);
            writeFloat(d, kUboOffsetFirstSample, static_cast<float>(logCh < static_cast<int>(m_gpuChannels.size()) ? m_gpuChannels[logCh].vboFirstSample : 0));
            writeFloat(d, kUboOffsetScrollSample, scrollSample());
            writeFloat(d, kUboOffsetSampPerPixel, samplesPerPixel());
            writeFloat(d, kUboOffsetViewWidth, vw);
            writeFloat(d, kUboOffsetViewHeight, vh);
            writeFloat(d, kUboOffsetChannelYCenter, yCenter);
            writeFloat(d, kUboOffsetChannelYRange, yRng);
            writeFloat(d, kUboOffsetAmplitudeMax, m_bZScoreMode ? 4.f : info.amplitudeMax);
            writeFloat(d, kUboOffsetShowClipping, (m_bShowClipping && !info.bad && !m_bZScoreMode) ? 1.f : 0.f);

            batch->updateDynamicBuffer(m_ubo.get(),
                                       logCh * m_uboStride,
                                       m_uboStride,
                                       buf.constData());
        }
        return;
    }

    // ── Normal mode: one UBO slot per visible row ──
    int firstCh = qBound(0, m_view.first_visible_channel(), totalCh);
    int visCnt = qMin(m_view.visible_channel_count(), totalCh - firstCh);
    int nCh = qMin(visCnt, kMaxChannels);
    if (nCh <= 0)
        return;

    float vw = static_cast<float>(viewWidth());
    float vh = static_cast<float>(viewHeight());
    float laneRange = 2.f / nCh; // NDC height of one visible channel row

    QVarLengthArray<quint8> buf(m_uboStride, 0);

    for (int i = 0; i < nCh; ++i) {
        int logCh = firstCh + i;         // logical (filtered) index
        int ch = actualChannelAt(logCh); // actual model channel index
        memset(buf.data(), 0, m_uboStride);

        auto info = (ch >= 0) ? m_model->channelInfo(ch) : ChannelDisplayInfo{};
        bool hideThis = (ch < 0) || (m_hideBadChannels && info.bad);
        // When hiding: use background colour so no trace is painted
        QColor col = hideThis ? m_bgColor
                              : (info.bad ? QColor(200, 60, 60, 180) : info.color);
        // When hiding bad channel: zero amplitude range → flat invisible line
        float yRng = hideThis ? 0.f : laneRange;

        float rgba[4] = {
            static_cast<float>(col.redF()),
            static_cast<float>(col.greenF()),
            static_cast<float>(col.blueF()),
            static_cast<float>(col.alphaF())};

        // Visible row i: top at NDC +1, bottom at NDC -1
        float yCenter = 1.f - laneRange * (i + 0.5f);

        auto* d = buf.data();
        writeFloats(d, kUboOffsetColor, rgba, 4);
        // VBO indexed by logical channel (logCh), not model channel
        writeFloat(d, kUboOffsetFirstSample, static_cast<float>(logCh < static_cast<int>(m_gpuChannels.size()) ? m_gpuChannels[logCh].vboFirstSample : 0));
        writeFloat(d, kUboOffsetScrollSample, scrollSample());
        writeFloat(d, kUboOffsetSampPerPixel, samplesPerPixel());
        writeFloat(d, kUboOffsetViewWidth, vw);
        writeFloat(d, kUboOffsetViewHeight, vh);
        writeFloat(d, kUboOffsetChannelYCenter, yCenter);
        writeFloat(d, kUboOffsetChannelYRange, yRng);
        writeFloat(d, kUboOffsetAmplitudeMax, m_bZScoreMode ? 4.f : info.amplitudeMax);
        writeFloat(d, kUboOffsetShowClipping, (m_bShowClipping && !info.bad && !m_bZScoreMode) ? 1.f : 0.f);

        // UBO slot i corresponds to visible row i
        batch->updateDynamicBuffer(m_ubo.get(),
                                   i * m_uboStride,
                                   m_uboStride,
                                   buf.constData());
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

    if (logicalWidth <= 0 || logicalHeight <= 0 || static_cast<float>(m_view.sfreq()) <= 0.f || samplesPerPixel() <= 0.f) {
        m_overlayDirty = false;
        return;
    }

    // The overlay covers m_overlayFirstSample .. m_overlayFirstSample + m_overlayTotalSamples.
    // Map sample positions to pixel X using the overlay's own coordinate system.
    const float overlayFirst = m_overlayFirstSample;
    const float overlayTotal = m_overlayTotalSamples;
    const float overlayPixelsPerSample = (overlayTotal > 0.f)
        ? static_cast<float>(logicalWidth) / overlayTotal
        : 0.f;

    QPainter p(&m_overlayImage);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // Note: alternating per-second bands are now computed per-pixel in the
    // fragment shader — no QPainter band rendering here.

    // ── Annotation spans ────────────────────────────────────────────
    if (m_bShowAnnotations && !m_view.annotations().empty()) {
        QFont font = p.font();
        font.setPointSizeF(8.0);
        font.setBold(true);
        p.setFont(font);

        for (const AnnotationSpan& annotation : annotationSpans()) {
            const float xStart = (static_cast<float>(annotation.startSample) - overlayFirst) * overlayPixelsPerSample;
            const float xEnd = (static_cast<float>(annotation.endSample + 1) - overlayFirst) * overlayPixelsPerSample;
            if (xEnd < -2.f || xStart > logicalWidth + 2.f) {
                continue;
            }

            const float clippedStart = qBound(0.f, xStart, static_cast<float>(logicalWidth));
            const float clippedEnd = qBound(0.f, xEnd, static_cast<float>(logicalWidth));
            if (clippedEnd <= clippedStart) {
                continue;
            }

            QColor fillColor = annotation.color;
            fillColor.setAlpha(48);
            p.fillRect(QRectF(clippedStart, 0.f, clippedEnd - clippedStart, static_cast<float>(logicalHeight)),
                       fillColor);

            QColor borderColor = annotation.color;
            borderColor.setAlpha(165);
            p.setPen(QPen(borderColor, 1));
            p.drawLine(QPointF(clippedStart, 0.f), QPointF(clippedStart, static_cast<float>(logicalHeight)));
            p.drawLine(QPointF(clippedEnd, 0.f), QPointF(clippedEnd, static_cast<float>(logicalHeight)));

            if (!mne::trimmed(annotation.label).empty()) {
                QString label = QString::fromStdString(std::string(mne::trimmed(annotation.label)));
                QFontMetrics metrics(font);
                QRect labelRect = metrics.boundingRect(label);
                labelRect.adjust(-6, -2, 6, 2);
                const int labelX = qBound(4,
                                          static_cast<int>(clippedStart) + 4,
                                          qMax(4, logicalWidth - labelRect.width() - 4));
                labelRect.moveTopLeft(QPoint(labelX, 4));
                QColor pillColor = annotation.color;
                pillColor.setAlpha(215);
                p.fillRect(labelRect, pillColor);
                p.setPen(Qt::white);
                p.drawText(labelRect, Qt::AlignCenter, label);
            }
        }
    }

    // ── Event / stimulus marker lines ───────────────────────────────
    if (m_bShowEvents && !m_events.empty()) {
        for (const EventMarker& ev : m_events) {
            float xF = (static_cast<float>(ev.sample) - overlayFirst) * overlayPixelsPerSample;
            if (xF < -2.f || xF > logicalWidth + 2.f)
                continue;
            QColor lineColor = ev.color;
            lineColor.setAlpha(180);
            p.setPen(QPen(lineColor, 1));
            p.drawLine(QPointF(xF, 0.f), QPointF(xF, static_cast<float>(logicalHeight)));
        }
    }

    // ── Epoch trigger marker lines ──────────────────────────────────
    if (m_bShowEpochMarkers && !m_epochTriggerSamples.empty()) {
        QPen epochPen(QColor(100, 100, 100, 140), 1, Qt::DashLine);
        p.setPen(epochPen);
        for (int trigSample : m_epochTriggerSamples) {
            float xF = (static_cast<float>(trigSample) - overlayFirst) * overlayPixelsPerSample;
            if (xF < -2.f || xF > logicalWidth + 2.f)
                continue;
            p.drawLine(QPointF(xF, 0.f), QPointF(xF, static_cast<float>(logicalHeight)));
        }
    }

    m_overlayDirty = false;
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

        int totalCh = totalLogicalChannels();

        if (m_view.butterfly_mode()) {
            // Butterfly: render ALL channels; UBO slots indexed by logical channel
            int nToRender = qMin(totalCh, kMaxChannels);
            for (int logCh = 0; logCh < nToRender; ++logCh) {
                if (logCh >= static_cast<int>(m_gpuChannels.size()))
                    break;
                auto& gd = m_gpuChannels[logCh];
                if (!gd.vbo || gd.vertexCount < 2)
                    continue;

                quint32 dynOffset = static_cast<quint32>(logCh * m_uboStride);
                QRhiCommandBuffer::DynamicOffset dynOff{0, dynOffset};
                cb->setShaderResources(m_srb.get(), 1, &dynOff);

                QRhiCommandBuffer::VertexInput vi(gd.vbo.get(), 0);
                cb->setVertexInput(0, 1, &vi);
                cb->draw(static_cast<quint32>(gd.vertexCount));
            }
        } else {
            // Normal: render only the visible channel window
            int firstCh = qBound(0, m_view.first_visible_channel(), totalCh);
            int visCnt = qMin(m_view.visible_channel_count(), totalCh - firstCh);
            int nToRender = qMin(visCnt, kMaxChannels);

            for (int i = 0; i < nToRender; ++i) {
                int logCh = firstCh + i;
                if (logCh >= static_cast<int>(m_gpuChannels.size()))
                    break;
                auto& gd = m_gpuChannels[logCh];
                if (!gd.vbo || gd.vertexCount < 2)
                    continue;

                quint32 dynOffset = static_cast<quint32>(i * m_uboStride);
                QRhiCommandBuffer::DynamicOffset dynOff{0, dynOffset};
                cb->setShaderResources(m_srb.get(), 1, &dynOff);

                QRhiCommandBuffer::VertexInput vi(gd.vbo.get(), 0);
                cb->setVertexInput(0, 1, &vi);
                cb->draw(static_cast<quint32>(gd.vertexCount));
            }
        } // end butterfly/normal branch
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

    // Same rows and horizontal scale as on screen: the time span shown stays the same at any output width
    const float spp = samplesPerPixel() * static_cast<float>(qMax(viewWidth(), 1)) / static_cast<float>(pw);
    const float scrollSample = this->scrollSample();
    const QColor bgColor = m_bgColor;
    const bool gridVisible = m_gridVisible;
    const float sfreq = static_cast<float>(m_view.sfreq());
    const int firstFileSample = m_view.first_file_sample();
    const bool zScoreMode = m_bZScoreMode;
    const bool showClipping = m_bShowClipping;
    const std::vector<EventMarker> events = m_bShowEvents ? m_events : std::vector<EventMarker>();
    const std::vector<AnnotationSpan> annotations = m_bShowAnnotations ? annotationSpans() : std::vector<AnnotationSpan>();
    const std::vector<int> epochMarkers = m_bShowEpochMarkers ? m_epochTriggerSamples : std::vector<int>();
    const ChannelDataModel* model = m_model.data();

    // (model channel, lane) for every trace to draw, laid out exactly like updateUBO()
    std::vector<std::pair<int, int>> traces;
    int laneCount = 0;
    if (m_view.butterfly_mode()) {
        const std::vector<ButterflyTypeGroup> groups = butterflyTypeGroups();
        laneCount = static_cast<int>(groups.size());
        for (int lane = 0; lane < laneCount; ++lane) {
            for (int ch : groups[lane].channelIndices) {
                traces.push_back({ch, lane});
            }
        }
    } else {
        const std::vector<int> channelIndices = effectiveChannelIndices();
        const int firstCh = qBound(0, m_view.first_visible_channel(), static_cast<int>(channelIndices.size()));
        laneCount = qMin(m_view.visible_channel_count(), static_cast<int>(channelIndices.size()) - firstCh);
        for (int lane = 0; lane < laneCount; ++lane) {
            traces.push_back({channelIndices[firstCh + lane], lane});
        }
    }
    if (laneCount <= 0) {
        QImage blank(pw, ph, QImage::Format_RGB32);
        blank.fill(bgColor.rgb());
        return blank;
    }

    QImage img(pw, ph, QImage::Format_RGB32);
    img.fill(bgColor.rgb());

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, false);

    const float laneH = static_cast<float>(ph) / static_cast<float>(laneCount);
    int firstSample = static_cast<int>(scrollSample);
    int lastSample = firstSample + static_cast<int>(pw * spp) + 1;

    // ── Alternating per-second background bands ─────────────────────
    // Draw subtle alternating grey/white bands every second, like MNE-Python browser.
    if (gridVisible && sfreq > 0.f) {
        float samplesPerSec = sfreq;
        float firstBound = std::floor(
                               (scrollSample - static_cast<float>(firstFileSample)) / samplesPerSec) *
                samplesPerSec +
            static_cast<float>(firstFileSample);

        // Determine parity of the first band (0 = even, 1 = odd)
        long long bandIndex = static_cast<long long>(
            (firstBound - static_cast<float>(firstFileSample)) / samplesPerSec);
        bool oddBand = (bandIndex & 1) != 0;

        // Odd seconds are darkened by 8 %, as in overlay.frag
        const QColor altColor(qRound(bgColor.red() * 0.92),
                              qRound(bgColor.green() * 0.92),
                              qRound(bgColor.blue() * 0.92));

        for (int band = 0;; ++band, oddBand = !oddBand) {
            const float s = firstBound + static_cast<float>(band) * samplesPerSec;
            if (s >= static_cast<float>(lastSample))
                break;
            if (!oddBand)
                continue; // even seconds use the regular bgColor already filled
            float xStart = (s - scrollSample) / spp;
            float xEnd = xStart + samplesPerSec / spp;
            xStart = qBound(0.f, xStart, static_cast<float>(pw));
            xEnd = qBound(0.f, xEnd, static_cast<float>(pw));
            if (xEnd > xStart)
                p.fillRect(QRectF(xStart, 0, xEnd - xStart, ph), altColor);
        }
    }

    // ── Grid pass ──────────────────────────────────────────────────────
    if (gridVisible) {
        for (int i = 0; i < laneCount; ++i) {
            float yMid = (i + 0.5f) * laneH;
            float yTop = i * laneH;

            if (i > 0) {
                p.setPen(QPen(QColor(205, 205, 215), 1));
                p.drawLine(QPointF(0, yTop), QPointF(pw, yTop));
            }

            QPen guidePen(QColor(228, 228, 235), 1, Qt::DotLine);
            guidePen.setDashPattern({3, 4});
            p.setPen(guidePen);
            p.drawLine(QPointF(0, yMid - laneH * 0.44f), QPointF(pw, yMid - laneH * 0.44f));
            p.drawLine(QPointF(0, yMid + laneH * 0.44f), QPointF(pw, yMid + laneH * 0.44f));

            p.setPen(QPen(QColor(210, 210, 218), 1));
            p.drawLine(QPointF(0, yMid), QPointF(pw, yMid));
        }

        if (sfreq > 0.f) {
            static const float kNiceIntervals[] = {
                0.05f, 0.1f, 0.2f, 0.5f, 1.f, 2.f, 5.f, 10.f, 30.f, 60.f};
            float pxPerSecond = sfreq / spp;
            float tickIntervalS = kNiceIntervals[0];
            for (float iv : kNiceIntervals) {
                tickIntervalS = iv;
                if (iv * pxPerSecond >= 80.f)
                    break;
            }
            float tickSamples = tickIntervalS * sfreq;
            float origin = static_cast<float>(firstFileSample);
            float firstTick = std::ceil((scrollSample - origin) / tickSamples) * tickSamples + origin;

            p.setPen(QPen(QColor(205, 205, 210), 1));
            for (int tick = 0;; ++tick) {
                const float s = firstTick + static_cast<float>(tick) * tickSamples;
                if (s >= static_cast<float>(lastSample))
                    break;
                float xPx = (s - scrollSample) / spp;
                p.drawLine(QPointF(xPx, 0), QPointF(xPx, ph));
            }
        }
    }

    // ── Annotation span pass ────────────────────────────────────────
    if (!annotations.empty()) {
        QFont font = p.font();
        font.setPointSizeF(8.0);
        font.setBold(true);
        p.setFont(font);

        for (const AnnotationSpan& annotation : annotations) {
            float xStart = (static_cast<float>(annotation.startSample) - scrollSample) / spp;
            float xEnd = (static_cast<float>(annotation.endSample + 1) - scrollSample) / spp;

            if (xEnd < -2.f || xStart > pw + 2.f) {
                continue;
            }

            xStart = qBound(0.f, xStart, static_cast<float>(pw));
            xEnd = qBound(0.f, xEnd, static_cast<float>(pw));
            if (xEnd <= xStart) {
                continue;
            }

            QColor fillColor = annotation.color;
            fillColor.setAlpha(48);
            p.fillRect(QRectF(xStart, 0.f, xEnd - xStart, static_cast<float>(ph)), fillColor);

            QColor borderColor = annotation.color;
            borderColor.setAlpha(165);
            p.setPen(QPen(borderColor, 1));
            p.drawLine(QPointF(xStart, 0.f), QPointF(xStart, static_cast<float>(ph)));
            p.drawLine(QPointF(xEnd, 0.f), QPointF(xEnd, static_cast<float>(ph)));

            if (!mne::trimmed(annotation.label).empty()) {
                const QString label = QString::fromStdString(std::string(mne::trimmed(annotation.label)));
                QFontMetrics metrics(font);
                QRect labelRect = metrics.boundingRect(label);
                labelRect.adjust(-6, -2, 6, 2);
                const int labelX = qBound(4,
                                          static_cast<int>(xStart) + 4,
                                          qMax(4, pw - labelRect.width() - 4));
                labelRect.moveTopLeft(QPoint(labelX, 4));
                QColor pillColor = annotation.color;
                pillColor.setAlpha(215);
                p.fillRect(labelRect, pillColor);
                p.setPen(Qt::white);
                p.drawText(labelRect, Qt::AlignCenter, label);
            }
        }
    }

    // ── Channel waveform pass (same mapping as channeldata.vert / .frag) ──
    for (const auto& [ch, lane] : std::as_const(traces)) {
        const ChannelDisplayInfo info = model->channelInfo(ch);

        int vboFirst = 0;
        std::vector<float> verts = model->decimatedVertices(ch, firstSample, lastSample, pw, vboFirst);
        const int nVerts = static_cast<int>(verts.size() / 2);
        if (nVerts < 2) {
            continue;
        }

        // Z-score mode: (y - mean) / std, with ±4 std filling the row like the GPU path
        float offset = 0.f;
        float amplitudeMax = info.amplitudeMax;
        if (zScoreMode) {
            double sum = 0.0;
            double sumSq = 0.0;
            for (int v = 0; v < nVerts; ++v) {
                const double amp = static_cast<double>(verts[v * 2 + 1]);
                sum += amp;
                sumSq += amp * amp;
            }
            const double mean = sum / nVerts;
            const double var = sumSq / nVerts - mean * mean;
            offset = static_cast<float>(mean);
            amplitudeMax = 4.f * (var > 0.0 ? static_cast<float>(std::sqrt(var)) : 1.f);
        }

        const QPen normalPen(info.bad ? QColor(200, 60, 60, 180) : info.color, 1.2);
        const QPen clipPen(QColor(255, 0, 0), 1.6);
        const bool doClip = showClipping && !info.bad && !zScoreMode;
        const float yMid = (static_cast<float>(lane) + 0.5f) * laneH;

        QPolygonF seg;
        seg.reserve(nVerts);
        bool prevClipped = false;
        for (int v = 0; v < nVerts; ++v) {
            const float norm = amplitudeMax > 0.f
                ? qBound(-2.f, (verts[v * 2 + 1] - offset) / amplitudeMax, 2.f)
                : 0.f;
            const QPointF pt((static_cast<float>(vboFirst) + verts[v * 2] - scrollSample) / spp,
                             yMid - norm * 0.45f * laneH);
            const bool clipped = doClip && qAbs(norm) >= 0.95f;
            if (v > 0 && clipped != prevClipped) {
                seg.append(pt);
                p.setPen(prevClipped ? clipPen : normalPen);
                p.drawPolyline(seg);
                seg.clear();
            }
            seg.append(pt);
            prevClipped = clipped;
        }
        if (seg.size() > 1) {
            p.setPen(prevClipped ? clipPen : normalPen);
            p.drawPolyline(seg);
        }
    }

    // ── Event / stimulus marker pass ─────────────────────────────────
    // Draw coloured vertical lines spanning the full channel area.
    // Label chips are shown in the TimeRulerWidget stim lane.
    if (!events.empty() && spp > 0.f) {
        for (const EventMarker& ev : events) {
            float xF = (static_cast<float>(ev.sample) - scrollSample) / spp;
            if (xF < -2.f || xF > pw + 2.f)
                continue;
            int ix = static_cast<int>(xF);

            QColor lineColor = ev.color;
            lineColor.setAlpha(180);
            p.setPen(QPen(lineColor, 1));
            p.drawLine(ix, 0, ix, ph);
        }
    }

    // ── Epoch trigger marker pass ────────────────────────────────────
    // Draw dashed grey vertical lines at epoch trigger positions.
    if (!epochMarkers.empty() && spp > 0.f) {
        QPen epochPen(QColor(100, 100, 100, 140), 1, Qt::DashLine);
        p.setPen(epochPen);
        for (int trigSample : epochMarkers) {
            float xF = (static_cast<float>(trigSample) - scrollSample) / spp;
            if (xF < -2.f || xF > pw + 2.f)
                continue;
            int ix = static_cast<int>(xF);
            p.drawLine(ix, 0, ix, ph);
        }
    }

    return img;
}


//=============================================================================================================
// Shared event handlers
//=============================================================================================================

void ChannelRhiRenderer::handleResize()
{
    m_vboDirty = true;
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
