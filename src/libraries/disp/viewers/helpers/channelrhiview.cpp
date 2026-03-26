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
#  include <QFile>
#  include <QShader>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPropertyAnimation>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QPainterPath>
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
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);

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
            update();
        });
        connect(m_model, &ChannelDataModel::metaChanged, this, [this] {
#ifdef MNE_DISP_RHIWIDGET
            m_vboDirty    = true;
            m_pipelineDirty = true; // channel count may have changed
#endif
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
    if (qFuzzyCompare(m_scrollSample, sample))
        return;

    float oldScroll = m_scrollSample;
    m_scrollSample = sample;

#ifdef MNE_DISP_RHIWIDGET
    // Check whether the prefetch window is still valid
    float visible = width() * m_samplesPerPixel;
    float margin  = m_prefetchFactor * visible;
    if (sample < m_vboWindowFirst + margin ||
        sample + visible > m_vboWindowLast - margin) {
        m_vboDirty = true;
    }
#else
    Q_UNUSED(oldScroll)
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

    int nCh = m_model ? m_model->channelCount() : 0;
    nCh = qMax(nCh, 1);
    nCh = qMin(nCh, kMaxChannels);

    // ── Uniform buffer ──────────────────────────────────────────────────
    if (!m_ubo || m_ubo->size() < nCh * m_uboStride) {
        m_ubo.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                   QRhiBuffer::UniformBuffer,
                                   nCh * m_uboStride));
        m_ubo->create();
    }

    // ── Shader resource bindings ────────────────────────────────────────
    if (!m_srb) {
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
    QShader vs = loadShader(QStringLiteral(":/disp/shaders/channeldata.vert.qsb"));
    QShader fs = loadShader(QStringLiteral(":/disp/shaders/channeldata.frag.qsb"));

    if (!vs.isValid() || !fs.isValid()) {
        qWarning() << "ChannelRhiView: shaders not found. "
                      "Ensure qt_add_shaders is configured in CMakeLists.";
        return;
    }

    // ── Graphics pipeline ───────────────────────────────────────────────
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
    m_pipeline->setRenderPassDescriptor(renderPassDescriptor());
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

    int nCh  = m_model->channelCount();
    nCh      = qMin(nCh, kMaxChannels);
    float vw = static_cast<float>(width());
    float vh = static_cast<float>(height());
    float laneRange = (nCh > 0) ? (2.f / nCh) : 2.f; // NDC height of one channel row

    QVarLengthArray<quint8> buf(m_uboStride, 0);

    for (int ch = 0; ch < nCh; ++ch) {
        memset(buf.data(), 0, m_uboStride);

        auto info  = m_model->channelInfo(ch);
        QColor col = info.bad ? QColor(200, 60, 60, 180) : info.color;

        float rgba[4] = {
            static_cast<float>(col.redF()),
            static_cast<float>(col.greenF()),
            static_cast<float>(col.blueF()),
            static_cast<float>(col.alphaF())
        };

        // Channel y-center in NDC: top channel at +1 - laneRange/2, bottom at -1 + laneRange/2
        // NDC y = +1 is the top of the screen
        float yCenter = 1.f - laneRange * (ch + 0.5f);

        auto *d = buf.data();
        writeFloats(d, kUboOffsetColor,          rgba, 4);
        writeFloat (d, kUboOffsetFirstSample,    static_cast<float>(m_gpuChannels.size() > ch
                                                                      ? m_gpuChannels[ch].vboFirstSample : 0));
        writeFloat (d, kUboOffsetScrollSample,   m_scrollSample);
        writeFloat (d, kUboOffsetSampPerPixel,   m_samplesPerPixel);
        writeFloat (d, kUboOffsetViewWidth,      vw);
        writeFloat (d, kUboOffsetViewHeight,     vh);
        writeFloat (d, kUboOffsetChannelYCenter, yCenter);
        writeFloat (d, kUboOffsetChannelYRange,  laneRange);
        writeFloat (d, kUboOffsetAmplitudeMax,   info.amplitudeMax);

        batch->updateDynamicBuffer(m_ubo.get(),
                                   ch * m_uboStride,
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
        QRhiColorClearValue clear(bg.redF(), bg.greenF(), bg.blueF(), 1.f);
        cb->beginPass(renderTarget(), clear, {1.f, 0}, u);
        cb->endPass();
        return;
    }

    // ── Ensure GPU resources ─────────────────────────────────────────────
    ensurePipeline();
    if (!m_pipeline)
        return;

    QRhiResourceUpdateBatch *batch = rhi()->nextResourceUpdateBatch();

    if (isVboDirty())
        rebuildVBOs(batch);

    updateUBO(batch);

    // ── Render pass ──────────────────────────────────────────────────────
    QColor bg = m_bgColor;
    QRhiColorClearValue clear(bg.redF(), bg.greenF(), bg.blueF(), 1.f);

    cb->beginPass(renderTarget(), clear, {1.f, 0}, batch);

    cb->setGraphicsPipeline(m_pipeline.get());

    QSize ps = renderTarget()->pixelSize();
    cb->setViewport(QRhiViewport(0.f, 0.f, static_cast<float>(ps.width()),
                                             static_cast<float>(ps.height())));

    int nCh = qMin(m_model->channelCount(), kMaxChannels);

    for (int ch = 0; ch < nCh; ++ch) {
        if (ch >= m_gpuChannels.size())
            break;
        auto &gd = m_gpuChannels[ch];
        if (!gd.vbo || gd.vertexCount < 2)
            continue;

        quint32 dynOffset = static_cast<quint32>(ch * m_uboStride);
        QRhiCommandBuffer::DynamicOffset dynOff{0, dynOffset};
        cb->setShaderResources(m_srb.get(), 1, &dynOff);

        QRhiCommandBuffer::VertexInput vi(gd.vbo.get(), 0);
        cb->setVertexInput(0, 1, &vi);
        cb->draw(static_cast<quint32>(gd.vertexCount));
    }

    cb->endPass();
}

#else // !MNE_DISP_RHIWIDGET
//=============================================================================================================
// QPainter fallback
//=============================================================================================================

void ChannelRhiView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false); // speed
    p.fillRect(rect(), m_bgColor);

    if (!m_model || m_model->channelCount() == 0)
        return;

    int nCh = m_model->channelCount();
    float laneH = static_cast<float>(height()) / nCh;
    int px = width();
    int firstSample = static_cast<int>(m_scrollSample);
    int lastSample  = firstSample + static_cast<int>(px * m_samplesPerPixel) + 1;

    for (int ch = 0; ch < nCh; ++ch) {
        int vboFirst = 0;
        QVector<float> verts = m_model->decimatedVertices(
            ch, firstSample, lastSample, px, vboFirst);
        if (verts.size() < 4)
            continue;

        auto info  = m_model->channelInfo(ch);
        QColor col = info.bad ? QColor(200, 60, 60) : info.color;
        p.setPen(col);

        float yMid = (ch + 0.5f) * laneH;
        float yScale = (laneH * 0.45f) / (info.amplitudeMax > 0.f ? info.amplitudeMax : 1.f);

        QPainterPath path;
        bool first = true;
        int nVerts = verts.size() / 2;
        for (int v = 0; v < nVerts; ++v) {
            float xOff = verts[v * 2 + 0];
            float yAmp = verts[v * 2 + 1];

            float samplePos = vboFirst + xOff;
            float xPx = (samplePos - m_scrollSample) / m_samplesPerPixel;
            float yPx = yMid - yAmp * yScale;

            if (first) { path.moveTo(xPx, yPx); first = false; }
            else        { path.lineTo(xPx, yPx); }
        }
        p.drawPath(path);
    }
}

#endif // MNE_DISP_RHIWIDGET

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
    update();
#endif
}

//=============================================================================================================

void ChannelRhiView::wheelEvent(QWheelEvent *event)
{
    // Horizontal scroll: one wheel step = 10 % of visible window
    if (event->modifiers() & Qt::ControlModifier) {
        // Ctrl + wheel = zoom
        float factor = (event->angleDelta().y() > 0) ? 0.8f : 1.25f;
        zoomTo(m_samplesPerPixel * factor, 150);
    } else {
        float step = width() * m_samplesPerPixel * 0.1f;
        if (event->angleDelta().y() > 0)
            scrollTo(m_scrollSample - step, 150);
        else
            scrollTo(m_scrollSample + step, 150);
    }
    event->accept();
}

//=============================================================================================================

void ChannelRhiView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && (event->modifiers() & Qt::AltModifier))) {
        m_dragging        = true;
        m_dragStartX      = event->position().toPoint().x();
        m_dragStartScroll = m_scrollSample;
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        float samplePos = m_scrollSample
            + static_cast<float>(event->position().x()) * m_samplesPerPixel;
        emit sampleClicked(static_cast<int>(samplePos));
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
#ifdef MNE_DISP_RHIWIDGET
    QRhiWidget::mouseReleaseEvent(event);
#else
    QWidget::mouseReleaseEvent(event);
#endif
}
