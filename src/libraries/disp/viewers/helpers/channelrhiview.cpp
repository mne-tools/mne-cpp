//=============================================================================================================
/**
 * @file     channelrhiview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the ChannelRhiView class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channelrhiview.h"

#include <rhi/qrhi.h>
#include <rhi/qshader.h>
#include <QFile>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QPropertyAnimation>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QPolygonF>
#include <QtMath>

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// Uniform block layout — must match channeldata.vert / channeldata.frag
//=============================================================================================================

namespace {
// Byte offsets inside one aligned UBO slot
constexpr int kUboOffsetColor          =  0; // vec4  (16 bytes)
constexpr int kUboOffsetFirstSample    = 16; // float
constexpr int kUboOffsetScrollSample   = 20; // float
constexpr int kUboOffsetSampPerPixel   = 24; // float
constexpr int kUboOffsetViewWidth      = 28; // float
constexpr int kUboOffsetViewHeight     = 32; // float
constexpr int kUboOffsetChannelYCenter = 36; // float
constexpr int kUboOffsetChannelYRange  = 40; // float
constexpr int kUboOffsetAmplitudeMax   = 44; // float
// Total used: 48 bytes  — padded to m_uboStride (≥ 256) per dynamic offset rules.

constexpr int kMaxChannels = 1024;  // Upper hard limit for UBO pre-allocation

// Prefetch: VBO covers (1 + 2*prefetch) × visible window.
// A scroll of up to prefetch×visible in either direction needs no VBO rebuild.
constexpr float kDefaultPrefetch = 1.0f;
} // namespace

//=============================================================================================================
// HELPERS
//=============================================================================================================

static QShader loadShader(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "ChannelRhiView: cannot open shader" << filename;
        return {};
    }
    return QShader::fromSerialized(f.readAll());
}

static void writeFloat(quint8 *base, int byteOffset, float v)
{
    memcpy(base + byteOffset, &v, sizeof(float));
}

static void writeFloats(quint8 *base, int byteOffset, const float *data, int count)
{
    memcpy(base + byteOffset, data, count * sizeof(float));
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

//=============================================================================================================
// CrosshairOverlay — lightweight transparent child widget for crosshair/scalebar
// painting.  Sits on top of the QRhiWidget and repaints independently so that
// mouse-tracking updates do NOT trigger the expensive GPU render pipeline.
//=============================================================================================================

class CrosshairOverlay : public QWidget
{
public:
    explicit CrosshairOverlay(ChannelRhiView *parent)
        : QWidget(parent), m_view(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
        setMouseTracking(false);
    }

    void syncSize() { setGeometry(0, 0, parentWidget()->width(), parentWidget()->height()); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (!m_view) return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);

        if (m_view->crosshairEnabled())
            m_view->drawCrosshair(p);
        if (m_view->scalebarsVisible())
            m_view->drawScalebars(p);
        if (m_view->rulerActive())
            m_view->drawRulerOverlay(p);
    }

private:
    ChannelRhiView *m_view;
};

ChannelRhiView::ChannelRhiView(QWidget *parent)
    : QRhiWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    m_overlay = new CrosshairOverlay(this);
    m_overlay->raise();
    m_overlay->show();

    // ── Async tile rebuild: swap in finished tile without blocking paintEvent ──
    connect(&m_tileWatcher, &QFutureWatcher<TileResult>::finished, this, [this]() {
        m_tileRebuildPending = false;
        // Check BEFORE we clear the flag: was data dirtied while we were building?
        bool dirtiedDuringBuild = m_tileDirty;
        bool tileAccepted = false;

        if (!m_tileWatcher.isCanceled()) {
            TileResult r = m_tileWatcher.result();
            if (!r.image.isNull()) {
                m_tileImage           = std::move(r.image);
                m_tileSampleFirst     = r.sampleFirst;
                m_tileSamplesPerPixel = r.samplesPerPixel;
                m_tileFirstChannel    = r.firstChannel;
                m_tileVisibleCount    = r.visibleCount;
                m_tileDirty           = false;
                tileAccepted = true;
            }
        }

        // Only repaint when we have something new to show: a freshly accepted tile,
        // or new data that arrived while the build was in-flight (needs a fresh build).
        // When the build returned null (no channels/data yet) and nothing changed,
        // skipping update() avoids a CPU-burning infinite repaint loop.
        if (tileAccepted || dirtiedDuringBuild) {
            if (dirtiedDuringBuild)
                m_tileDirty = true;
            update();
        }
    });

    // Platform-specific backend selection
#  if defined(WASMBUILD) || defined(__EMSCRIPTEN__)
    setApi(QRhiWidget::Api::OpenGL);  // WebGL 2
#  elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    setApi(QRhiWidget::Api::Metal);
#  elif defined(Q_OS_WIN)
    setApi(QRhiWidget::Api::Direct3D11);
#  else
    setApi(QRhiWidget::Api::OpenGL);
#  endif
    setSampleCount(1);
    // Force a native window so Metal/OpenGL can create their backing surface.
    // Without this, QRhiWidget may fail to obtain an NSView handle on macOS.
    setAttribute(Qt::WA_NativeWindow);

    // Repaint overlays (bands + event lines) when the app regains focus.
    // The ruler overlay still uses QPainter and needs an explicit refresh.
    connect(qApp, &QApplication::applicationStateChanged,
            this, [this](Qt::ApplicationState s) {
                if (s == Qt::ApplicationActive)
                    update();
            });
}

//=============================================================================================================

ChannelRhiView::~ChannelRhiView() = default;

//=============================================================================================================

void ChannelRhiView::setModel(ChannelDataModel *model)
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
            m_tileDirty = true;
            update();
        });
        connect(m_model, &ChannelDataModel::metaChanged, this, [this] {
            m_vboDirty      = true;
            m_pipelineDirty = true;
            m_tileDirty = true;
            update();
        });
    }
    m_vboDirty    = true;
    m_pipelineDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setScrollSample(float sample)
{
    // Never scroll before the first available sample
    if (m_model && m_model->totalSamples() > 0)
        sample = qMax(sample, static_cast<float>(m_model->firstSample()));
    else
        sample = qMax(sample, 0.f);

    // Never scroll past the file end (clamp upper bound when file bounds are known)
    if (m_lastFileSample >= 0) {
        float maxScroll = static_cast<float>(m_lastFileSample - visibleSampleCount() + 1);
        maxScroll = qMax(maxScroll, static_cast<float>(m_firstFileSample));
        sample = qMin(sample, maxScroll);
    }

    if (qFuzzyCompare(m_scrollSample, sample))
        return;

    m_scrollSample = sample;

    // Mark tile dirty when the new scroll position falls outside the tile's
    // comfortable range.  This ensures a rebuild is queued even if another
    // build is currently in-flight (the finished handler will see dirtiedDuringBuild
    // and let the next paintEvent restart for the new position).
    if (!m_tileImage.isNull() && m_tileSamplesPerPixel > 0.f) {
        float vis     = width() * m_samplesPerPixel;
        float tileEnd = m_tileSampleFirst + m_tileImage.width() * m_tileSamplesPerPixel;
        if (m_scrollSample < m_tileSampleFirst + vis ||
            m_scrollSample + vis > tileEnd - vis)
            m_tileDirty = true;
    }

    // Check whether the prefetch window is still valid
    float visible = width() * m_samplesPerPixel;
    float margin  = m_prefetchFactor * visible;
    if (sample < m_vboWindowFirst + margin ||
        sample + visible > m_vboWindowLast - margin) {
        m_vboDirty = true;
    }
    m_overlayDirty = true;

    emit scrollSampleChanged(m_scrollSample);
    update();
}

//=============================================================================================================

void ChannelRhiView::setSamplesPerPixel(float spp)
{
    spp = qMax(spp, 1e-4f);
    if (qFuzzyCompare(m_samplesPerPixel, spp))
        return;
    m_samplesPerPixel = spp;
    m_vboDirty = true; // zoom change → decimation changes
    m_overlayDirty = true;
    m_tileDirty = true;
    emit samplesPerPixelChanged(m_samplesPerPixel);
    update();
}

//=============================================================================================================

void ChannelRhiView::scrollTo(float targetSample, int durationMs)
{
    if (durationMs <= 0) {
        setScrollSample(targetSample);
        return;
    }
    auto *anim = new QPropertyAnimation(this, "scrollSample", this);
    anim->setDuration(durationMs);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setStartValue(m_scrollSample);
    anim->setEndValue(targetSample);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

//=============================================================================================================

void ChannelRhiView::zoomTo(float targetSpp, int durationMs)
{
    targetSpp = qMax(targetSpp, 1e-4f);
    if (durationMs <= 0) {
        setSamplesPerPixel(targetSpp);
        return;
    }
    auto *anim = new QPropertyAnimation(this, "samplesPerPixel", this);
    anim->setDuration(durationMs);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setStartValue(m_samplesPerPixel);
    anim->setEndValue(targetSpp);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

//=============================================================================================================

void ChannelRhiView::setBackgroundColor(const QColor &color)
{
    m_bgColor = color;
    m_tileDirty = true;
    m_overlayDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setPrefetchFactor(float factor)
{
    m_prefetchFactor = qMax(factor, 0.1f);
}

//=============================================================================================================

int ChannelRhiView::visibleFirstSample() const
{
    return static_cast<int>(m_scrollSample);
}

//=============================================================================================================

int ChannelRhiView::visibleSampleCount() const
{
    return static_cast<int>(width() * m_samplesPerPixel);
}

//=============================================================================================================

void ChannelRhiView::setFirstVisibleChannel(int ch)
{
    int maxFirst = qMax(0, totalLogicalChannels() - m_visibleChannelCount);
    ch = qBound(0, ch, maxFirst);
    if (ch == m_firstVisibleChannel)
        return;
    m_firstVisibleChannel = ch;
    m_tileDirty = true;
    m_vboDirty      = true;
    m_pipelineDirty = true;
    emit channelOffsetChanged(m_firstVisibleChannel);
    update();
}

//=============================================================================================================

void ChannelRhiView::setVisibleChannelCount(int count)
{
    count = qMax(1, count);
    if (count == m_visibleChannelCount)
        return;
    m_visibleChannelCount = count;
    m_tileDirty = true;
    m_vboDirty      = true;
    m_pipelineDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setFrozen(bool frozen)
{
    m_frozen = frozen;
    if (m_frozen && m_pInertialAnim) {
        m_pInertialAnim->stop();
        m_pInertialAnim = nullptr;
    }
}

//=============================================================================================================

void ChannelRhiView::setGridVisible(bool visible)
{
    if (visible == m_gridVisible)
        return;
    m_gridVisible = visible;
    m_tileDirty   = true;
    m_overlayDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setSfreq(float sfreq)
{
    m_sfreq = qMax(sfreq, 0.f);
    m_tileDirty = true;
    m_overlayDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setFirstFileSample(int first)
{
    if (first == m_firstFileSample)
        return;
    m_firstFileSample = first;
    m_tileDirty = true;
    m_overlayDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setLastFileSample(int last)
{
    m_lastFileSample = last;
}

//=============================================================================================================

void ChannelRhiView::setHideBadChannels(bool hide)
{
    if (m_hideBadChannels == hide)
        return;

    const int previousFirstVisibleChannel = m_firstVisibleChannel;
    m_hideBadChannels = hide;
    const int maxFirst = qMax(0, totalLogicalChannels() - m_visibleChannelCount);
    m_firstVisibleChannel = qBound(0, m_firstVisibleChannel, maxFirst);
    if (m_firstVisibleChannel != previousFirstVisibleChannel) {
        emit channelOffsetChanged(m_firstVisibleChannel);
    }
    m_vboDirty = true;
    m_pipelineDirty = true;
    m_tileDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setWheelScrollsChannels(bool channelsMode)
{
    m_wheelScrollsChannels = channelsMode;
}

//=============================================================================================================

void ChannelRhiView::setScrollSpeedFactor(float factor)
{
    m_scrollSpeedFactor = qBound(0.25f, factor, 4.0f);
}

//=============================================================================================================

void ChannelRhiView::setCrosshairEnabled(bool enabled)
{
    if (m_crosshairEnabled == enabled)
        return;
    m_crosshairEnabled = enabled;
    if (enabled) {
        setMouseTracking(true);
    } else {
        setMouseTracking(false);
        m_crosshairX = m_crosshairY = -1;
    }
    update();
}

//=============================================================================================================

void ChannelRhiView::setScalebarsVisible(bool visible)
{
    if (m_scalebarsVisible == visible)
        return;
    m_scalebarsVisible = visible;
    update();
}

//=============================================================================================================

void ChannelRhiView::setButterflyMode(bool enabled)
{
    if (m_butterflyMode == enabled)
        return;
    m_butterflyMode = enabled;
    m_vboDirty = true;
    m_pipelineDirty = true;
    m_tileDirty = true;
    m_overlayDirty = true;
    update();
}

//=============================================================================================================

QVector<ChannelRhiView::ButterflyTypeGroup> ChannelRhiView::butterflyTypeGroups() const
{
    QVector<ButterflyTypeGroup> groups;
    if (!m_model)
        return groups;

    const QVector<int> allCh = effectiveChannelIndices();
    QMap<QString, int> typeToGroup; // typeLabel → index in groups

    for (int ch : allCh) {
        auto info = m_model->channelInfo(ch);
        if (m_hideBadChannels && info.bad)
            continue;
        int gIdx;
        if (typeToGroup.contains(info.typeLabel)) {
            gIdx = typeToGroup[info.typeLabel];
        } else {
            gIdx = groups.size();
            typeToGroup[info.typeLabel] = gIdx;
            ButterflyTypeGroup g;
            g.typeLabel = info.typeLabel;
            g.color = info.color;
            g.amplitudeMax = info.amplitudeMax;
            groups.append(g);
        }
        groups[gIdx].channelIndices.append(ch);
    }
    return groups;
}

//=============================================================================================================

int ChannelRhiView::butterflyLaneCount() const
{
    if (!m_model)
        return 0;
    const QVector<int> allCh = effectiveChannelIndices();
    QSet<QString> types;
    for (int ch : allCh) {
        auto info = m_model->channelInfo(ch);
        if (m_hideBadChannels && info.bad)
            continue;
        types.insert(info.typeLabel);
    }
    return types.size();
}

//=============================================================================================================

void ChannelRhiView::setChannelIndices(const QVector<int> &indices)
{
    const int previousFirstVisibleChannel = m_firstVisibleChannel;
    m_filteredChannels = indices;
    // Clamp scroll to new range
    int maxFirst = qMax(0, totalLogicalChannels() - m_visibleChannelCount);
    m_firstVisibleChannel = qBound(0, m_firstVisibleChannel, maxFirst);
    if (m_firstVisibleChannel != previousFirstVisibleChannel) {
        emit channelOffsetChanged(m_firstVisibleChannel);
    }
    m_vboDirty      = true;
    m_pipelineDirty = true;
    m_tileDirty = true;
    update();
}

//=============================================================================================================

int ChannelRhiView::totalLogicalChannels() const
{
    return effectiveChannelIndices().size();
}

//=============================================================================================================

int ChannelRhiView::actualChannelAt(int logicalIdx) const
{
    const QVector<int> indices = effectiveChannelIndices();
    if (logicalIdx < 0 || logicalIdx >= indices.size())
        return -1;
    return indices.at(logicalIdx);
}

//=============================================================================================================

QVector<int> ChannelRhiView::effectiveChannelIndices() const
{
    QVector<int> indices;

    if (!m_model) {
        return indices;
    }

    if (m_filteredChannels.isEmpty()) {
        indices.reserve(m_model->channelCount());
        for (int channelIndex = 0; channelIndex < m_model->channelCount(); ++channelIndex) {
            indices.append(channelIndex);
        }
    } else {
        indices = m_filteredChannels;
    }

    if (!m_hideBadChannels) {
        return indices;
    }

    QVector<int> visibleIndices;
    visibleIndices.reserve(indices.size());
    for (int channelIndex : std::as_const(indices)) {
        if (channelIndex < 0) {
            continue;
        }

        const ChannelDisplayInfo info = m_model->channelInfo(channelIndex);
        if (!info.bad) {
            visibleIndices.append(channelIndex);
        }
    }

    return visibleIndices;
}

//=============================================================================================================

void ChannelRhiView::setEvents(const QVector<EventMarker> &events)
{
    m_events = events;
    m_tileDirty = true;
    m_overlayDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setEpochMarkers(const QVector<int> &triggerSamples)
{
    m_epochTriggerSamples = triggerSamples;
    m_tileDirty = true;
    m_overlayDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setEpochMarkersVisible(bool visible)
{
    if (m_bShowEpochMarkers == visible)
        return;
    m_bShowEpochMarkers = visible;
    m_tileDirty = true;
    m_overlayDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setAnnotations(const QVector<AnnotationSpan> &annotations)
{
    m_annotations = annotations;
    m_tileDirty = true;
    m_overlayDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setAnnotationSelectionEnabled(bool enabled)
{
    m_annotationSelectionEnabled = enabled;
}

//=============================================================================================================

void ChannelRhiView::setEventsVisible(bool visible)
{
    if (m_bShowEvents == visible) return;
    m_bShowEvents = visible;
    m_tileDirty = true;
    m_overlayDirty = true;
    update();
}

bool ChannelRhiView::eventsVisible() const { return m_bShowEvents; }

//=============================================================================================================

void ChannelRhiView::setAnnotationsVisible(bool visible)
{
    if (m_bShowAnnotations == visible) return;
    m_bShowAnnotations = visible;
    m_tileDirty = true;
    m_overlayDirty = true;
    update();
}

bool ChannelRhiView::annotationsVisible() const { return m_bShowAnnotations; }

//=============================================================================================================

int ChannelRhiView::hitTestAnnotationBoundary(int px, bool &isStart) const
{
    for (int i = 0; i < m_annotations.size(); ++i) {
        const float xStart = (static_cast<float>(m_annotations[i].startSample) - m_scrollSample) / m_samplesPerPixel;
        const float xEnd   = (static_cast<float>(m_annotations[i].endSample + 1) - m_scrollSample) / m_samplesPerPixel;

        if (qAbs(px - static_cast<int>(xStart)) <= kAnnBoundaryHitPx) {
            isStart = true;
            return i;
        }
        if (qAbs(px - static_cast<int>(xEnd)) <= kAnnBoundaryHitPx) {
            isStart = false;
            return i;
        }
    }
    return -1;
}

//=============================================================================================================
void ChannelRhiView::initialize(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
    m_pipelineDirty = true;
    m_vboDirty      = true;
}

//=============================================================================================================

void ChannelRhiView::releaseResources()
{
    m_pipeline.reset();
    m_srb.reset();
    m_ubo.reset();
    m_gpuChannels.clear();
    m_pipelineDirty = true;
    m_vboDirty      = true;

    m_overlayPipeline.reset();
    m_overlaySrb.reset();
    m_overlaySampler.reset();
    m_overlayTex.reset();
    m_overlayVbo.reset();
    m_overlayDirty = true;
}

//=============================================================================================================

void ChannelRhiView::ensurePipeline()
{
    if (!m_pipelineDirty)
        return;

    QRhi *rhi = this->rhi();
    if (!rhi)
        return;

    m_uboStride = static_cast<int>(
        (48 + rhi->ubufAlignment() - 1) & ~(rhi->ubufAlignment() - 1));

    // UBO has one slot per *visible* channel row, not all channels
    // In butterfly mode, we need a slot for EVERY channel (all overlaid)
    int totalCh = totalLogicalChannels();
    int nCh;
    if (m_butterflyMode) {
        nCh = qMin(totalCh, kMaxChannels);
    } else {
        nCh = qMin(m_visibleChannelCount, totalCh - m_firstVisibleChannel);
    }
    nCh = qMax(nCh, 1);
    nCh = qMin(nCh, kMaxChannels);

    // ── Uniform buffer ──────────────────────────────────────────────────
    bool uboRecreated = false;
    if (!m_ubo || m_ubo->size() < nCh * m_uboStride) {
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
        m_srb->setBindings({
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
                0,
                QRhiShaderResourceBinding::VertexStage |
                QRhiShaderResourceBinding::FragmentStage,
                m_ubo.get(),
                48 // visible block size for the shader
            )
        });
        m_srb->create();
    }

    // ── Shaders ─────────────────────────────────────────────────────────
    // Resource path matches qt_add_shaders PREFIX + file path (including subdirectory).
    QShader vs = loadShader(QStringLiteral(":/disp/shaders/viewers/helpers/shaders/channeldata.vert.qsb"));
    QShader fs = loadShader(QStringLiteral(":/disp/shaders/viewers/helpers/shaders/channeldata.frag.qsb"));

    if (!vs.isValid() || !fs.isValid()) {
        qWarning() << "ChannelRhiView: shaders not found. "
                      "Ensure qt_add_shaders is configured in CMakeLists.";
        return;
    }

    // ── Graphics pipeline ───────────────────────────────────────────────
    // Destroy any existing pipeline before creating a new one.
    m_pipeline.reset();
    m_pipeline.reset(rhi->newGraphicsPipeline());
    m_pipeline->setShaderStages({
        { QRhiShaderStage::Vertex,   vs },
        { QRhiShaderStage::Fragment, fs }
    });

    QRhiVertexInputLayout il;
    il.setBindings({{ 2 * sizeof(float) }});                         // stride = vec2
    il.setAttributes({{ 0, 0, QRhiVertexInputAttribute::Float2, 0 }}); // location 0 = vec2

    m_pipeline->setVertexInputLayout(il);
    m_pipeline->setShaderResourceBindings(m_srb.get());
    m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
    m_pipeline->setTopology(QRhiGraphicsPipeline::LineStrip);
    m_pipeline->setDepthTest(false);
    m_pipeline->setDepthWrite(false);

    // Alpha blending for anti-aliased lines (if multisampling is disabled)
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable       = true;
    blend.srcColor     = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor     = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha     = QRhiGraphicsPipeline::One;
    blend.dstAlpha     = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_pipeline->setTargetBlends({ blend });

    if (!m_pipeline->create()) {
        qWarning() << "ChannelRhiView: failed to create graphics pipeline";
        m_pipeline.reset();
        return;
    }

    m_pipelineDirty = false;
}

//=============================================================================================================

bool ChannelRhiView::isVboDirty() const
{
    if (!m_model)
        return false;
    float visible = width() * m_samplesPerPixel;
    float margin  = m_prefetchFactor * visible;
    return m_vboDirty
        || m_scrollSample < m_vboWindowFirst + margin
        || (m_scrollSample + visible) > m_vboWindowLast - margin;
}

//=============================================================================================================

void ChannelRhiView::rebuildVBOs(QRhiResourceUpdateBatch *batch)
{
    if (!m_model)
        return;

    QRhi *rhi = this->rhi();
    if (!rhi)
        return;

    int nCh      = totalLogicalChannels();
    int px       = width();
    float visible = px * m_samplesPerPixel;

    // Prefetch window: [scroll - prefetch*visible, scroll + (1+prefetch)*visible]
    float windowFirst = m_scrollSample - m_prefetchFactor * visible;
    float windowLast  = m_scrollSample + (1.f + m_prefetchFactor) * visible;

    int iFirst = qMax(static_cast<int>(windowFirst), m_model->firstSample());
    int iLast  = qMin(static_cast<int>(windowLast),
                      m_model->firstSample() + m_model->totalSamples());
    if (iFirst >= iLast) {
        m_vboDirty = false;
        return;
    }

    m_vboWindowFirst = iFirst;
    m_vboWindowLast  = iLast;

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
        QVector<float> verts = m_model->decimatedVertices(
            ch, iFirst, iLast, static_cast<int>(prefetchedSamples / m_samplesPerPixel), vboFirst);

        if (verts.isEmpty()) {
            m_gpuChannels[logCh].vertexCount  = 0;
            continue;
        }

        int vertexCount = verts.size() / 2; // each vertex is (x, y) = 2 floats
        quint32 byteSize = static_cast<quint32>(verts.size() * sizeof(float));

        auto &gd = m_gpuChannels[logCh];

        // Re-create buffer if size changed significantly
        if (!gd.vbo || static_cast<quint32>(gd.vbo->size()) < byteSize) {
            gd.vbo.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                        QRhiBuffer::VertexBuffer,
                                        byteSize));
            if (!gd.vbo->create()) {
                qWarning() << "ChannelRhiView: VBO create failed for channel" << logCh;
                gd.vertexCount = 0;
                continue;
            }
        }

        batch->updateDynamicBuffer(gd.vbo.get(), 0, byteSize,
                                   verts.constData());
        gd.vertexCount     = vertexCount;
        gd.vboFirstSample  = vboFirst;
    }

    m_vboDirty = false;
}

//=============================================================================================================

void ChannelRhiView::updateUBO(QRhiResourceUpdateBatch *batch)
{
    if (!m_model || !m_ubo)
        return;

    int totalCh = totalLogicalChannels();

    // ── Butterfly mode: one UBO slot per channel, type-based lane positions ──
    if (m_butterflyMode) {
        const auto groups = butterflyTypeGroups();
        int nLanes = groups.size();
        if (nLanes <= 0)
            return;

        // Build channel→lane map
        QHash<int, int> chToLane;  // model channel idx → lane index
        for (int g = 0; g < groups.size(); ++g)
            for (int ch : groups[g].channelIndices)
                chToLane[ch] = g;

        float vw = static_cast<float>(width());
        float vh = static_cast<float>(height());
        float laneRange = 2.f / nLanes; // NDC height per lane

        QVarLengthArray<quint8> buf(m_uboStride, 0);
        int nToUpload = qMin(totalCh, kMaxChannels);

        for (int logCh = 0; logCh < nToUpload; ++logCh) {
            int ch = actualChannelAt(logCh);
            memset(buf.data(), 0, m_uboStride);

            auto info = (ch >= 0) ? m_model->channelInfo(ch) : ChannelDisplayInfo{};
            bool hideThis = (ch < 0) || (m_hideBadChannels && info.bad);

            int lane = chToLane.value(ch, -1);
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
                static_cast<float>(col.alphaF())
            };

            auto *d = buf.data();
            writeFloats(d, kUboOffsetColor,          rgba, 4);
            writeFloat (d, kUboOffsetFirstSample,    static_cast<float>(logCh < static_cast<int>(m_gpuChannels.size())
                                                                          ? m_gpuChannels[logCh].vboFirstSample : 0));
            writeFloat (d, kUboOffsetScrollSample,   m_scrollSample);
            writeFloat (d, kUboOffsetSampPerPixel,   m_samplesPerPixel);
            writeFloat (d, kUboOffsetViewWidth,      vw);
            writeFloat (d, kUboOffsetViewHeight,     vh);
            writeFloat (d, kUboOffsetChannelYCenter, yCenter);
            writeFloat (d, kUboOffsetChannelYRange,  yRng);
            writeFloat (d, kUboOffsetAmplitudeMax,   info.amplitudeMax);

            batch->updateDynamicBuffer(m_ubo.get(),
                                       logCh * m_uboStride,
                                       m_uboStride,
                                       buf.constData());
        }
        return;
    }

    // ── Normal mode: one UBO slot per visible row ──
    int firstCh = qBound(0, m_firstVisibleChannel, totalCh);
    int visCnt  = qMin(m_visibleChannelCount, totalCh - firstCh);
    int nCh     = qMin(visCnt, kMaxChannels);
    if (nCh <= 0)
        return;

    float vw        = static_cast<float>(width());
    float vh        = static_cast<float>(height());
    float laneRange = 2.f / nCh; // NDC height of one visible channel row

    QVarLengthArray<quint8> buf(m_uboStride, 0);

    for (int i = 0; i < nCh; ++i) {
        int logCh  = firstCh + i;                   // logical (filtered) index
        int ch     = actualChannelAt(logCh);         // actual model channel index
        memset(buf.data(), 0, m_uboStride);

        auto info  = (ch >= 0) ? m_model->channelInfo(ch) : ChannelDisplayInfo{};
        bool hideThis = (ch < 0) || (m_hideBadChannels && info.bad);
        // When hiding: use background colour so no trace is painted
        QColor col = hideThis ? m_bgColor
                              : (info.bad ? QColor(200, 60, 60, 180) : info.color);
        // When hiding bad channel: zero amplitude range → flat invisible line
        float  yRng = hideThis ? 0.f : laneRange;

        float rgba[4] = {
            static_cast<float>(col.redF()),
            static_cast<float>(col.greenF()),
            static_cast<float>(col.blueF()),
            static_cast<float>(col.alphaF())
        };

        // Visible row i: top at NDC +1, bottom at NDC -1
        float yCenter = 1.f - laneRange * (i + 0.5f);

        auto *d = buf.data();
        writeFloats(d, kUboOffsetColor,          rgba, 4);
        // VBO indexed by logical channel (logCh), not model channel
        writeFloat (d, kUboOffsetFirstSample,    static_cast<float>(logCh < static_cast<int>(m_gpuChannels.size())
                                                                      ? m_gpuChannels[logCh].vboFirstSample : 0));
        writeFloat (d, kUboOffsetScrollSample,   m_scrollSample);
        writeFloat (d, kUboOffsetSampPerPixel,   m_samplesPerPixel);
        writeFloat (d, kUboOffsetViewWidth,      vw);
        writeFloat (d, kUboOffsetViewHeight,     vh);
        writeFloat (d, kUboOffsetChannelYCenter, yCenter);
        writeFloat (d, kUboOffsetChannelYRange,  yRng);
        writeFloat (d, kUboOffsetAmplitudeMax,   info.amplitudeMax);

        // UBO slot i corresponds to visible row i
        batch->updateDynamicBuffer(m_ubo.get(),
                                   i * m_uboStride,
                                   m_uboStride,
                                   buf.constData());
    }
}

//=============================================================================================================
// Overlay blit — bands + event lines baked into a QImage and uploaded as a QRhiTexture
// so they are part of the Metal texture and remain visible even when the app loses focus.
//=============================================================================================================

void ChannelRhiView::rebuildOverlayImage(int logicalWidth, int logicalHeight, qreal devicePixelRatio)
{
    const qreal dpr = qMax(devicePixelRatio, 1.0);
    const int pixelWidth  = qMax(1, qRound(logicalWidth * dpr));
    const int pixelHeight = qMax(1, qRound(logicalHeight * dpr));

    m_overlayImage = QImage(pixelWidth, pixelHeight, QImage::Format_RGBA8888);
    m_overlayImage.setDevicePixelRatio(dpr);
    m_overlayImage.fill(Qt::transparent);

    if (logicalWidth <= 0 || logicalHeight <= 0 || m_sfreq <= 0.f || m_samplesPerPixel <= 0.f) {
        m_overlayDirty = false;
        return;
    }

    QPainter p(&m_overlayImage);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // ── Alternating per-second bands ────────────────────────────────
    if (m_gridVisible) {
        const float samplesPerSec = m_sfreq;
        const float origin        = static_cast<float>(m_firstFileSample);
        float firstBound = std::floor((m_scrollSample - origin) / samplesPerSec)
                           * samplesPerSec + origin;
        long long bandIdx = static_cast<long long>((firstBound - origin) / samplesPerSec);
        bool oddBand      = (bandIdx & 1) != 0;

        QColor tint(0, 0, 0, 20); // 8 % opacity dark tint on odd seconds
        const float vw = static_cast<float>(logicalWidth);
        const float vh = static_cast<float>(logicalHeight);
        const float sppF = m_samplesPerPixel;

        for (float s = firstBound;
             s < m_scrollSample + logicalWidth * sppF + samplesPerSec;
             s += samplesPerSec, oddBand = !oddBand) {
            if (!oddBand) continue;
            float xStart = (s - m_scrollSample) / sppF;
            float xEnd   = xStart + samplesPerSec / sppF;
            xStart = qBound(0.f, xStart, vw);
            xEnd   = qBound(0.f, xEnd,   vw);
            if (xEnd > xStart)
                p.fillRect(QRectF(xStart, 0, xEnd - xStart, vh), tint);
        }
    }

    // ── Annotation spans ────────────────────────────────────────────
    if (m_bShowAnnotations && !m_annotations.isEmpty()) {
        QFont font = p.font();
        font.setPointSizeF(8.0);
        font.setBold(true);
        p.setFont(font);

        for (const AnnotationSpan &annotation : m_annotations) {
            const float xStart = (static_cast<float>(annotation.startSample) - m_scrollSample) / m_samplesPerPixel;
            const float xEnd = (static_cast<float>(annotation.endSample + 1) - m_scrollSample) / m_samplesPerPixel;
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

            if (!annotation.label.trimmed().isEmpty()) {
                QString label = annotation.label.trimmed();
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
    if (m_bShowEvents && !m_events.isEmpty()) {
        for (const EventMarker &ev : m_events) {
            float xF = (static_cast<float>(ev.sample) - m_scrollSample) / m_samplesPerPixel;
            if (xF < -2.f || xF > logicalWidth + 2.f)
                continue;
            QColor lineColor = ev.color;
            lineColor.setAlpha(180);
            p.setPen(QPen(lineColor, 1));
            p.drawLine(QPointF(xF, 0.f), QPointF(xF, static_cast<float>(logicalHeight)));
        }
    }

    // ── Epoch trigger marker lines ──────────────────────────────────
    if (m_bShowEpochMarkers && !m_epochTriggerSamples.isEmpty()) {
        QPen epochPen(QColor(100, 100, 100, 140), 1, Qt::DashLine);
        p.setPen(epochPen);
        for (int trigSample : m_epochTriggerSamples) {
            float xF = (static_cast<float>(trigSample) - m_scrollSample) / m_samplesPerPixel;
            if (xF < -2.f || xF > logicalWidth + 2.f)
                continue;
            p.drawLine(QPointF(xF, 0.f), QPointF(xF, static_cast<float>(logicalHeight)));
        }
    }

    m_overlayDirty = false;
}

//=============================================================================================================

void ChannelRhiView::ensureOverlayPipeline()
{
    QRhi *rhi = this->rhi();
    if (!rhi || !renderTarget())
        return;
    if (m_overlayPipeline)
        return;

    // Full-screen quad VBO: (pos.x, pos.y, uv.x, uv.y) per vertex, TriangleStrip.
    // Dynamic so it can be uploaded in render() via the existing batch.
    // NDC Y+ = top. Image UV Y=0 = top.
    //   NDC(-1,-1)=bottom-left → UV(0,1)
    //   NDC(-1, 1)=top-left    → UV(0,0)
    //   NDC( 1,-1)=bottom-right→ UV(1,1)
    //   NDC( 1, 1)=top-right   → UV(1,0)
    static constexpr int kQuadBytes = 4 * 4 * sizeof(float);
    m_overlayVbo.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                      QRhiBuffer::VertexBuffer,
                                      kQuadBytes));
    if (!m_overlayVbo->create()) {
        m_overlayVbo.reset();
        return;
    }

    // Texture — placeholder 1×1; resized lazily in render() when pw/ph are known.
    m_overlayTex.reset(rhi->newTexture(QRhiTexture::RGBA8, QSize(1, 1)));
    m_overlayTex->create();

    m_overlaySampler.reset(rhi->newSampler(
        QRhiSampler::Linear, QRhiSampler::Linear,
        QRhiSampler::None,
        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    m_overlaySampler->create();

    // SRB: binding 1 = combined image sampler
    m_overlaySrb.reset(rhi->newShaderResourceBindings());
    m_overlaySrb->setBindings({
        QRhiShaderResourceBinding::sampledTexture(
            1, QRhiShaderResourceBinding::FragmentStage,
            m_overlayTex.get(), m_overlaySampler.get())
    });
    m_overlaySrb->create();

    auto loadShader = [](const QString &path) -> QShader {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << "ChannelRhiView: cannot open shader" << path;
            return {};
        }
        return QShader::fromSerialized(f.readAll());
    };

    QShader vs = loadShader(QStringLiteral(":/disp/shaders/viewers/helpers/shaders/overlay.vert.qsb"));
    QShader fs = loadShader(QStringLiteral(":/disp/shaders/viewers/helpers/shaders/overlay.frag.qsb"));
    if (!vs.isValid() || !fs.isValid()) {
        m_overlayVbo.reset();
        m_overlaySrb.reset();
        m_overlaySampler.reset();
        m_overlayTex.reset();
        return;
    }

    m_overlayPipeline.reset(rhi->newGraphicsPipeline());
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable   = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_overlayPipeline->setTargetBlends({ blend });
    m_overlayPipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    m_overlayPipeline->setDepthTest(false);
    m_overlayPipeline->setDepthWrite(false);
    m_overlayPipeline->setShaderStages({
        { QRhiShaderStage::Vertex,   vs },
        { QRhiShaderStage::Fragment, fs },
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ QRhiVertexInputBinding(4 * sizeof(float)) });
    inputLayout.setAttributes({
        QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0),
        QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float))
    });
    m_overlayPipeline->setVertexInputLayout(inputLayout);
    m_overlayPipeline->setShaderResourceBindings(m_overlaySrb.get());
    m_overlayPipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());

    if (!m_overlayPipeline->create()) {
        qWarning() << "ChannelRhiView: overlay pipeline create failed";
        m_overlayPipeline.reset();
    }
}

//=============================================================================================================

void ChannelRhiView::render(QRhiCommandBuffer *cb)
{
    if (!m_model || totalLogicalChannels() == 0) {
        // Clear to background colour only
        QRhiResourceUpdateBatch *u = rhi()->nextResourceUpdateBatch();
        QColor bg = m_bgColor;
        cb->beginPass(renderTarget(), bg, {1.f, 0}, u);
        cb->endPass();
        return;
    }

    // ── Ensure GPU resources ─────────────────────────────────────────────
    ensurePipeline();
    if (!m_pipeline) {
        // Pipeline not ready: show RED background so the failure is visible
        QRhiResourceUpdateBatch *u = rhi()->nextResourceUpdateBatch();
        cb->beginPass(renderTarget(), QColor(220, 0, 0), {1.f, 0}, u);
        cb->endPass();
        return;
    }

    QSize ps = renderTarget()->pixelSize();
    const int pw = ps.width();
    const int ph = ps.height();
    const int logicalW = width();
    const int logicalH = height();
    const qreal overlayDpr = (logicalW > 0) ? (static_cast<qreal>(pw) / static_cast<qreal>(logicalW)) : 1.0;

    QRhiResourceUpdateBatch *batch = rhi()->nextResourceUpdateBatch();

    if (isVboDirty())
        rebuildVBOs(batch);

    updateUBO(batch);

    // ── Overlay image (bands + event lines) ──────────────────────────────
    ensureOverlayPipeline();
    bool overlayReady = false;
    if (m_overlayPipeline && m_overlayVbo && pw > 0 && ph > 0 && logicalW > 0 && logicalH > 0) {
        // Rebuild the overlay QImage if dirty or resized
        if (m_overlayDirty || QSize(pw, ph) != m_overlayTexSize) {
            rebuildOverlayImage(logicalW, logicalH, overlayDpr);
            // (Re)create texture at the correct pixel size
            if (QSize(pw, ph) != m_overlayTexSize) {
                m_overlayTex.reset(rhi()->newTexture(QRhiTexture::RGBA8, QSize(pw, ph)));
                m_overlayTex->create();
                // Re-create SRB because it references the texture
                m_overlaySrb->setBindings({
                    QRhiShaderResourceBinding::sampledTexture(
                        1, QRhiShaderResourceBinding::FragmentStage,
                        m_overlayTex.get(), m_overlaySampler.get())
                });
                m_overlaySrb->create();
                m_overlayTexSize = QSize(pw, ph);
            }
            QRhiTextureUploadEntry entry(0, 0, QRhiTextureSubresourceUploadDescription(m_overlayImage));
            batch->uploadTexture(m_overlayTex.get(), entry);
        }

        // Upload quad VBO (dynamic, 4 vertices × 4 floats)
        static const float kQuadVerts[] = {
            -1.f, -1.f,  0.f, 1.f,
            -1.f,  1.f,  0.f, 0.f,
             1.f, -1.f,  1.f, 1.f,
             1.f,  1.f,  1.f, 0.f,
        };
        batch->updateDynamicBuffer(m_overlayVbo.get(), 0, sizeof(kQuadVerts), kQuadVerts);
        overlayReady = true;
    }

    // ── Render pass ──────────────────────────────────────────────────────
    QColor bg = m_bgColor;
    cb->beginPass(renderTarget(), bg, {1.f, 0}, batch);

    cb->setViewport(QRhiViewport(0.f, 0.f, static_cast<float>(pw),
                                             static_cast<float>(ph)));

    // ── Waveform traces ──────────────────────────────────────────────────
    cb->setGraphicsPipeline(m_pipeline.get());

    int totalCh = totalLogicalChannels();

    if (m_butterflyMode) {
        // Butterfly: render ALL channels; UBO slots indexed by logical channel
        int nToRender = qMin(totalCh, kMaxChannels);
        for (int logCh = 0; logCh < nToRender; ++logCh) {
            if (logCh >= static_cast<int>(m_gpuChannels.size()))
                break;
            auto &gd = m_gpuChannels[logCh];
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
        int firstCh = qBound(0, m_firstVisibleChannel, totalCh);
        int visCnt  = qMin(m_visibleChannelCount, totalCh - firstCh);
        int nToRender = qMin(visCnt, kMaxChannels);

        for (int i = 0; i < nToRender; ++i) {
            int logCh = firstCh + i;
            if (logCh >= static_cast<int>(m_gpuChannels.size()))
                break;
            auto &gd = m_gpuChannels[logCh];
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

    // ── Overlay blit (bands + event lines) — drawn after waveforms ───────
    if (overlayReady) {
        cb->setGraphicsPipeline(m_overlayPipeline.get());
        cb->setShaderResources(m_overlaySrb.get());
        QRhiCommandBuffer::VertexInput overlayVi(m_overlayVbo.get(), 0);
        cb->setVertexInput(0, 1, &overlayVi);
        cb->draw(4); // TriangleStrip: 4 vertices = 2 triangles = full-screen quad
    }

    cb->endPass();
}

void ChannelRhiView::paintEvent(QPaintEvent *event)
{
    QRhiWidget::paintEvent(event);
    drawOverlays();
}

//=============================================================================================================
// Tile cache helpers retained for off-thread waveform staging.
//=============================================================================================================

bool ChannelRhiView::isTileFresh() const
{
    if (m_tileDirty || m_tileImage.isNull() || m_tileSamplesPerPixel <= 0.f)
        return false;
    if (!qFuzzyCompare(m_tileSamplesPerPixel, m_samplesPerPixel))
        return false;
    if (m_tileFirstChannel != m_firstVisibleChannel)
        return false;
    int totalCh      = totalLogicalChannels();
    int visibleCount = qMin(m_visibleChannelCount, totalCh - m_firstVisibleChannel);
    if (m_tileVisibleCount != visibleCount)
        return false;

    // Tile is stale if current scroll is within one visible-width of either edge
    float visibleSamples = width() * m_samplesPerPixel;
    float tileEnd = m_tileSampleFirst + m_tileImage.width() * m_tileSamplesPerPixel;
    if (m_scrollSample < m_tileSampleFirst + visibleSamples)
        return false;
    if (m_scrollSample + visibleSamples > tileEnd - visibleSamples)
        return false;

    return true;
}

//=============================================================================================================

void ChannelRhiView::scheduleTileRebuild()
{
    // Guard: already a rebuild in flight — it will re-check m_tileDirty when done
    if (m_tileRebuildPending)
        return;

    if (!m_model || totalLogicalChannels() == 0 || width() <= 0 || height() <= 0) {
        // No model / no channels / zero-size: produce a stable blank tile synchronously.
        // This prevents an infinite repaint loop: the async worker would return a null
        // image → watcher fires update() → paintEvent → rebuild → repeat.
        m_tileImage = QImage(qMax(width(), 1), qMax(height(), 1), QImage::Format_RGB32);
        m_tileImage.fill(m_bgColor.rgb());
        m_tileSampleFirst     = m_scrollSample;
        m_tileSamplesPerPixel = qMax(m_samplesPerPixel, 1e-4f);
        m_tileFirstChannel    = m_firstVisibleChannel;
        m_tileVisibleCount    = 0;
        m_tileDirty           = false;
        return;
    }

    // Snapshot all view state for the worker (worker must NOT touch 'this')
    ChannelDataModel *model           = m_model.data();
    float             scrollSample    = m_scrollSample;
    float             spp             = m_samplesPerPixel;
    int               firstCh         = m_firstVisibleChannel;
    int               visCnt          = m_visibleChannelCount;
    int               pw              = width();
    int               ph              = height();
    QColor            bg              = m_bgColor;
    bool              gridVis         = m_gridVisible;
    float             sfreq           = m_sfreq;
    int               firstFileSample = m_firstFileSample;
    bool              hideBad         = m_hideBadChannels;
    QVector<int>      chIndices       = m_filteredChannels; // snapshot for worker
    QVector<EventMarker> eventsSnap   = m_bShowEvents ? m_events : QVector<EventMarker>();
    QVector<AnnotationSpan> annotationsSnap = m_bShowAnnotations ? m_annotations : QVector<AnnotationSpan>();
    QVector<int> epochSnap = m_bShowEpochMarkers ? m_epochTriggerSamples : QVector<int>();

    m_tileDirty          = false; // cleared now — any new event will set it true again
    m_tileRebuildPending = true;
    m_tileWatcher.setFuture(QtConcurrent::run([=]() {
        return ChannelRhiView::buildTile(model, scrollSample, spp, firstCh, visCnt,
                                         pw, ph, bg, gridVis, sfreq, firstFileSample,
                                         hideBad, chIndices, eventsSnap, annotationsSnap, epochSnap);
    }));
}

//=============================================================================================================

ChannelRhiView::TileResult ChannelRhiView::buildTile(
    ChannelDataModel *model,
    float scrollSample, float spp,
    int firstCh, int visCnt,
    int pw, int ph,
    QColor bgColor, bool gridVisible,
    float sfreq, int firstFileSample,
    bool hideBadChannels,
    const QVector<int> &channelIndices,
    const QVector<EventMarker> &events,
    const QVector<AnnotationSpan> &annotations,
    const QVector<int> &epochMarkers)
{
    TileResult out;
    out.samplesPerPixel = spp;
    out.firstChannel    = firstCh;

    if (!model || pw <= 0 || ph <= 0 || spp <= 0.f)
        return out;

    int totalCh      = channelIndices.isEmpty() ? model->channelCount() : channelIndices.size();
    int visibleCount = qMin(visCnt, totalCh - firstCh);
    if (visibleCount <= 0)
        return out;

    out.visibleCount = visibleCount;

    const int kTileMult  = 5;
    int   tilePixWidth   = pw * kTileMult;
    float visibleSamples = pw * spp;
    float tileStart      = scrollSample - 2.f * visibleSamples;

    out.sampleFirst = tileStart;

    QImage img(tilePixWidth, ph, QImage::Format_RGB32);
    img.fill(bgColor.rgb());

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, false);

    float laneH     = static_cast<float>(ph) / visibleCount;
    int firstSample = static_cast<int>(tileStart);
    int lastSample  = firstSample + static_cast<int>(tilePixWidth * spp) + 1;

    // ── Alternating per-second background bands ─────────────────────
    // Draw subtle alternating grey/white bands every second, like MNE-Python browser.
    if (sfreq > 0.f) {
        float samplesPerSec = sfreq;
        float firstBound = std::floor(
            (tileStart - static_cast<float>(firstFileSample)) / samplesPerSec
        ) * samplesPerSec + static_cast<float>(firstFileSample);

        // Determine parity of the first band (0 = even, 1 = odd)
        long long bandIndex = static_cast<long long>(
            (firstBound - static_cast<float>(firstFileSample)) / samplesPerSec);
        bool oddBand = (bandIndex & 1) != 0;

        // Compute a slightly darker shade for odd bands relative to bgColor
        QColor altColor(
            qBound(0, bgColor.red()   - 10, 255),
            qBound(0, bgColor.green() - 10, 255),
            qBound(0, bgColor.blue()  - 10, 255)
        );

        for (float s = firstBound; s < lastSample; s += samplesPerSec, oddBand = !oddBand) {
            if (!oddBand)
                continue; // even seconds use the regular bgColor already filled
            float xStart = (s - tileStart) / spp;
            float xEnd   = xStart + samplesPerSec / spp;
            xStart = qBound(0.f, xStart, static_cast<float>(tilePixWidth));
            xEnd   = qBound(0.f, xEnd,   static_cast<float>(tilePixWidth));
            if (xEnd > xStart)
                p.fillRect(QRectF(xStart, 0, xEnd - xStart, ph), altColor);
        }
    }

    // ── Grid pass ──────────────────────────────────────────────────────
    if (gridVisible) {
        for (int i = 0; i < visibleCount; ++i) {
            float yMid = (i + 0.5f) * laneH;
            float yTop = i * laneH;

            if (i > 0) {
                p.setPen(QPen(QColor(205, 205, 215), 1));
                p.drawLine(QPointF(0, yTop), QPointF(tilePixWidth, yTop));
            }

            QPen guidePen(QColor(228, 228, 235), 1, Qt::DotLine);
            guidePen.setDashPattern({3, 4});
            p.setPen(guidePen);
            p.drawLine(QPointF(0, yMid - laneH * 0.44f), QPointF(tilePixWidth, yMid - laneH * 0.44f));
            p.drawLine(QPointF(0, yMid + laneH * 0.44f), QPointF(tilePixWidth, yMid + laneH * 0.44f));

            p.setPen(QPen(QColor(210, 210, 218), 1));
            p.drawLine(QPointF(0, yMid), QPointF(tilePixWidth, yMid));
        }

        if (sfreq > 0.f) {
            static const float kNiceIntervals[] = {
                0.05f, 0.1f, 0.2f, 0.5f, 1.f, 2.f, 5.f, 10.f, 30.f, 60.f
            };
            float pxPerSecond   = sfreq / spp;
            float tickIntervalS = kNiceIntervals[0];
            for (float iv : kNiceIntervals) {
                tickIntervalS = iv;
                if (iv * pxPerSecond >= 80.f)
                    break;
            }
            float tickSamples = tickIntervalS * sfreq;
            float origin    = static_cast<float>(firstFileSample);
            float firstTick = std::ceil((tileStart - origin) / tickSamples) * tickSamples + origin;

            p.setPen(QPen(QColor(205, 205, 210), 1));
            for (float s = firstTick; s < lastSample; s += tickSamples) {
                float xPx = (s - tileStart) / spp;
                p.drawLine(QPointF(xPx, 0), QPointF(xPx, ph));
            }
        }
    }

    // ── Annotation span pass ────────────────────────────────────────
    if (!annotations.isEmpty()) {
        QFont font = p.font();
        font.setPointSizeF(8.0);
        font.setBold(true);
        p.setFont(font);

        for (const AnnotationSpan &annotation : annotations) {
            float xStart = (static_cast<float>(annotation.startSample) - tileStart) / spp;
            float xEnd = (static_cast<float>(annotation.endSample + 1) - tileStart) / spp;

            if (xEnd < -2.f || xStart > tilePixWidth + 2.f) {
                continue;
            }

            xStart = qBound(0.f, xStart, static_cast<float>(tilePixWidth));
            xEnd = qBound(0.f, xEnd, static_cast<float>(tilePixWidth));
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

            if (!annotation.label.trimmed().isEmpty()) {
                const QString label = annotation.label.trimmed();
                QFontMetrics metrics(font);
                QRect labelRect = metrics.boundingRect(label);
                labelRect.adjust(-6, -2, 6, 2);
                const int labelX = qBound(4,
                                          static_cast<int>(xStart) + 4,
                                          qMax(4, tilePixWidth - labelRect.width() - 4));
                labelRect.moveTopLeft(QPoint(labelX, 4));
                QColor pillColor = annotation.color;
                pillColor.setAlpha(215);
                p.fillRect(labelRect, pillColor);
                p.setPen(Qt::white);
                p.drawText(labelRect, Qt::AlignCenter, label);
            }
        }
    }

    // ── Channel waveform pass ───────────────────────────────────────────
    for (int i = 0; i < visibleCount; ++i) {
        int logIdx = firstCh + i;
        int ch = channelIndices.isEmpty() ? logIdx
               : (logIdx < channelIndices.size() ? channelIndices[logIdx] : -1);
        if (ch < 0)
            continue;
        auto info = model->channelInfo(ch);

        // Skip trace for bad channels when hiding
        if (hideBadChannels && info.bad)
            continue;

        int vboFirst = 0;
        QVector<float> verts = model->decimatedVertices(
            ch, firstSample, lastSample, tilePixWidth, vboFirst);
        if (verts.size() < 4)
            continue;

        QColor col = info.bad ? QColor(190, 40, 40) : info.color;
        p.setPen(QPen(col, 1.2));

        float yMid   = (i + 0.5f) * laneH;
        float yScale = (laneH * 0.45f) / (info.amplitudeMax > 0.f ? info.amplitudeMax : 1.f);

        int nVerts = verts.size() / 2;
        QPolygonF poly;
        poly.reserve(nVerts);
        for (int v = 0; v < nVerts; ++v) {
            float samplePos = vboFirst + verts[v * 2];
            float xPx = (samplePos - tileStart) / spp;
            float yPx = yMid - verts[v * 2 + 1] * yScale;
            poly.append(QPointF(xPx, yPx));
        }
        p.drawPolyline(poly);
    }

    // ── Event / stimulus marker pass ─────────────────────────────────
    // Draw coloured vertical lines spanning the full channel area.
    // Label chips are shown in the TimeRulerWidget stim lane.
    if (!events.isEmpty() && spp > 0.f) {
        for (const EventMarker &ev : events) {
            float xF = (static_cast<float>(ev.sample) - tileStart) / spp;
            if (xF < -2.f || xF > tilePixWidth + 2.f)
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
    if (!epochMarkers.isEmpty() && spp > 0.f) {
        QPen epochPen(QColor(100, 100, 100, 140), 1, Qt::DashLine);
        p.setPen(epochPen);
        for (int trigSample : epochMarkers) {
            float xF = (static_cast<float>(trigSample) - tileStart) / spp;
            if (xF < -2.f || xF > tilePixWidth + 2.f)
                continue;
            int ix = static_cast<int>(xF);
            p.drawLine(ix, 0, ix, ph);
        }
    }

    out.image = std::move(img);
    return out;
}

//=============================================================================================================

void ChannelRhiView::drawOverlays()
{
    // Schedule an overlay repaint so crosshair/scalebars/ruler stay in sync
    // after GPU-driven scroll/zoom repaints.  Use update() (asynchronous)
    // instead of repaint() because this is called from within paintEvent;
    // synchronous repaint() from inside a paint handler can starve sibling
    // widgets (e.g. the overview bar) of paint cycles.
    if (m_overlay && (m_crosshairEnabled || m_scalebarsVisible || m_rulerActive))
        m_overlay->update();
}

//=============================================================================================================

static QString formatAmplitude(float amp, const QString &unit)
{
    float absAmp = qAbs(amp);
    if (absAmp == 0.f)
        return QStringLiteral("0 ") + unit;
    if (absAmp < 1e-9f)
        return QString::number(amp * 1e12f, 'f', 1) + QStringLiteral(" p") + unit;
    if (absAmp < 1e-6f)
        return QString::number(amp * 1e9f, 'f', 1) + QStringLiteral(" n") + unit;
    if (absAmp < 1e-3f)
        return QString::number(amp * 1e6f, 'f', 1) + QStringLiteral(" µ") + unit;
    if (absAmp < 1.f)
        return QString::number(amp * 1e3f, 'f', 1) + QStringLiteral(" m") + unit;
    return QString::number(amp, 'f', 3) + QStringLiteral(" ") + unit;
}

static QString unitForType(const QString &typeLabel)
{
    if (typeLabel == QStringLiteral("MEG grad"))
        return QStringLiteral("T/m");
    if (typeLabel == QStringLiteral("MEG mag") || typeLabel == QStringLiteral("MEG"))
        return QStringLiteral("T");
    if (typeLabel == QStringLiteral("EEG") ||
        typeLabel == QStringLiteral("EOG") ||
        typeLabel == QStringLiteral("ECG") ||
        typeLabel == QStringLiteral("EMG"))
        return QStringLiteral("V");
    return QStringLiteral("AU");
}

//=============================================================================================================

void ChannelRhiView::drawCrosshair(QPainter &p)
{
    if (m_crosshairX < 0 || m_crosshairY < 0)
        return;
    if (!m_model || totalLogicalChannels() == 0)
        return;

    const int w = width();
    const int h = height();

    // Draw crosshair lines
    QPen crossPen(QColor(80, 80, 80, 160), 1, Qt::DashLine);
    p.setPen(crossPen);
    p.drawLine(m_crosshairX, 0, m_crosshairX, h);
    p.drawLine(0, m_crosshairY, w, m_crosshairY);

    int sample = static_cast<int>(m_scrollSample + static_cast<float>(m_crosshairX) * m_samplesPerPixel);
    float timeSec = (m_sfreq > 0.f) ? static_cast<float>(sample - m_firstFileSample) / m_sfreq : 0.f;
    QString channelLabel;
    QString unitStr;
    float value = 0.f;

    if (m_butterflyMode) {
        // In butterfly mode, lanes correspond to type groups
        const auto groups = butterflyTypeGroups();
        int nLanes = groups.size();
        if (nLanes <= 0)
            return;
        float laneH = static_cast<float>(h) / nLanes;
        int lane = qBound(0, static_cast<int>(m_crosshairY / laneH), nLanes - 1);
        channelLabel = groups[lane].typeLabel;
        unitStr = unitForType(groups[lane].typeLabel);
    } else {
        // Normal mode: determine the channel and sample under the cursor
        int totalCh = totalLogicalChannels();
        int visCnt  = qMin(m_visibleChannelCount, totalCh - m_firstVisibleChannel);
        if (visCnt <= 0)
            return;

        float laneH = static_cast<float>(h) / visCnt;
        int   row   = qBound(0, static_cast<int>(m_crosshairY / laneH), visCnt - 1);
        int   ch    = actualChannelAt(m_firstVisibleChannel + row);
        if (ch < 0)
            return;

        auto info = m_model->channelInfo(ch);
        value = m_model->sampleValueAt(ch, sample);
        channelLabel = info.name;
        unitStr = unitForType(info.typeLabel);
    }

    // Draw info label near cursor
    QString timeStr;
    if (m_useClockTime && timeSec >= 0.f) {
        int totalMs = static_cast<int>(timeSec * 1000.f + 0.5f);
        int m   = totalMs / 60000;
        int sec = (totalMs % 60000) / 1000;
        int ms  = totalMs % 1000;
        timeStr = QString("%1:%2.%3")
            .arg(m, 2, 10, QChar('0'))
            .arg(sec, 2, 10, QChar('0'))
            .arg(ms, 3, 10, QChar('0'));
    } else {
        timeStr = QString::number(static_cast<double>(timeSec), 'f', 3) + QStringLiteral(" s");
    }
    QString label = QString("%1  %2  %3")
                    .arg(channelLabel,
                         timeStr,
                         formatAmplitude(value, unitStr));

    QFont f = font();
    f.setPointSizeF(8.5);
    p.setFont(f);
    QFontMetrics fm(f);
    QRect labelRect = fm.boundingRect(label);
    int lx = m_crosshairX + 10;
    int ly = m_crosshairY - 10;
    if (lx + labelRect.width() + 8 > w)
        lx = m_crosshairX - labelRect.width() - 18;
    if (ly - labelRect.height() < 4)
        ly = m_crosshairY + labelRect.height() + 6;
    labelRect.moveTopLeft(QPoint(lx, ly - labelRect.height()));
    labelRect.adjust(-4, -2, 4, 2);
    p.fillRect(labelRect, QColor(255, 255, 255, 220));
    p.setPen(QColor(30, 30, 30));
    p.drawText(labelRect, Qt::AlignCenter, label);
}

//=============================================================================================================

void ChannelRhiView::emitCursorData()
{
    if (m_crosshairX < 0 || m_crosshairY < 0)
        return;
    if (!m_model || totalLogicalChannels() == 0)
        return;

    const int h = height();
    int sample = static_cast<int>(m_scrollSample + static_cast<float>(m_crosshairX) * m_samplesPerPixel);
    float timeSec = (m_sfreq > 0.f) ? static_cast<float>(sample - m_firstFileSample) / m_sfreq : 0.f;

    if (m_butterflyMode) {
        const auto groups = butterflyTypeGroups();
        int nLanes = groups.size();
        if (nLanes <= 0) return;
        float laneH = static_cast<float>(h) / nLanes;
        int lane = qBound(0, static_cast<int>(m_crosshairY / laneH), nLanes - 1);
        emit cursorDataChanged(timeSec, 0.f,
                               groups[lane].typeLabel,
                               unitForType(groups[lane].typeLabel));
    } else {
        int totalCh = totalLogicalChannels();
        int visCnt  = qMin(m_visibleChannelCount, totalCh - m_firstVisibleChannel);
        if (visCnt <= 0) return;
        float laneH = static_cast<float>(h) / visCnt;
        int   row   = qBound(0, static_cast<int>(m_crosshairY / laneH), visCnt - 1);
        int   ch    = actualChannelAt(m_firstVisibleChannel + row);
        if (ch < 0) return;
        auto info = m_model->channelInfo(ch);
        float value = m_model->sampleValueAt(ch, sample);
        emit cursorDataChanged(timeSec, value, info.name, unitForType(info.typeLabel));
    }
}

//=============================================================================================================

void ChannelRhiView::drawScalebars(QPainter &p)
{
    if (!m_model || totalLogicalChannels() == 0)
        return;

    int visCnt;
    if (m_butterflyMode) {
        visCnt = butterflyLaneCount();
    } else {
        int totalCh = totalLogicalChannels();
        visCnt = qMin(m_visibleChannelCount, totalCh - m_firstVisibleChannel);
    }
    if (visCnt <= 0)
        return;

    float laneH = static_cast<float>(height()) / visCnt;

    // Collect unique channel types and their amplitude scales
    QMap<QString, float> typeScales;
    if (m_butterflyMode) {
        const auto groups = butterflyTypeGroups();
        for (const auto &g : groups)
            if (g.amplitudeMax > 0.f)
                typeScales[g.typeLabel] = g.amplitudeMax;
    } else {
        for (int i = 0; i < visCnt; ++i) {
            int ch = actualChannelAt(m_firstVisibleChannel + i);
            if (ch < 0) continue;
            auto info = m_model->channelInfo(ch);
            if (!typeScales.contains(info.typeLabel) && info.amplitudeMax > 0.f)
                typeScales[info.typeLabel] = info.amplitudeMax;
        }
    }

    if (typeScales.isEmpty())
        return;

    QFont f = font();
    f.setPointSizeF(8.0);
    p.setFont(f);
    QFontMetrics fm(f);

    // Draw scalebars in the bottom-right corner
    const int margin = 12;
    const int barHeight = qBound(20, static_cast<int>(laneH * 0.35f), 60);
    int x = width() - margin;
    int y = height() - margin;

    for (auto it = typeScales.constEnd(); it != typeScales.constBegin(); ) {
        --it;
        QString unit = unitForType(it.key());
        float ampValue = it.value();
        QString label = it.key() + QStringLiteral(": ") + formatAmplitude(ampValue, unit);

        int textW = fm.horizontalAdvance(label);
        int barX = x - textW - 14;

        // Background pill
        QRect bgRect(barX - 4, y - barHeight - fm.height() - 4, textW + 22, barHeight + fm.height() + 8);
        p.fillRect(bgRect, QColor(255, 255, 255, 200));

        // Draw bar
        QPen barPen(QColor(40, 40, 40), 2);
        p.setPen(barPen);
        int barTop = y - barHeight;
        p.drawLine(barX + 4, barTop, barX + 4, y);
        // Tick marks
        p.drawLine(barX, barTop, barX + 8, barTop);
        p.drawLine(barX, y, barX + 8, y);

        // Label
        p.setPen(QColor(30, 30, 30));
        p.drawText(barX + 14, y - barHeight / 2 + fm.ascent() / 2, label);

        y -= barHeight + fm.height() + 16;
    }
}

//=============================================================================================================

void ChannelRhiView::drawRulerOverlay(QPainter &p)
{
    int x0 = m_rulerX0, y0 = m_rulerY0;
    int x1 = m_rulerX1, y1 = m_rulerY1;

    const bool snapH = (m_rulerSnap == RulerSnap::Horizontal);
    const bool snapV = (m_rulerSnap == RulerSnap::Vertical);

    const QColor activeColor(40, 120, 200, 220);
    const QColor dimColor(130, 160, 200, 120);
    int tickLen = 5;

    // ── Semi-transparent "frozen" overlay over the measured area ──
    {
        QRect measured;
        if (snapH)
            measured = QRect(QPoint(qMin(x0, x1), 0),
                             QPoint(qMax(x0, x1), height()));
        else if (snapV)
            measured = QRect(QPoint(0, qMin(y0, y1)),
                             QPoint(width(), qMax(y0, y1)));
        else
            measured = QRect(QPoint(qMin(x0, x1), qMin(y0, y1)),
                             QPoint(qMax(x0, x1), qMax(y0, y1)));
        p.fillRect(measured, QColor(255, 255, 255, 60));
        // Subtle border around the measured region
        p.setPen(QPen(QColor(40, 120, 200, 80), 1));
        p.drawRect(measured);
    }

    // Vertical guide lines at the two X positions
    QPen vLinePen(snapV ? dimColor : activeColor, 1, Qt::DashLine);
    p.setPen(vLinePen);
    p.drawLine(x0, 0, x0, height());
    if (!snapV)
        p.drawLine(x1, 0, x1, height());

    // Horizontal guide lines at the two Y positions (only when vertical snap)
    if (snapV) {
        QPen hGuidePen(activeColor, 1, Qt::DashLine);
        p.setPen(hGuidePen);
        p.drawLine(0, y0, width(), y0);
        p.drawLine(0, y1, width(), y1);
    }

    // Horizontal span line at y0
    QPen hLinePen(snapV ? dimColor : activeColor, snapH ? 2 : 1);
    p.setPen(hLinePen);
    if (!snapV)
        p.drawLine(qMin(x0, x1), y0, qMax(x0, x1), y0);

    // Vertical span line at x0
    QPen vSpanPen(snapH ? dimColor : activeColor, snapV ? 2 : 1);
    p.setPen(vSpanPen);
    if (!snapH)
        p.drawLine(x0, qMin(y0, y1), x0, qMax(y0, y1));

    // End tick marks
    if (!snapV) {
        p.setPen(QPen(activeColor, 1));
        p.drawLine(x0 - tickLen, y0, x0 + tickLen, y0);
        p.drawLine(x1 - tickLen, y0, x1 + tickLen, y0);
    }
    if (!snapH) {
        p.setPen(QPen(activeColor, 1));
        p.drawLine(x0, y0 - tickLen, x0, y0 + tickLen);
        p.drawLine(x0, y1 - tickLen, x0, y1 + tickLen);
    }

    // ── Measurement labels ────────────────────────────────────────────
    float deltaSamples = static_cast<float>(x1 - x0) * m_samplesPerPixel;
    float deltaSec     = (m_sfreq > 0.f) ? deltaSamples / m_sfreq : 0.f;

    float deltaAmp  = 0.f;
    QString ampUnit = QStringLiteral("AU");
    if (m_model && totalLogicalChannels() > 0) {
        int totalCh = totalLogicalChannels();
        int visCnt  = qMin(m_visibleChannelCount, totalCh - m_firstVisibleChannel);
        if (visCnt > 0) {
            float laneH = static_cast<float>(height()) / visCnt;
            int   row   = qBound(0, static_cast<int>(y0 / laneH), visCnt - 1);
            int   ch    = actualChannelAt(m_firstVisibleChannel + row);
            if (ch < 0) ch = 0;
            auto  info  = m_model->channelInfo(ch);
            if (info.amplitudeMax > 0.f) {
                float dyPx   = static_cast<float>(y1 - y0);
                float yScale = info.amplitudeMax / (laneH * 0.45f);
                deltaAmp = -dyPx * yScale;

                ampUnit = unitForType(info.typeLabel);
            }
        }
    }

    auto fmtTime = [](float sec) -> QString {
        float absSec = qAbs(sec);
        if (absSec < 1.f)
            return QString::number(sec * 1000.f, 'f', 1) + QStringLiteral(" ms");
        return QString::number(sec, 'f', 3) + QStringLiteral(" s");
    };
    auto fmtAmp = [](float amp, const QString &unit) -> QString {
        float absAmp = qAbs(amp);
        if (absAmp < 1e-6f)
            return QString::number(amp * 1e9f, 'f', 3) + QStringLiteral(" n") + unit;
        if (absAmp < 1e-3f)
            return QString::number(amp * 1e6f, 'f', 3) + QStringLiteral(" µ") + unit;
        if (absAmp < 1.f)
            return QString::number(amp * 1e3f, 'f', 3) + QStringLiteral(" m") + unit;
        return QString::number(amp, 'f', 3) + QStringLiteral(" ") + unit;
    };

    QString timeLabel = QStringLiteral("\u0394T = ") + fmtTime(deltaSec);
    QString ampLabel  = QStringLiteral("\u0394A = ") + fmtAmp(deltaAmp, ampUnit);

    QFont f = font();
    f.setPointSizeF(9.0);
    f.setBold(true);
    p.setFont(f);
    QFontMetrics fm(f);

    // Time label (shown unless vertical snap)
    if (!snapV) {
        int labelX = (x0 + x1) / 2;
        int labelY = y0 - 6;
        if (labelY < 14)
            labelY = y0 + 16;

        QRect tRect = fm.boundingRect(timeLabel);
        tRect.moveCenter(QPoint(labelX, labelY));
        tRect.adjust(-4, -2, 4, 2);
        p.fillRect(tRect, QColor(255, 255, 255, 210));
        p.setPen(QColor(20, 80, 160));
        p.drawText(tRect, Qt::AlignCenter, timeLabel);
    }

    // Amplitude label (shown unless horizontal snap)
    if (!snapH) {
        int aLabelX = x0 + 8;
        int aLabelY = (y0 + y1) / 2;
        QRect aRect = fm.boundingRect(ampLabel);
        aRect.moveCenter(QPoint(aLabelX + aRect.width() / 2, aLabelY));
        aRect.adjust(-4, -2, 4, 2);
        p.fillRect(aRect, QColor(255, 255, 255, 210));
        p.setPen(QColor(20, 80, 160));
        p.drawText(aRect, Qt::AlignCenter, ampLabel);
    }
}

//=============================================================================================================
// Shared event handlers
//=============================================================================================================

void ChannelRhiView::resizeEvent(QResizeEvent *event)
{
    QRhiWidget::resizeEvent(event);
    m_vboDirty = true;
    m_overlayDirty = true;
    m_tileDirty = true;
    if (m_overlay) m_overlay->syncSize();
    emit viewResized(width(), height());
    update();
}

//=============================================================================================================

void ChannelRhiView::wheelEvent(QWheelEvent *event)
{
    const QPoint delta = event->angleDelta();

    if (event->modifiers() & Qt::ControlModifier) {
        // Ctrl + wheel → zoom time axis
        float factor = (delta.y() > 0) ? 0.8f : 1.25f;
        zoomTo(m_samplesPerPixel * factor, 150);

    } else if (qAbs(delta.x()) > qAbs(delta.y())) {
        // Predominantly horizontal gesture (trackpad swipe left/right) → scroll time
        if (!m_frozen) {
            float step = width() * m_samplesPerPixel * 0.1f * m_scrollSpeedFactor
                         * (delta.x() > 0 ? -1.f : 1.f);
            scrollTo(m_scrollSample + step, 100);
        }

    } else if (m_wheelScrollsChannels) {
        // Vertical wheel → scroll channels (up = earlier, down = later)
        int channelStep = (delta.y() > 0) ? -1 : 1;
        int maxFirst = qMax(0, totalLogicalChannels() - m_visibleChannelCount);
        setFirstVisibleChannel(qBound(0, m_firstVisibleChannel + channelStep, maxFirst));
    } else {
        // Vertical wheel → scroll time
        if (!m_frozen) {
            float step = width() * m_samplesPerPixel * 0.15f * m_scrollSpeedFactor
                         * (delta.y() > 0 ? -1.f : 1.f);
            scrollTo(m_scrollSample + step, 100);
        }
    }

    event->accept();
}

//=============================================================================================================

void ChannelRhiView::mousePressEvent(QMouseEvent *event)
{
    // Right-click → start ruler measurement
    if (event->button() == Qt::RightButton) {
        // Stop any running inertial scroll before measuring
        if (m_pInertialAnim) {
            m_pInertialAnim->stop();
            m_pInertialAnim = nullptr;
        }
        m_rulerActive = true;
        m_rulerSnap   = RulerSnap::Free;
        m_rulerX0 = m_rulerX1 = m_rulerRawX1 = event->position().toPoint().x();
        m_rulerY0 = m_rulerY1 = m_rulerRawY1 = event->position().toPoint().y();
        if (m_overlay) m_overlay->repaint();
        event->accept();
        return;
    }

    if (!m_frozen &&
        (event->button() == Qt::MiddleButton ||
         (event->button() == Qt::LeftButton && (event->modifiers() & Qt::AltModifier)))) {
        m_dragging        = true;
        m_dragStartX      = event->position().toPoint().x();
        m_dragStartScroll = m_scrollSample;
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        // Stop any running inertial scroll
        if (m_pInertialAnim) {
            m_pInertialAnim->stop();
            m_pInertialAnim = nullptr;
        }

        // Check if clicking on an annotation boundary for drag-resize
        if (m_annotationSelectionEnabled && !m_annotations.isEmpty()) {
            bool isStart = false;
            int hitIdx = hitTestAnnotationBoundary(event->position().toPoint().x(), isStart);
            if (hitIdx >= 0) {
                m_annDragging    = true;
                m_annDragIndex   = hitIdx;
                m_annDragIsStart = isStart;
                event->accept();
                return;
            }
        }

        if (m_frozen) {
            // Frozen: clicks still emit sampleClicked but no drag
            float samplePos = m_scrollSample
                + static_cast<float>(event->position().x()) * m_samplesPerPixel;
            emit sampleClicked(static_cast<int>(samplePos));
            event->accept();
            return;
        }
        // Record start position; activate drag on move (threshold in mouseMoveEvent)
        m_leftButtonDown    = true;
        m_leftDragActivated = false;
        m_leftDownX         = event->position().toPoint().x();
        m_leftDownScroll    = m_scrollSample;
        m_velocityHistory.clear();
        m_dragTimer.start();
        m_velocityHistory.append({m_leftDownX, 0});
        event->accept();
        return;
    }
    QRhiWidget::mousePressEvent(event);
}

//=============================================================================================================

void ChannelRhiView::mouseMoveEvent(QMouseEvent *event)
{
    // ── Annotation boundary drag-resize ──────────────────────────────
    if (m_annDragging) {
        // Visually update the annotation boundary while dragging
        int newSample = static_cast<int>(m_scrollSample
            + static_cast<float>(event->position().toPoint().x()) * m_samplesPerPixel);
        newSample = qMax(newSample, m_firstFileSample);
        if (m_lastFileSample >= 0)
            newSample = qMin(newSample, m_lastFileSample);

        if (m_annDragIndex >= 0 && m_annDragIndex < m_annotations.size()) {
            if (m_annDragIsStart)
                m_annotations[m_annDragIndex].startSample = newSample;
            else
                m_annotations[m_annDragIndex].endSample = newSample;
            m_overlayDirty = true;
            m_tileDirty = true;
            update();
        }
        event->accept();
        return;
    }

    if (m_rulerActive) {
        m_rulerRawX1 = event->position().toPoint().x();
        m_rulerRawY1 = event->position().toPoint().y();

        // Snap logic: if displacement is dominantly horizontal → lock to horizontal,
        // if dominantly vertical → lock to vertical, otherwise free
        int dx = qAbs(m_rulerRawX1 - m_rulerX0);
        int dy = qAbs(m_rulerRawY1 - m_rulerY0);
        const int kSnapThresh = 8;  // minimum movement before snapping
        if (dx < kSnapThresh && dy < kSnapThresh) {
            m_rulerSnap = RulerSnap::Free;
        } else if (dx > dy * 2) {
            m_rulerSnap = RulerSnap::Horizontal;
        } else if (dy > dx * 2) {
            m_rulerSnap = RulerSnap::Vertical;
        } else {
            m_rulerSnap = RulerSnap::Free;
        }

        // Apply snap
        switch (m_rulerSnap) {
        case RulerSnap::Horizontal:
            m_rulerX1 = m_rulerRawX1;
            m_rulerY1 = m_rulerY0;  // lock Y
            break;
        case RulerSnap::Vertical:
            m_rulerX1 = m_rulerX0;  // lock X
            m_rulerY1 = m_rulerRawY1;
            break;
        default:
            m_rulerX1 = m_rulerRawX1;
            m_rulerY1 = m_rulerRawY1;
            break;
        }

        if (m_overlay) m_overlay->repaint();
        event->accept();
        return;
    }

    if (m_dragging) {
        int dx = event->position().toPoint().x() - m_dragStartX;
        float newScroll = m_dragStartScroll - static_cast<float>(dx) * m_samplesPerPixel;
        setScrollSample(newScroll);
        event->accept();
        return;
    }
    if (m_leftButtonDown) {
        int x = event->position().toPoint().x();
        int dx = x - m_leftDownX;
        if (!m_leftDragActivated && qAbs(dx) > 5)
            m_leftDragActivated = true;
        if (m_leftDragActivated) {
            float newScroll = m_leftDownScroll - static_cast<float>(dx) * m_samplesPerPixel;
            setScrollSample(newScroll);

            // Record velocity sample; keep only the last 100 ms
            qint64 now = m_dragTimer.elapsed();
            m_velocityHistory.append({x, now});
            while (m_velocityHistory.size() > 1 &&
                   now - m_velocityHistory.first().t > 100)
                m_velocityHistory.removeFirst();

            event->accept();
            return;
        }
    }

    // ── Crosshair tracking (passive mouse tracking without buttons) ──
    if (m_crosshairEnabled) {
        m_crosshairX = event->position().toPoint().x();
        m_crosshairY = event->position().toPoint().y();
        if (m_overlay) m_overlay->repaint();

        // Emit cursor data signal here (not from drawCrosshair) to keep
        // signal emission out of the paint path and avoid repaint cascades.
        emitCursorData();
    }

    // ── Annotation boundary hover cursor ─────────────────────────────
    if (m_annotationSelectionEnabled && !m_annotations.isEmpty()) {
        bool isStart = false;
        int hitIdx = hitTestAnnotationBoundary(event->position().toPoint().x(), isStart);
        if (hitIdx >= 0) {
            if (m_annHoverIndex != hitIdx || m_annHoverIsStart != isStart) {
                m_annHoverIndex   = hitIdx;
                m_annHoverIsStart = isStart;
                setCursor(Qt::SizeHorCursor);
            }
        } else if (m_annHoverIndex >= 0) {
            m_annHoverIndex = -1;
            unsetCursor();
        }
    }

    QRhiWidget::mouseMoveEvent(event);
}

//=============================================================================================================

void ChannelRhiView::mouseReleaseEvent(QMouseEvent *event)
{
    // ── Annotation boundary drag-resize completion ───────────────────
    if (m_annDragging && event->button() == Qt::LeftButton) {
        int newSample = static_cast<int>(m_scrollSample
            + static_cast<float>(event->position().toPoint().x()) * m_samplesPerPixel);
        newSample = qMax(newSample, m_firstFileSample);
        if (m_lastFileSample >= 0)
            newSample = qMin(newSample, m_lastFileSample);

        emit annotationBoundaryMoved(m_annDragIndex, m_annDragIsStart, newSample);
        m_annDragging  = false;
        m_annDragIndex = -1;
        event->accept();
        return;
    }

    if (m_rulerActive && event->button() == Qt::RightButton) {
        m_rulerRawX1 = event->position().toPoint().x();
        m_rulerRawY1 = event->position().toPoint().y();
        // Apply final snap
        switch (m_rulerSnap) {
        case RulerSnap::Horizontal:
            m_rulerX1 = m_rulerRawX1; m_rulerY1 = m_rulerY0; break;
        case RulerSnap::Vertical:
            m_rulerX1 = m_rulerX0; m_rulerY1 = m_rulerRawY1; break;
        default:
            m_rulerX1 = m_rulerRawX1; m_rulerY1 = m_rulerRawY1; break;
        }
        m_rulerActive = false;
        if (m_overlay) m_overlay->repaint();

        const int x0 = qMin(m_rulerX0, m_rulerX1);
        const int x1 = qMax(m_rulerX0, m_rulerX1);
        if (m_annotationSelectionEnabled && qAbs(x1 - x0) > 3) {
            int startSample = static_cast<int>(m_scrollSample + static_cast<float>(x0) * m_samplesPerPixel);
            int endSample = static_cast<int>(m_scrollSample + static_cast<float>(x1) * m_samplesPerPixel);

            startSample = qMax(startSample, m_firstFileSample);
            if (m_lastFileSample >= 0) {
                endSample = qMin(endSample, m_lastFileSample);
            }

            if (endSample >= startSample) {
                emit sampleRangeSelected(startSample, endSample);
            }
        }

        event->accept();
        return;
    }

    if (m_dragging && (event->button() == Qt::MiddleButton ||
                       event->button() == Qt::LeftButton)) {
        m_dragging = false;
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && m_leftButtonDown) {
        if (!m_leftDragActivated) {
            // Short tap — emit click position, no inertia
            float samplePos = m_leftDownScroll
                + static_cast<float>(event->position().x()) * m_samplesPerPixel;
            emit sampleClicked(static_cast<int>(samplePos));
        } else {
            // Compute velocity from recent history and launch inertial animation
            if (m_velocityHistory.size() >= 2) {
                auto oldest = m_velocityHistory.first();
                auto newest = m_velocityHistory.last();
                float dt = static_cast<float>(newest.t - oldest.t);
                if (dt > 5.f) {
                    float dx = static_cast<float>(newest.x - oldest.x);
                    // px/ms → samples/ms (positive dx = dragging right = going backward)
                    float velSampPerMs = -(dx / dt) * m_samplesPerPixel;
                    float speed = qAbs(velSampPerMs);
                    if (speed > 0.3f) { // threshold: ~300 samples/s minimum
                        // OutCubic: f'(0) = 3, so travel = v × duration / 3.
                        // Longer duration and distance for a smooth, phone-like glide.
                        float durationMs = qBound(500.f, speed * 1.0f, 5000.f);
                        float targetScroll = m_scrollSample + velSampPerMs * durationMs / 3.f;
                        targetScroll = qMax(targetScroll, static_cast<float>(m_firstFileSample));

                        m_pInertialAnim = new QPropertyAnimation(this, "scrollSample", this);
                        m_pInertialAnim->setDuration(static_cast<int>(durationMs));
                        m_pInertialAnim->setEasingCurve(QEasingCurve::OutCubic);
                        m_pInertialAnim->setStartValue(m_scrollSample);
                        m_pInertialAnim->setEndValue(targetScroll);
                        connect(m_pInertialAnim, &QPropertyAnimation::finished, this, [this]() {
                            m_pInertialAnim = nullptr;
                        });
                        m_pInertialAnim->start(QAbstractAnimation::DeleteWhenStopped);
                    }
                }
            }
        }
        m_leftButtonDown    = false;
        m_leftDragActivated = false;
        event->accept();
        return;
    }
    QRhiWidget::mouseReleaseEvent(event);
}

//=============================================================================================================

void ChannelRhiView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_model || event->button() != Qt::LeftButton) {
        QRhiWidget::mouseDoubleClickEvent(event);
        return;
    }

    int totalCh = totalLogicalChannels();
    int visCnt  = qMin(m_visibleChannelCount, totalCh - m_firstVisibleChannel);
    if (visCnt <= 0)
        return;

    float laneH = static_cast<float>(height()) / visCnt;
    int   row   = static_cast<int>(event->position().y() / laneH);
    if (row >= 0 && row < visCnt) {
        int  ch   = actualChannelAt(m_firstVisibleChannel + row);
        if (ch >= 0) {
            auto info = m_model->channelInfo(ch);
            m_model->setChannelBad(ch, !info.bad);
        }
    }
    event->accept();
}
