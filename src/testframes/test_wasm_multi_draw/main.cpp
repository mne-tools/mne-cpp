/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2010-2026 MNE-CPP Authors
 *

 * @file main.cpp
 * @brief Minimal single-pass, multi-draw QRhi test for WASM/WebGL.
 *
 * @author Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * Purpose
 * -------
 * Probe which multi-draw patterns render on the QRhi GLES2/WebGL backend. A
 * second draw call per pass used to blank the whole frame in disp3D on WebGL,
 * which led to merging every surface into one VBO/IBO with a single draw. The
 * cause was disp3D's uniform slot stride, reproduced by the shared-ubuf modes.
 *
 * What this renders
 * -----------------
 * In ONE beginPass()/endPass() cycle:
 *   - draw 1: RED  triangle (left half)   — its own VBO + IBO
 *   - draw 2: GREEN triangle (right half) — its own VBO + IBO
 *
 * Both triangles share the same pipeline; they are two independent drawIndexed()
 * calls in the same pass (the exact pattern that failed on WebGL in 6.11.0).
 *
 * Self-verification
 * -----------------
 * After the pass the color texture is read back and inspected:
 *   - RED on the left AND GREEN on the right  -> PASS  (multi-draw works)
 *   - neither colour present (only clear)     -> FAIL  (classic blank-frame bug)
 *   - exactly one colour present              -> PARTIAL
 * The verdict is written to qDebug() (browser console / emrun stdout) and the
 * window title.
 *
 * Four cases run in sequence: first with all resource updates committed via
 * beginPass(), then the disp3D pattern of a cb->resourceUpdate() per draw
 * INSIDE the pass, which QRhi documents as "Cannot be called inside a pass".
 * The last two share one dynamic-offset uniform buffer like disp3D: slots spaced
 * by rhi->ubufAlignment() overlap on GL, where that alignment is 1 byte, and
 * slots spaced by rhi->ubufAligned(block size) do not.
 */

#include <QApplication>
#include <QRhiWidget>
#include <rhi/qrhi.h>
#include <QFile>
#include <QTimer>

// ─── Vertex data: two independent triangles ─────────────────────────────────

struct Vertex
{
    float pos[3];
    float col[3];
};

// RED triangle — left half
static const Vertex triangleRed[] = {
    {{-0.9f, -0.6f, 0.0f}, {1, 0, 0}},
    {{-0.1f, -0.6f, 0.0f}, {1, 0, 0}},
    {{-0.5f, 0.6f, 0.0f}, {1, 0, 0}},
};

// GREEN triangle — right half
static const Vertex triangleGreen[] = {
    {{0.1f, -0.6f, 0.0f}, {0, 1, 0}},
    {{0.9f, -0.6f, 0.0f}, {0, 1, 0}},
    {{0.5f, 0.6f, 0.0f}, {0, 1, 0}},
};

static const uint32_t indices[] = {0, 1, 2};

// ─── Widget ─────────────────────────────────────────────────────────────────

class MultiDrawWidget : public QRhiWidget
{
public:
    explicit MultiDrawWidget(QWidget* parent = nullptr)
    : QRhiWidget(parent)
    {
    }

private:
    void initialize(QRhiCommandBuffer*) override;
    void render(QRhiCommandBuffer* cb) override;
    void evaluate();

    struct TriMesh
    {
        std::unique_ptr<QRhiBuffer> vbuf;
        std::unique_ptr<QRhiBuffer> ibuf;
        std::unique_ptr<QRhiBuffer> ubuf;
        std::unique_ptr<QRhiShaderResourceBindings> srb;
        bool uploaded = false;
    };
    TriMesh m_red;
    TriMesh m_green;

    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    // One uniform buffer for both draws, addressed with dynamic offsets
    std::unique_ptr<QRhiBuffer> m_sharedUbuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_sharedSrb;
    std::unique_ptr<QRhiGraphicsPipeline> m_sharedPipeline;

    enum Mode
    {
        UpdatesBeforePass,
        UpdatesInsidePass,
        SharedUbufAlignmentStride,
        SharedUbufBlockStride,
        Done
    };
    Mode m_mode = UpdatesBeforePass;
    QRhiReadbackResult m_rb;
    int m_frame = 0;
    bool m_readbackScheduled = false;
    bool m_evaluated = false;
};

// ─── Shader loading ─────────────────────────────────────────────────────────

static QShader loadShader(QShader::Stage stage)
{
    const QString resPath = (stage == QShader::VertexStage)
        ? QStringLiteral(":/multidraw.vert.qsb")
        : QStringLiteral(":/multidraw.frag.qsb");

    QFile f(resPath);
    if (f.open(QIODevice::ReadOnly)) {
        QShader shader = QShader::fromSerialized(f.readAll());
        if (shader.isValid())
            return shader;
    }
    qFatal("Shader not found: %s", qPrintable(resPath));
    return {};
}

// ─── Initialization ─────────────────────────────────────────────────────────

void MultiDrawWidget::initialize(QRhiCommandBuffer*)
{
    QRhi* rhi = this->rhi();

    if (!m_red.vbuf) {
        auto makeMesh = [&](TriMesh& mesh) {
            mesh.vbuf.reset(rhi->newBuffer(QRhiBuffer::Immutable,
                                           QRhiBuffer::VertexBuffer,
                                           3 * sizeof(Vertex)));
            mesh.vbuf->create();
            mesh.ibuf.reset(rhi->newBuffer(QRhiBuffer::Immutable,
                                           QRhiBuffer::IndexBuffer,
                                           3 * sizeof(uint32_t)));
            mesh.ibuf->create();
            mesh.ubuf.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                           QRhiBuffer::UniformBuffer, 64));
            mesh.ubuf->create();
            mesh.srb.reset(rhi->newShaderResourceBindings());
            mesh.srb->setBindings({QRhiShaderResourceBinding::uniformBuffer(
                0, QRhiShaderResourceBinding::VertexStage, mesh.ubuf.get())});
            mesh.srb->create();
        };
        makeMesh(m_red);
        makeMesh(m_green);

        m_sharedUbuf.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 2 * rhi->ubufAligned(256)));
        m_sharedUbuf->create();
        m_sharedSrb.reset(rhi->newShaderResourceBindings());
        m_sharedSrb->setBindings({QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
            0, QRhiShaderResourceBinding::VertexStage, m_sharedUbuf.get(), 64)});
        m_sharedSrb->create();
    }

    auto makePipeline = [&](std::unique_ptr<QRhiGraphicsPipeline>& pipeline, QRhiShaderResourceBindings* srb) {
        pipeline.reset(rhi->newGraphicsPipeline());
        pipeline->setShaderStages({
            {QRhiShaderStage::Vertex, loadShader(QShader::VertexStage)},
            {QRhiShaderStage::Fragment, loadShader(QShader::FragmentStage)},
        });

        QRhiVertexInputLayout layout;
        layout.setBindings({{sizeof(Vertex)}});
        layout.setAttributes({
            {0, 0, QRhiVertexInputAttribute::Float3, offsetof(Vertex, pos)},
            {0, 1, QRhiVertexInputAttribute::Float3, offsetof(Vertex, col)},
        });
        pipeline->setVertexInputLayout(layout);
        pipeline->setShaderResourceBindings(srb);
        pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
        pipeline->setDepthTest(true);
        pipeline->setDepthWrite(true);
        pipeline->setCullMode(QRhiGraphicsPipeline::None);
        pipeline->create();
    };
    if (!m_pipeline) {
        makePipeline(m_pipeline, m_red.srb.get());
        makePipeline(m_sharedPipeline, m_sharedSrb.get());
    }
}

// ─── Render ─────────────────────────────────────────────────────────────────

void MultiDrawWidget::render(QRhiCommandBuffer* cb)
{
    QRhi* rhi = this->rhi();
    QRhiRenderTarget* rt = renderTarget();
    const QSize sz = rt->pixelSize();
    ++m_frame;

    const QMatrix4x4 mvp; // identity → NDC coordinates straight through
    auto queueUpdates = [&](QRhiResourceUpdateBatch* u, TriMesh& mesh, const Vertex* data) {
        if (!mesh.uploaded) {
            u->uploadStaticBuffer(mesh.vbuf.get(), data);
            u->uploadStaticBuffer(mesh.ibuf.get(), indices);
            mesh.uploaded = true;
        }
        u->updateDynamicBuffer(mesh.ubuf.get(), 0, 64, mvp.constData());
    };

    const bool shared = m_mode == SharedUbufAlignmentStride || m_mode == SharedUbufBlockStride;
    const quint32 slotStride = m_mode == SharedUbufAlignmentStride ? quint32(rhi->ubufAlignment()) : quint32(rhi->ubufAligned(256));

    auto drawTriangle = [&](TriMesh& mesh, quint32 slot) {
        if (shared) {
            const QRhiCommandBuffer::DynamicOffset offset = {0, slot * slotStride};
            cb->setShaderResources(m_sharedSrb.get(), 1, &offset);
        } else {
            cb->setShaderResources(mesh.srb.get());
        }
        const QRhiCommandBuffer::VertexInput vbuf(mesh.vbuf.get(), 0);
        cb->setVertexInput(0, 1, &vbuf, mesh.ibuf.get(), 0,
                           QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(3);
    };

    // ── ONE render pass, TWO drawIndexed() calls ────────────────────────
    QRhiResourceUpdateBatch* pre = rhi->nextResourceUpdateBatch();
    if (m_mode != UpdatesInsidePass) {
        queueUpdates(pre, m_red, triangleRed);
        queueUpdates(pre, m_green, triangleGreen);
    }
    if (shared) {
        pre->updateDynamicBuffer(m_sharedUbuf.get(), 0, 64, mvp.constData());
        pre->updateDynamicBuffer(m_sharedUbuf.get(), slotStride, 64, mvp.constData());
    }
    cb->beginPass(rt, QColor(30, 30, 30), {1.0f, 0}, pre);
    cb->setGraphicsPipeline(shared ? m_sharedPipeline.get() : m_pipeline.get());
    cb->setViewport(QRhiViewport(0, 0, sz.width(), sz.height()));

    quint32 slot = 0;
    for (auto [mesh, data] : {std::pair{&m_red, triangleRed}, std::pair{&m_green, triangleGreen}}) {
        if (m_mode == UpdatesInsidePass) {
            QRhiResourceUpdateBatch* u = rhi->nextResourceUpdateBatch();
            queueUpdates(u, *mesh, data);
            cb->resourceUpdate(u);
        }
        drawTriangle(*mesh, slot++);
    }

    cb->endPass();

    // Schedule a one-shot read-back of the rendered color texture so the test
    // can decide PASS/FAIL without a human squinting at the canvas.
    if (!m_readbackScheduled && m_frame >= 2) {
        if (QRhiTexture* tex = colorTexture()) {
            QRhiResourceUpdateBatch* rbBatch = rhi->nextResourceUpdateBatch();
            m_rb.completed = [this]() {
                QTimer::singleShot(0, this, [this]() { evaluate(); });
            };
            rbBatch->readBackTexture(QRhiReadbackDescription(tex), &m_rb);
            cb->resourceUpdate(rbBatch);
            m_readbackScheduled = true;
        }
    }

    if (m_mode != Done)
        update(); // keep the loop alive until both read-backs resolve
}

// ─── Verdict ────────────────────────────────────────────────────────────────

void MultiDrawWidget::evaluate()
{
    if (m_evaluated)
        return;
    m_evaluated = true;

    const int w = m_rb.pixelSize.width();
    const int h = m_rb.pixelSize.height();
    const QByteArray& px = m_rb.data;

    if (w <= 0 || h <= 0 || px.size() < w * h * 4) {
        qWarning("[multi-draw] read-back unavailable (w=%d h=%d bytes=%lld)",
                 w, h, static_cast<long long>(px.size()));
        return;
    }

    const auto* p = reinterpret_cast<const uchar*>(px.constData());
    bool leftRed = false;
    bool rightGreen = false;

    // Scan a coarse grid; tolerate RGBA/BGRA by checking dominant channels.
    for (int y = 0; y < h; y += 4) {
        for (int x = 0; x < w; x += 4) {
            const uchar* c = p + (static_cast<qsizetype>(y) * w + x) * 4;
            const int c0 = c[0], c1 = c[1], c2 = c[2];
            const bool isRed = (c0 > 170 && c1 < 90 && c2 < 90) ||
                (c2 > 170 && c1 < 90 && c0 < 90);
            const bool isGreen = (c1 > 170 && c0 < 90 && c2 < 90);
            if (isRed && x < w / 2)
                leftRed = true;
            if (isGreen && x >= w / 2)
                rightGreen = true;
        }
    }

    QString verdict;
    if (leftRed && rightGreen)
        verdict = QStringLiteral("PASS — both drawIndexed() calls in one pass rendered");
    else if (!leftRed && !rightGreen)
        verdict = QStringLiteral("FAIL — frame blank, neither draw rendered");
    else if (leftRed && !rightGreen)
        verdict = QStringLiteral("PARTIAL — only draw 1 (red) rendered");
    else
        verdict = QStringLiteral("PARTIAL — only draw 2 (green) rendered");

    static const char* const modeNames[] = {"updates-before-pass", "updates-inside-pass",
                                            "shared-ubuf-alignment-stride", "shared-ubuf-block-stride"};
    const char* mode = modeNames[m_mode];
    qInfo("[multi-draw] mode=%s backend=%s ubufAlignment=%d  size=%dx%d  leftRed=%d  rightGreen=%d",
          mode, rhi() ? rhi()->backendName() : "?", rhi() ? rhi()->ubufAlignment() : 0, w, h, int(leftRed), int(rightGreen));
    qInfo("[multi-draw] mode=%s %s", mode, qPrintable(verdict));
    setWindowTitle(QStringLiteral("Multi-Draw QRhi Test — ") + verdict);

    // Re-arm for the next mode; buffers are re-uploaded inside the pass there.
    m_mode = static_cast<Mode>(m_mode + 1);
    if (m_mode != Done) {
        m_red.uploaded = m_green.uploaded = false;
        m_frame = 0;
        m_readbackScheduled = false;
        m_evaluated = false;
        m_rb = {};
        update();
    }
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MultiDrawWidget w;
    w.setWindowTitle(QStringLiteral(
        "Multi-Draw QRhi Test — expect RED (left) + GREEN (right)"));
    w.resize(800, 600);
    w.show();

    return app.exec();
}
