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

#include <QFile>
#include <QDebug>
#include <map>

//=============================================================================================================
// PIMPL
//=============================================================================================================

struct BrainRenderer::Impl
{
    void createResources(QRhi *rhi, QRhiRenderPassDescriptor *rp, int sampleCount);

    std::unique_ptr<QRhiShaderResourceBindings> srb;

    // Pipelines for each mode
    std::map<ShaderMode, std::unique_ptr<QRhiGraphicsPipeline>> pipelines;
    std::map<ShaderMode, std::unique_ptr<QRhiGraphicsPipeline>> pipelinesBackColor; // For Holographic back faces

    std::unique_ptr<QRhiBuffer> uniformBuffer;
    int uniformBufferOffsetAlignment = 0;
    int currentUniformOffset = 0;

    bool resourcesDirty = true;
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
                il.setBindings({{ 32 }});
                il.setAttributes({{ 0, 0, QRhiVertexInputAttribute::Float3, 0 }, 
                                  { 0, 1, QRhiVertexInputAttribute::Float3, 12 }, 
                                  { 0, 2, QRhiVertexInputAttribute::UNormByte4, 24 },
                                  { 0, 3, QRhiVertexInputAttribute::UNormByte4, 28 }});
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

void BrainRenderer::beginFrame(QRhiCommandBuffer *cb, QRhiRenderTarget *rt)
{
    d->currentUniformOffset = 0;
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

void BrainRenderer::endFrame(QRhiCommandBuffer *cb)
{
    cb->endPass();
}

//=============================================================================================================

void BrainRenderer::renderSurface(QRhiCommandBuffer *cb, QRhi *rhi, const SceneData &data, BrainSurface *surface, ShaderMode mode)
{
    if (!surface || !surface->isVisible()) return;

    // Check if pipeline for this mode exists
    if (d->pipelines.find(mode) == d->pipelines.end()) return;
    
    auto *pipeline = d->pipelines[mode].get();
    if (!pipeline) return;

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    surface->updateBuffers(rhi, u);

    // Dynamic slot update
    int offset = d->currentUniformOffset;
    d->currentUniformOffset += d->uniformBufferOffsetAlignment;
    if (d->currentUniformOffset >= d->uniformBuffer->size()) {
        qWarning("BrainRenderer: uniform buffer overflow (%d / %d bytes) — too many surfaces. Some draws will be skipped.",
                 d->currentUniformOffset, (int)d->uniformBuffer->size());
        return;  // Skip this draw rather than silently corrupt earlier viewport data
    }

    u->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetMVP, 64, data.mvp.constData());
    u->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetCameraPos, 12, &data.cameraPos);
    
    float selected = surface->isSelected() ? 1.0f : 0.0f;
    u->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetSelected, 4, &selected);
    
    u->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetLightDir, 12, &data.lightDir);
    
    // Pass tissue type for anatomical shader (offset 92, uses original _pad2 slot)
    float tissueType = static_cast<float>(surface->tissueType());
    u->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetTissueType, 4, &tissueType);
    
    float lighting = data.lightingEnabled ? 1.0f : 0.0f;
    u->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetLighting, 4, &lighting);

    float overlayMode = data.overlayMode;
    u->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetOverlayMode, 4, &overlayMode);

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

    if (mode == Holographic && d->pipelinesBackColor.count(Holographic) > 0) {
        draw(d->pipelinesBackColor[Holographic].get());
    }
    
    draw(pipeline);
}

//=============================================================================================================

void BrainRenderer::renderDipoles(QRhiCommandBuffer *cb, QRhi *rhi, const SceneData &data, DipoleObject *dipoles)
{
    if (!dipoles || !dipoles->isVisible() || dipoles->instanceCount() == 0) return;

    if (d->pipelines.find(Dipole) == d->pipelines.end()) return;
    
    auto *pipeline = d->pipelines[Dipole].get();
    if (!pipeline) return;

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    dipoles->updateBuffers(rhi, u);

    // Dynamic slot update
    int offset = d->currentUniformOffset;
    d->currentUniformOffset += d->uniformBufferOffsetAlignment;
    if (d->currentUniformOffset >= d->uniformBuffer->size()) d->currentUniformOffset = 0;

    u->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetMVP, 64, data.mvp.constData());
    u->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetCameraPos, 12, &data.cameraPos);
    u->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetLightDir, 12, &data.lightDir);
    float lighting = data.lightingEnabled ? 1.0f : 0.0f;
    u->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetLighting, 4, &lighting);

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

    if (d->pipelines.find(Dipole) == d->pipelines.end()) return;

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

        uNodes->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetMVP, 64, data.mvp.constData());
        uNodes->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetCameraPos, 12, &data.cameraPos);
        uNodes->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetLightDir, 12, &data.lightDir);
        float lighting = data.lightingEnabled ? 1.0f : 0.0f;
        uNodes->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetLighting, 4, &lighting);

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

        uEdges->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetMVP, 64, data.mvp.constData());
        uEdges->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetCameraPos, 12, &data.cameraPos);
        uEdges->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetLightDir, 12, &data.lightDir);
        float lighting = data.lightingEnabled ? 1.0f : 0.0f;
        uEdges->updateDynamicBuffer(d->uniformBuffer.get(), offset + kOffsetLighting, 4, &lighting);

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
