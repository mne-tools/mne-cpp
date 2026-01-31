//=============================================================================================================
/**
 * @file     brainrenderer.h
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
 * @brief    BrainRenderer class declaration.
 *
 */

#ifndef BRAINRENDERER_H
#define BRAINRENDERER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <rhi/qrhi.h>
#include <QMatrix4x4>
#include <QVector3D>
#include <memory>
#include "brainsurface.h"

//=============================================================================================================
/**
 * BrainRenderer handles the low-level RHI rendering logic, managing pipelines, shaders, and draw calls for brain surfaces.
 *
 * @brief    BrainRenderer class.
 */
class BrainRenderer
{
public:
    //=========================================================================================================
    /**
     * Default Constructor
     */
    BrainRenderer();

    //=========================================================================================================
    /**
     * Destructor
     */
    ~BrainRenderer();
    
    enum ShaderMode {
        Standard,
        Holographic,
        Atlas
    };

    struct SceneData {
        QMatrix4x4 mvp;
        QVector3D cameraPos;
        QVector3D lightDir;
        bool lightingEnabled;
    };
    
    //=========================================================================================================
    /**
     * Initialize resources (shaders, pipelines) for the given RHI and render pass.
     *
     * @param[in] rhi        Pointer to QRhi instance.
     * @param[in] rp         Render pass descriptor.
     * @param[in] sampleCount  MSAA sample count.
     * @param[in] mode       Shader mode to use.
     */
    void initialize(QRhi *rhi, QRhiRenderPassDescriptor *rp, int sampleCount, ShaderMode mode);
    
    //=========================================================================================================
    /**
     * Begin a rendering frame/pass. Clears the target.
     *
     * @param[in] cb         Command buffer to record to.
     * @param[in] rt         Render target to draw into.
     */
    void beginFrame(QRhiCommandBuffer *cb, QRhiRenderTarget *rt);

    //=========================================================================================================
    /**
     * Render a single surface. Must be called between beginFrame and endFrame.
     *
     * @param[in] cb         Command buffer.
     * @param[in] rhi        QRhi pointer.
     * @param[in] data       Scene uniforms (MVP, light, etc).
     * @param[in] surface    Pointer to surface to draw.
     */
    void renderSurface(QRhiCommandBuffer *cb, QRhi *rhi, const SceneData &data, BrainSurface *surface);

    //=========================================================================================================
    /**
     * End the rendering frame/pass.
     *
     * @param[in] cb         Command buffer.
     */
    void endFrame(QRhiCommandBuffer *cb);
    
private:
    void createResources(QRhi *rhi, QRhiRenderPassDescriptor *rp, int sampleCount);
    
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipelineBackColor; // For Holographic back faces
    std::unique_ptr<QRhiBuffer> m_uniformBuffer;
    
    ShaderMode m_mode = Standard;
    bool m_resourcesDirty = true;
};

#endif // BRAINRENDERER_H
