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
#include "renderable/electrodeobject.h"
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
#include <unordered_map>

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

    // Copy the frame's uniforms written so far into the uniform buffer, growing it before the first pass
    void flushUniforms(QRhiResourceUpdateBatch* u);

    // ── GPU copies of renderable streams; renderables keep only CPU data ──
    static constexpr int kMaxStreams = 4;
    struct GpuMesh
    {
        std::weak_ptr<const void> owner;
        std::array<std::unique_ptr<QRhiBuffer>, kMaxStreams> buffers;
        std::array<quint64, kMaxStreams> generations{};
        quint64 lastUsedFrame = 0;
    };
    GpuMesh& mesh(const RenderKey& key);
    const GpuMesh* findMesh(const RenderKey& key) const;
    QRhiBuffer* upload(GpuMesh& mesh, int slot, QRhiBuffer::UsageFlags usage, const GpuStream& stream, QRhiResourceUpdateBatch* u);
    bool uploadSurface(BrainSurface* surface, QRhiResourceUpdateBatch* u);
    bool uploadInstanced(const RenderKey& key, const InstancedStreams& streams, QRhiResourceUpdateBatch* u);
    void bindSurface(QRhiCommandBuffer* cb, const GpuMesh& mesh);
    void issueInstanced(QRhiCommandBuffer* cb, int uniformOffset, const RenderKey& key, const InstancedStreams& streams,
                        QRhiGraphicsPipeline* pipeline);
    int writeInstancedUniforms(const SceneData& data, const char* caller);

    QRhi* rhi = nullptr;
    std::unordered_map<const void*, GpuMesh> meshes;
    quint64 frameIndex = 0;

    bool videoOverlayUsesDepth(const VideoOverlay* overlay) const
    {
        return overlay->isDepthEnabled() && overlay->hasDepthFrame() && videoOverlay.depthInitialized && videoOverlay.surfaceDepthPipeline;
    }

    std::unique_ptr<QRhiShaderResourceBindings> srb;

    // Pipelines for each mode — indexed by ShaderMode enum (O(1) lookup)
    std::array<std::unique_ptr<QRhiGraphicsPipeline>, kNumShaderModes> pipelines{};
    std::array<std::unique_ptr<QRhiGraphicsPipeline>, kNumShaderModes> pipelinesBackColor{};
    std::unique_ptr<QRhiGraphicsPipeline> electrodePipeline;

    std::unique_ptr<QRhiBuffer> uniformBuffer;
    // Whole bound block, not ubufAlignment(): that is 1 on GL, where slots would overlap
    int uniformSlotStride = 0;
    int currentUniformOffset = 0;
    QByteArray uniformShadow; // The frame's per-draw uniforms, uploaded in one piece
    int flushedUniformBytes = 0;
    bool overflowReported = false; // Reset per frame so the log gets one line, not one per skipped draw
    bool passRecording = false;
    bool passBegunThisFrame = false;

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

// Surface streams: geometry (position, normal), colour, annotation colour
static QRhiVertexInputLayout surfaceInputLayout()
{
    QRhiVertexInputLayout il;
    il.setBindings({{sizeof(VertexData)}, {sizeof(uint32_t)}, {sizeof(uint32_t)}});
    il.setAttributes({{0, 0, QRhiVertexInputAttribute::Float3, 0},
                      {0, 1, QRhiVertexInputAttribute::Float3, 12},
                      {1, 2, QRhiVertexInputAttribute::UNormByte4, 0},
                      {2, 3, QRhiVertexInputAttribute::UNormByte4, 0}});
    return il;
}

// Instanced glyphs: unit mesh (position, normal) plus a model matrix, RGBA colour and selected flag per instance
static QRhiVertexInputLayout instancedInputLayout()
{
    QRhiVertexInputLayout il;
    il.setBindings({{6 * sizeof(float)}, {21 * sizeof(float), QRhiVertexInputBinding::PerInstance}});
    il.setAttributes({{0, 0, QRhiVertexInputAttribute::Float3, 0},
                      {0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float)},
                      {1, 2, QRhiVertexInputAttribute::Float4, 0},
                      {1, 3, QRhiVertexInputAttribute::Float4, 4 * sizeof(float)},
                      {1, 4, QRhiVertexInputAttribute::Float4, 8 * sizeof(float)},
                      {1, 5, QRhiVertexInputAttribute::Float4, 12 * sizeof(float)},
                      {1, 6, QRhiVertexInputAttribute::Float4, 16 * sizeof(float)},
                      {1, 7, QRhiVertexInputAttribute::Float, 20 * sizeof(float)}});
    return il;
}

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
// Uniform buffer sizing: grows by doubling up to the maximum number of draws per frame
constexpr int kInitialUniformSlots = 256;
constexpr int kMaxUniformSlots = 16384;
constexpr int kUniformBlockSize = 256;                     // Bound size per SRB dynamic slot (bytes)
constexpr quint64 kEvictAfterFrames = 300;                 // GPU copies of renderables not drawn for this long are freed
constexpr int kVideoOverlaySlots = 64;                     // Video overlay draws per frame
constexpr quint32 kVideoQuadBytes = 4 * 5 * sizeof(float); // 4 verts × (3 pos + 2 uv)

// Per-object uniform byte offsets (must match .vert shader layout).
// Kept as documentation of the UBO layout even where not directly indexed.
[[maybe_unused]] constexpr int kOffsetMVP = 0;             // mat4  (64 bytes)
[[maybe_unused]] constexpr int kOffsetCameraPos = 64;      // vec3  (12 bytes)
[[maybe_unused]] constexpr int kOffsetSelected = 76;       // float
[[maybe_unused]] constexpr int kOffsetLightDir = 80;       // vec3  (12 bytes)
[[maybe_unused]] constexpr int kOffsetTissueType = 92;     // float
[[maybe_unused]] constexpr int kOffsetLighting = 96;       // float
[[maybe_unused]] constexpr int kOffsetOverlayMode = 100;   // float
[[maybe_unused]] constexpr int kOffsetLabelActive = 104;   // float
[[maybe_unused]] constexpr int kOffsetSelectedLabel = 112; // vec4  (16 bytes)
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
    // Once a pass has begun the buffer can no longer grow this frame
    const qint64 capacity = passBegunThisFrame ? qint64(uniformBuffer->size()) : qint64(kMaxUniformSlots) * uniformSlotStride;
    if (offset + uniformSlotStride > capacity) {
        if (!overflowReported) {
            qWarning("BrainRenderer: uniform buffer full at %s; further draws this frame are skipped", caller);
            overflowReported = true;
        }
        return -1;
    }
    currentUniformOffset += uniformSlotStride;
    if (uniformShadow.size() < currentUniformOffset)
        uniformShadow.resize(currentUniformOffset);
    return offset;
}

//=============================================================================================================

void BrainRenderer::Impl::flushUniforms(QRhiResourceUpdateBatch* u)
{
    if (currentUniformOffset <= flushedUniformBytes)
        return;
    if (quint32(currentUniformOffset) > uniformBuffer->size()) {
        int slotCount = int(uniformBuffer->size()) / uniformSlotStride;
        while (slotCount * uniformSlotStride < currentUniformOffset)
            slotCount *= 2;
        uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, quint32(slotCount * uniformSlotStride)));
        uniformBuffer->create();
        srb->setBindings({QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, uniformBuffer.get(), kUniformBlockSize)});
        srb->create();
        flushedUniformBytes = 0;
    }
    u->updateDynamicBuffer(uniformBuffer.get(), quint32(flushedUniformBytes), quint32(currentUniformOffset - flushedUniformBytes),
                           uniformShadow.constData() + flushedUniformBytes);
    flushedUniformBytes = currentUniformOffset;
}

//=============================================================================================================

BrainRenderer::Impl::GpuMesh& BrainRenderer::Impl::mesh(const RenderKey& key)
{
    GpuMesh& m = meshes[key.id()];
    // A new entry, or a stale one whose renderable died and whose address was reused
    if (m.owner.expired()) {
        m = GpuMesh{};
        m.owner = key.token();
        m.generations.fill(std::numeric_limits<quint64>::max());
    }
    m.lastUsedFrame = frameIndex;
    return m;
}

//=============================================================================================================

const BrainRenderer::Impl::GpuMesh* BrainRenderer::Impl::findMesh(const RenderKey& key) const
{
    const auto it = meshes.find(key.id());
    return (it == meshes.end() || it->second.owner.expired()) ? nullptr : &it->second;
}

//=============================================================================================================

QRhiBuffer* BrainRenderer::Impl::upload(GpuMesh& m, int slot, QRhiBuffer::UsageFlags usage, const GpuStream& stream, QRhiResourceUpdateBatch* u)
{
    auto& buffer = m.buffers[slot];
    if (!stream.data || stream.bytes == 0) {
        buffer.reset();
        return nullptr;
    }
    // Static buffers are updated in place; only a size change reallocates
    if (!buffer || buffer->size() != stream.bytes) {
        buffer.reset(rhi->newBuffer(QRhiBuffer::Static, usage, stream.bytes));
        buffer->create();
        m.generations[slot] = std::numeric_limits<quint64>::max();
    }
    if (m.generations[slot] != stream.generation) {
        u->uploadStaticBuffer(buffer.get(), 0, stream.bytes, stream.data);
        m.generations[slot] = stream.generation;
    }
    return buffer.get();
}

//=============================================================================================================

bool BrainRenderer::Impl::uploadSurface(BrainSurface* surface, QRhiResourceUpdateBatch* u)
{
    if (!surface || surface->indexCount() == 0)
        return false;
    GpuMesh& m = mesh(surface->renderKey());
    return upload(m, 0, QRhiBuffer::VertexBuffer, surface->geometryStream(), u) && upload(m, 1, QRhiBuffer::VertexBuffer, surface->colorStream(), u) && upload(m, 2, QRhiBuffer::VertexBuffer, surface->annotationStream(), u) && upload(m, 3, QRhiBuffer::IndexBuffer, surface->indexStream(), u);
}

//=============================================================================================================

bool BrainRenderer::Impl::uploadInstanced(const RenderKey& key, const InstancedStreams& streams, QRhiResourceUpdateBatch* u)
{
    if (streams.indexCount <= 0 || streams.instanceCount <= 0)
        return false;
    GpuMesh& m = mesh(key);
    return upload(m, 0, QRhiBuffer::VertexBuffer, streams.vertices, u) && upload(m, 1, QRhiBuffer::VertexBuffer, streams.instances, u) && upload(m, 2, QRhiBuffer::IndexBuffer, streams.indices, u);
}

//=============================================================================================================

void BrainRenderer::Impl::bindSurface(QRhiCommandBuffer* cb, const GpuMesh& m)
{
    const QRhiCommandBuffer::VertexInput inputs[3] = {{m.buffers[0].get(), 0}, {m.buffers[1].get(), 0}, {m.buffers[2].get(), 0}};
    cb->setVertexInput(0, 3, inputs, m.buffers[3].get(), 0, QRhiCommandBuffer::IndexUInt32);
}

//=============================================================================================================

void BrainRenderer::Impl::createResources(QRhi* newRhi, QRhiRenderPassDescriptor* rp, int sampleCount)
{
    rhi = newRhi;
    uniformSlotStride = rhi->ubufAligned(kUniformBlockSize);

    if (!uniformBuffer) {
        uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, kInitialUniformSlots * uniformSlotStride));
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

            p->setVertexInputLayout(mode == Dipole ? instancedInputLayout() : surfaceInputLayout());
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

    // Electrodes: opaque and depth-written, unlike the translucent dipole glyphs
    const QShader evS = getShader(QStringLiteral(":/electrode.vert.qsb"));
    const QShader efS = getShader(QStringLiteral(":/electrode.frag.qsb"));
    electrodePipeline.reset();
    if (evS.isValid() && efS.isValid()) {
        electrodePipeline.reset(rhi->newGraphicsPipeline());
        electrodePipeline->setShaderStages({{QRhiShaderStage::Vertex, evS}, {QRhiShaderStage::Fragment, efS}});
        electrodePipeline->setVertexInputLayout(instancedInputLayout());
        electrodePipeline->setShaderResourceBindings(srb.get());
        electrodePipeline->setRenderPassDescriptor(rp);
        electrodePipeline->setSampleCount(sampleCount);
        electrodePipeline->setCullMode(QRhiGraphicsPipeline::None);
        electrodePipeline->setDepthTest(true);
        electrodePipeline->setDepthWrite(true);
        electrodePipeline->setFlags(QRhiGraphicsPipeline::UsesScissor);
        electrodePipeline->create();
    } else {
        qWarning() << "BrainRenderer: Could not load the electrode shaders";
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
    d->flushedUniformBytes = 0;
    d->overflowReported = false;
    d->passBegunThisFrame = false;
    d->videoOverlay.currentUniformOffset = 0;
    d->sliceRes.currentUniformOffset = 0;

    ++d->frameIndex;
    for (auto it = d->meshes.begin(); it != d->meshes.end();) {
        if (it->second.owner.expired() || d->frameIndex - it->second.lastUsedFrame > kEvictAfterFrames)
            it = d->meshes.erase(it);
        else
            ++it;
    }
}

//=============================================================================================================

int BrainRenderer::cachedMeshCount() const
{
    return int(d->meshes.size());
}

//=============================================================================================================

void BrainRenderer::Impl::beginPass(QRhiCommandBuffer* cb, QRhiRenderTarget* rt, QRhiResourceUpdateBatch* updates)
{
    if (currentUniformOffset > flushedUniformBytes) {
        if (!updates)
            updates = rhi->nextResourceUpdateBatch();
        flushUniforms(updates);
    }
    cb->beginPass(rt, QColor(0, 0, 0), {1.0f, 0}, updates);
    passRecording = true;
    passBegunThisFrame = true;
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
        k.surfacePipeline->setVertexInputLayout(surfaceInputLayout());
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
                k.surfaceDepthPipeline->setVertexInputLayout(surfaceInputLayout());
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
    if (surface && !d->uploadSurface(surface, u))
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
        const Impl::GpuMesh* m = d->findMesh(surface->renderKey());
        if (!m)
            return;
        const bool useDepth = d->videoOverlayUsesDepth(overlay);
        cb->setGraphicsPipeline(useDepth ? k.surfaceDepthPipeline.get() : k.surfacePipeline.get());
        cb->setShaderResources(useDepth ? k.srbDepth.get() : k.srb.get(), 1, &srbOffset);
        d->bindSurface(cb, *m);
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
    if (!surface || !surface->isVisible() || d->insidePass("prepareSurfaceDraw"))
        return -1;
    if (!d->uploadSurface(surface, u))
        return -1;

    const int offset = d->claimUniformSlot("prepareSurfaceDraw");
    if (offset < 0)
        return -1;

    // A highlighted region or vertex range replaces the whole-surface glow
    const std::optional<uint32_t> label = surface->selectedLabel();

    struct
    {
        float mvp[16];          // 0..63
        float cameraPos[3];     // 64..75
        float isSelected;       // 76..79
        float lightDir[3];      // 80..91
        float tissueType;       // 92..95
        float lightingEnabled;  // 96..99
        float overlayMode;      // 100..103
        float labelActive;      // 104..107
        float pad;              // 108..111
        float selectedLabel[4]; // 112..127
    } ub;
    memcpy(ub.mvp, data.mvp.constData(), 64);
    memcpy(ub.cameraPos, &data.cameraPos, 12);
    ub.isSelected = (surface->isSelected() && !label) ? 1.0f : 0.0f;
    memcpy(ub.lightDir, &data.lightDir, 12);
    ub.tissueType = static_cast<float>(surface->tissueType());
    ub.lightingEnabled = data.lightingEnabled ? 1.0f : 0.0f;
    ub.overlayMode = data.overlayMode;
    ub.labelActive = label ? 1.0f : 0.0f;
    ub.pad = 0.0f;
    const uint32_t key = label.value_or(0u);
    for (int c = 0; c < 4; ++c)
        ub.selectedLabel[c] = float((key >> (8 * c)) & 0xFFu) / 255.0f;

    memcpy(d->uniformShadow.data() + offset, &ub, sizeof(ub));
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
    const Impl::GpuMesh* m = d->findMesh(surface->renderKey());
    if (!pipeline || !m)
        return;

    auto draw = [&](QRhiGraphicsPipeline* p) {
        cb->setGraphicsPipeline(p);
        const QRhiCommandBuffer::DynamicOffset srbOffset = {0, uint32_t(uniformOffset)};
        cb->setShaderResources(d->srb.get(), 1, &srbOffset);
        d->bindSurface(cb, *m);
        cb->drawIndexed(surface->indexCount());
    };

    if (mode == Holographic && d->pipelinesBackColor[Holographic]) {
        draw(d->pipelinesBackColor[Holographic].get());
    }

    draw(pipeline);
}

//=============================================================================================================

int BrainRenderer::Impl::writeInstancedUniforms(const SceneData& data, const char* caller)
{
    const int offset = claimUniformSlot(caller);
    if (offset < 0)
        return -1;

    struct
    {
        float mvp[16];         // 0..63
        float cameraPos[3];    // 64..75
        float _pad0;           // 76..79
        float lightDir[3];     // 80..91
        float lightingEnabled; // 92..95: std140 packs a float into the vec3's last slot
    } ub;
    memcpy(ub.mvp, data.mvp.constData(), 64);
    memcpy(ub.cameraPos, &data.cameraPos, 12);
    ub._pad0 = 0.0f;
    memcpy(ub.lightDir, &data.lightDir, 12);
    ub.lightingEnabled = data.lightingEnabled ? 1.0f : 0.0f;
    memcpy(uniformShadow.data() + offset, &ub, sizeof(ub));
    return offset;
}

//=============================================================================================================

int BrainRenderer::prepareDipoleDraw(QRhiResourceUpdateBatch* u, const SceneData& data, DipoleObject* dipoles)
{
    if (!dipoles || !dipoles->isVisible() || d->insidePass("prepareDipoleDraw"))
        return -1;
    if (!d->uploadInstanced(dipoles->renderKey(), dipoles->streams(), u))
        return -1;
    return d->writeInstancedUniforms(data, "prepareDipoleDraw");
}

//=============================================================================================================

int BrainRenderer::prepareNetworkDraw(QRhiResourceUpdateBatch* u, const SceneData& data, NetworkObject* network)
{
    if (!network || !network->isVisible() || !network->hasData() || d->insidePass("prepareNetworkDraw"))
        return -1;
    const bool nodes = d->uploadInstanced(network->nodeRenderKey(), network->nodeStreams(), u);
    const bool edges = d->uploadInstanced(network->edgeRenderKey(), network->edgeStreams(), u);
    if (!nodes && !edges)
        return -1;
    return d->writeInstancedUniforms(data, "prepareNetworkDraw");
}

//=============================================================================================================

int BrainRenderer::preparePolylineDraw(QRhiResourceUpdateBatch* u, const SceneData& data, PolylineObject* polyline)
{
    if (!polyline || !polyline->isVisible() || !polyline->hasData() || d->insidePass("preparePolylineDraw"))
        return -1;
    if (!d->uploadInstanced(polyline->renderKey(), polyline->streams(), u))
        return -1;
    return d->writeInstancedUniforms(data, "preparePolylineDraw");
}

//=============================================================================================================

int BrainRenderer::prepareElectrodeDraw(QRhiResourceUpdateBatch* u, const SceneData& data, ElectrodeObject* electrodes)
{
    if (!electrodes || !electrodes->isVisible() || d->insidePass("prepareElectrodeDraw"))
        return -1;
    const bool bodies = d->uploadInstanced(electrodes->bodyRenderKey(), electrodes->bodyStreams(), u);
    const bool contacts = d->uploadInstanced(electrodes->contactRenderKey(), electrodes->contactStreams(), u);
    if (!bodies && !contacts)
        return -1;
    return d->writeInstancedUniforms(data, "prepareElectrodeDraw");
}

//=============================================================================================================

void BrainRenderer::issueElectrodeDraw(QRhiCommandBuffer* cb, ElectrodeObject* electrodes, int uniformOffset)
{
    if (!electrodes || !electrodes->isVisible())
        return;
    QRhiGraphicsPipeline* pipeline = d->electrodePipeline.get();
    d->issueInstanced(cb, uniformOffset, electrodes->bodyRenderKey(), electrodes->bodyStreams(), pipeline);
    d->issueInstanced(cb, uniformOffset, electrodes->contactRenderKey(), electrodes->contactStreams(), pipeline);
}

//=============================================================================================================

void BrainRenderer::Impl::issueInstanced(QRhiCommandBuffer* cb,
                                         int uniformOffset,
                                         const RenderKey& key,
                                         const InstancedStreams& streams,
                                         QRhiGraphicsPipeline* pipeline)
{
    const GpuMesh* m = findMesh(key);
    if (!pipeline || !m || uniformOffset < 0 || streams.indexCount <= 0 || streams.instanceCount <= 0)
        return;

    cb->setGraphicsPipeline(pipeline);
    const QRhiCommandBuffer::DynamicOffset srbOffset = {0, uint32_t(uniformOffset)};
    cb->setShaderResources(srb.get(), 1, &srbOffset);
    const QRhiCommandBuffer::VertexInput bindings[2] = {{m->buffers[0].get(), 0}, {m->buffers[1].get(), 0}};
    cb->setVertexInput(0, 2, bindings, m->buffers[2].get(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(quint32(streams.indexCount), quint32(streams.instanceCount));
}

//=============================================================================================================

void BrainRenderer::issueDipoleDraw(QRhiCommandBuffer* cb, DipoleObject* dipoles, int uniformOffset)
{
    if (dipoles && dipoles->isVisible())
        d->issueInstanced(cb, uniformOffset, dipoles->renderKey(), dipoles->streams(), d->pipelines[Dipole].get());
}

//=============================================================================================================

void BrainRenderer::issueNetworkDraw(QRhiCommandBuffer* cb, NetworkObject* network, int uniformOffset)
{
    if (!network || !network->isVisible() || !network->hasData())
        return;
    d->issueInstanced(cb, uniformOffset, network->nodeRenderKey(), network->nodeStreams(), d->pipelines[Dipole].get());
    d->issueInstanced(cb, uniformOffset, network->edgeRenderKey(), network->edgeStreams(), d->pipelines[Dipole].get());
}

//=============================================================================================================

void BrainRenderer::issuePolylineDraw(QRhiCommandBuffer* cb, PolylineObject* polyline, int uniformOffset)
{
    if (polyline && polyline->isVisible() && polyline->hasData())
        d->issueInstanced(cb, uniformOffset, polyline->renderKey(), polyline->streams(), d->pipelines[Dipole].get());
}

} // namespace DISP3DLIB
