/**
 * @file main.cpp
 * @brief Minimal single-pass, multi-draw QRhi test for WASM/WebGL.
 *
 * @author Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * Purpose
 * -------
 * Probe whether the Qt QRhi GLES2/WebGL backend still breaks when more than one
 * drawIndexed() is issued inside a SINGLE render pass. Historically (Qt 6.11.0)
 * a second draw call per pass blanked the entire frame on WebGL, forcing
 * mne_inspect to merge every surface into one VBO/IBO and emit a single draw.
 * See /memories/repo/qrhi_webgl_single_draw_constraint.md.
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
 */

#include <QApplication>
#include <QRhiWidget>
#include <rhi/qrhi.h>
#include <QFile>
#include <QTimer>

// ─── Vertex data: two independent triangles ─────────────────────────────────

struct Vertex {
    float pos[3];
    float col[3];
};

// RED triangle — left half
static const Vertex triangleRed[] = {
    {{ -0.9f, -0.6f, 0.0f }, { 1, 0, 0 }},
    {{ -0.1f, -0.6f, 0.0f }, { 1, 0, 0 }},
    {{ -0.5f,  0.6f, 0.0f }, { 1, 0, 0 }},
};

// GREEN triangle — right half
static const Vertex triangleGreen[] = {
    {{  0.1f, -0.6f, 0.0f }, { 0, 1, 0 }},
    {{  0.9f, -0.6f, 0.0f }, { 0, 1, 0 }},
    {{  0.5f,  0.6f, 0.0f }, { 0, 1, 0 }},
};

static const uint32_t indices[] = { 0, 1, 2 };

// ─── Widget ─────────────────────────────────────────────────────────────────

class MultiDrawWidget : public QRhiWidget
{
public:
    explicit MultiDrawWidget(QWidget *parent = nullptr) : QRhiWidget(parent) {}

private:
    void initialize(QRhiCommandBuffer *) override;
    void render(QRhiCommandBuffer *cb) override;
    void evaluate();

    struct TriMesh {
        std::unique_ptr<QRhiBuffer> vbuf;
        std::unique_ptr<QRhiBuffer> ibuf;
        bool uploaded = false;
    };
    TriMesh m_red;
    TriMesh m_green;

    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    QRhiReadbackResult m_rb;
    int  m_frame = 0;
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

void MultiDrawWidget::initialize(QRhiCommandBuffer *)
{
    QRhi *rhi = this->rhi();

    if (!m_red.vbuf) {
        auto makeMesh = [&](TriMesh &mesh) {
            mesh.vbuf.reset(rhi->newBuffer(QRhiBuffer::Immutable,
                                           QRhiBuffer::VertexBuffer,
                                           3 * sizeof(Vertex)));
            mesh.vbuf->create();
            mesh.ibuf.reset(rhi->newBuffer(QRhiBuffer::Immutable,
                                           QRhiBuffer::IndexBuffer,
                                           3 * sizeof(uint32_t)));
            mesh.ibuf->create();
        };
        makeMesh(m_red);
        makeMesh(m_green);

        m_ubuf.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                    QRhiBuffer::UniformBuffer, 64));
        m_ubuf->create();

        m_srb.reset(rhi->newShaderResourceBindings());
        m_srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(
                0, QRhiShaderResourceBinding::VertexStage, m_ubuf.get())
        });
        m_srb->create();
    }

    if (!m_pipeline) {
        m_pipeline.reset(rhi->newGraphicsPipeline());
        m_pipeline->setShaderStages({
            { QRhiShaderStage::Vertex,   loadShader(QShader::VertexStage) },
            { QRhiShaderStage::Fragment, loadShader(QShader::FragmentStage) },
        });

        QRhiVertexInputLayout layout;
        layout.setBindings({ { sizeof(Vertex) } });
        layout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float3, offsetof(Vertex, pos) },
            { 0, 1, QRhiVertexInputAttribute::Float3, offsetof(Vertex, col) },
        });
        m_pipeline->setVertexInputLayout(layout);
        m_pipeline->setShaderResourceBindings(m_srb.get());
        m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
        m_pipeline->setDepthTest(true);
        m_pipeline->setDepthWrite(true);
        m_pipeline->setCullMode(QRhiGraphicsPipeline::None);
        m_pipeline->create();
    }
}

// ─── Render ─────────────────────────────────────────────────────────────────

void MultiDrawWidget::render(QRhiCommandBuffer *cb)
{
    QRhi *rhi = this->rhi();
    QRhiRenderTarget *rt = renderTarget();
    const QSize sz = rt->pixelSize();
    ++m_frame;

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();

    auto uploadMesh = [&](TriMesh &mesh, const Vertex *data) {
        if (!mesh.uploaded) {
            u->uploadStaticBuffer(mesh.vbuf.get(), data);
            u->uploadStaticBuffer(mesh.ibuf.get(), indices);
            mesh.uploaded = true;
        }
    };
    uploadMesh(m_red, triangleRed);
    uploadMesh(m_green, triangleGreen);

    QMatrix4x4 mvp;  // identity → NDC coordinates straight through
    u->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());

    auto drawTriangle = [&](TriMesh &mesh) {
        const QRhiCommandBuffer::VertexInput vbuf(mesh.vbuf.get(), 0);
        cb->setVertexInput(0, 1, &vbuf, mesh.ibuf.get(), 0,
                           QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(3);
    };

    // ── ONE render pass, TWO drawIndexed() calls ────────────────────────
    cb->beginPass(rt, QColor(30, 30, 30), { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setViewport(QRhiViewport(0, 0, sz.width(), sz.height()));
    cb->setShaderResources(m_srb.get());

    drawTriangle(m_red);     // draw 1
    drawTriangle(m_green);   // draw 2 — the call that blanked WebGL in 6.11.0

    cb->endPass();

    // Schedule a one-shot read-back of the rendered color texture so the test
    // can decide PASS/FAIL without a human squinting at the canvas.
    if (!m_readbackScheduled && m_frame >= 2) {
        if (QRhiTexture *tex = colorTexture()) {
            QRhiResourceUpdateBatch *rbBatch = rhi->nextResourceUpdateBatch();
            m_rb.completed = [this]() {
                QTimer::singleShot(0, this, [this]() { evaluate(); });
            };
            rbBatch->readBackTexture(QRhiReadbackDescription(tex), &m_rb);
            cb->resourceUpdate(rbBatch);
            m_readbackScheduled = true;
        }
    }

    if (!m_evaluated)
        update();  // keep the loop alive until the read-back resolves
}

// ─── Verdict ────────────────────────────────────────────────────────────────

void MultiDrawWidget::evaluate()
{
    if (m_evaluated)
        return;
    m_evaluated = true;

    const int w = m_rb.pixelSize.width();
    const int h = m_rb.pixelSize.height();
    const QByteArray &px = m_rb.data;

    if (w <= 0 || h <= 0 || px.size() < w * h * 4) {
        qWarning("[multi-draw] read-back unavailable (w=%d h=%d bytes=%lld)",
                 w, h, static_cast<long long>(px.size()));
        return;
    }

    const auto *p = reinterpret_cast<const uchar *>(px.constData());
    bool leftRed = false;
    bool rightGreen = false;

    // Scan a coarse grid; tolerate RGBA/BGRA by checking dominant channels.
    for (int y = 0; y < h; y += 4) {
        for (int x = 0; x < w; x += 4) {
            const uchar *c = p + (static_cast<qsizetype>(y) * w + x) * 4;
            const int c0 = c[0], c1 = c[1], c2 = c[2];
            const bool isRed   = (c0 > 170 && c1 < 90 && c2 < 90) ||
                                 (c2 > 170 && c1 < 90 && c0 < 90);
            const bool isGreen = (c1 > 170 && c0 < 90 && c2 < 90);
            if (isRed   && x <  w / 2) leftRed = true;
            if (isGreen && x >= w / 2) rightGreen = true;
        }
    }

    QString verdict;
    if (leftRed && rightGreen)
        verdict = QStringLiteral("PASS — two drawIndexed() in one pass BOTH rendered (WebGL multi-draw bug is FIXED)");
    else if (!leftRed && !rightGreen)
        verdict = QStringLiteral("FAIL — frame blank; multi-draw still blanks the pass (bug PRESENT)");
    else if (leftRed && !rightGreen)
        verdict = QStringLiteral("PARTIAL — only draw 1 (red) survived; draw 2 dropped (bug PRESENT)");
    else
        verdict = QStringLiteral("PARTIAL — only draw 2 (green) survived; draw 1 dropped (bug PRESENT)");

    qInfo("[multi-draw] backend=%s  size=%dx%d  leftRed=%d  rightGreen=%d",
          rhi() ? rhi()->backendName() : "?", w, h, int(leftRed), int(rightGreen));
    qInfo("[multi-draw] %s", qPrintable(verdict));
    setWindowTitle(QStringLiteral("Multi-Draw QRhi Test — ") + verdict);
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MultiDrawWidget w;
    w.setWindowTitle(QStringLiteral(
        "Multi-Draw QRhi Test — expect RED (left) + GREEN (right)"));
    w.resize(800, 600);
    w.show();

    return app.exec();
}
