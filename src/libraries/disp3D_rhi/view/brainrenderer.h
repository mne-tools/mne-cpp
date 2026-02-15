//=============================================================================================================
/**
 * @file     brainrenderer.h
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
 * @brief    BrainRenderer class declaration.
 *
 */

#ifndef BRAINRENDERER_H
#define BRAINRENDERER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include "../core/rendertypes.h"

#include <rhi/qrhi.h>
#include <QMatrix4x4>
#include <QVector3D>
#include <map>
#include <memory>
#include "renderable/brainsurface.h"
#include "renderable/dipoleobject.h"
#include "renderable/networkobject.h"

//=============================================================================================================
/**
 * BrainRenderer handles the low-level RHI rendering logic, managing pipelines, shaders, and draw calls for brain surfaces.
 *
 * @brief    BrainRenderer class.
 */
class DISP3DRHISHARED_EXPORT BrainRenderer
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

    // ShaderMode is defined in core/rendertypes.h for lightweight inclusion.
    // These aliases preserve backward compatibility.
    using ShaderMode = ::ShaderMode;
    static constexpr ShaderMode Standard     = ::Standard;
    static constexpr ShaderMode Holographic  = ::Holographic;
    static constexpr ShaderMode Anatomical   = ::Anatomical;
    static constexpr ShaderMode Dipole       = ::Dipole;
    static constexpr ShaderMode XRay         = ::XRay;
    static constexpr ShaderMode ShowNormals  = ::ShowNormals;

    struct SceneData {
        QMatrix4x4 mvp;
        QVector3D cameraPos;
        QVector3D lightDir;
        bool lightingEnabled;
        float overlayMode = 0.0f;       // 0=Surface, 1=Annotation, 2=Scientific, 3=STC
        QRhiViewport viewport;          // per-pane viewport – re-asserted before every draw
        QRhiScissor scissor;            // per-pane scissor – hard pixel clip
    };
    
    //=========================================================================================================
    /**
     * Initialize resources (shaders, pipelines) for the given RHI and render pass.
     * Ensures pipelines for all supported modes are created.
     *
     * @param[in] rhi        Pointer to QRhi instance.
     * @param[in] rp         Render pass descriptor.
     * @param[in] sampleCount  MSAA sample count.
     */
    void initialize(QRhi *rhi, QRhiRenderPassDescriptor *rp, int sampleCount);
    
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
     * Set uniforms that are shared for the entire frame.
     * 
     * @param[in] data       Scene uniforms (MVP, light, etc).
     */
    void updateSceneUniforms(QRhi *rhi, const SceneData &data);

    //=========================================================================================================
    /**
     * Render a single surface. Must be called between beginFrame and endFrame.
     *
     * @param[in] cb         Command buffer.
     * @param[in] rhi        QRhi pointer.
     * @param[in] data       Scene uniforms (MVP, light, etc).
     * @param[in] surface    Pointer to surface to draw.
     * @param[in] mode       Shader mode to use for this surface.
     */
    void renderSurface(QRhiCommandBuffer *cb, QRhi *rhi, const SceneData &data, BrainSurface *surface, ShaderMode mode);

    //=========================================================================================================
    /**
     * Render dipoles using instanced rendering.
     *
     * @param[in] cb         Command buffer.
     * @param[in] rhi        QRhi pointer.
     * @param[in] data       Scene uniforms.
     * @param[in] dipoles    Pointer to DipoleObject.
     */
    void renderDipoles(QRhiCommandBuffer *cb, QRhi *rhi, const SceneData &data, DipoleObject *dipoles);

    //=========================================================================================================
    /**
     * Render a connectivity network using instanced rendering.
     * Renders both node spheres and edge cylinders as two draw calls.
     *
     * @param[in] cb         Command buffer.
     * @param[in] rhi        QRhi pointer.
     * @param[in] data       Scene uniforms.
     * @param[in] network    Pointer to NetworkObject.
     */
    void renderNetwork(QRhiCommandBuffer *cb, QRhi *rhi, const SceneData &data, NetworkObject *network);

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
    
    // Pipelines for each mode
    std::map<ShaderMode, std::unique_ptr<QRhiGraphicsPipeline>> m_pipelines;
    std::map<ShaderMode, std::unique_ptr<QRhiGraphicsPipeline>> m_pipelinesBackColor; // For Holographic back faces
    
    std::unique_ptr<QRhiBuffer> m_uniformBuffer;
    int m_uniformBufferOffsetAlignment = 0;
    int m_currentUniformOffset = 0;
    
    bool m_resourcesDirty = true;
};

#endif // BRAINRENDERER_H
