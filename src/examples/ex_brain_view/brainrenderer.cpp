//=============================================================================================================
/**
 * @file     brainrenderer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
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

void BrainRenderer::initialize(QRhi *rhi, QRhiRenderPassDescriptor *rp, int sampleCount, ShaderMode mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        m_resourcesDirty = true;
    }
    
    if (m_resourcesDirty) {
        createResources(rhi, rp, sampleCount);
    }
}

//=============================================================================================================

void BrainRenderer::createResources(QRhi *rhi, QRhiRenderPassDescriptor *rp, int sampleCount)
{
    auto getShader = [](const QString &name) {
        QFile f(name);
        return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
    };

    QString vert = (m_mode == Holographic) ? ":/holographic.vert.qsb" : (m_mode == Atlas) ? ":/glossy.vert.qsb" : ":/standard.vert.qsb";
    QString frag = (m_mode == Holographic) ? ":/holographic.frag.qsb" : (m_mode == Atlas) ? ":/glossy.frag.qsb" : ":/standard.frag.qsb";

    QShader vS = getShader(vert);
    QShader fS = getShader(frag);

    if (!vS.isValid() || !fS.isValid()) {
        qWarning() << "BrainRenderer: Could not load shaders" << vert << frag;
        return;
    }

    if (!m_uniformBuffer) {
        m_uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 128));
        m_uniformBuffer->create();
    }

    m_srb.reset(rhi->newShaderResourceBindings());
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_uniformBuffer.get())
    });
    m_srb->create();

    m_pipeline.reset(rhi->newGraphicsPipeline());
    m_pipelineBackColor.reset();

    QRhiGraphicsPipeline::TargetBlend blend;
    if (m_mode == Holographic) {
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::One;
        blend.srcAlpha = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstAlpha = QRhiGraphicsPipeline::One;
    }

    auto setup = [&](std::unique_ptr<QRhiGraphicsPipeline>& p, QRhiGraphicsPipeline::CullMode cull) {
        p->setShaderStages({{ QRhiShaderStage::Vertex, vS }, { QRhiShaderStage::Fragment, fS }});
        
        QRhiVertexInputLayout il;
        il.setBindings({{ 28 }});
        il.setAttributes({{ 0, 0, QRhiVertexInputAttribute::Float3, 0 }, 
                          { 0, 1, QRhiVertexInputAttribute::Float3, 12 }, 
                          { 0, 2, QRhiVertexInputAttribute::UNormByte4, 24 }});
        p->setVertexInputLayout(il);
        p->setShaderResourceBindings(m_srb.get());
        p->setRenderPassDescriptor(rp);
        p->setSampleCount(sampleCount);
        p->setCullMode(cull);
        if (m_mode == Holographic) {
            p->setTargetBlends({blend});
            p->setDepthTest(false);
            p->setDepthWrite(false);
        } else {
            p->setDepthTest(true);
            p->setDepthWrite(true);
        }
        p->create();
    };

    if (m_mode == Holographic) {
        m_pipelineBackColor.reset(rhi->newGraphicsPipeline());
        setup(m_pipelineBackColor, QRhiGraphicsPipeline::Front);
    }
    setup(m_pipeline, QRhiGraphicsPipeline::Back);

    m_resourcesDirty = false;
}

//=============================================================================================================

void BrainRenderer::beginFrame(QRhiCommandBuffer *cb, QRhiRenderTarget *rt)
{
    cb->beginPass(rt, QColor(0, 0, 0), { 1.0f, 0 });
    cb->setViewport(QRhiViewport(0, 0, rt->pixelSize().width(), rt->pixelSize().height()));
}

//=============================================================================================================

void BrainRenderer::endFrame(QRhiCommandBuffer *cb)
{
    cb->endPass();
}

//=============================================================================================================

void BrainRenderer::renderSurface(QRhiCommandBuffer *cb, QRhi *rhi, const SceneData &data, BrainSurface *surface)
{
    if (!surface || !m_pipeline || !surface->isVisible()) return;

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    surface->updateBuffers(rhi, u);

    u->updateDynamicBuffer(m_uniformBuffer.get(), 0, 64, data.mvp.constData());
    u->updateDynamicBuffer(m_uniformBuffer.get(), 64, 12, &data.cameraPos);
    u->updateDynamicBuffer(m_uniformBuffer.get(), 80, 12, &data.lightDir);
    float lighting = data.lightingEnabled ? 1.0f : 0.0f;
    u->updateDynamicBuffer(m_uniformBuffer.get(), 92, 4, &lighting); // Correct std140 padding

    cb->resourceUpdate(u); // Ensure resource updates happen before pass

    auto draw = [&](QRhiGraphicsPipeline *p) {
        cb->setGraphicsPipeline(p);
        cb->setShaderResources(m_srb.get());
        const QRhiCommandBuffer::VertexInput vbuf(surface->vertexBuffer(), 0);
        cb->setVertexInput(0, 1, &vbuf, surface->indexBuffer(), 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(surface->indexCount());
    };

    if (m_pipelineBackColor) draw(m_pipelineBackColor.get());
    draw(m_pipeline.get());
}
