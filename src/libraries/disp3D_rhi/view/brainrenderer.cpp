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
#include <QFile>
#include <QDebug>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================
#include <QFile>
#include <QDebug>

//=============================================================================================================

BrainRenderer::BrainRenderer()
{
}

//=============================================================================================================

BrainRenderer::~BrainRenderer()
{
}

//=============================================================================================================

void BrainRenderer::initialize(QRhi *rhi, QRhiRenderPassDescriptor *rp, int sampleCount)
{
    if (m_resourcesDirty) {
        createResources(rhi, rp, sampleCount);
    }
}

//=============================================================================================================

void BrainRenderer::createResources(QRhi *rhi, QRhiRenderPassDescriptor *rp, int sampleCount)
{
    m_uniformBufferOffsetAlignment = rhi->ubufAlignment();
    
    // Create Uniform Buffer
    if (!m_uniformBuffer) {
        // Size for 8192 slots with alignment — enough for 4 viewports × ~1000 surfaces
        m_uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 8192 * m_uniformBufferOffsetAlignment));
        m_uniformBuffer->create();
    }
    
    // Create SRB
    if (!m_srb) {
        m_srb.reset(rhi->newShaderResourceBindings());
        m_srb->setBindings({
            // Use dynamic offset for the uniform buffer. 
            // The size of one uniform block in the shader is ~104 bytes, 
            // but we use m_uniformBufferOffsetAlignment for the stride.
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_uniformBuffer.get(), 256)
        });
        m_srb->create();
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
            p->setShaderResourceBindings(m_srb.get());
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
            m_pipelinesBackColor[mode] = std::move(pipelineBack);
            setup(pipeline.get(), QRhiGraphicsPipeline::Back); // Front faces
        } else {
             // Culling: None (Double-sided) to be safe for FreeSurfer meshes
             setup(pipeline.get(), QRhiGraphicsPipeline::None);
        }
        m_pipelines[mode] = std::move(pipeline);
    }

    m_resourcesDirty = false;
}

//=============================================================================================================

void BrainRenderer::beginFrame(QRhiCommandBuffer *cb, QRhiRenderTarget *rt)
{
    m_currentUniformOffset = 0;
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
    if (m_pipelines.find(mode) == m_pipelines.end()) return;
    
    auto *pipeline = m_pipelines[mode].get();
    if (!pipeline) return;

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    surface->updateBuffers(rhi, u);

    // Dynamic slot update
    int offset = m_currentUniformOffset;
    m_currentUniformOffset += m_uniformBufferOffsetAlignment;
    if (m_currentUniformOffset >= m_uniformBuffer->size()) {
        qWarning("BrainRenderer: uniform buffer overflow (%d / %d bytes) — too many surfaces. Some draws will be skipped.",
                 m_currentUniformOffset, (int)m_uniformBuffer->size());
        return;  // Skip this draw rather than silently corrupt earlier viewport data
    }

    u->updateDynamicBuffer(m_uniformBuffer.get(), offset + 0, 64, data.mvp.constData());
    u->updateDynamicBuffer(m_uniformBuffer.get(), offset + 64, 12, &data.cameraPos);
    
    float selected = surface->isSelected() ? 1.0f : 0.0f;
    u->updateDynamicBuffer(m_uniformBuffer.get(), offset + 76, 4, &selected);
    
    u->updateDynamicBuffer(m_uniformBuffer.get(), offset + 80, 12, &data.lightDir);
    
    // Pass tissue type for anatomical shader (offset 92, uses original _pad2 slot)
    float tissueType = static_cast<float>(surface->tissueType());
    u->updateDynamicBuffer(m_uniformBuffer.get(), offset + 92, 4, &tissueType);
    
    float lighting = data.lightingEnabled ? 1.0f : 0.0f;
    u->updateDynamicBuffer(m_uniformBuffer.get(), offset + 96, 4, &lighting);

    float overlayMode = data.overlayMode;
    u->updateDynamicBuffer(m_uniformBuffer.get(), offset + 100, 4, &overlayMode);

    cb->resourceUpdate(u);

    // Re-assert the per-pane viewport and scissor after resourceUpdate.
    // The scissor provides a hard pixel clip that guarantees no cross-pane
    // bleeding, regardless of Metal render-encoder restarts.
    cb->setViewport(data.viewport);
    cb->setScissor(data.scissor);

    auto draw = [&](QRhiGraphicsPipeline *p) {
        cb->setGraphicsPipeline(p);
        
        const QRhiCommandBuffer::DynamicOffset srbOffset = { 0, uint32_t(offset) };
        cb->setShaderResources(m_srb.get(), 1, &srbOffset);
        const QRhiCommandBuffer::VertexInput vbuf(surface->vertexBuffer(), 0);
        cb->setVertexInput(0, 1, &vbuf, surface->indexBuffer(), 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(surface->indexCount());
    };

    if (mode == Holographic && m_pipelinesBackColor.count(Holographic) > 0) {
        draw(m_pipelinesBackColor[Holographic].get());
    }
    
    draw(pipeline);
}

//=============================================================================================================

void BrainRenderer::renderDipoles(QRhiCommandBuffer *cb, QRhi *rhi, const SceneData &data, DipoleObject *dipoles)
{
    if (!dipoles || !dipoles->isVisible() || dipoles->instanceCount() == 0) return;

    if (m_pipelines.find(Dipole) == m_pipelines.end()) return;
    
    auto *pipeline = m_pipelines[Dipole].get();
    if (!pipeline) return;

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    dipoles->updateBuffers(rhi, u);

    // Dynamic slot update
    int offset = m_currentUniformOffset;
    m_currentUniformOffset += m_uniformBufferOffsetAlignment;
    if (m_currentUniformOffset >= m_uniformBuffer->size()) m_currentUniformOffset = 0;

    u->updateDynamicBuffer(m_uniformBuffer.get(), offset + 0, 64, data.mvp.constData());
    u->updateDynamicBuffer(m_uniformBuffer.get(), offset + 64, 12, &data.cameraPos);
    u->updateDynamicBuffer(m_uniformBuffer.get(), offset + 80, 12, &data.lightDir);
    float lighting = data.lightingEnabled ? 1.0f : 0.0f;
    u->updateDynamicBuffer(m_uniformBuffer.get(), offset + 92, 4, &lighting);

    cb->resourceUpdate(u);

    // Re-assert the per-pane viewport and scissor.
    cb->setViewport(data.viewport);
    cb->setScissor(data.scissor);

    cb->setGraphicsPipeline(pipeline);
    
    const QRhiCommandBuffer::DynamicOffset srbOffset = { 0, uint32_t(offset) };
    cb->setShaderResources(m_srb.get(), 1, &srbOffset);
    
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

    if (m_pipelines.find(Dipole) == m_pipelines.end()) return;

    auto *pipeline = m_pipelines[Dipole].get();
    if (!pipeline) return;

    // --- Render Nodes (instanced spheres) ---
    if (network->nodeInstanceCount() > 0) {
        QRhiResourceUpdateBatch *uNodes = rhi->nextResourceUpdateBatch();
        network->updateNodeBuffers(rhi, uNodes);

        int offset = m_currentUniformOffset;
        m_currentUniformOffset += m_uniformBufferOffsetAlignment;
        if (m_currentUniformOffset >= m_uniformBuffer->size()) {
            qWarning("BrainRenderer: uniform buffer overflow in renderNetwork (nodes)");
            return;
        }

        uNodes->updateDynamicBuffer(m_uniformBuffer.get(), offset + 0, 64, data.mvp.constData());
        uNodes->updateDynamicBuffer(m_uniformBuffer.get(), offset + 64, 12, &data.cameraPos);
        uNodes->updateDynamicBuffer(m_uniformBuffer.get(), offset + 80, 12, &data.lightDir);
        float lighting = data.lightingEnabled ? 1.0f : 0.0f;
        uNodes->updateDynamicBuffer(m_uniformBuffer.get(), offset + 92, 4, &lighting);

        cb->resourceUpdate(uNodes);
        cb->setViewport(data.viewport);
        cb->setScissor(data.scissor);

        cb->setGraphicsPipeline(pipeline);

        const QRhiCommandBuffer::DynamicOffset srbOffset = { 0, uint32_t(offset) };
        cb->setShaderResources(m_srb.get(), 1, &srbOffset);

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

        int offset = m_currentUniformOffset;
        m_currentUniformOffset += m_uniformBufferOffsetAlignment;
        if (m_currentUniformOffset >= m_uniformBuffer->size()) {
            qWarning("BrainRenderer: uniform buffer overflow in renderNetwork (edges)");
            return;
        }

        uEdges->updateDynamicBuffer(m_uniformBuffer.get(), offset + 0, 64, data.mvp.constData());
        uEdges->updateDynamicBuffer(m_uniformBuffer.get(), offset + 64, 12, &data.cameraPos);
        uEdges->updateDynamicBuffer(m_uniformBuffer.get(), offset + 80, 12, &data.lightDir);
        float lighting = data.lightingEnabled ? 1.0f : 0.0f;
        uEdges->updateDynamicBuffer(m_uniformBuffer.get(), offset + 92, 4, &lighting);

        cb->resourceUpdate(uEdges);
        cb->setViewport(data.viewport);
        cb->setScissor(data.scissor);

        cb->setGraphicsPipeline(pipeline);

        const QRhiCommandBuffer::DynamicOffset srbOffset = { 0, uint32_t(offset) };
        cb->setShaderResources(m_srb.get(), 1, &srbOffset);

        const QRhiCommandBuffer::VertexInput edgeBindings[2] = {
            QRhiCommandBuffer::VertexInput(network->edgeVertexBuffer(), 0),
            QRhiCommandBuffer::VertexInput(network->edgeInstanceBuffer(), 0)
        };

        cb->setVertexInput(0, 2, edgeBindings, network->edgeIndexBuffer(), 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(network->edgeIndexCount(), network->edgeInstanceCount());
    }
}
