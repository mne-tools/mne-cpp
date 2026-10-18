//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainrenderer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Shader pipeline creation, dual-render-target setup and per-frame QRhi draw orchestration for the disp3D scene.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainrenderer.h"

#include <rhi/qrhi.h>
#include "renderable/brainsurface.h"
#include "renderable/dipoleobject.h"
#include "renderable/networkobject.h"
#include "renderable/polylineobject.h"
#include "renderable/videooverlay.h"
#include "renderable/sliceobject.h"

using DISP3DLIB::VideoOverlay;

#include <QFile>
#include <QDebug>
#include <QImage>
#include <QVector3D>
#include <array>
#include <limits>
#include <cstring>

namespace DISP3DLIB
{

//=============================================================================================================
// PIMPL
//=============================================================================================================

static constexpr int kNumShaderModes = 6; // Standard..ShowNormals

struct BrainRenderer::Impl
{
    void createResources(QRhi* rhi, QRhiRenderPassDescriptor* rp, int sampleCount);

    // Offset of the next per-draw uniform slot, or -1 (with a warning) when the buffer is full or a pass is recorded
    int claimUniformSlot(const char* caller);

    void beginPass(QRhiCommandBuffer* cb, QRhiRenderTarget* rt, QRhiResourceUpdateBatch* updates);

    // QRhi forbids resource updates inside a pass; true (with a warning) when a prepare call comes too late
    bool insidePass(const char* caller) const;

    void issueInstanced(QRhiCommandBuffer* cb, int uniformOffset, QRhiBuffer* vertexBuffer, QRhiBuffer* instanceBuffer,
                        QRhiBuffer* indexBuffer, int indexCount, int instanceCount);

    bool videoOverlayUsesDepth(const VideoOverlay* overlay) const
    {
        return overlay->isDepthEnabled() && overlay->hasDepthFrame() && videoOverlay.depthInitialized && videoOverlay.surfaceDepthPipeline;
    }

    std::unique_ptr<QRhiShaderResourceBindings> srb;

    // Pipelines for each mode — indexed by ShaderMode enum (O(1) lookup)
    std::array<std::unique_ptr<QRhiGraphicsPipeline>, kNumShaderModes> pipelines{};
    std::array<std::unique_ptr<QRhiGraphicsPipeline>, kNumShaderModes> pipelinesBackColor{};

    std::unique_ptr<QRhiBuffer> uniformBuffer;
    // Whole bound block, not ubufAlignment(): that is 1 on GL, where slots would overlap
    int uniformSlotStride = 0;
    int currentUniformOffset = 0;
    bool overflowReported = false; // Reset per frame so the log gets one line, not one per skipped draw
    bool passRecording = false;

    bool resourcesDirty = true;

    // ── Dual render targets for multi-pass rendering ─────────────────
    // Qt bakes load/store flags at create() time, so we need two separate
    // render targets sharing the same color texture + depth buffer:
    //   - rtClear:    clears framebuffer    (first pass of each frame)
    //   - rtPreserve: preserves contents    (subsequent passes)
    // Validated in test_wasm_multi_pass on both Metal and WebGL.
    std::unique_ptr<QRhiRenderBuffer> dsBuffer;
    std::unique_ptr<QRhiTextureRenderTarget> rtClear;
    std::unique_ptr<QRhiTextureRenderTarget> rtPreserve;
    std::unique_ptr<QRhiRenderPassDescriptor> rpClear;
    std::unique_ptr<QRhiRenderPassDescriptor> rpPreserve;
    QSize rtSize;
    QRhiTexture* rtColorTex = nullptr; // Track texture pointer for rebuild

    // ── Generic video overlay ───────────────────────────────────────
    // Camera-facing textured quad rendered last (depth test off) at the
    // current focus point. Resources are created lazily on first use.
    struct VideoOverlayResources
    {
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        std::unique_ptr<QRhiGraphicsPipeline> surfacePipeline;
        std::unique_ptr<QRhiGraphicsPipeline> surfaceDepthPipeline; // POM-enhanced decal
        std::unique_ptr<QRhiShaderResourceBindings> srb;
        std::unique_ptr<QRhiShaderResourceBindings> srbDepth; // SRB with depth texture at binding 2
        std::unique_ptr<QRhiBuffer> uniformBuffer;
        std::unique_ptr<QRhiBuffer> vertexBuffer;
        std::unique_ptr<QRhiBuffer> indexBuffer;
        std::unique_ptr<QRhiTexture> texture;
        std::unique_ptr<QRhiTexture> depthTexture;
        std::unique_ptr<QRhiSampler> sampler;
        std::unique_ptr<QRhiSampler> depthSampler; // mipmap-enabled for vertex displacement
        QSize textureSize;
        QSize depthTextureSize;
        int uniformSlotStride = 0;
        int currentUniformOffset = 0;
        quint64 uploadedFrameGen = std::numeric_limits<quint64>::max();
        quint64 uploadedDepthFrameGen = std::numeric_limits<quint64>::max();
        bool indexUploaded = false;
        bool initialized = false;
        bool depthInitialized = false;
    };
    VideoOverlayResources videoOverlay;

    // ── MRI slice rendering ─────────────────────────────────────────
    // Up to 3 ortho slices (axial, coronal, sagittal) rendered as
    // textured quads with depth test and alpha blending.
    static constexpr int kMaxSliceSlots = 3;
    struct SliceSlot
    {
        std::unique_ptr<QRhiTexture> texture;
        QSize textureSize;
        bool dirty = true;       // needs texture re-upload
        bool visible = false;    // whether this slot has valid data
        QVector<float> vertices; // 4 verts × (3 pos + 2 uv) = 20 floats
        float opacity = 0.8f;
        float windowCenter = 0.5f;
        float windowWidth = 1.0f;
    };
    struct SliceResources
    {
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        std::unique_ptr<QRhiShaderResourceBindings> srb[kMaxSliceSlots];
        std::unique_ptr<QRhiBuffer> uniformBuffer;
        std::unique_ptr<QRhiBuffer> vertexBuffer[kMaxSliceSlots];
        std::unique_ptr<QRhiBuffer> indexBuffer;
        std::unique_ptr<QRhiSampler> sampler;
        SliceSlot sliceSlots[kMaxSliceSlots];
        int uniformSlotStride = 0;
        int currentUniformOffset = 0;
        bool indexUploaded = false;
        bool initialized = false;
    };
    SliceResources sliceRes;
};

//=============================================================================================================
// Helpers
//=============================================================================================================

static QImage tightlyPackedRgba(const QImage& source)
{
    if (source.isNull())
        return {};

    QImage rgba = source.convertToFormat(QImage::Format_RGBA8888);
    const qsizetype tightStride = qsizetype(rgba.width()) * 4;
    if (rgba.bytesPerLine() == tightStride)
        return rgba.copy();

    QImage packed(rgba.size(), QImage::Format_RGBA8888);
    for (int y = 0; y < rgba.height(); ++y) {
        memcpy(packed.scanLine(y), rgba.constScanLine(y), size_t(tightStride));
    }
    return packed;
}

//=============================================================================================================
// Uniform buffer layout constants — single source of truth for shader ↔ C++ interface
//=============================================================================================================

namespace
{
// Uniform buffer sizing
constexpr int kUniformSlotCount = 8192; // Max draw calls before overflow
constexpr int kUniformBlockSize = 256;  // Bound size per SRB dynamic slot (bytes)
constexpr int kVideoOverlaySlots = 64;  // Video overlay draws per frame
constexpr quint32 kVideoQuadBytes = 4 * 5 * sizeof(float); // 4 verts × (3 pos + 2 uv)

// Per-object uniform byte offsets (must match .vert shader layout).
// Kept as documentation of the UBO layout even where not directly indexed.
[[maybe_unused]] constexpr int kOffsetMVP = 0;                 // mat4  (64 bytes)
[[maybe_unused]] constexpr int kOffsetCameraPos = 64;          // vec3  (12 bytes)
[[maybe_unused]] constexpr int kOffsetSelected = 76;           // float
[[maybe_unused]] constexpr int kOffsetLightDir = 80;           // vec3  (12 bytes)
[[maybe_unused]] constexpr int kOffsetTissueType = 92;         // float
[[maybe_unused]] constexpr int kOffsetLighting = 96;           // float
[[maybe_unused]] constexpr int kOffsetOverlayMode = 100;       // float
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

//=============================================================================================================

BrainRenderer::BrainRenderer()
: d(std::make_unique<Impl>())
{
}

//=============================================================================================================

BrainRenderer::~BrainRenderer() = default;

//=============================================================================================================

void BrainRenderer::initialize(QRhi* rhi, QRhiRenderPassDescriptor* rp, int sampleCount)
{
    if (d->resourcesDirty) {
        d->createResources(rhi, rp, sampleCount);
    }
}

//=============================================================================================================

int BrainRenderer::Impl::claimUniformSlot(const char* caller)
{
    if (insidePass(caller))
        return -1;
    const int offset = currentUniformOffset;
    if (static_cast<quint32>(offset + uniformSlotStride) > uniformBuffer->size()) {
        if (!overflowReported) {
            qWarning("BrainRenderer: uniform buffer full at %s; further draws this frame are skipped", caller);
            overflowReported = true;
        }
        return -1;
    }
    currentUniformOffset += uniformSlotStride;
    return offset;
}

//=============================================================================================================

void BrainRenderer::Impl::createResources(QRhi* rhi, QRhiRenderPassDescriptor* rp, int sampleCount)
{
    uniformSlotStride = rhi->ubufAligned(kUniformBlockSize);

    // Create Uniform Buffer
    if (!uniformBuffer) {
        // Size for 8192 slots with alignment — enough for 4 viewports × ~1000 surfaces
        uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, kUniformSlotCount * uniformSlotStride));
        uniformBuffer->create();
    }

    // Create SRB
    if (!srb) {
        srb.reset(rhi->newShaderResourceBindings());
        srb->setBindings({// Use dynamic offset for the uniform buffer.
                          QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, uniformBuffer.get(), kUniformBlockSize)});
        srb->create();
    }

    // Shader Loader
    auto getShader = [](const QString& name) {
        QFile f(name);
        return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
    };

    // List of modes to initialize
    QList<ShaderMode> modes = {Standard, Holographic, Anatomical, Dipole, XRay, ShowNormals};

    for (ShaderMode mode : modes) {
        QString vert = (mode == Holographic || mode == XRay) ? ":/holographic.vert.qsb" : (mode == Anatomical) ? ":/anatomical.vert.qsb"
            : (mode == Dipole)                                                                                 ? ":/dipole.vert.qsb"
            : (mode == ShowNormals)                                                                            ? ":/shownormals.vert.qsb"
                                                                                                               : ":/standard.vert.qsb";

        QString frag = (mode == Holographic || mode == XRay) ? ":/holographic.frag.qsb" : (mode == Anatomical) ? ":/anatomical.frag.qsb"
            : (mode == Dipole)                                                                                 ? ":/dipole.frag.qsb"
            : (mode == ShowNormals)                                                                            ? ":/shownormals.frag.qsb"
                                                                                                               : ":/standard.frag.qsb";

        QShader vS = getShader(vert);
        QShader fS = getShader(frag);

        if (!vS.isValid() || !fS.isValid()) {
            qWarning() << "BrainRenderer: Could not load shaders for mode" << mode << vert << frag;
            continue;
        }

        // Setup Pipeline
        auto pipeline = std::unique_ptr<QRhiGraphicsPipeline>(rhi->newGraphicsPipeline());

        QRhiGraphicsPipeline::TargetBlend blend;
        if (mode == Holographic || mode == XRay) {
            blend.enable = true;
            blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
            blend.dstColor = QRhiGraphicsPipeline::One;
            blend.srcAlpha = QRhiGraphicsPipeline::SrcAlpha;
            blend.dstAlpha = QRhiGraphicsPipeline::One;
        } else if (mode == Dipole) {
            blend.enable = true;
            blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
            blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
            blend.srcAlpha = QRhiGraphicsPipeline::SrcAlpha;
            blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        }

        auto setup = [&](QRhiGraphicsPipeline* p, QRhiGraphicsPipeline::CullMode cull) {
            p->setShaderStages({{QRhiShaderStage::Vertex, vS}, {QRhiShaderStage::Fragment, fS}});

            QRhiVertexInputLayout il;

            if (mode == Dipole) {
                il.setBindings({
                    {6 * sizeof(float)},                                      // Binding 0: Vertex Data (Pos + Normal) -> stride 6 floats
                    {21 * sizeof(float), QRhiVertexInputBinding::PerInstance} // Binding 1: Instance Data (Mat4 + Color + Selected) -> stride 21 floats
                });

                il.setAttributes({                                                             // Vertex Buffer (Binding 0)
                                  {0, 0, QRhiVertexInputAttribute::Float3, 0},                 // Pos
                                  {0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float)}, // Normal

                                  // Instance Buffer (Binding 1)
                                  // Model Matrix (4 x vec4)
                                  {1, 2, QRhiVertexInputAttribute::Float4, 0},
                                  {1, 3, QRhiVertexInputAttribute::Float4, 4 * sizeof(float)},
                                  {1, 4, QRhiVertexInputAttribute::Float4, 8 * sizeof(float)},
                                  {1, 5, QRhiVertexInputAttribute::Float4, 12 * sizeof(float)},
                                  // Color
                                  {1, 6, QRhiVertexInputAttribute::Float4, 16 * sizeof(float)},
                                  // isSelected
                                  {1, 7, QRhiVertexInputAttribute::Float, 20 * sizeof(float)}});
            } else {
                il.setBindings({{sizeof(VertexData)}});
                il.setAttributes({{0, 0, QRhiVertexInputAttribute::Float3, 0},
                                  {0, 1, QRhiVertexInputAttribute::Float3, 12},
                                  {0, 2, QRhiVertexInputAttribute::UNormByte4, 24},
                                  {0, 3, QRhiVertexInputAttribute::UNormByte4, 28}});
            }

            p->setVertexInputLayout(il);
            p->setShaderResourceBindings(srb.get());
            p->setRenderPassDescriptor(rp);
            p->setSampleCount(sampleCount);
            p->setCullMode(cull);
            if (mode == Holographic) {
                p->setTargetBlends({blend});
                p->setDepthTest(true);
                p->setDepthWrite(false);
            } else if (mode == XRay) {
                p->setTargetBlends({blend});
                p->setDepthTest(false); // Disable Depth Test to see through head
                p->setDepthWrite(false);
            } else if (mode == Dipole) {
                p->setTargetBlends({blend});
                p->setCullMode(QRhiGraphicsPipeline::None);
                p->setDepthTest(true);
                p->setDepthWrite(false);
            } else {
                p->setDepthTest(true);
                p->setDepthWrite(true);
            }
            p->setFlags(QRhiGraphicsPipeline::UsesScissor);
            p->create();
        };

        if (mode == Holographic || mode == XRay) { // Handle XRay back-faces same as Holographic
            auto pipelineBack = std::unique_ptr<QRhiGraphicsPipeline>(rhi->newGraphicsPipeline());
            setup(pipelineBack.get(), QRhiGraphicsPipeline::Front);
            pipelinesBackColor[mode] = std::move(pipelineBack);
            setup(pipeline.get(), QRhiGraphicsPipeline::Back); // Front faces
        } else {
            // Culling: None (Double-sided) to be safe for FreeSurfer meshes
            setup(pipeline.get(), QRhiGraphicsPipeline::None);
        }
        pipelines[mode] = std::move(pipeline);
    }

    resourcesDirty = false;
}

//=============================================================================================================

//=============================================================================================================

void BrainRenderer::ensureRenderTargets(QRhi* rhi, QRhiTexture* colorTex, const QSize& pixelSize)
{
    // Rebuild when size changes OR when the backing texture changes
    // (QRhiWidget may return a different colorTexture() each frame).
    if (d->rtClear && d->rtSize == pixelSize && d->rtColorTex == colorTex)
        return;

    d->rtSize = pixelSize;
    d->rtColorTex = colorTex;

    // Shared depth-stencil buffer
    d->dsBuffer.reset(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, pixelSize));
    d->dsBuffer->create();

    QRhiColorAttachment colorAtt(colorTex);
    QRhiTextureRenderTargetDescription desc(colorAtt);
    desc.setDepthStencilBuffer(d->dsBuffer.get());

    // RT 1: Clearing (no preserve flags) — used for the first pass of each frame
    d->rtClear.reset(rhi->newTextureRenderTarget(desc));
    d->rpClear.reset(d->rtClear->newCompatibleRenderPassDescriptor());
    d->rtClear->setRenderPassDescriptor(d->rpClear.get());
    d->rtClear->create();

    // RT 2: Preserving (load previous contents) — used for passes 2+
    d->rtPreserve.reset(rhi->newTextureRenderTarget(desc,
                                                    QRhiTextureRenderTarget::PreserveColorContents | QRhiTextureRenderTarget::PreserveDepthStencilContents));
    d->rpPreserve.reset(d->rtPreserve->newCompatibleRenderPassDescriptor());
    d->rtPreserve->setRenderPassDescriptor(d->rpPreserve.get());
    d->rtPreserve->create();
}

//=============================================================================================================

QRhiRenderTarget* BrainRenderer::rtClear() const
{
    return d->rtClear.get();
}

QRhiRenderTarget* BrainRenderer::rtPreserve() const
{
    return d->rtPreserve.get();
}

//=============================================================================================================

void BrainRenderer::newFrame()
{
    d->currentUniformOffset = 0;
    d->overflowReported = false;
    d->videoOverlay.currentUniformOffset = 0;
    d->sliceRes.currentUniformOffset = 0;
}

//=============================================================================================================

void BrainRenderer::Impl::beginPass(QRhiCommandBuffer* cb, QRhiRenderTarget* rt, QRhiResourceUpdateBatch* updates)
{
    cb->beginPass(rt, QColor(0, 0, 0), {1.0f, 0}, updates);
    passRecording = true;
    const int w = rt->pixelSize().width();
    const int h = rt->pixelSize().height();
    cb->setViewport(QRhiViewport(0, 0, w, h));
    cb->setScissor(QRhiScissor(0, 0, w, h));
}

//=============================================================================================================

bool BrainRenderer::Impl::insidePass(const char* caller) const
{
    if (passRecording)
        qWarning("BrainRenderer: %s called inside a render pass; resource updates must precede beginFrame()", caller);
    return passRecording;
}

//=============================================================================================================

void BrainRenderer::beginFrame(QRhiCommandBuffer* cb, QRhiResourceUpdateBatch* updates)
{
    d->beginPass(cb, d->rtClear.get(), updates);
}

//=============================================================================================================

void BrainRenderer::beginPreservingPass(QRhiCommandBuffer* cb, QRhiResourceUpdateBatch* updates)
{
    d->beginPass(cb, d->rtPreserve.get(), updates);
}

//=============================================================================================================

void BrainRenderer::endPass(QRhiCommandBuffer* cb)
{
    cb->endPass();
    d->passRecording = false;
}

//=============================================================================================================
// Video overlay rendering
//=============================================================================================================

void BrainRenderer::prepareVideoOverlay(QRhi* rhi,
                                        QRhiResourceUpdateBatch* u,
                                        VideoOverlay* overlay)
{
    if (!overlay || !overlay->isEnabled())
        return;
    if (!overlay->hasFrame())
        return;
    if (!rhi || !u || d->insidePass("prepareVideoOverlay"))
        return;

    QImage frame = tightlyPackedRgba(overlay->frame());
    if (frame.isNull())
        return;

    auto& k = d->videoOverlay;

    // ── Lazy resource creation ──────────────────────────────────────
    if (!k.initialized) {
        // Shaders
        QFile vFile(QStringLiteral(":/video_overlay.vert.qsb"));
        QFile fFile(QStringLiteral(":/video_overlay.frag.qsb"));
        if (!vFile.open(QIODevice::ReadOnly) || !fFile.open(QIODevice::ReadOnly)) {
            qWarning() << "BrainRenderer: failed to open video overlay shaders";
            return;
        }
        QShader vShader = QShader::fromSerialized(vFile.readAll());
        QShader fShader = QShader::fromSerialized(fFile.readAll());
        if (!vShader.isValid() || !fShader.isValid()) {
            qWarning() << "BrainRenderer: invalid video overlay shaders";
            return;
        }

        // Uniform buffer (mat4 + vec4 + 4 floats = 96 bytes, pad to 256)
        k.uniformSlotStride = rhi->ubufAligned(kUniformBlockSize);
        k.uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                             QRhiBuffer::UniformBuffer,
                                             kVideoOverlaySlots * k.uniformSlotStride));
        k.uniformBuffer->create();

        // Vertex buffer: one billboard quad per uniform slot — refilled each frame
        k.vertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                            QRhiBuffer::VertexBuffer, kVideoOverlaySlots * kVideoQuadBytes));
        k.vertexBuffer->create();

        // Index buffer: 2 triangles, 6 indices — uploaded once
        constexpr int kIbSize = 6 * sizeof(quint32);
        k.indexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable,
                                           QRhiBuffer::IndexBuffer, kIbSize));
        k.indexBuffer->create();

        // Sampler (linear, clamp)
        k.sampler.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear,
                                        QRhiSampler::None,
                                        QRhiSampler::ClampToEdge,
                                        QRhiSampler::ClampToEdge));
        k.sampler->create();

        // Initial texture uses the first real frame size.
        k.texture.reset(rhi->newTexture(QRhiTexture::RGBA8, frame.size()));
        k.texture->create();
        k.textureSize = frame.size();

        // SRB binding: uniform @ 0, sampled image @ 1
        k.srb.reset(rhi->newShaderResourceBindings());
        k.srb->setBindings({QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0,
                                                                                      QRhiShaderResourceBinding::VertexStage |
                                                                                          QRhiShaderResourceBinding::FragmentStage,
                                                                                      k.uniformBuffer.get(), kUniformBlockSize),
                            QRhiShaderResourceBinding::sampledTexture(1,
                                                                      QRhiShaderResourceBinding::FragmentStage,
                                                                      k.texture.get(), k.sampler.get())});
        k.srb->create();

        // Pipeline (alpha-blended, depth-test off so it sits on top)
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

        k.pipeline.reset(rhi->newGraphicsPipeline());
        k.pipeline->setShaderStages({{QRhiShaderStage::Vertex, vShader},
                                     {QRhiShaderStage::Fragment, fShader}});
        QRhiVertexInputLayout il;
        il.setBindings({{5 * sizeof(float)}});
        il.setAttributes({{0, 0, QRhiVertexInputAttribute::Float3, 0},
                          {0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float)}});
        k.pipeline->setVertexInputLayout(il);
        k.pipeline->setShaderResourceBindings(k.srb.get());
        k.pipeline->setRenderPassDescriptor(d->rtClear->renderPassDescriptor());
        k.pipeline->setSampleCount(d->rtClear->sampleCount());
        k.pipeline->setCullMode(QRhiGraphicsPipeline::None);
        k.pipeline->setTargetBlends({blend});
        k.pipeline->setDepthTest(false);
        k.pipeline->setDepthWrite(false);
        k.pipeline->setFlags(QRhiGraphicsPipeline::UsesScissor);
        k.pipeline->create();

        QFile dvFile(QStringLiteral(":/video_decal.vert.qsb"));
        QFile dfFile(QStringLiteral(":/video_decal.frag.qsb"));
        if (!dvFile.open(QIODevice::ReadOnly) || !dfFile.open(QIODevice::ReadOnly)) {
            qWarning() << "BrainRenderer: failed to open video decal shaders";
            return;
        }
        QShader dvShader = QShader::fromSerialized(dvFile.readAll());
        QShader dfShader = QShader::fromSerialized(dfFile.readAll());
        if (!dvShader.isValid() || !dfShader.isValid()) {
            qWarning() << "BrainRenderer: invalid video decal shaders";
            return;
        }

        QRhiGraphicsPipeline::TargetBlend decalBlend;
        decalBlend.enable = true;
        decalBlend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        decalBlend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        decalBlend.srcAlpha = QRhiGraphicsPipeline::One;
        decalBlend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

        k.surfacePipeline.reset(rhi->newGraphicsPipeline());
        k.surfacePipeline->setShaderStages({{QRhiShaderStage::Vertex, dvShader},
                                            {QRhiShaderStage::Fragment, dfShader}});
        QRhiVertexInputLayout dil;
        dil.setBindings({{sizeof(VertexData)}});
        dil.setAttributes({{0, 0, QRhiVertexInputAttribute::Float3, 0},
                           {0, 1, QRhiVertexInputAttribute::Float3, 12},
                           {0, 2, QRhiVertexInputAttribute::UNormByte4, 24},
                           {0, 3, QRhiVertexInputAttribute::UNormByte4, 28}});
        k.surfacePipeline->setVertexInputLayout(dil);
        k.surfacePipeline->setShaderResourceBindings(k.srb.get());
        k.surfacePipeline->setRenderPassDescriptor(d->rtClear->renderPassDescriptor());
        k.surfacePipeline->setSampleCount(d->rtClear->sampleCount());
        k.surfacePipeline->setCullMode(QRhiGraphicsPipeline::None);
        k.surfacePipeline->setTargetBlends({decalBlend});
        k.surfacePipeline->setDepthTest(true);
        k.surfacePipeline->setDepthWrite(false);
        k.surfacePipeline->setFlags(QRhiGraphicsPipeline::UsesScissor);
        k.surfacePipeline->create();

        k.initialized = true;
    }

    // ── Lazy depth-enhanced pipeline creation ───────────────────────
    if (overlay->isDepthEnabled() && !k.depthInitialized && k.initialized) {
        QFile ddvFile(QStringLiteral(":/video_decal_depth.vert.qsb"));
        QFile ddfFile(QStringLiteral(":/video_decal_depth.frag.qsb"));
        if (ddvFile.open(QIODevice::ReadOnly) && ddfFile.open(QIODevice::ReadOnly)) {
            QShader ddvShader = QShader::fromSerialized(ddvFile.readAll());
            QShader ddfShader = QShader::fromSerialized(ddfFile.readAll());
            if (ddvShader.isValid() && ddfShader.isValid()) {
                // Mipmap-enabled sampler for depth texture (vertex shader
                // samples a high LOD for smooth displacement on sparse meshes)
                k.depthSampler.reset(rhi->newSampler(
                    QRhiSampler::Linear, QRhiSampler::Linear,
                    QRhiSampler::Linear, // mip filtering
                    QRhiSampler::ClampToEdge,
                    QRhiSampler::ClampToEdge));
                k.depthSampler->create();

                // 1x1 placeholder depth texture with mipmaps
                k.depthTexture.reset(rhi->newTexture(
                    QRhiTexture::RGBA8, QSize(1, 1), 1, QRhiTexture::MipMapped));
                k.depthTexture->create();
                k.depthTextureSize = QSize(1, 1);

                // SRB with depth texture at binding 2 (vertex + fragment)
                k.srbDepth.reset(rhi->newShaderResourceBindings());
                k.srbDepth->setBindings({QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0,
                                                                                                   QRhiShaderResourceBinding::VertexStage |
                                                                                                       QRhiShaderResourceBinding::FragmentStage,
                                                                                                   k.uniformBuffer.get(), kUniformBlockSize),
                                         QRhiShaderResourceBinding::sampledTexture(1,
                                                                                   QRhiShaderResourceBinding::FragmentStage,
                                                                                   k.texture.get(), k.sampler.get()),
                                         QRhiShaderResourceBinding::sampledTexture(2,
                                                                                   QRhiShaderResourceBinding::VertexStage |
                                                                                       QRhiShaderResourceBinding::FragmentStage,
                                                                                   k.depthTexture.get(), k.depthSampler.get())});
                k.srbDepth->create();

                QRhiGraphicsPipeline::TargetBlend depthDecalBlend;
                depthDecalBlend.enable = true;
                depthDecalBlend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
                depthDecalBlend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
                depthDecalBlend.srcAlpha = QRhiGraphicsPipeline::One;
                depthDecalBlend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

                k.surfaceDepthPipeline.reset(rhi->newGraphicsPipeline());
                k.surfaceDepthPipeline->setShaderStages({{QRhiShaderStage::Vertex, ddvShader},
                                                         {QRhiShaderStage::Fragment, ddfShader}});
                QRhiVertexInputLayout ddil;
                ddil.setBindings({{sizeof(VertexData)}});
                ddil.setAttributes({{0, 0, QRhiVertexInputAttribute::Float3, 0},
                                    {0, 1, QRhiVertexInputAttribute::Float3, 12},
                                    {0, 2, QRhiVertexInputAttribute::UNormByte4, 24},
                                    {0, 3, QRhiVertexInputAttribute::UNormByte4, 28}});
                k.surfaceDepthPipeline->setVertexInputLayout(ddil);
                k.surfaceDepthPipeline->setShaderResourceBindings(k.srbDepth.get());
                k.surfaceDepthPipeline->setRenderPassDescriptor(d->rtClear->renderPassDescriptor());
                k.surfaceDepthPipeline->setSampleCount(d->rtClear->sampleCount());
                k.surfaceDepthPipeline->setCullMode(QRhiGraphicsPipeline::None);
                k.surfaceDepthPipeline->setTargetBlends({depthDecalBlend});
                k.surfaceDepthPipeline->setDepthTest(false); // inward-displaced vertices must not be culled by brain surface z
                k.surfaceDepthPipeline->setDepthWrite(false);
                k.surfaceDepthPipeline->setFlags(QRhiGraphicsPipeline::UsesScissor);
                k.surfaceDepthPipeline->create();
                k.depthInitialized = true;
            }
        }
    }

    // ── Index buffer (one-shot upload) ──────────────────────────────
    if (!k.indexUploaded) {
        const quint32 idx[6] = {0, 1, 2, 2, 1, 3};
        u->uploadStaticBuffer(k.indexBuffer.get(), idx);
        k.indexUploaded = true;
    }

    // ── Texture (re-create on size change, re-upload on new frame) ──
    if (frame.size() != k.textureSize) {
        k.texture.reset(rhi->newTexture(QRhiTexture::RGBA8, frame.size()));
        k.texture->create();
        k.textureSize = frame.size();
        // Rebuild SRB to point at the new texture
        k.srb->setBindings({QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0,
                                                                                      QRhiShaderResourceBinding::VertexStage |
                                                                                          QRhiShaderResourceBinding::FragmentStage,
                                                                                      k.uniformBuffer.get(), kUniformBlockSize),
                            QRhiShaderResourceBinding::sampledTexture(1,
                                                                      QRhiShaderResourceBinding::FragmentStage,
                                                                      k.texture.get(), k.sampler.get())});
        k.srb->create();
        // Also rebuild depth SRB if it exists
        if (k.srbDepth && k.depthTexture) {
            k.srbDepth->setBindings({QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0,
                                                                                               QRhiShaderResourceBinding::VertexStage |
                                                                                                   QRhiShaderResourceBinding::FragmentStage,
                                                                                               k.uniformBuffer.get(), kUniformBlockSize),
                                     QRhiShaderResourceBinding::sampledTexture(1,
                                                                               QRhiShaderResourceBinding::FragmentStage,
                                                                               k.texture.get(), k.sampler.get()),
                                     QRhiShaderResourceBinding::sampledTexture(2,
                                                                               QRhiShaderResourceBinding::VertexStage |
                                                                                   QRhiShaderResourceBinding::FragmentStage,
                                                                               k.depthTexture.get(), k.depthSampler.get())});
            k.srbDepth->create();
        }
        k.uploadedFrameGen = std::numeric_limits<quint64>::max();
    }
    if (overlay->frameGeneration() != k.uploadedFrameGen) {
        QRhiTextureSubresourceUploadDescription sub(frame);
        QRhiTextureUploadDescription desc({0, 0, sub});
        u->uploadTexture(k.texture.get(), desc);
        k.uploadedFrameGen = overlay->frameGeneration();
    }

    // ── Depth texture upload ────────────────────────────────────────
    if (overlay->isDepthEnabled() && overlay->hasDepthFrame() && k.depthInitialized) {
        QImage depthFrame = tightlyPackedRgba(overlay->depthFrame());
        if (!depthFrame.isNull()) {
            if (depthFrame.size() != k.depthTextureSize) {
                k.depthTexture.reset(rhi->newTexture(
                    QRhiTexture::RGBA8, depthFrame.size(), 1, QRhiTexture::MipMapped));
                k.depthTexture->create();
                k.depthTextureSize = depthFrame.size();
                // Rebuild depth SRB with new depth texture
                k.srbDepth->setBindings({QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0,
                                                                                                   QRhiShaderResourceBinding::VertexStage |
                                                                                                       QRhiShaderResourceBinding::FragmentStage,
                                                                                                   k.uniformBuffer.get(), kUniformBlockSize),
                                         QRhiShaderResourceBinding::sampledTexture(1,
                                                                                   QRhiShaderResourceBinding::FragmentStage,
                                                                                   k.texture.get(), k.sampler.get()),
                                         QRhiShaderResourceBinding::sampledTexture(2,
                                                                                   QRhiShaderResourceBinding::VertexStage |
                                                                                       QRhiShaderResourceBinding::FragmentStage,
                                                                                   k.depthTexture.get(), k.depthSampler.get())});
                k.srbDepth->create();
                k.uploadedDepthFrameGen = std::numeric_limits<quint64>::max();
            }
            if (overlay->depthFrameGeneration() != k.uploadedDepthFrameGen) {
                QRhiTextureSubresourceUploadDescription depthSub(depthFrame);
                QRhiTextureUploadDescription depthDesc({0, 0, depthSub});
                u->uploadTexture(k.depthTexture.get(), depthDesc);
                u->generateMips(k.depthTexture.get());
                k.uploadedDepthFrameGen = overlay->depthFrameGeneration();
            }
        }
    }
}

int BrainRenderer::prepareVideoOverlayDraw(QRhiResourceUpdateBatch* u,
                                           const SceneData& data,
                                           VideoOverlay* overlay,
                                           BrainSurface* surface)
{
    if (!overlay || !overlay->isEnabled() || !overlay->hasFrame())
        return -1;

    auto& k = d->videoOverlay;
    if (!k.initialized || (surface && !k.surfacePipeline))
        return -1;
    if (d->insidePass("prepareVideoOverlayDraw"))
        return -1;

    const int uniformOffset = k.currentUniformOffset;
    if (static_cast<quint32>(uniformOffset + kUniformBlockSize) > k.uniformBuffer->size())
        return -1;
    k.currentUniformOffset += k.uniformSlotStride;

    if (!surface) {
        // ── Billboard the quad to face the camera ──────────────────────
        const QVector3D centre = overlay->focusPosition();
        const float sizeM = overlay->sizeMeters();

        // Preserve the video frame's aspect ratio: sizeMeters controls the
        // width, and the height is derived from the frame dimensions.
        const QImage& frm = overlay->frame();
        const float aspect = (frm.height() > 0)
            ? static_cast<float>(frm.width()) / frm.height()
            : 1.0f;
        const float halfW = 0.5f * sizeM;
        const float halfH = (aspect > 0.0f) ? halfW / aspect : halfW;

        QVector3D viewDir = centre - data.cameraPos;
        if (viewDir.lengthSquared() < 1e-12f)
            viewDir = QVector3D(0, 0, -1);
        viewDir.normalize();

        // Use the upHint (tracker→objective axis) when available so that
        // the quad's long edge is perpendicular to the optical axis.
        const QVector3D hint = overlay->upHint();
        QVector3D up;
        if (hint.lengthSquared() > 1e-8f) {
            // Project the hint onto the plane perpendicular to viewDir
            up = (hint - QVector3D::dotProduct(hint, viewDir) * viewDir).normalized();
            if (up.lengthSquared() < 1e-8f)
                up = QVector3D(0.0f, 0.0f, 1.0f);
        } else {
            QVector3D worldUp(0.0f, 0.0f, 1.0f);
            if (std::abs(QVector3D::dotProduct(viewDir, worldUp)) > 0.95f)
                worldUp = QVector3D(0.0f, 1.0f, 0.0f);
            up = QVector3D::crossProduct(
                     QVector3D::crossProduct(viewDir, worldUp), viewDir)
                     .normalized();
        }
        QVector3D right = QVector3D::crossProduct(viewDir, up).normalized();

        const QVector3D c00 = centre - right * halfW - up * halfH;
        const QVector3D c10 = centre + right * halfW - up * halfH;
        const QVector3D c01 = centre - right * halfW + up * halfH;
        const QVector3D c11 = centre + right * halfW + up * halfH;

        const float verts[4 * 5] = {
            c00.x(), c00.y(), c00.z(), 0.0f, 1.0f, // image y is flipped vs uv
            c10.x(), c10.y(), c10.z(), 1.0f, 1.0f,
            c01.x(), c01.y(), c01.z(), 0.0f, 0.0f,
            c11.x(), c11.y(), c11.z(), 1.0f, 0.0f};
        // Each slot has its own quad: every pane faces its own camera
        static_assert(sizeof(verts) == kVideoQuadBytes);
        u->updateDynamicBuffer(k.vertexBuffer.get(), quint32(uniformOffset / k.uniformSlotStride) * kVideoQuadBytes, sizeof(verts), verts);

        // ── Uniforms (mat4 mvp, vec4 borderColor, float opacity, 3 pad) ─
        struct
        {
            float mvp[16];
            float borderColor[4];
            float opacity;
            float pad0, pad1, pad2;
        } ub;
        memcpy(ub.mvp, data.mvp.constData(), 64);
        ub.borderColor[0] = 0.0f;
        ub.borderColor[1] = 0.85f;
        ub.borderColor[2] = 1.0f;
        ub.borderColor[3] = 1.0f;
        ub.opacity = overlay->opacity();
        ub.pad0 = ub.pad1 = ub.pad2 = 0.0f;
        u->updateDynamicBuffer(k.uniformBuffer.get(), uniformOffset, sizeof(ub), &ub);
        return uniformOffset;
    }

    // Approximate the local scalp normal from the focus position vector.
    QVector3D localNormal = overlay->focusPosition();
    if (localNormal.lengthSquared() < 1e-12f) {
        localNormal = QVector3D(0.0f, 0.0f, 1.0f);
    }
    localNormal.normalize();

    // Use the tracker-derived up hint when available — this keeps the
    // decal orientation locked to the physical microscope and avoids
    // the 180° flip singularity that occurs with a fixed reference axis.
    QVector3D referenceUp = overlay->upHint();
    if (referenceUp.lengthSquared() < 1e-6f) {
        // Fallback when no tracker up is available.
        referenceUp = QVector3D(0.0f, 0.0f, 1.0f);
        if (std::abs(QVector3D::dotProduct(localNormal, referenceUp)) > 0.95f) {
            referenceUp = QVector3D(1.0f, 0.0f, 0.0f);
        }
    }
    // Orthogonalise against the surface normal so the frame is tangent.
    referenceUp = (referenceUp - QVector3D::dotProduct(referenceUp, localNormal) * localNormal);
    if (referenceUp.lengthSquared() < 1e-12f) {
        referenceUp = QVector3D(0.0f, 0.0f, 1.0f);
    }
    referenceUp.normalize();

    const QVector3D axisV = referenceUp;
    const QVector3D axisU = QVector3D::crossProduct(axisV, localNormal).normalized();

    const bool useDepth = d->videoOverlayUsesDepth(overlay);

    // Uniform block — the depth-enhanced shader has an extra vec4 depthParams
    // but the base 7×vec4 layout (112 bytes) still fits within kUniformBlockSize
    // (256 bytes) even with the extra vec4 (128 bytes total).
    struct
    {
        float mvp[16];
        float focusAndSize[4];
        float axisUAndOpacity[4];
        float axisVAndOffset[4];
        float axisNAndDepth[4];
        float cameraPosAndFacing[4];
        float borderColor[4];
        float depthParams[4];
    } ub;
    memcpy(ub.mvp, data.mvp.constData(), 64);
    ub.focusAndSize[0] = overlay->focusPosition().x();
    ub.focusAndSize[1] = overlay->focusPosition().y();
    ub.focusAndSize[2] = overlay->focusPosition().z();
    ub.focusAndSize[3] = overlay->sizeMeters();
    ub.axisUAndOpacity[0] = axisU.x();
    ub.axisUAndOpacity[1] = axisU.y();
    ub.axisUAndOpacity[2] = axisU.z();
    ub.axisUAndOpacity[3] = overlay->opacity();
    ub.axisVAndOffset[0] = axisV.x();
    ub.axisVAndOffset[1] = axisV.y();
    ub.axisVAndOffset[2] = axisV.z();
    ub.axisVAndOffset[3] = 0.00045f;
    ub.axisNAndDepth[0] = localNormal.x();
    ub.axisNAndDepth[1] = localNormal.y();
    ub.axisNAndDepth[2] = localNormal.z();
    ub.axisNAndDepth[3] = std::max(0.015f, overlay->sizeMeters() * 0.45f);
    ub.cameraPosAndFacing[0] = data.cameraPos.x();
    ub.cameraPosAndFacing[1] = data.cameraPos.y();
    ub.cameraPosAndFacing[2] = data.cameraPos.z();
    const QImage& decalFrame = overlay->frame();
    ub.cameraPosAndFacing[3] = (decalFrame.height() > 0)
        ? static_cast<float>(decalFrame.width()) / decalFrame.height()
        : 1.0f;
    ub.borderColor[0] = 0.0f;
    ub.borderColor[1] = 0.85f;
    ub.borderColor[2] = 1.0f;
    ub.borderColor[3] = 1.0f;
    ub.depthParams[0] = useDepth ? overlay->depthScale() : 0.0f;
    ub.depthParams[1] = useDepth ? static_cast<float>(overlay->depthSteps()) : 0.0f;
    ub.depthParams[2] = useDepth ? 1.0f : 0.0f;
    ub.depthParams[3] = 0.0f;
    u->updateDynamicBuffer(k.uniformBuffer.get(), uniformOffset, sizeof(ub), &ub);
    return uniformOffset;
}

//=============================================================================================================

void BrainRenderer::issueVideoOverlayDraw(QRhiCommandBuffer* cb,
                                          VideoOverlay* overlay,
                                          BrainSurface* surface,
                                          int uniformOffset)
{
    if (!overlay || uniformOffset < 0)
        return;

    auto& k = d->videoOverlay;
    const QRhiCommandBuffer::DynamicOffset srbOffset = {0, uint32_t(uniformOffset)};
    if (surface) {
        const bool useDepth = d->videoOverlayUsesDepth(overlay);
        cb->setGraphicsPipeline(useDepth ? k.surfaceDepthPipeline.get() : k.surfacePipeline.get());
        cb->setShaderResources(useDepth ? k.srbDepth.get() : k.srb.get(), 1, &srbOffset);
        const QRhiCommandBuffer::VertexInput vbuf(surface->vertexBuffer(), 0);
        cb->setVertexInput(0, 1, &vbuf, surface->indexBuffer(), 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(surface->indexCount());
        return;
    }

    cb->setGraphicsPipeline(k.pipeline.get());
    cb->setShaderResources(k.srb.get(), 1, &srbOffset);
    const QRhiCommandBuffer::VertexInput vbuf(k.vertexBuffer.get(), quint32(uniformOffset / k.uniformSlotStride) * kVideoQuadBytes);
    cb->setVertexInput(0, 1, &vbuf, k.indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(6);
}

//=============================================================================================================
// MRI slice rendering
//=============================================================================================================

void BrainRenderer::prepareSlice(QRhi* rhi,
                                 QRhiResourceUpdateBatch* u,
                                 DISP3DLIB::SliceObject* slice,
                                 int slotIndex)
{
    if (!rhi || !u || d->insidePass("prepareSlice"))
        return;
    if (slotIndex < 0 || slotIndex >= Impl::kMaxSliceSlots)
        return;

    auto& k = d->sliceRes;
    auto& sliceSlot = k.sliceSlots[slotIndex];

    if (!slice) {
        sliceSlot.visible = false;
        return;
    }

    const QImage& img = slice->image();
    if (img.isNull()) {
        sliceSlot.visible = false;
        return;
    }

    // ── Lazy one-time resource creation ─────────────────────────────
    if (!k.initialized) {
        QFile vFile(QStringLiteral(":/slice.vert.qsb"));
        QFile fFile(QStringLiteral(":/slice.frag.qsb"));
        if (!vFile.open(QIODevice::ReadOnly) || !fFile.open(QIODevice::ReadOnly)) {
            qWarning() << "BrainRenderer: failed to open MRI slice shaders";
            return;
        }
        QShader vShader = QShader::fromSerialized(vFile.readAll());
        QShader fShader = QShader::fromSerialized(fFile.readAll());
        if (!vShader.isValid() || !fShader.isValid()) {
            qWarning() << "BrainRenderer: invalid MRI slice shaders";
            return;
        }

        k.uniformSlotStride = rhi->ubufAligned(kUniformBlockSize);
        k.uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                             QRhiBuffer::UniformBuffer,
                                             64 * k.uniformSlotStride));
        k.uniformBuffer->create();

        // Index buffer (shared by all 3 slots): 2 triangles = 6 indices
        k.indexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable,
                                           QRhiBuffer::IndexBuffer,
                                           6 * sizeof(quint32)));
        k.indexBuffer->create();

        k.sampler.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear,
                                        QRhiSampler::None,
                                        QRhiSampler::ClampToEdge,
                                        QRhiSampler::ClampToEdge));
        k.sampler->create();

        // Per-slot resources: vertex buffer + texture + SRB
        for (int i = 0; i < Impl::kMaxSliceSlots; ++i) {
            constexpr int kVbSize = 4 * 5 * sizeof(float); // 4 verts × (3 pos + 2 uv)
            k.vertexBuffer[i].reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                                   QRhiBuffer::VertexBuffer, kVbSize));
            k.vertexBuffer[i]->create();

            // 1×1 placeholder texture — real size set on first data
            k.sliceSlots[i].texture.reset(rhi->newTexture(QRhiTexture::R8, QSize(1, 1)));
            k.sliceSlots[i].texture->create();
            k.sliceSlots[i].textureSize = QSize(1, 1);

            k.srb[i].reset(rhi->newShaderResourceBindings());
            k.srb[i]->setBindings({QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0,
                                                                                             QRhiShaderResourceBinding::VertexStage |
                                                                                                 QRhiShaderResourceBinding::FragmentStage,
                                                                                             k.uniformBuffer.get(), kUniformBlockSize),
                                   QRhiShaderResourceBinding::sampledTexture(1,
                                                                             QRhiShaderResourceBinding::FragmentStage,
                                                                             k.sliceSlots[i].texture.get(), k.sampler.get())});
            k.srb[i]->create();
        }

        // Pipeline: alpha-blended, depth-test on, depth-write off
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

        k.pipeline.reset(rhi->newGraphicsPipeline());
        k.pipeline->setShaderStages({{QRhiShaderStage::Vertex, vShader},
                                     {QRhiShaderStage::Fragment, fShader}});
        QRhiVertexInputLayout il;
        il.setBindings({{5 * sizeof(float)}});
        il.setAttributes({{0, 0, QRhiVertexInputAttribute::Float3, 0},
                          {0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float)}});
        k.pipeline->setVertexInputLayout(il);
        k.pipeline->setShaderResourceBindings(k.srb[0].get());
        k.pipeline->setRenderPassDescriptor(d->rtClear->renderPassDescriptor());
        k.pipeline->setSampleCount(d->rtClear->sampleCount());
        k.pipeline->setCullMode(QRhiGraphicsPipeline::None);
        k.pipeline->setTargetBlends({blend});
        k.pipeline->setDepthTest(true);
        k.pipeline->setDepthWrite(false);
        k.pipeline->setFlags(QRhiGraphicsPipeline::UsesScissor);
        k.pipeline->create();

        k.initialized = true;
    }

    // ── Index buffer (one-shot upload) ──────────────────────────────
    if (!k.indexUploaded) {
        const quint32 idx[6] = {0, 1, 2, 2, 1, 3};
        u->uploadStaticBuffer(k.indexBuffer.get(), idx);
        k.indexUploaded = true;
    }

    // ── Texture (re-create on size change, upload data) ─────────────
    // Convert to R8 (grayscale) for the slice shader
    QImage gray = img.convertToFormat(QImage::Format_Grayscale8);
    const QSize imgSize = gray.size();

    if (imgSize != sliceSlot.textureSize) {
        sliceSlot.texture.reset(rhi->newTexture(QRhiTexture::R8, imgSize));
        sliceSlot.texture->create();
        sliceSlot.textureSize = imgSize;
        // Rebuild SRB to point at new texture
        k.srb[slotIndex]->setBindings({QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0,
                                                                                                 QRhiShaderResourceBinding::VertexStage |
                                                                                                     QRhiShaderResourceBinding::FragmentStage,
                                                                                                 k.uniformBuffer.get(), kUniformBlockSize),
                                       QRhiShaderResourceBinding::sampledTexture(1,
                                                                                 QRhiShaderResourceBinding::FragmentStage,
                                                                                 sliceSlot.texture.get(), k.sampler.get())});
        k.srb[slotIndex]->create();
    }

    // Upload texture — R8 needs tightly-packed row data
    {
        const qsizetype tightStride = qsizetype(gray.width());
        QByteArray texData;
        texData.resize(gray.height() * tightStride);
        for (int y = 0; y < gray.height(); ++y) {
            memcpy(texData.data() + y * tightStride,
                   gray.constScanLine(y),
                   size_t(tightStride));
        }
        QRhiTextureSubresourceUploadDescription sub(texData.constData(), texData.size());
        sub.setSourceSize(imgSize);
        QRhiTextureUploadDescription desc({0, 0, sub});
        u->uploadTexture(sliceSlot.texture.get(), desc);
    }

    // ── Vertex data ─────────────────────────────────────────────────
    slice->generateQuadVertices(sliceSlot.vertices);
    u->updateDynamicBuffer(k.vertexBuffer[slotIndex].get(), 0,
                           static_cast<quint32>(sliceSlot.vertices.size() * sizeof(float)),
                           sliceSlot.vertices.constData());

    sliceSlot.opacity = slice->opacity();
    sliceSlot.windowCenter = slice->windowCenter();
    sliceSlot.windowWidth = slice->windowWidth();
    sliceSlot.visible = true;
}

//=============================================================================================================

int BrainRenderer::prepareSliceDraw(QRhiResourceUpdateBatch* u,
                                    const SceneData& data,
                                    int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= Impl::kMaxSliceSlots)
        return -1;

    auto& k = d->sliceRes;
    if (!k.initialized || !k.pipeline)
        return -1;

    const auto& sliceSlot = k.sliceSlots[slotIndex];
    if (!sliceSlot.visible || d->insidePass("prepareSliceDraw"))
        return -1;

    const int uniformOffset = k.currentUniformOffset;
    k.currentUniformOffset += k.uniformSlotStride;
    if (static_cast<quint32>(uniformOffset + kUniformBlockSize) > k.uniformBuffer->size())
        return -1;

    // Uniform block matches slice.vert / slice.frag layout:
    //   mat4 mvp         (64 bytes)
    //   mat4 sliceToWorld (64 bytes)
    //   float opacity     (4)
    //   float windowCenter(4)
    //   float windowWidth (4)
    //   float _pad0       (4)
    struct
    {
        float mvp[16];
        float sliceToWorld[16];
        float opacity;
        float windowCenter;
        float windowWidth;
        float _pad0;
    } ub;
    memcpy(ub.mvp, data.mvp.constData(), 64);
    // SliceToWorld is identity for pre-transformed vertices (already in world coords)
    QMatrix4x4 identity;
    identity.setToIdentity();
    memcpy(ub.sliceToWorld, identity.constData(), 64);
    ub.opacity = sliceSlot.opacity;
    ub.windowCenter = sliceSlot.windowCenter;
    ub.windowWidth = sliceSlot.windowWidth;
    ub._pad0 = 0.0f;
    u->updateDynamicBuffer(k.uniformBuffer.get(), uniformOffset, sizeof(ub), &ub);

    return uniformOffset;
}

//=============================================================================================================

void BrainRenderer::issueSliceDraw(QRhiCommandBuffer* cb,
                                   int slotIndex,
                                   int uniformOffset)
{
    if (slotIndex < 0 || slotIndex >= Impl::kMaxSliceSlots)
        return;
    if (uniformOffset < 0)
        return;

    auto& k = d->sliceRes;
    if (!k.initialized || !k.pipeline)
        return;

    cb->setGraphicsPipeline(k.pipeline.get());
    const QRhiCommandBuffer::DynamicOffset srbOffset = {0, uint32_t(uniformOffset)};
    cb->setShaderResources(k.srb[slotIndex].get(), 1, &srbOffset);
    const QRhiCommandBuffer::VertexInput vbuf(k.vertexBuffer[slotIndex].get(), 0);
    cb->setVertexInput(0, 1, &vbuf, k.indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(6);
}

//=============================================================================================================

int BrainRenderer::prepareSurfaceDraw(QRhiResourceUpdateBatch* u,
                                      const SceneData& data,
                                      BrainSurface* surface)
{
    if (!surface || !surface->isVisible())
        return -1;

    const int offset = d->claimUniformSlot("prepareSurfaceDraw");
    if (offset < 0)
        return -1;

    float selected = surface->isSelected() ? 1.0f : 0.0f;
    if (surface->isSelected() && (surface->selectedRegionId() != -1 || surface->selectedVertexStart() >= 0)) {
        selected = 0.0f;
    }

    struct
    {
        float mvp[16];
        float cameraPos[3];
        float isSelected;
        float lightDir[3];
        float tissueType;
        float lightingEnabled;
        float overlayMode;
    } ub;
    memcpy(ub.mvp, data.mvp.constData(), 64);
    memcpy(ub.cameraPos, &data.cameraPos, 12);
    ub.isSelected = selected;
    memcpy(ub.lightDir, &data.lightDir, 12);
    ub.tissueType = static_cast<float>(surface->tissueType());
    ub.lightingEnabled = data.lightingEnabled ? 1.0f : 0.0f;
    ub.overlayMode = data.overlayMode;

    u->updateDynamicBuffer(d->uniformBuffer.get(), offset, sizeof(ub), &ub);
    return offset;
}

//=============================================================================================================

void BrainRenderer::issueSurfaceDraw(QRhiCommandBuffer* cb,
                                     BrainSurface* surface,
                                     ShaderMode mode,
                                     int uniformOffset)
{
    if (!surface || uniformOffset < 0)
        return;

    auto* pipeline = d->pipelines[mode].get();
    if (!pipeline)
        return;

    auto draw = [&](QRhiGraphicsPipeline* p) {
        cb->setGraphicsPipeline(p);
        const QRhiCommandBuffer::DynamicOffset srbOffset = {0, uint32_t(uniformOffset)};
        cb->setShaderResources(d->srb.get(), 1, &srbOffset);
        const QRhiCommandBuffer::VertexInput vbuf(surface->vertexBuffer(), 0);
        cb->setVertexInput(0, 1, &vbuf, surface->indexBuffer(), 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(surface->indexCount());
    };

    if (mode == Holographic && d->pipelinesBackColor[Holographic]) {
        draw(d->pipelinesBackColor[Holographic].get());
    }

    draw(pipeline);
}

//=============================================================================================================

int BrainRenderer::prepareInstancedDraw(QRhiResourceUpdateBatch* u, const SceneData& data)
{
    const int offset = d->claimUniformSlot("prepareInstancedDraw");
    if (offset < 0)
        return -1;

    struct
    {
        float mvp[16];         // 0..63
        float cameraPos[3];    // 64..75
        float _pad0;           // 76..79
        float lightDir[3];     // 80..91
        float _pad1;           // 92..95
        float lightingEnabled; // 96..99
    } ub;
    memcpy(ub.mvp, data.mvp.constData(), 64);
    memcpy(ub.cameraPos, &data.cameraPos, 12);
    ub._pad0 = 0.0f;
    memcpy(ub.lightDir, &data.lightDir, 12);
    ub._pad1 = 0.0f;
    ub.lightingEnabled = data.lightingEnabled ? 1.0f : 0.0f;
    u->updateDynamicBuffer(d->uniformBuffer.get(), offset, sizeof(ub), &ub);
    return offset;
}

//=============================================================================================================

void BrainRenderer::Impl::issueInstanced(QRhiCommandBuffer* cb,
                                         int uniformOffset,
                                         QRhiBuffer* vertexBuffer,
                                         QRhiBuffer* instanceBuffer,
                                         QRhiBuffer* indexBuffer,
                                         int indexCount,
                                         int instanceCount)
{
    // Dipoles, network nodes and edges, and polylines share the Dipole pipeline's instance layout
    QRhiGraphicsPipeline* pipeline = pipelines[Dipole].get();
    if (!pipeline || uniformOffset < 0 || indexCount <= 0 || instanceCount <= 0)
        return;

    cb->setGraphicsPipeline(pipeline);
    const QRhiCommandBuffer::DynamicOffset srbOffset = {0, uint32_t(uniformOffset)};
    cb->setShaderResources(srb.get(), 1, &srbOffset);
    const QRhiCommandBuffer::VertexInput bindings[2] = {
        QRhiCommandBuffer::VertexInput(vertexBuffer, 0),
        QRhiCommandBuffer::VertexInput(instanceBuffer, 0)};
    cb->setVertexInput(0, 2, bindings, indexBuffer, 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(quint32(indexCount), quint32(instanceCount));
}

//=============================================================================================================

void BrainRenderer::issueDipoleDraw(QRhiCommandBuffer* cb, DipoleObject* dipoles, int uniformOffset)
{
    if (!dipoles || !dipoles->isVisible())
        return;
    d->issueInstanced(cb, uniformOffset, dipoles->vertexBuffer(), dipoles->instanceBuffer(),
                      dipoles->indexBuffer(), dipoles->indexCount(), dipoles->instanceCount());
}

//=============================================================================================================

void BrainRenderer::issueNetworkDraw(QRhiCommandBuffer* cb, NetworkObject* network, int uniformOffset)
{
    if (!network || !network->isVisible() || !network->hasData())
        return;
    d->issueInstanced(cb, uniformOffset, network->nodeVertexBuffer(), network->nodeInstanceBuffer(),
                      network->nodeIndexBuffer(), network->nodeIndexCount(), network->nodeInstanceCount());
    d->issueInstanced(cb, uniformOffset, network->edgeVertexBuffer(), network->edgeInstanceBuffer(),
                      network->edgeIndexBuffer(), network->edgeIndexCount(), network->edgeInstanceCount());
}

//=============================================================================================================

void BrainRenderer::issuePolylineDraw(QRhiCommandBuffer* cb, PolylineObject* polyline, int uniformOffset)
{
    if (!polyline || !polyline->isVisible() || !polyline->hasData())
        return;
    d->issueInstanced(cb, uniformOffset, polyline->vertexBuffer(), polyline->instanceBuffer(),
                      polyline->indexBuffer(), polyline->indexCount(), polyline->instanceCount());
}

} // namespace DISP3DLIB
