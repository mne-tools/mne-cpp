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

#ifdef MNE_DISP_RHIWIDGET
#  include <rhi/qrhi.h>
#  include <rhi/qshader.h>
#  include <QFile>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPropertyAnimation>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QPolygonF>
#include <QtMath>

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

#ifdef MNE_DISP_RHIWIDGET
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
#endif

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelRhiView::ChannelRhiView(QWidget *parent)
#ifdef MNE_DISP_RHIWIDGET
    : QRhiWidget(parent)
#else
    : QWidget(parent)
#endif
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

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

#ifdef MNE_DISP_RHIWIDGET
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

    // When Metal/OpenGL init fails, fall back to QPainter for all subsequent paint events
    connect(this, &QRhiWidget::renderFailed, this, [this]() {
        // Zero-size failures are transient – the widget just hasn't been laid out yet.
        // QRhiWidget will retry automatically on the next non-zero-size frame.
        if (width() <= 0 || height() <= 0)
            return;

        if (!m_rhiFailed) {
            // Non-zero size + Metal failed → try OpenGL before giving up
            if (api() == QRhiWidget::Api::Metal) {
                setApi(QRhiWidget::Api::OpenGL);
                update();
                return;
            }
            // Both Metal and OpenGL failed → QPainter fallback
            m_rhiFailed = true;
            update();
        }
    });

#endif
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
#ifdef MNE_DISP_RHIWIDGET
            m_vboDirty = true;
#endif
            m_tileDirty = true;
            update();
        });
        connect(m_model, &ChannelDataModel::metaChanged, this, [this] {
#ifdef MNE_DISP_RHIWIDGET
            m_vboDirty      = true;
            m_pipelineDirty = true;
#endif
            m_tileDirty = true;
            update();
        });
    }
#ifdef MNE_DISP_RHIWIDGET
    m_vboDirty    = true;
    m_pipelineDirty = true;
#endif
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

#ifdef MNE_DISP_RHIWIDGET
    // Check whether the prefetch window is still valid
    float visible = width() * m_samplesPerPixel;
    float margin  = m_prefetchFactor * visible;
    if (sample < m_vboWindowFirst + margin ||
        sample + visible > m_vboWindowLast - margin) {
        m_vboDirty = true;
    }
#endif

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
#ifdef MNE_DISP_RHIWIDGET
    m_vboDirty = true; // zoom change → decimation changes
#endif
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
    int maxFirst = m_model ? qMax(0, m_model->channelCount() - m_visibleChannelCount) : 0;
    ch = qBound(0, ch, maxFirst);
    if (ch == m_firstVisibleChannel)
        return;
    m_firstVisibleChannel = ch;
    m_tileDirty = true;
#ifdef MNE_DISP_RHIWIDGET
    m_vboDirty      = true;
    m_pipelineDirty = true;
#endif
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
#ifdef MNE_DISP_RHIWIDGET
    m_vboDirty      = true;
    m_pipelineDirty = true;
#endif
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
    update();
}

//=============================================================================================================

void ChannelRhiView::setSfreq(float sfreq)
{
    m_sfreq = qMax(sfreq, 0.f);
    m_tileDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setFirstFileSample(int first)
{
    if (first == m_firstFileSample)
        return;
    m_firstFileSample = first;
    m_tileDirty = true;
    update();
}

//=============================================================================================================

void ChannelRhiView::setLastFileSample(int last)
{
    m_lastFileSample = last;
}

//=============================================================================================================

void ChannelRhiView::setWheelScrollsChannels(bool channelsMode)
{
    m_wheelScrollsChannels = channelsMode;
}

//=============================================================================================================
// QRhiWidget path
//=============================================================================================================

#ifdef MNE_DISP_RHIWIDGET

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
    int totalCh = m_model ? m_model->channelCount() : 0;
    int nCh = qMin(m_visibleChannelCount, totalCh - m_firstVisibleChannel);
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

    int nCh      = m_model->channelCount();
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

    m_gpuChannels.resize(nCh);

    // Compute the max vertex count across channels to right-size allocations
    int prefetchedSamples = iLast - iFirst;
    // With decimation, vertices ≤ 2 * px * prefetchFactor * (1 + prefetchFactor)
    // Use a conservative upper bound
    int maxVertices = qMax(prefetchedSamples * 2, 2 * px * 4);

    for (int ch = 0; ch < nCh; ++ch) {
        int vboFirst = 0;
        QVector<float> verts = m_model->decimatedVertices(
            ch, iFirst, iLast, static_cast<int>(prefetchedSamples / m_samplesPerPixel), vboFirst);

        if (verts.isEmpty()) {
            m_gpuChannels[ch].vertexCount  = 0;
            continue;
        }

        int vertexCount = verts.size() / 2; // each vertex is (x, y) = 2 floats
        quint32 byteSize = static_cast<quint32>(verts.size() * sizeof(float));

        auto &gd = m_gpuChannels[ch];

        // Re-create buffer if size changed significantly
        if (!gd.vbo || static_cast<quint32>(gd.vbo->size()) < byteSize) {
            gd.vbo.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                        QRhiBuffer::VertexBuffer,
                                        byteSize));
            if (!gd.vbo->create()) {
                qWarning() << "ChannelRhiView: VBO create failed for channel" << ch;
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

    int totalCh = m_model->channelCount();
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
        int ch = firstCh + i;
        memset(buf.data(), 0, m_uboStride);

        auto info  = m_model->channelInfo(ch);
        QColor col = info.bad ? QColor(200, 60, 60, 180) : info.color;

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
        writeFloat (d, kUboOffsetFirstSample,    static_cast<float>(static_cast<int>(m_gpuChannels.size()) > ch
                                                                      ? m_gpuChannels[ch].vboFirstSample : 0));
        writeFloat (d, kUboOffsetScrollSample,   m_scrollSample);
        writeFloat (d, kUboOffsetSampPerPixel,   m_samplesPerPixel);
        writeFloat (d, kUboOffsetViewWidth,      vw);
        writeFloat (d, kUboOffsetViewHeight,     vh);
        writeFloat (d, kUboOffsetChannelYCenter, yCenter);
        writeFloat (d, kUboOffsetChannelYRange,  laneRange);
        writeFloat (d, kUboOffsetAmplitudeMax,   info.amplitudeMax);

        // UBO slot i corresponds to visible row i
        batch->updateDynamicBuffer(m_ubo.get(),
                                   i * m_uboStride,
                                   m_uboStride,
                                   buf.constData());
    }
}

//=============================================================================================================

void ChannelRhiView::render(QRhiCommandBuffer *cb)
{
    if (!m_model || m_model->channelCount() == 0) {
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

    QRhiResourceUpdateBatch *batch = rhi()->nextResourceUpdateBatch();

    if (isVboDirty())
        rebuildVBOs(batch);

    updateUBO(batch);

    // ── Render pass ──────────────────────────────────────────────────────
    QColor bg = m_bgColor;
    cb->beginPass(renderTarget(), bg, {1.f, 0}, batch);

    cb->setGraphicsPipeline(m_pipeline.get());

    QSize ps = renderTarget()->pixelSize();
    cb->setViewport(QRhiViewport(0.f, 0.f, static_cast<float>(ps.width()),
                                             static_cast<float>(ps.height())));

    // Render only the visible channel window
    int totalCh = m_model->channelCount();
    int firstCh = qBound(0, m_firstVisibleChannel, totalCh);
    int visCnt  = qMin(m_visibleChannelCount, totalCh - firstCh);
    int nToRender = qMin(visCnt, kMaxChannels);

    for (int i = 0; i < nToRender; ++i) {
        int ch = firstCh + i;
        if (ch >= static_cast<int>(m_gpuChannels.size()))
            break;
        auto &gd = m_gpuChannels[ch];
        if (!gd.vbo || gd.vertexCount < 2)
            continue;

        // UBO slot i corresponds to visible row i (set in updateUBO)
        quint32 dynOffset = static_cast<quint32>(i * m_uboStride);
        QRhiCommandBuffer::DynamicOffset dynOff{0, dynOffset};
        cb->setShaderResources(m_srb.get(), 1, &dynOff);

        QRhiCommandBuffer::VertexInput vi(gd.vbo.get(), 0);
        cb->setVertexInput(0, 1, &vi);
        cb->draw(static_cast<quint32>(gd.vertexCount));
    }

    cb->endPass();
}

#endif // MNE_DISP_RHIWIDGET

//=============================================================================================================
// QPainter fallback paintEvent – never blocks: schedules async tile rebuild if stale.
//=============================================================================================================

static void doPaintTile(QWidget *widget, const QImage &tileImage,
                        float tileSampleFirst, float tileSamplesPerPixel,
                        float scrollSample, const QColor &bgColor)
{
    QPainter p(widget);
    // Always fill background first so uncovered areas are clean
    p.fillRect(widget->rect(), bgColor);
    if (tileImage.isNull() || tileSamplesPerPixel <= 0.f)
        return;
    float tileXPx = (tileSampleFirst - scrollSample) / tileSamplesPerPixel;
    p.drawImage(QPointF(tileXPx, 0.f), tileImage);
}

#ifdef MNE_DISP_RHIWIDGET
void ChannelRhiView::paintEvent(QPaintEvent *event)
{
    if (!m_rhiFailed) {
        QRhiWidget::paintEvent(event);
        return;
    }

    if (!isTileFresh() && !m_tileRebuildPending)
        scheduleTileRebuild();

    doPaintTile(this, m_tileImage, m_tileSampleFirst, m_tileSamplesPerPixel,
                m_scrollSample, m_bgColor);
}

#else // !MNE_DISP_RHIWIDGET

void ChannelRhiView::paintEvent(QPaintEvent *)
{
    if (!isTileFresh() && !m_tileRebuildPending)
        scheduleTileRebuild();

    doPaintTile(this, m_tileImage, m_tileSampleFirst, m_tileSamplesPerPixel,
                m_scrollSample, m_bgColor);
}

#endif // MNE_DISP_RHIWIDGET

//=============================================================================================================
// Tile cache helpers (used by both QPainter paths)
//=============================================================================================================

bool ChannelRhiView::isTileFresh() const
{
    if (m_tileDirty || m_tileImage.isNull() || m_tileSamplesPerPixel <= 0.f)
        return false;
    if (!qFuzzyCompare(m_tileSamplesPerPixel, m_samplesPerPixel))
        return false;
    if (m_tileFirstChannel != m_firstVisibleChannel)
        return false;
    int totalCh      = m_model ? m_model->channelCount() : 0;
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

    if (!m_model || m_model->channelCount() == 0 || width() <= 0 || height() <= 0) {
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

    m_tileDirty          = false; // cleared now — any new event will set it true again
    m_tileRebuildPending = true;
    m_tileWatcher.setFuture(QtConcurrent::run([=]() {
        return ChannelRhiView::buildTile(model, scrollSample, spp, firstCh, visCnt,
                                         pw, ph, bg, gridVis, sfreq, firstFileSample);
    }));
}

//=============================================================================================================

ChannelRhiView::TileResult ChannelRhiView::buildTile(
    ChannelDataModel *model,
    float scrollSample, float spp,
    int firstCh, int visCnt,
    int pw, int ph,
    QColor bgColor, bool gridVisible,
    float sfreq, int firstFileSample)
{
    TileResult out;
    out.samplesPerPixel = spp;
    out.firstChannel    = firstCh;

    if (!model || pw <= 0 || ph <= 0 || spp <= 0.f)
        return out;

    int totalCh      = model->channelCount();
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

    // ── Channel waveform pass ───────────────────────────────────────────
    for (int i = 0; i < visibleCount; ++i) {
        int ch = firstCh + i;
        int vboFirst = 0;
        QVector<float> verts = model->decimatedVertices(
            ch, firstSample, lastSample, tilePixWidth, vboFirst);
        if (verts.size() < 4)
            continue;

        auto info  = model->channelInfo(ch);
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

    out.image = std::move(img);
    return out;
}

//=============================================================================================================
// Shared event handlers (both paths)
//=============================================================================================================

void ChannelRhiView::resizeEvent(QResizeEvent *event)
{
#ifdef MNE_DISP_RHIWIDGET
    QRhiWidget::resizeEvent(event);
    m_vboDirty = true;
#else
    QWidget::resizeEvent(event);
#endif
    m_tileDirty = true;
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
            float step = width() * m_samplesPerPixel * 0.1f
                         * (delta.x() > 0 ? -1.f : 1.f);
            scrollTo(m_scrollSample + step, 100);
        }

    } else if (m_wheelScrollsChannels) {
        // Vertical wheel → scroll channels (up = earlier, down = later)
        int channelStep = (delta.y() > 0) ? -1 : 1;
        int maxFirst = m_model ? qMax(0, m_model->channelCount() - m_visibleChannelCount) : 0;
        setFirstVisibleChannel(qBound(0, m_firstVisibleChannel + channelStep, maxFirst));
    } else {
        // Vertical wheel → scroll time
        if (!m_frozen) {
            float step = width() * m_samplesPerPixel * 0.15f
                         * (delta.y() > 0 ? -1.f : 1.f);
            scrollTo(m_scrollSample + step, 100);
        }
    }

    event->accept();
}

//=============================================================================================================

void ChannelRhiView::mousePressEvent(QMouseEvent *event)
{
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
#ifdef MNE_DISP_RHIWIDGET
    QRhiWidget::mousePressEvent(event);
#else
    QWidget::mousePressEvent(event);
#endif
}

//=============================================================================================================

void ChannelRhiView::mouseMoveEvent(QMouseEvent *event)
{
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
#ifdef MNE_DISP_RHIWIDGET
    QRhiWidget::mouseMoveEvent(event);
#else
    QWidget::mouseMoveEvent(event);
#endif
}

//=============================================================================================================

void ChannelRhiView::mouseReleaseEvent(QMouseEvent *event)
{
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
#ifdef MNE_DISP_RHIWIDGET
    QRhiWidget::mouseReleaseEvent(event);
#else
    QWidget::mouseReleaseEvent(event);
#endif
}
