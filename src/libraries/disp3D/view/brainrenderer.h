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

#include "../disp3D_global.h"

#include "../core/rendertypes.h"

#include <QMatrix4x4>
#include <QVector3D>
#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QRhi;
class QRhiCommandBuffer;
class QRhiRenderTarget;
class QRhiRenderPassDescriptor;
class QRhiResourceUpdateBatch;
class QRhiTexture;
class QRhiRenderBuffer;
class QRhiTextureRenderTarget;
class BrainSurface;
class DipoleObject;
class NetworkObject;

//=============================================================================================================
/**
 * BrainRenderer handles the low-level RHI rendering logic, managing pipelines, shaders, and draw calls for brain surfaces.
 *
 * @brief Qt RHI-based 3-D renderer managing scene objects, lighting, camera, and render pipeline for brain visualization.
 */
class DISP3DSHARED_EXPORT BrainRenderer
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

    /**
     * @brief Aggregated GPU resources and render state for the 3-D brain visualization scene.
     *
     * Lightweight scene uniform block passed to every draw call.
     *
     * Viewport and scissor are stored as plain numbers so that this struct
     * does not pull in any Qt-private QRhi headers.
     */
    struct SceneData {
        QMatrix4x4 mvp;
        QVector3D cameraPos;
        QVector3D lightDir;
        bool lightingEnabled;
        float overlayMode = 0.0f;       // 0=FsSurface, 1=FsAnnotation, 2=Scientific, 3=STC

        // Viewport rectangle (floating-point, pixels)
        float viewportX = 0, viewportY = 0, viewportW = 0, viewportH = 0;
        // Scissor rectangle (integer, pixels)
        int scissorX = 0, scissorY = 0, scissorW = 0, scissorH = 0;
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
     * Create the dual render-target pair for multi-pass rendering.
     *
     * Both render targets share the same color texture and depth-stencil
     * buffer but differ in their load/store flags:
     *   - m_rtClear:    clears color+depth on beginPass  (first pass)
     *   - m_rtPreserve: preserves previous output        (passes 2+)
     *
     * Qt bakes these flags into native resources at create() time, so
     * calling setFlags() dynamically does NOT work (especially on WebGL).
     * This is the validated pattern from test_wasm_multi_pass.
     *
     * @param[in] rhi        QRhi instance.
     * @param[in] colorTex   Color texture (owned by QRhiWidget or caller).
     * @param[in] pixelSize  Render target dimensions.
     */
    void ensureRenderTargets(QRhi *rhi, QRhiTexture *colorTex, const QSize &pixelSize);

    //=========================================================================================================
    /**
     * @return The clearing render target (pass 1).
     */
    QRhiRenderTarget *rtClear() const;

    /**
     * @return The preserving render target (passes 2+).
     */
    QRhiRenderTarget *rtPreserve() const;

    //=========================================================================================================
    /**
     * Begin the first render pass of a frame. Clears color and depth.
     *
     * @param[in] cb         Command buffer to record to.
     */
    void beginFrame(QRhiCommandBuffer *cb);

    //=========================================================================================================
    /**
     * Begin an additional render pass that preserves previous output.
     * Uses the internal preserving render target (m_rtPreserve).
     *
     * @param[in] cb         Command buffer.
     */
    void beginPreservingPass(QRhiCommandBuffer *cb);

    //=========================================================================================================
    /**
     * Set uniforms that are shared for the entire frame.
     * 
     * @param[in] data       Scene uniforms (MVP, light, etc).
     */
    void updateSceneUniforms(QRhi *rhi, const SceneData &data);

    //=========================================================================================================
    /**
     * Render a single surface. Must be called between beginFrame/beginPreservingPass and endPass.
     *
     * @param[in] cb         Command buffer.
     * @param[in] rhi        QRhi pointer.
     * @param[in] data       Scene uniforms (MVP, light, etc).
     * @param[in] surface    Pointer to surface to draw.
     * @param[in] mode       Shader mode to use for this surface.
     */
    void renderSurface(QRhiCommandBuffer *cb, QRhi *rhi, const SceneData &data, BrainSurface *surface, ShaderMode mode);

    //=========================================================================================================
    // ── WORKAROUND(QRhi-GLES2) ──────────────────────────────────────────
    // The Qt QRhi GLES2/WebGL backend has a bug where only the first
    // drawIndexed() per render pass produces visible output.  On WASM we
    // work around this by merging all brain surfaces into a single VBO/IBO
    // and issuing one drawIndexed().  When the upstream bug is fixed these
    // two methods (and the merged buffers in Impl) can be removed.
    //
    /**
     * Prepare merged brain surface geometry for single-drawIndexed rendering.
     * Call BEFORE beginFrame() with a pre-upload resource batch.
     *
     * Uses dirty-flag caching: geometry is only rebuilt when the surface
     * list changes (add/remove/visibility toggle).  Per-vertex color
     * updates (STC animation) only re-upload the color channel.
     *
     * @param[in] rhi        QRhi pointer.
     * @param[in] u          Resource update batch (pre-render uploads).
     * @param[in] surfaces   Brain surfaces to merge.
     * @param[in] groupName  Category name (e.g. "brain", "bem", "srcsp").
     */
    void prepareMergedSurfaces(QRhi *rhi, QRhiResourceUpdateBatch *u,
                               const QVector<BrainSurface*> &surfaces,
                               const QString &groupName = QStringLiteral("default"));

    /**
     * Mark a merged group as dirty so it is rebuilt on the next
     * prepareMergedSurfaces call.  Call when surfaces are added/removed
     * or visibility changes.
     *
     * @param[in] groupName  Category name to invalidate.
     */
    void invalidateMergedGroup(const QString &groupName = QStringLiteral("default"));

    /**
     * Check if a merged group has drawable geometry (indexCount > 0).
     * Use before beginPreservingPass() to avoid empty render passes
     * which can clear the framebuffer on some WebGL implementations.
     *
     * @param[in] groupName  Category name to check.
     * @return true if the group exists and has indices to draw.
     */
    bool hasMergedContent(const QString &groupName) const;

    /**
     * Draw previously prepared merged surfaces in a single drawIndexed.
     * Call between beginFrame()/beginPreservingPass() and endPass().
     *
     * @param[in] cb         Command buffer.
     * @param[in] rhi        QRhi pointer.
     * @param[in] data       Scene uniforms.
     * @param[in] mode       Shader mode.
     */
    void drawMergedSurfaces(QRhiCommandBuffer *cb, QRhi *rhi,
                            const SceneData &data, ShaderMode mode,
                            const QString &groupName = QStringLiteral("default"));

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
     * End any render pass (clearing or preserving).
     *
     * @param[in] cb         Command buffer.
     */
    void endPass(QRhiCommandBuffer *cb);
    
private:
    /** @brief Private implementation holding QRhi pipelines, shader resources, and uniform buffers (PIMPL). */
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // BRAINRENDERER_H
