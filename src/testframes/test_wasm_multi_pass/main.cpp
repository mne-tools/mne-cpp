/**
 * @file main.cpp
 * @brief Minimal multi-pass QRhi rendering test for WASM/WebGL.
 *
 * Validates that multiple beginPass()/endPass() cycles with
 * PreserveColorContents each support one drawIndexed() on the
 * QRhi GLES2/WebGL backend — the prerequisite for per-category
 * rendering in mne_inspect on WASM.
 *
 * Uses two render targets sharing the same color texture:
 *   - m_rtClear:    no PreserveColorContents (clears, for pass 1)
 *   - m_rtPreserve: with PreserveColorContents (preserves, for passes 2+)
 *
 * This is required because QRhi bakes load/store behavior into native
 * resources at create() time — calling setFlags() alone does not work.
 *
 * Pass 1: Red triangle (left half, clears framebuffer)
 * Pass 2: Green triangle (right half, preserves pass 1)
 * Pass 3: Blue triangle (top center, preserves passes 1+2)
 *
 * SUCCESS: all three triangles visible simultaneously.
 * FAILURE: only last triangle visible (PreserveColorContents broken).
 */

#include <QApplication>
#include <QRhiWidget>
#include <rhi/qrhi.h>
#include <QFile>

// ─── Vertex data: 3 triangles, each with position + color ───────────────────

struct Vertex {
    float pos[3];
    float col[3];
};

// Pass 1: Red triangle — left half
static const Vertex triangleA[] = {
    {{ -0.9f, -0.5f, 0.0f }, { 1, 0, 0 }},
    {{ -0.1f, -0.5f, 0.0f }, { 1, 0, 0 }},
    {{ -0.5f,  0.5f, 0.0f }, { 1, 0, 0 }},
};

// Pass 2: Green triangle — right half
static const Vertex triangleB[] = {
    {{  0.1f, -0.5f, 0.0f }, { 0, 1, 0 }},
    {{  0.9f, -0.5f, 0.0f }, { 0, 1, 0 }},
    {{  0.5f,  0.5f, 0.0f }, { 0, 1, 0 }},
};

// Pass 3: Blue triangle — top center
static const Vertex triangleC[] = {
    {{ -0.3f,  0.0f, 0.0f }, { 0, 0, 1 }},
    {{  0.3f,  0.0f, 0.0f }, { 0, 0, 1 }},
    {{  0.0f,  0.8f, 0.0f }, { 0, 0, 1 }},
};

static const uint32_t indices[] = { 0, 1, 2 };

// ─── Widget ─────────────────────────────────────────────────────────────────

class MultiPassWidget : public QRhiWidget
{
public:
    MultiPassWidget()
    {
        setAutoRenderTarget(false);  // We manage our own render targets
    }

private:
    void initialize(QRhiCommandBuffer *) override;
    void render(QRhiCommandBuffer *cb) override;

    // One VBO + IBO per triangle (deliberate — each drawn in its own pass)
    struct TriMesh {
        std::unique_ptr<QRhiBuffer> vbuf;
        std::unique_ptr<QRhiBuffer> ibuf;
        bool uploaded = false;
    };
    TriMesh m_tri[3];

    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;

    // Two render targets sharing the same color texture + depth buffer:
    std::unique_ptr<QRhiRenderBuffer> m_ds;          // depth-stencil
    std::unique_ptr<QRhiTextureRenderTarget> m_rtClear;    // pass 1: clears
    std::unique_ptr<QRhiTextureRenderTarget> m_rtPreserve; // passes 2+: preserves
    std::unique_ptr<QRhiRenderPassDescriptor> m_rpClear;
    std::unique_ptr<QRhiRenderPassDescriptor> m_rpPreserve;

    // Pipeline — compatible with both render pass descriptors
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

    QSize m_lastSize;
};

// ─── Shader loading ─────────────────────────────────────────────────────────

static QShader loadShader(QShader::Stage stage)
{
    const QString resPath = (stage == QShader::VertexStage)
        ? QStringLiteral(":/multipass.vert.qsb")
        : QStringLiteral(":/multipass.frag.qsb");

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

void MultiPassWidget::initialize(QRhiCommandBuffer *)
{
    QRhi *rhi = this->rhi();
    QRhiTexture *tex = colorTexture();
    const QSize sz = tex->pixelSize();

    // Rebuild render targets when size changes or on first call
    if (m_lastSize != sz || !m_rtClear) {
        m_lastSize = sz;

        // Shared depth-stencil buffer
        m_ds.reset(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, sz));
        m_ds->create();

        QRhiColorAttachment colorAtt(tex);
        QRhiTextureRenderTargetDescription desc(colorAtt);
        desc.setDepthStencilBuffer(m_ds.get());

        // RT 1: Clearing (no preserve flags)
        m_rtClear.reset(rhi->newTextureRenderTarget(desc));
        m_rpClear.reset(m_rtClear->newCompatibleRenderPassDescriptor());
        m_rtClear->setRenderPassDescriptor(m_rpClear.get());
        m_rtClear->create();

        // RT 2: Preserving (load previous contents)
        m_rtPreserve.reset(rhi->newTextureRenderTarget(desc,
            QRhiTextureRenderTarget::PreserveColorContents
            | QRhiTextureRenderTarget::PreserveDepthStencilContents));
        m_rpPreserve.reset(m_rtPreserve->newCompatibleRenderPassDescriptor());
        m_rtPreserve->setRenderPassDescriptor(m_rpPreserve.get());
        m_rtPreserve->create();

        // (Re)create pipeline — must be compatible with both RPs
        // Both share the same attachment layout, so either RP works.
        m_pipeline.reset();
    }

    // Create mesh buffers once
    if (!m_tri[0].vbuf) {
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
        makeMesh(m_tri[0]);
        makeMesh(m_tri[1]);
        makeMesh(m_tri[2]);

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
        m_pipeline->setRenderPassDescriptor(m_rpClear.get());
        m_pipeline->setDepthTest(true);
        m_pipeline->setDepthWrite(true);
        m_pipeline->setCullMode(QRhiGraphicsPipeline::None);
        m_pipeline->create();
    }
}

// ─── Render ─────────────────────────────────────────────────────────────────

void MultiPassWidget::render(QRhiCommandBuffer *cb)
{
    QRhi *rhi = this->rhi();
    const QSize sz = m_lastSize;

    // Upload vertex/index data on first frame
    {
        QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
        auto uploadMesh = [&](TriMesh &mesh, const Vertex *data) {
            if (!mesh.uploaded) {
                u->uploadStaticBuffer(mesh.vbuf.get(), data);
                u->uploadStaticBuffer(mesh.ibuf.get(), indices);
                mesh.uploaded = true;
            }
        };
        uploadMesh(m_tri[0], triangleA);
        uploadMesh(m_tri[1], triangleB);
        uploadMesh(m_tri[2], triangleC);

        QMatrix4x4 mvp;
        u->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());
        cb->resourceUpdate(u);
    }

    auto drawTriangle = [&](TriMesh &mesh) {
        cb->setGraphicsPipeline(m_pipeline.get());
        cb->setShaderResources(m_srb.get());
        const QRhiCommandBuffer::VertexInput vbuf(mesh.vbuf.get(), 0);
        cb->setVertexInput(0, 1, &vbuf, mesh.ibuf.get(), 0,
                           QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(3);
    };

    // ── Pass 1: Red triangle (clear framebuffer) ────────────────────────
    cb->beginPass(m_rtClear.get(), QColor(30, 30, 30), { 1.0f, 0 });
    cb->setViewport(QRhiViewport(0, 0, sz.width(), sz.height()));
    cb->setScissor(QRhiScissor(0, 0, sz.width(), sz.height()));
    drawTriangle(m_tri[0]);
    cb->endPass();

    // ── Pass 2: Green triangle (preserve pass 1) ────────────────────────
    cb->beginPass(m_rtPreserve.get(), QColor(0, 0, 0), { 1.0f, 0 });
    cb->setViewport(QRhiViewport(0, 0, sz.width(), sz.height()));
    cb->setScissor(QRhiScissor(0, 0, sz.width(), sz.height()));
    drawTriangle(m_tri[1]);
    cb->endPass();

    // ── Pass 3: Blue triangle (preserve passes 1+2) ────────────────────
    cb->beginPass(m_rtPreserve.get(), QColor(0, 0, 0), { 1.0f, 0 });
    cb->setViewport(QRhiViewport(0, 0, sz.width(), sz.height()));
    cb->setScissor(QRhiScissor(0, 0, sz.width(), sz.height()));
    drawTriangle(m_tri[2]);
    cb->endPass();
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MultiPassWidget w;
    w.setWindowTitle(QStringLiteral(
        "Multi-Pass QRhi Test — expect RED + GREEN + BLUE triangles"));
    w.resize(800, 600);
    w.show();

    return app.exec();
}
