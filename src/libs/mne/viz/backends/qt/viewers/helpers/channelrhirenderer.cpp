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
#include "../../plots/helpers/sceneinput.h"
#include "../../plots/helpers/scenepainter.h"
#include <mne/viz/views/trace_geometry.h>

#include <rhi/qrhi.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

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
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;
namespace scene = mne::viz;

//=============================================================================================================
// TRACE SOURCE
//=============================================================================================================

/**
 * The channels of the renderer's model as the TraceView sees them: the model's viz::TraceBuffer under the
 * logical (filtered) channel indices.
 */
class ChannelRhiRenderer::TraceSourceAdapter : public viz::TraceSource
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
        const viz::TraceSource* source = buffer();
        if (!source || ch < 0)
            return std::nullopt;
        return source->channel(ch);
    }

    float value_at(int index, int sample) const override
    {
        const int ch = m_renderer.actualChannelAt(index);
        const viz::TraceSource* source = buffer();
        return (source && ch >= 0) ? source->value_at(ch, sample) : 0.f;
    }

    int first_sample() const override
    {
        const viz::TraceSource* source = buffer();
        return (source && source->sample_count() > 0) ? source->first_sample() : 0;
    }

    int sample_count() const override
    {
        const viz::TraceSource* source = buffer();
        return source ? source->sample_count() : 0;
    }

    viz::TraceVertices vertices(int index, int first, int last, int pixelWidth) const override
    {
        const int ch = m_renderer.actualChannelAt(index);
        const viz::TraceSource* source = buffer();
        if (!source || ch < 0)
            return {first, {}};
        return source->vertices(ch, first, last, pixelWidth);
    }

private:
    const viz::TraceSource* buffer() const
    {
        return m_renderer.m_model ? &m_renderer.m_model->buffer() : nullptr;
    }

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
    m_frameRenderer.addShaderPrefix(":/disp/shaders/viewers/helpers/shaders/");

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
    viz::TraceView::Effects effects;
    effects.scroll_changed = [this](double) {
        emit scrollSampleChanged(scrollSample());
        requestUpdate();
    };
    effects.zoom_changed = [this](double) {
        m_frameBuilder.geometry().invalidate(); // zoom change → decimation changes
        m_frameBuilder.invalidate_overlay();
        emit samplesPerPixelChanged(samplesPerPixel());
        requestUpdate();
    };
    effects.channel_offset_changed = [this](int firstChannel) {
        m_frameBuilder.geometry().invalidate();
        emit channelOffsetChanged(firstChannel);
        requestUpdate();
    };
    effects.animate_scroll = [this](const viz::TraceView::ScrollAnimation& animation) {
        m_animator.start_scroll(animation, sceneTimestampMs());
        startAnimationTimer();
    };
    effects.animate_zoom = [this](const viz::TraceView::ZoomAnimation& animation) {
        m_animator.start_zoom(animation, sceneTimestampMs());
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
        m_frameBuilder.invalidate_overlay();
        requestUpdate();
    };
    effects.channel_double_clicked = [this](int logicalChannel) {
        const int ch = actualChannelAt(logicalChannel);
        if (m_model && ch >= 0) {
            m_model->setChannelBad(ch, !m_model->channelInfo(ch).bad);
        }
    };
    effects.cursor_changed = [this](viz::CursorShape shape) {
        if (!m_pHost)
            return;
        if (shape == viz::CursorShape::SizeHorizontal)
            m_pHost->setCursorShape(Qt::SizeHorCursor);
        else
            m_pHost->unsetCursorShape();
    };
    effects.cursor_data_changed = [this](const viz::TraceView::CursorData& data) {
        emit cursorDataChanged(data.time_sec, data.amplitude, QString::fromStdString(data.channel_name),
                               QString::fromStdString(data.unit_label));
    };
    effects.redraw = [this] {
        requestUpdate();
    };
    m_view.set_effects(std::move(effects));
}

//=============================================================================================================

mne::viz::TraceView& ChannelRhiRenderer::view() const
{
    m_view.set_view_size(viewWidth(), viewHeight());
    return m_view;
}

//=============================================================================================================

void ChannelRhiRenderer::tickAnimations()
{
    if ((!m_animator.running() || !m_animator.tick(sceneTimestampMs(), view())) && m_animationTimerId != 0) {
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

mne::viz::TraceStyle ChannelRhiRenderer::traceStyle() const
{
    viz::TraceStyle style;
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
    m_frameBuilder.geometry().invalidate();
    m_frameBuilder.invalidate_overlay();
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
            m_frameBuilder.geometry().invalidate();
            requestUpdate();
        });
        connect(m_model, &ChannelDataModel::metaChanged, this, [this] {
            m_frameBuilder.geometry().invalidate();
            requestUpdate();
        });
    }
    m_frameBuilder.geometry().invalidate();
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
    m_frameBuilder.invalidate_overlay();
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setPrefetchFactor(float factor)
{
    m_frameBuilder.geometry().set_prefetch_factor(factor);
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
    m_frameBuilder.geometry().invalidate();
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
    m_frameBuilder.invalidate_overlay();
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setSfreq(float sfreq)
{
    m_view.set_sfreq(sfreq);
    m_frameBuilder.invalidate_overlay();
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setFirstFileSample(int first)
{
    if (!m_view.set_first_file_sample(first))
        return;
    m_frameBuilder.invalidate_overlay();
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
    m_frameBuilder.geometry().invalidate();
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
    m_frameBuilder.geometry().invalidate();
    m_frameBuilder.invalidate_overlay();
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setChannelIndices(const std::vector<int>& indices)
{
    m_filteredChannels = indices;
    // Clamp the channel window to the new range
    m_view.set_first_visible_channel(m_view.first_visible_channel());
    m_frameBuilder.geometry().invalidate();
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
    const viz::TraceBuffer& buffer = m_model->buffer();
    return viz::visible_channels(buffer.channel_count(), m_filteredChannels, m_hideBadChannels, [&buffer](int channel) {
        const std::optional<viz::TraceSource::Channel> info = buffer.channel(channel);
        return info && info->bad;
    });
}

//=============================================================================================================

void ChannelRhiRenderer::setEvents(const std::vector<EventMarker>& events)
{
    std::vector<viz::TraceMarkers::Event> sceneEvents;
    sceneEvents.reserve(events.size());
    for (const EventMarker& event : events)
        sceneEvents.push_back({event.sample, event.type, toSceneColor(event.color), event.label});
    m_markers.set_events(std::move(sceneEvents));
    m_frameBuilder.invalidate_overlay();
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setEpochMarkers(const std::vector<int>& triggerSamples)
{
    m_markers.set_epoch_markers(triggerSamples);
    m_frameBuilder.invalidate_overlay();
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setEpochMarkersVisible(bool visible)
{
    if (!m_markers.set_epoch_markers_visible(visible))
        return;
    m_frameBuilder.invalidate_overlay();
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
    m_frameBuilder.geometry().invalidate(); // VBO data changes (z-score normalization)
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setAnnotations(const std::vector<AnnotationSpan>& annotations)
{
    std::vector<viz::TraceView::Annotation> spans;
    spans.reserve(annotations.size());
    for (const AnnotationSpan& annotation : annotations) {
        spans.push_back({annotation.startSample, annotation.endSample, toSceneColor(annotation.color), annotation.label});
    }
    m_view.set_annotations(std::move(spans));
    m_frameBuilder.invalidate_overlay();
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
    m_frameBuilder.invalidate_overlay();
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::setAnnotationsVisible(bool visible)
{
    if (!m_markers.set_annotations_visible(visible))
        return;
    m_frameBuilder.invalidate_overlay();
    requestUpdate();
}

//=============================================================================================================

void ChannelRhiRenderer::initializeResources()
{
    m_frameBuilder.geometry().invalidate();
    m_frameBuilder.invalidate_overlay();
}

//=============================================================================================================

void ChannelRhiRenderer::releaseResources()
{
    m_frameRenderer.releaseResources();
}

//=============================================================================================================
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
//=============================================================================================================

void ChannelRhiRenderer::renderFrame(QRhi* rhi, QRhiRenderTarget* target, QRhiCommandBuffer* cb)
{
    tickAnimations();
    const int logicalW = viewWidth();
    const qreal dpr = (logicalW > 0) ? (static_cast<qreal>(target->pixelSize().width()) / static_cast<qreal>(logicalW))
                                     : (m_pHost ? m_pHost->surfaceDevicePixelRatio() : 1.0);
    const QFont font = hostFont();
    const QtTextMetrics metrics(font);
    m_frameRenderer.setFont(font);
    const viz::GpuFrame frame = m_frameBuilder.build(view(), *m_traceSource, traceStyle(), m_markers, metrics,
                                                     RhiFrameRenderer::frameTarget(rhi, target, dpr));
    m_frameRenderer.render(rhi, target, cb, frame, m_frameBuilder.resources());
}

//=============================================================================================================

QImage ChannelRhiRenderer::hudImage() const
{
    return m_frameRenderer.textureImage(m_frameBuilder.hud_texture());
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
    viz::Canvas canvas(pw, ph);
    viz::draw_trace_figure(canvas, metrics, viz::Font{}, view(), *m_traceSource, m_markers, traceStyle());

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
    m_frameBuilder.geometry().invalidate();
    m_frameBuilder.invalidate_overlay();
    emit viewResized(viewWidth(), viewHeight());
    requestUpdate();
}

//=============================================================================================================

bool ChannelRhiRenderer::handleWheel(QWheelEvent* event)
{
    view().handle_wheel(toSceneWheelEvent(*event));
    event->accept();
    return true;
}

//=============================================================================================================

bool ChannelRhiRenderer::handleMousePress(QMouseEvent* event)
{
    return forwardPointer(event, viz::PointerEvent::Type::Press);
}

//=============================================================================================================

bool ChannelRhiRenderer::handleMouseMove(QMouseEvent* event)
{
    return forwardPointer(event, viz::PointerEvent::Type::Move);
}

//=============================================================================================================

bool ChannelRhiRenderer::handleMouseRelease(QMouseEvent* event)
{
    return forwardPointer(event, viz::PointerEvent::Type::Release);
}

//=============================================================================================================

bool ChannelRhiRenderer::handleMouseDoubleClick(QMouseEvent* event)
{
    // Double clicks toggle the bad state of a model channel
    if (!m_model)
        return false;
    return forwardPointer(event, viz::PointerEvent::Type::DoubleClick);
}

//=============================================================================================================

bool ChannelRhiRenderer::forwardPointer(QMouseEvent* event, viz::PointerEvent::Type type)
{
    const bool used = view().handle_pointer(toScenePointerEvent(*event, type));
    if (used)
        event->accept();
    return used;
}
