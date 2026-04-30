//=============================================================================================================
/**
 * @file     brainrenderer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    2.0.0
 * @date     January, 2026
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
 * @brief    BrainRenderer class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainrenderer.h"

#include <rhi/qrhi.h>
#include "renderable/brainsurface.h"
#include "renderable/dipoleobject.h"
#include "renderable/networkobject.h"
#include "renderable/videooverlay.h"

using DISP3DLIB::VideoOverlay;

#include <QFile>
#include <QDebug>
#include <QImage>
#include <QVector3D>
#include <array>
#include <limits>
#include <map>
#include <cstring>

//=============================================================================================================
// PIMPL
//=============================================================================================================

static constexpr int kNumShaderModes = 6; // Standard..ShowNormals

struct BrainRenderer::Impl
{
    void createResources(QRhi *rhi, QRhiRenderPassDescriptor *rp, int sampleCount);

    std::unique_ptr<QRhiShaderResourceBindings> srb;

    // Pipelines for each mode — indexed by ShaderMode enum (O(1) lookup)
    std::array<std::unique_ptr<QRhiGraphicsPipeline>, kNumShaderModes> pipelines{};
    std::array<std::unique_ptr<QRhiGraphicsPipeline>, kNumShaderModes> pipelinesBackColor{};

    std::unique_ptr<QRhiBuffer> uniformBuffer;
    int uniformBufferOffsetAlignment = 0;
    int currentUniformOffset = 0;

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
    QRhiTexture *rtColorTex = nullptr;  // Track texture pointer for rebuild

    // ── WORKAROUND(QRhi-GLES2): merged single-drawIndexed buffers ────
    // Used on WASM to avoid the multi-drawIndexed bug in QRhi's GLES2
    // backend.  Each surface category (brain, BEM, sensors, etc.) gets
    // its own merged buffer set, drawn in separate render passes.
    // Remove when upstream Qt fixes the issue.
    struct MergedGroup {
        QVector<BrainSurface*> surfaces;
        std::unique_ptr<QRhiBuffer> vertexBuffer;
        std::unique_ptr<QRhiBuffer> indexBuffer;
        int indexCount = 0;
        int totalVertexCount = 0; // cached vertex count from last full rebuild
        bool dirty = true;  // Geometry needs rebuild (surface list changed)
        bool gpuVertexDirty = true; // Vertex data changed, needs GPU re-upload
        bool gpuIndexDirty  = true; // Index data changed, needs GPU re-upload
        QByteArray vertexRaw;
        QByteArray indexRaw;
        QVector<quint64> surfaceGenerations; // per-surface vertex generation snapshot
    };
    std::map<QString, MergedGroup> mergedGroups;  // keyed by category name

    // ── Generic video overlay ───────────────────────────────────────
    // Camera-facing textured quad rendered last (depth test off) at the
    // current focus point. Resources are created lazily on first use.
    struct VideoOverlayResources {
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        std::unique_ptr<QRhiGraphicsPipeline> surfacePipeline;
        std::unique_ptr<QRhiShaderResourceBindings> srb;
        std::unique_ptr<QRhiBuffer> uniformBuffer;
        std::unique_ptr<QRhiBuffer> vertexBuffer;
        std::unique_ptr<QRhiBuffer> indexBuffer;
        std::unique_ptr<QRhiTexture> texture;
        std::unique_ptr<QRhiSampler> sampler;
        QSize textureSize;
        int uniformBufferOffsetAlignment = 0;
        int currentUniformOffset = 0;
        quint64 uploadedFrameGen = std::numeric_limits<quint64>::max();
        bool indexUploaded = false;
        bool initialized = false;
    };
    VideoOverlayResources videoOverlay;
};

//=============================================================================================================
// Helpers
//=============================================================================================================

static inline QRhiViewport toViewport(const BrainRenderer::SceneData &d)
{
    return QRhiViewport(d.viewportX, d.viewportY, d.viewportW, d.viewportH);
}

static inline QRhiScissor toScissor(const BrainRenderer::SceneData &d)
{
    return QRhiScissor(d.scissorX, d.scissorY, d.scissorW, d.scissorH);
}

static QImage tightlyPackedRgba(const QImage &source)
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

namespace {
    // Uniform buffer sizing
    constexpr int kUniformSlotCount   = 8192;   // Max draw calls before overflow
    constexpr int kUniformBlockSize   = 256;    // Bound size per SRB dynamic slot (bytes)

    // Per-object uniform byte offsets (must match .vert shader layout)
    constexpr int kOffsetMVP          = 0;      // mat4  (64 bytes)
    constexpr int kOffsetCameraPos    = 64;     // vec3  (12 bytes)
    constexpr int kOffsetSelected     = 76;     // float
    constexpr int kOffsetLightDir     = 80;     // vec3  (12 bytes)
    constexpr int kOffsetTissueType   = 92;     // float
    constexpr int kOffsetLighting     = 96;     // float
    constexpr int kOffsetOverlayMode  = 100;    // float
    constexpr int kOffsetSelectedSurfaceId = 104; // float — WORKAROUND(QRhi-GLES2)
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

void BrainRenderer::initialize(QRhi *rhi, QRhiRenderPassDescriptor *rp, int sampleCount)
{
    if (d->resourcesDirty) {
        d->createResources(rhi, rp, sampleCount);
    }
}

//=============================================================================================================

void BrainRenderer::Impl::createResources(QRhi *rhi, QRhiRenderPassDescriptor *rp, int sampleCount)
{
    uniformBufferOffsetAlignment = rhi->ubufAlignment();
    
    // Create Uniform Buffer
    if (!uniformBuffer) {
        // Size for 8192 slots with alignment — enough for 4 viewports × ~1000 surfaces
        uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, kUniformSlotCount * uniformBufferOffsetAlignment));
        uniformBuffer->create();
    }
    
    // Create SRB
    if (!srb) {
        srb.reset(rhi->newShaderResourceBindings());
        srb->setBindings({
            // Use dynamic offset for the uniform buffer. 
            // The size of one uniform block in the shader is ~104 bytes, 
            // but we use uniformBufferOffsetAlignment for the stride.
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, uniformBuffer.get(),kUniformBlockSize)
        });
        srb->create();
    }
    
    // Shader Loader
    auto getShader = [](const QString &name) {
        QFile f(name);
        return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
    };
    
    // List of modes to initialize
    QList<ShaderMode> modes = {Standard, Holographic, Anatomical, Dipole, XRay, ShowNormals};
    
    for (ShaderMode mode : modes) {
        QString vert = (mode == Holographic || mode == XRay) ? ":/holographic.vert.qsb" : 
                       (mode == Anatomical) ? ":/anatomical.vert.qsb" : 
                       (mode == Dipole) ? ":/dipole.vert.qsb" :
                       (mode == ShowNormals) ? ":/shownormals.vert.qsb" : ":/standard.vert.qsb";
        
        QString frag = (mode == Holographic || mode == XRay) ? ":/holographic.frag.qsb" : 
                       (mode == Anatomical) ? ":/anatomical.frag.qsb" : 
                       (mode == Dipole) ? ":/dipole.frag.qsb" :
                       (mode == ShowNormals) ? ":/shownormals.frag.qsb" : ":/standard.frag.qsb";
        
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
            p->setShaderStages({{ QRhiShaderStage::Vertex, vS }, { QRhiShaderStage::Fragment, fS }});
            
            QRhiVertexInputLayout il;
            
            if (mode == Dipole) {
                il.setBindings({
                    { 6 * sizeof(float) },       // Binding 0: Vertex Data (Pos + Normal) -> stride 6 floats
                    { 21 * sizeof(float), QRhiVertexInputBinding::PerInstance } // Binding 1: Instance Data (Mat4 + Color + Selected) -> stride 21 floats
                });
                
                il.setAttributes({
                    // Vertex Buffer (Binding 0)
                    { 0, 0, QRhiVertexInputAttribute::Float3, 0 },                   // Pos
                    { 0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float) },   // Normal
                    
                    // Instance Buffer (Binding 1)
                    // Model Matrix (4 x vec4)
                    { 1, 2, QRhiVertexInputAttribute::Float4, 0 },
                    { 1, 3, QRhiVertexInputAttribute::Float4, 4 * sizeof(float) },
                    { 1, 4, QRhiVertexInputAttribute::Float4, 8 * sizeof(float) },
                    { 1, 5, QRhiVertexInputAttribute::Float4, 12 * sizeof(float) },
                    // Color
                    { 1, 6, QRhiVertexInputAttribute::Float4, 16 * sizeof(float) },
                    // isSelected
                    { 1, 7, QRhiVertexInputAttribute::Float, 20 * sizeof(float) }
                });
            } else {
                il.setBindings({{ 36 }});  // sizeof(VertexData) = 36 with surfaceId
                il.setAttributes({{ 0, 0, QRhiVertexInputAttribute::Float3, 0 }, 
                                  { 0, 1, QRhiVertexInputAttribute::Float3, 12 }, 
                                  { 0, 2, QRhiVertexInputAttribute::UNormByte4, 24 },
                                  { 0, 3, QRhiVertexInputAttribute::UNormByte4, 28 },
                                  { 0, 4, QRhiVertexInputAttribute::Float, 32 }});   // surfaceId
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

void BrainRenderer::ensureRenderTargets(QRhi *rhi, QRhiTexture *colorTex, const QSize &pixelSize)
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
        QRhiTextureRenderTarget::PreserveColorContents
        | QRhiTextureRenderTarget::PreserveDepthStencilContents));
    d->rpPreserve.reset(d->rtPreserve->newCompatibleRenderPassDescriptor());
    d->rtPreserve->setRenderPassDescriptor(d->rpPreserve.get());
    d->rtPreserve->create();
}

//=============================================================================================================

QRhiRenderTarget *BrainRenderer::rtClear() const
{
    return d->rtClear.get();
}

QRhiRenderTarget *BrainRenderer::rtPreserve() const
{
    return d->rtPreserve.get();
}

//=============================================================================================================

void BrainRenderer::beginFrame(QRhiCommandBuffer *cb)
{
    d->currentUniformOffset = 0;

    auto *rt = d->rtClear.get();
    cb->beginPass(rt, QColor(0, 0, 0), { 1.0f, 0 });
    const int w = rt->pixelSize().width();
    const int h = rt->pixelSize().height();
    cb->setViewport(QRhiViewport(0, 0, w, h));
    cb->setScissor(QRhiScissor(0, 0, w, h));
}

//=============================================================================================================

void BrainRenderer::updateSceneUniforms(QRhi *rhi, const SceneData &data)
{
    // NO-OP: packed into per-object slots for simplicity
}

//=============================================================================================================

void BrainRenderer::beginPreservingPass(QRhiCommandBuffer *cb)
{
    auto *rt = d->rtPreserve.get();
    cb->beginPass(rt, QColor(0, 0, 0), { 1.0f, 0 });
    const int w = rt->pixelSize().width();
    const int h = rt->pixelSize().height();
    cb->setViewport(QRhiViewport(0, 0, w, h));
    cb->setScissor(QRhiScissor(0, 0, w, h));
}

//=============================================================================================================

void BrainRenderer::endPass(QRhiCommandBuffer *cb)
{
    cb->endPass();
}

//=============================================================================================================
// Video overlay rendering
//=============================================================================================================

void BrainRenderer::prepareVideoOverlay(QRhi *rhi,
                                         QRhiResourceUpdateBatch *u,
                                         VideoOverlay *overlay)
{
    if (!overlay || !overlay->isEnabled()) return;
    if (!overlay->hasFrame()) return;
    if (!rhi || !u) return;

    QImage frame = tightlyPackedRgba(overlay->frame());
    if (frame.isNull()) return;

    auto &k = d->videoOverlay;
    k.currentUniformOffset = 0;

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
        constexpr int kUbSize = 256;
        k.uniformBufferOffsetAlignment = rhi->ubufAlignment();
        k.uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                             QRhiBuffer::UniformBuffer,
                                             64 * k.uniformBufferOffsetAlignment));
        k.uniformBuffer->create();

        // Vertex buffer: 4 vertices × (3 pos + 2 uv) floats — refilled each frame
        constexpr int kVbSize = 4 * 5 * sizeof(float);
        k.vertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                            QRhiBuffer::VertexBuffer, kVbSize));
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
        k.srb->setBindings({
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0,
                QRhiShaderResourceBinding::VertexStage |
                QRhiShaderResourceBinding::FragmentStage,
                k.uniformBuffer.get(), kUniformBlockSize),
            QRhiShaderResourceBinding::sampledTexture(1,
                QRhiShaderResourceBinding::FragmentStage,
                k.texture.get(), k.sampler.get())
        });
        k.srb->create();

        // Pipeline (alpha-blended, depth-test off so it sits on top)
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

        k.pipeline.reset(rhi->newGraphicsPipeline());
        k.pipeline->setShaderStages({
            { QRhiShaderStage::Vertex, vShader },
            { QRhiShaderStage::Fragment, fShader }
        });
        QRhiVertexInputLayout il;
        il.setBindings({{ 5 * sizeof(float) }});
        il.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
            { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) }
        });
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
        k.surfacePipeline->setShaderStages({
            { QRhiShaderStage::Vertex, dvShader },
            { QRhiShaderStage::Fragment, dfShader }
        });
        QRhiVertexInputLayout dil;
        dil.setBindings({{ 36 }});
        dil.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
            { 0, 1, QRhiVertexInputAttribute::Float3, 12 },
            { 0, 2, QRhiVertexInputAttribute::UNormByte4, 24 },
            { 0, 3, QRhiVertexInputAttribute::UNormByte4, 28 },
            { 0, 4, QRhiVertexInputAttribute::Float, 32 }
        });
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

    // ── Index buffer (one-shot upload) ──────────────────────────────
    if (!k.indexUploaded) {
        const quint32 idx[6] = { 0, 1, 2,  2, 1, 3 };
        u->uploadStaticBuffer(k.indexBuffer.get(), idx);
        k.indexUploaded = true;
    }

    // ── Texture (re-create on size change, re-upload on new frame) ──
    if (frame.size() != k.textureSize) {
        k.texture.reset(rhi->newTexture(QRhiTexture::RGBA8, frame.size()));
        k.texture->create();
        k.textureSize = frame.size();
        // Rebuild SRB to point at the new texture
        k.srb->setBindings({
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0,
                QRhiShaderResourceBinding::VertexStage |
                QRhiShaderResourceBinding::FragmentStage,
                k.uniformBuffer.get(), kUniformBlockSize),
            QRhiShaderResourceBinding::sampledTexture(1,
                QRhiShaderResourceBinding::FragmentStage,
                k.texture.get(), k.sampler.get())
        });
        k.srb->create();
        k.uploadedFrameGen = std::numeric_limits<quint64>::max();
    }
    if (overlay->frameGeneration() != k.uploadedFrameGen) {
        QRhiTextureSubresourceUploadDescription sub(frame);
        QRhiTextureUploadDescription desc({ 0, 0, sub });
        u->uploadTexture(k.texture.get(), desc);
        k.uploadedFrameGen = overlay->frameGeneration();
    }
}

void BrainRenderer::renderVideoOverlay(QRhiCommandBuffer *cb, QRhi *rhi,
                                        const SceneData &data,
                                        VideoOverlay *overlay)
{
    if (!overlay || !overlay->isEnabled()) return;
    if (!overlay->hasFrame()) return;

    auto &k = d->videoOverlay;
    if (!k.initialized) return;
    if (k.uniformBufferOffsetAlignment <= 0) return;

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    const int uniformOffset = k.currentUniformOffset;
    k.currentUniformOffset += k.uniformBufferOffsetAlignment;
    if (uniformOffset + kUniformBlockSize > k.uniformBuffer->size()) return;

    // ── Billboard the quad to face the camera ──────────────────────
    const QVector3D centre = overlay->focusPosition();
    const float halfSize = 0.5f * overlay->sizeMeters();

    QVector3D viewDir = centre - data.cameraPos;
    if (viewDir.lengthSquared() < 1e-12f) viewDir = QVector3D(0, 0, -1);
    viewDir.normalize();

    QVector3D worldUp(0.0f, 0.0f, 1.0f);
    if (std::abs(QVector3D::dotProduct(viewDir, worldUp)) > 0.95f) {
        worldUp = QVector3D(0.0f, 1.0f, 0.0f);
    }
    QVector3D right = QVector3D::crossProduct(viewDir, worldUp).normalized();
    QVector3D up = QVector3D::crossProduct(right, viewDir).normalized();

    const QVector3D c00 = centre - right * halfSize - up * halfSize;
    const QVector3D c10 = centre + right * halfSize - up * halfSize;
    const QVector3D c01 = centre - right * halfSize + up * halfSize;
    const QVector3D c11 = centre + right * halfSize + up * halfSize;

    const float verts[4 * 5] = {
        c00.x(), c00.y(), c00.z(),  0.0f, 1.0f,   // image y is flipped vs uv
        c10.x(), c10.y(), c10.z(),  1.0f, 1.0f,
        c01.x(), c01.y(), c01.z(),  0.0f, 0.0f,
        c11.x(), c11.y(), c11.z(),  1.0f, 0.0f
    };
    u->updateDynamicBuffer(k.vertexBuffer.get(), 0, sizeof(verts), verts);

    // ── Uniforms (mat4 mvp, vec4 borderColor, float opacity, 3 pad) ─
    struct {
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

    cb->resourceUpdate(u);

    // ── Draw ────────────────────────────────────────────────────────
    cb->setViewport(toViewport(data));
    cb->setScissor(toScissor(data));
    cb->setGraphicsPipeline(k.pipeline.get());
    const QRhiCommandBuffer::DynamicOffset srbOffset = { 0, uint32_t(uniformOffset) };
    cb->setShaderResources(k.srb.get(), 1, &srbOffset);
    const QRhiCommandBuffer::VertexInput vbuf(k.vertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &vbuf, k.indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(6);
}

void BrainRenderer::renderVideoOverlayOnSurface(QRhiCommandBuffer *cb, QRhi *rhi,
                                                const SceneData &data,
                                                VideoOverlay *overlay,
                                                BrainSurface *surface)
{
    Q_UNUSED(rhi);
    if (!overlay || !overlay->isEnabled() || !overlay->hasFrame()) return;
    if (!surface || !surface->isVisible()) return;

    auto &k = d->videoOverlay;
    if (!k.initialized || !k.surfacePipeline) return;
    if (k.uniformBufferOffsetAlignment <= 0) return;

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    const int uniformOffset = k.currentUniformOffset;
    k.currentUniformOffset += k.uniformBufferOffsetAlignment;
    if (uniformOffset + kUniformBlockSize > k.uniformBuffer->size()) return;

    // Keep the decal attached to the same head-space location and orientation
    // in every viewport. Approximate the local scalp normal from the current
    // focus vector; later this can be replaced by a microscope/fiducial normal.
    QVector3D localNormal = overlay->focusPosition();
    if (localNormal.lengthSquared() < 1e-12f) {
        localNormal = QVector3D(0.0f, 0.0f, 1.0f);
    }
    localNormal.normalize();

    QVector3D referenceUp(0.0f, 0.0f, 1.0f);
    if (std::abs(QVector3D::dotProduct(localNormal, referenceUp)) > 0.95f) {
        referenceUp = QVector3D(1.0f, 0.0f, 0.0f);
    }
    const QVector3D axisU = QVector3D::crossProduct(referenceUp, localNormal).normalized();
    const QVector3D axisV = QVector3D::crossProduct(localNormal, axisU).normalized();

    struct {
        float mvp[16];
        float focusAndSize[4];
        float axisUAndOpacity[4];
        float axisVAndOffset[4];
        float axisNAndDepth[4];
        float cameraPosAndFacing[4];
        float borderColor[4];
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
    ub.cameraPosAndFacing[3] = 0.0f;
    ub.borderColor[0] = 0.0f;
    ub.borderColor[1] = 0.85f;
    ub.borderColor[2] = 1.0f;
    ub.borderColor[3] = 1.0f;
    u->updateDynamicBuffer(k.uniformBuffer.get(), uniformOffset, sizeof(ub), &ub);
    cb->resourceUpdate(u);

    cb->setViewport(toViewport(data));
    cb->setScissor(toScissor(data));
    cb->setGraphicsPipeline(k.surfacePipeline.get());
    const QRhiCommandBuffer::DynamicOffset srbOffset = { 0, uint32_t(uniformOffset) };
    cb->setShaderResources(k.srb.get(), 1, &srbOffset);
    const QRhiCommandBuffer::VertexInput vbuf(surface->vertexBuffer(), 0);
    cb->setVertexInput(0, 1, &vbuf, surface->indexBuffer(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(surface->indexCount());
}

//=============================================================================================================

void BrainRenderer::renderSurface(QRhiCommandBuffer *cb, QRhi *rhi, const SceneData &data, BrainSurface *surface, ShaderMode mode)
{
    if (!surface || !surface->isVisible()) return;

    auto *pipeline = d->pipelines[mode].get();
    if (!pipeline) return;

    // NOTE: Buffer uploads are handled in the pre-render phase
    // (BrainView::render pre-upload loop).  Do not call
    // surface->updateBuffers() here — it would allocate a redundant
    // QRhiResourceUpdateBatch per surface.

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();

    // Dynamic slot update
    int offset = d->currentUniformOffset;
    d->currentUniformOffset += d->uniformBufferOffsetAlignment;
    if (d->currentUniformOffset >= d->uniformBuffer->size()) {
        qWarning("BrainRenderer: uniform buffer overflow (%d / %d bytes) — too many surfaces. Some draws will be skipped.",
                 d->currentUniformOffset, (int)d->uniformBuffer->size());
        return;  // Skip this draw rather than silently corrupt earlier viewport data
    }

    // On desktop, when a specific annotation region or vertex range is
    // selected the CPU vertex-color gold tint (in updateVertexColors)
    // provides per-region feedback.  Suppress the shader's whole-surface
    // gold glow so it doesn't drown out the region highlight.
    float selected = surface->isSelected() ? 1.0f : 0.0f;
#ifndef __EMSCRIPTEN__
    if (surface->isSelected()
        && (surface->selectedRegionId() != -1 || surface->selectedVertexStart() >= 0)) {
        selected = 0.0f;
    }
#endif

    // Pack ALL uniforms into a contiguous block for a single upload
    struct {
        float mvp[16];          // 0..63
        float cameraPos[3];     // 64..75
        float isSelected;       // 76..79
        float lightDir[3];      // 80..91
        float tissueType;       // 92..95
        float lightingEnabled;  // 96..99
        float overlayMode;      // 100..103
        float selectedSurfaceId;// 104..107
    } ub;
    memcpy(ub.mvp, data.mvp.constData(), 64);
    memcpy(ub.cameraPos, &data.cameraPos, 12);
    ub.isSelected = selected;
    memcpy(ub.lightDir, &data.lightDir, 12);
    ub.tissueType = static_cast<float>(surface->tissueType());
    ub.lightingEnabled = data.lightingEnabled ? 1.0f : 0.0f;
    ub.overlayMode = data.overlayMode;
    ub.selectedSurfaceId = -1.0f;  // Per-surface path: surfaceId selection disabled

    u->updateDynamicBuffer(d->uniformBuffer.get(), offset, sizeof(ub), &ub);

    cb->resourceUpdate(u);

    // Re-assert the per-pane viewport and scissor after resourceUpdate.
    // The scissor provides a hard pixel clip that guarantees no cross-pane
    // bleeding, regardless of Metal render-encoder restarts.
    cb->setViewport(toViewport(data));
    cb->setScissor(toScissor(data));

    auto draw = [&](QRhiGraphicsPipeline *p) {
        cb->setGraphicsPipeline(p);
        
        const QRhiCommandBuffer::DynamicOffset srbOffset = { 0, uint32_t(offset) };
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

int BrainRenderer::prepareSurfaceDraw(QRhiResourceUpdateBatch *u,
                                       const SceneData &data,
                                       BrainSurface *surface)
{
    if (!surface || !surface->isVisible()) return -1;

    int offset = d->currentUniformOffset;
    d->currentUniformOffset += d->uniformBufferOffsetAlignment;
    if (d->currentUniformOffset >= d->uniformBuffer->size()) {
        qWarning("BrainRenderer: uniform buffer overflow in prepareSurfaceDraw");
        return -1;
    }

    float selected = surface->isSelected() ? 1.0f : 0.0f;
    if (surface->isSelected()
        && (surface->selectedRegionId() != -1 || surface->selectedVertexStart() >= 0)) {
        selected = 0.0f;
    }

    struct {
        float mvp[16];
        float cameraPos[3];
        float isSelected;
        float lightDir[3];
        float tissueType;
        float lightingEnabled;
        float overlayMode;
        float selectedSurfaceId;
    } ub;
    memcpy(ub.mvp, data.mvp.constData(), 64);
    memcpy(ub.cameraPos, &data.cameraPos, 12);
    ub.isSelected = selected;
    memcpy(ub.lightDir, &data.lightDir, 12);
    ub.tissueType = static_cast<float>(surface->tissueType());
    ub.lightingEnabled = data.lightingEnabled ? 1.0f : 0.0f;
    ub.overlayMode = data.overlayMode;
    ub.selectedSurfaceId = -1.0f;

    u->updateDynamicBuffer(d->uniformBuffer.get(), offset, sizeof(ub), &ub);
    return offset;
}

//=============================================================================================================

void BrainRenderer::issueSurfaceDraw(QRhiCommandBuffer *cb,
                                      BrainSurface *surface,
                                      ShaderMode mode,
                                      int uniformOffset)
{
    if (!surface || uniformOffset < 0) return;

    auto *pipeline = d->pipelines[mode].get();
    if (!pipeline) return;

    auto draw = [&](QRhiGraphicsPipeline *p) {
        cb->setGraphicsPipeline(p);
        const QRhiCommandBuffer::DynamicOffset srbOffset = { 0, uint32_t(uniformOffset) };
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

void BrainRenderer::renderDipoles(QRhiCommandBuffer *cb, QRhi *rhi, const SceneData &data, DipoleObject *dipoles)
{
    if (!dipoles || !dipoles->isVisible() || dipoles->instanceCount() == 0) return;

    auto *pipeline = d->pipelines[Dipole].get();
    if (!pipeline) return;

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    dipoles->updateBuffers(rhi, u);

    // Dynamic slot update
    int offset = d->currentUniformOffset;
    d->currentUniformOffset += d->uniformBufferOffsetAlignment;
    if (d->currentUniformOffset >= d->uniformBuffer->size()) {
        qWarning("BrainRenderer: uniform buffer overflow in renderDipoles");
        return;
    }

    // Pack all uniforms into a single contiguous upload
    struct {
        float mvp[16];          // 0..63
        float cameraPos[3];     // 64..75
        float _pad0;            // 76..79
        float lightDir[3];      // 80..91
        float _pad1;            // 92..95
        float lightingEnabled;  // 96..99
    } dub;
    memcpy(dub.mvp, data.mvp.constData(), 64);
    memcpy(dub.cameraPos, &data.cameraPos, 12);
    dub._pad0 = 0.0f;
    memcpy(dub.lightDir, &data.lightDir, 12);
    dub._pad1 = 0.0f;
    dub.lightingEnabled = data.lightingEnabled ? 1.0f : 0.0f;
    u->updateDynamicBuffer(d->uniformBuffer.get(), offset, sizeof(dub), &dub);

    cb->resourceUpdate(u);

    // Re-assert the per-pane viewport and scissor.
    cb->setViewport(toViewport(data));
    cb->setScissor(toScissor(data));

    cb->setGraphicsPipeline(pipeline);
    
    const QRhiCommandBuffer::DynamicOffset srbOffset = { 0, uint32_t(offset) };
    cb->setShaderResources(d->srb.get(), 1, &srbOffset);
    
    const QRhiCommandBuffer::VertexInput bindings[2] = {
        QRhiCommandBuffer::VertexInput(dipoles->vertexBuffer(), 0),
        QRhiCommandBuffer::VertexInput(dipoles->instanceBuffer(), 0)
    };
    
    cb->setVertexInput(0, 2, bindings, dipoles->indexBuffer(), 0, QRhiCommandBuffer::IndexUInt32);
    
    cb->drawIndexed(dipoles->indexCount(), dipoles->instanceCount());
}

//=============================================================================================================

void BrainRenderer::renderNetwork(QRhiCommandBuffer *cb, QRhi *rhi, const SceneData &data, NetworkObject *network)
{
    if (!network || !network->isVisible() || !network->hasData()) return;

    auto *pipeline = d->pipelines[Dipole].get();
    if (!pipeline) return;

    // --- Render Nodes (instanced spheres) ---
    if (network->nodeInstanceCount() > 0) {
        QRhiResourceUpdateBatch *uNodes = rhi->nextResourceUpdateBatch();
        network->updateNodeBuffers(rhi, uNodes);

        int offset = d->currentUniformOffset;
        d->currentUniformOffset += d->uniformBufferOffsetAlignment;
        if (d->currentUniformOffset >= d->uniformBuffer->size()) {
            qWarning("BrainRenderer: uniform buffer overflow in renderNetwork (nodes)");
            return;
        }

        struct { float mvp[16]; float cp[3]; float _p0; float ld[3]; float _p1; float le; } nub;
        memcpy(nub.mvp, data.mvp.constData(), 64);
        memcpy(nub.cp, &data.cameraPos, 12); nub._p0 = 0.0f;
        memcpy(nub.ld, &data.lightDir, 12);  nub._p1 = 0.0f;
        nub.le = data.lightingEnabled ? 1.0f : 0.0f;
        uNodes->updateDynamicBuffer(d->uniformBuffer.get(), offset, sizeof(nub), &nub);

        cb->resourceUpdate(uNodes);
        cb->setViewport(toViewport(data));
        cb->setScissor(toScissor(data));

        cb->setGraphicsPipeline(pipeline);

        const QRhiCommandBuffer::DynamicOffset srbOffset = { 0, uint32_t(offset) };
        cb->setShaderResources(d->srb.get(), 1, &srbOffset);

        const QRhiCommandBuffer::VertexInput nodeBindings[2] = {
            QRhiCommandBuffer::VertexInput(network->nodeVertexBuffer(), 0),
            QRhiCommandBuffer::VertexInput(network->nodeInstanceBuffer(), 0)
        };

        cb->setVertexInput(0, 2, nodeBindings, network->nodeIndexBuffer(), 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(network->nodeIndexCount(), network->nodeInstanceCount());
    }

    // --- Render Edges (instanced cylinders) ---
    if (network->edgeInstanceCount() > 0) {
        QRhiResourceUpdateBatch *uEdges = rhi->nextResourceUpdateBatch();
        network->updateEdgeBuffers(rhi, uEdges);

        int offset = d->currentUniformOffset;
        d->currentUniformOffset += d->uniformBufferOffsetAlignment;
        if (d->currentUniformOffset >= d->uniformBuffer->size()) {
            qWarning("BrainRenderer: uniform buffer overflow in renderNetwork (edges)");
            return;
        }

        struct { float mvp[16]; float cp[3]; float _p0; float ld[3]; float _p1; float le; } eub;
        memcpy(eub.mvp, data.mvp.constData(), 64);
        memcpy(eub.cp, &data.cameraPos, 12); eub._p0 = 0.0f;
        memcpy(eub.ld, &data.lightDir, 12);  eub._p1 = 0.0f;
        eub.le = data.lightingEnabled ? 1.0f : 0.0f;
        uEdges->updateDynamicBuffer(d->uniformBuffer.get(), offset, sizeof(eub), &eub);

        cb->resourceUpdate(uEdges);
        cb->setViewport(toViewport(data));
        cb->setScissor(toScissor(data));

        cb->setGraphicsPipeline(pipeline);

        const QRhiCommandBuffer::DynamicOffset srbOffset = { 0, uint32_t(offset) };
        cb->setShaderResources(d->srb.get(), 1, &srbOffset);

        const QRhiCommandBuffer::VertexInput edgeBindings[2] = {
            QRhiCommandBuffer::VertexInput(network->edgeVertexBuffer(), 0),
            QRhiCommandBuffer::VertexInput(network->edgeInstanceBuffer(), 0)
        };

        cb->setVertexInput(0, 2, edgeBindings, network->edgeIndexBuffer(), 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(network->edgeIndexCount(), network->edgeInstanceCount());
    }
}

//=============================================================================================================
// WORKAROUND(QRhi-GLES2): Merged single-drawIndexed rendering.
// The Qt QRhi GLES2/WebGL backend has a bug where only the first
// drawIndexed() per render pass produces visible output.  These two
// methods merge all surfaces (brain, BEM, sensors, digitizers,
// source-space) into a single VBO/IBO so that all geometry is drawn
// in one call.
//
// Remove when upstream Qt fixes the issue.
//=============================================================================================================

void BrainRenderer::prepareMergedSurfaces(QRhi *rhi, QRhiResourceUpdateBatch * /*u*/,
                                           const QVector<BrainSurface*> &surfaces,
                                           const QString &groupName)
{
    auto &group = d->mergedGroups[groupName];

    // Check if surface list changed (different count or different pointers)
    if (!group.dirty) {
        if (group.surfaces.size() != surfaces.size()) {
            group.dirty = true;
        } else {
            for (int i = 0; i < surfaces.size(); ++i) {
                if (group.surfaces[i] != surfaces[i]) {
                    group.dirty = true;
                    break;
                }
            }
        }
    }

    // If geometry hasn't changed, check if any surface vertex data actually changed
    // (STC animation changes vertex colors but not topology)
    if (!group.dirty && group.indexCount > 0) {
        // Compare per-surface vertex generation counters
        bool anyChanged = false;
        if (group.surfaceGenerations.size() != surfaces.size()) {
            anyChanged = true;
        } else {
            for (int i = 0; i < surfaces.size(); ++i) {
                if (surfaces[i] && surfaces[i]->vertexGeneration() != group.surfaceGenerations[i]) {
                    anyChanged = true;
                    break;
                }
            }
        }

        if (!anyChanged) {
            // Nothing changed — skip vertex rebuild entirely
            return;
        }

        // Re-merge vertex data directly into vertexRaw (no temp allocation)
        // Safety: verify vertex count hasn't changed since the full rebuild.
        // If it has, fall through to the full rebuild path to update indices.
        int totalVerts = 0;
        for (int si = 0; si < surfaces.size(); ++si)
            if (surfaces[si]) totalVerts += surfaces[si]->vertexDataRef().size();
        if (totalVerts != group.totalVertexCount) {
            group.dirty = true;
            // Fall through to full rebuild below
        } else {
            float brainId = 0.0f;
            float nonBrainId = 100.0f; // offset so shaders can distinguish
            group.surfaceGenerations.resize(surfaces.size());
            VertexData *dst = reinterpret_cast<VertexData*>(group.vertexRaw.data());
            for (int si = 0; si < surfaces.size(); ++si) {
                BrainSurface *surf = surfaces[si];
                if (!surf) { brainId += 1.0f; nonBrainId += 1.0f; continue; }
                const bool isBrain = (surf->tissueType() == BrainSurface::TissueBrain);
                const float id = isBrain ? brainId : nonBrainId;
                const auto &srcVerts = surf->vertexDataRef();
                const int n = srcVerts.size();
                memcpy(dst, srcVerts.constData(), n * sizeof(VertexData));
                for (int j = 0; j < n; ++j)
                    dst[j].surfaceId = id;
                dst += n;
                group.surfaceGenerations[si] = surf->vertexGeneration();
                brainId += 1.0f;
                nonBrainId += 1.0f;
            }
            group.gpuVertexDirty = true;
            return;
        }
    }

    // Full rebuild: topology or surface list changed
    group.surfaces = surfaces;
    group.indexCount = 0;
    group.totalVertexCount = 0;
    group.dirty = false;

    // Build merged vertex + index arrays
    // Pre-calculate total sizes for single allocation
    int totalVerts = 0;
    int totalIndices = 0;
    for (int si = 0; si < surfaces.size(); ++si) {
        if (!surfaces[si]) continue;
        totalVerts += surfaces[si]->vertexDataRef().size();
        totalIndices += surfaces[si]->indexDataRef().size();
    }

    group.indexCount = totalIndices;
    if (group.indexCount == 0) return;

    const quint32 vbufSize = totalVerts   * sizeof(VertexData);
    const quint32 ibufSize = totalIndices * sizeof(uint32_t);

    // (Re-)create Dynamic buffers when they don't exist or are too small
    if (!group.vertexBuffer || group.vertexBuffer->size() < vbufSize) {
        group.vertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                                    QRhiBuffer::VertexBuffer, vbufSize));
        group.vertexBuffer->create();
    }
    if (!group.indexBuffer || group.indexBuffer->size() < ibufSize) {
        group.indexBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                                   QRhiBuffer::IndexBuffer, ibufSize));
        group.indexBuffer->create();
    }

    // Write directly into QByteArrays — no temp QVector intermediaries
    group.vertexRaw.resize(vbufSize);
    group.indexRaw.resize(ibufSize);
    group.surfaceGenerations.resize(surfaces.size());

    VertexData *vDst = reinterpret_cast<VertexData*>(group.vertexRaw.data());
    uint32_t *iDst = reinterpret_cast<uint32_t*>(group.indexRaw.data());
    float brainId = 0.0f;
    float nonBrainId = 100.0f; // offset so shaders can distinguish
    uint32_t vertexOffset = 0;
    for (int si = 0; si < surfaces.size(); ++si) {
        BrainSurface *surf = surfaces[si];
        if (!surf) { brainId += 1.0f; nonBrainId += 1.0f; continue; }

        const bool isBrain = (surf->tissueType() == BrainSurface::TissueBrain);
        const float id = isBrain ? brainId : nonBrainId;
        const auto &srcVerts = surf->vertexDataRef();
        const auto &srcIdx   = surf->indexDataRef();
        const int nv = srcVerts.size();
        const int ni = srcIdx.size();

        // Bulk copy vertices + stamp surfaceId
        memcpy(vDst, srcVerts.constData(), nv * sizeof(VertexData));
        for (int j = 0; j < nv; ++j)
            vDst[j].surfaceId = id;
        vDst += nv;

        // Copy indices with global vertex offset
        const uint32_t *srcI = srcIdx.constData();
        for (int j = 0; j < ni; ++j)
            iDst[j] = srcI[j] + vertexOffset;
        iDst += ni;

        vertexOffset += nv;
        group.surfaceGenerations[si] = surf->vertexGeneration();
        brainId += 1.0f;
        nonBrainId += 1.0f;
    }
    group.totalVertexCount = totalVerts;
    group.gpuVertexDirty = true;
    group.gpuIndexDirty  = true;
}

//=============================================================================================================

void BrainRenderer::invalidateMergedGroup(const QString &groupName)
{
    auto it = d->mergedGroups.find(groupName);
    if (it != d->mergedGroups.end()) {
        it->second.dirty = true;
    }
}

//=============================================================================================================

bool BrainRenderer::hasMergedContent(const QString &groupName) const
{
    auto it = d->mergedGroups.find(groupName);
    return it != d->mergedGroups.end() && it->second.indexCount > 0;
}

//=============================================================================================================

void BrainRenderer::drawMergedSurfaces(QRhiCommandBuffer *cb, QRhi *rhi,
                                        const SceneData &data, ShaderMode mode,
                                        const QString &groupName)
{
    auto it = d->mergedGroups.find(groupName);
    if (it == d->mergedGroups.end()) return;
    auto &group = it->second;

    if (group.indexCount == 0) return;

    auto *pipeline = d->pipelines[mode].get();
    if (!pipeline) return;

    // Determine which merged surface (if any) is selected.
    // surfaceId encoding: brain surfaces get ids 0,1,2...; non-brain get 100,101,102...
    float selectedSurfaceId = -1.0f;
    for (int i = 0; i < group.surfaces.size(); ++i) {
        if (group.surfaces[i] && group.surfaces[i]->isSelected()) {
            if (group.surfaces[i]->selectedRegionId() == -1
                && group.surfaces[i]->selectedVertexStart() < 0) {
                const bool isBrain = (group.surfaces[i]->tissueType() == BrainSurface::TissueBrain);
                selectedSurfaceId = static_cast<float>(isBrain ? i : 100 + i);
            }
            break;
        }
    }

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();

    // Re-upload merged geometry only when data actually changed.
    // Split VBO / IBO uploads: the fast-update path (STC color changes)
    // only modifies vertices; re-uploading the IBO via glBufferSubData on
    // WebGL can corrupt the VAO's element-buffer binding.
    if (group.gpuVertexDirty) {
        u->updateDynamicBuffer(group.vertexBuffer.get(), 0, group.vertexRaw.size(), group.vertexRaw.constData());
        group.gpuVertexDirty = false;
    }
    if (group.gpuIndexDirty) {
        u->updateDynamicBuffer(group.indexBuffer.get(),  0, group.indexRaw.size(),  group.indexRaw.constData());
        group.gpuIndexDirty = false;
    }

    int offset = d->currentUniformOffset;
    d->currentUniformOffset += d->uniformBufferOffsetAlignment;
    if (d->currentUniformOffset >= d->uniformBuffer->size()) {
        qWarning("BrainRenderer: uniform buffer overflow in drawMergedSurfaces");
        return;
    }

    // Pack all uniforms into a contiguous block for a single upload
    // Layout must match the shader's UniformBlock (std140).
    struct {
        float mvp[16];          // 0..63
        float cameraPos[3];     // 64..75
        float isSelected;       // 76..79
        float lightDir[3];      // 80..91
        float tissueType;       // 92..95
        float lightingEnabled;  // 96..99
        float overlayMode;      // 100..103
        float selectedSurfaceId;// 104..107
    } ub;
    memcpy(ub.mvp, data.mvp.constData(), 64);
    memcpy(ub.cameraPos, &data.cameraPos, 12);
    ub.isSelected = 0.0f;
    memcpy(ub.lightDir, &data.lightDir, 12);
    ub.tissueType = (!group.surfaces.isEmpty() && group.surfaces.first())
                  ? static_cast<float>(group.surfaces.first()->tissueType()) : 0.0f;
    ub.lightingEnabled = data.lightingEnabled ? 1.0f : 0.0f;
    ub.overlayMode = data.overlayMode;
    ub.selectedSurfaceId = selectedSurfaceId;

    u->updateDynamicBuffer(d->uniformBuffer.get(), offset, sizeof(ub), &ub);

    cb->resourceUpdate(u);

    cb->setViewport(toViewport(data));
    cb->setScissor(toScissor(data));

    auto draw = [&](QRhiGraphicsPipeline *p) {
        cb->setGraphicsPipeline(p);
        const QRhiCommandBuffer::DynamicOffset srbOffset = { 0, uint32_t(offset) };
        cb->setShaderResources(d->srb.get(), 1, &srbOffset);
        const QRhiCommandBuffer::VertexInput vbuf(group.vertexBuffer.get(), 0);
        cb->setVertexInput(0, 1, &vbuf, group.indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(group.indexCount);
    };

    // WORKAROUND(QRhi-GLES2): On WebGL, only one drawIndexed() per pass.
    // For Holographic mode, the back-face pass must happen in a separate
    // render pass.  The caller is responsible for wrapping each call in
    // its own beginPreservingPass/endPass on WASM.
#ifdef __EMSCRIPTEN__
    draw(pipeline);
#else
    if (mode == Holographic && d->pipelinesBackColor[Holographic]) {
        draw(d->pipelinesBackColor[Holographic].get());
    }

    draw(pipeline);
#endif
}
