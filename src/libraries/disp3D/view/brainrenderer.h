//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainrenderer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Qt-RHI scene renderer: shader pipelines, lighting, dual render targets and per-frame draw orchestration for disp3D.
 *
 * BrainRenderer is the only object in disp3D that talks directly to
 * QRhi. It creates the shader pipelines for every @ref DISP3DLIB::ShaderMode
 * (Standard Phong, Holographic, Anatomical, Dipole arrows, XRay,
 * ShowNormals), allocates the dual @c rtClear / @c rtPreserve render
 * targets needed for the multi-pass cortex + overlay composition, and
 * issues per-frame @c beginPass / @c drawIndexed calls for every
 * visible scene object.
 *
 * The renderer packs camera matrices, light direction and the active
 * overlay mode (@ref DISP3DLIB::BrainRenderer::SceneData) into one uniform
 * slot per draw. All slots and uploads of a frame are prepared before its
 * render pass, which then only binds and draws.
 *
 * All scene objects (@ref DISP3DLIB::BrainSurface, @ref DISP3DLIB::DipoleObject,
 * @ref DISP3DLIB::NetworkObject, @ref DISP3DLIB::VideoOverlay) are accepted through opaque
 * pointers so the renderer never pulls in the heavy headers of the
 * modalities it draws.
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

namespace DISP3DLIB
{

class BrainSurface;
class DipoleObject;
class NetworkObject;
class PolylineObject;
class VideoOverlay;
class SliceObject;

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
    using ShaderMode = DISP3DLIB::ShaderMode;
    static constexpr ShaderMode Standard = DISP3DLIB::Standard;
    static constexpr ShaderMode Holographic = DISP3DLIB::Holographic;
    static constexpr ShaderMode Anatomical = DISP3DLIB::Anatomical;
    static constexpr ShaderMode Dipole = DISP3DLIB::Dipole;
    static constexpr ShaderMode XRay = DISP3DLIB::XRay;
    static constexpr ShaderMode ShowNormals = DISP3DLIB::ShowNormals;

    /**
     * @brief Per-draw scene uniforms: camera matrices, light and overlay mode.
     */
    struct SceneData
    {
        QMatrix4x4 mvp;
        QVector3D cameraPos;
        QVector3D lightDir;
        bool lightingEnabled = true;
        float overlayMode = 0.0f; // 0=FsSurface, 1=FsAnnotation, 2=Scientific, 3=STC
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
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rp, int sampleCount);

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
    void ensureRenderTargets(QRhi* rhi, QRhiTexture* colorTex, const QSize& pixelSize);

    //=========================================================================================================
    /**
     * @return The clearing render target (pass 1).
     */
    QRhiRenderTarget* rtClear() const;

    /**
     * @return The preserving render target (passes 2+).
     */
    QRhiRenderTarget* rtPreserve() const;

    //=========================================================================================================
    /**
     * Start a new frame: frees all per-draw uniform slots. Call before the frame's first prepare call.
     *
     * A frame is recorded in two phases. First every prepare*() call writes its uniforms and
     * uploads into resource update batches. Then beginFrame() commits them, and the issue*()
     * calls only bind and draw: QRhi forbids resource updates inside a render pass, and prepare
     * calls made inside one are rejected with a warning.
     *
     * The renderer owns the GPU copies of the renderables' streams. They are created on the first
     * draw, updated when a stream's generation changes, and freed once the renderable is
     * destroyed or has not been drawn for a while.
     */
    void newFrame();

    /**
     * @return Number of renderables whose GPU copies the renderer currently holds.
     */
    int cachedMeshCount() const;

    //=========================================================================================================
    /**
     * Begin the first render pass of a frame. Clears color and depth.
     *
     * @param[in] cb         Command buffer to record to.
     * @param[in] updates    Resource updates to commit before the pass, or nullptr.
     */
    void beginFrame(QRhiCommandBuffer* cb, QRhiResourceUpdateBatch* updates = nullptr);

    //=========================================================================================================
    /**
     * Begin an additional render pass that preserves previous output.
     * Uses the internal preserving render target (m_rtPreserve).
     *
     * @param[in] cb         Command buffer.
     * @param[in] updates    Resource updates to commit before the pass, or nullptr.
     */
    void beginPreservingPass(QRhiCommandBuffer* cb, QRhiResourceUpdateBatch* updates = nullptr);

    //=========================================================================================================
    /**
     * Upload the surface's streams if they changed and write the uniforms of one surface draw.
     *
     * @param[in] u          Resource update batch, committed before the pass.
     * @param[in] data       Scene uniforms (MVP, light, etc).
     * @param[in] surface    Pointer to surface.
     * @return Uniform buffer offset for this draw, or -1 on failure.
     */
    int prepareSurfaceDraw(QRhiResourceUpdateBatch* u, const SceneData& data, BrainSurface* surface);

    /**
     * Draw a surface with the uniforms from prepareSurfaceDraw(). Call inside a pass.
     *
     * @param[in] cb             Command buffer.
     * @param[in] surface        Pointer to surface to draw.
     * @param[in] mode           Shader mode.
     * @param[in] uniformOffset  Offset returned by prepareSurfaceDraw().
     */
    void issueSurfaceDraw(QRhiCommandBuffer* cb, BrainSurface* surface, ShaderMode mode, int uniformOffset);

    //=========================================================================================================
    /**
     * Upload the dipoles' streams if they changed and write the uniforms of their draw.
     *
     * @param[in] u          Resource update batch, committed before the pass.
     * @param[in] data       Scene uniforms.
     * @param[in] dipoles    Pointer to DipoleObject.
     * @return Uniform buffer offset, or -1 when nothing is drawn.
     */
    int prepareDipoleDraw(QRhiResourceUpdateBatch* u, const SceneData& data, DipoleObject* dipoles);

    /**
     * Draw dipoles as instanced cones. Call inside a pass.
     *
     * @param[in] cb             Command buffer.
     * @param[in] dipoles        Pointer to DipoleObject.
     * @param[in] uniformOffset  Offset returned by prepareDipoleDraw().
     */
    void issueDipoleDraw(QRhiCommandBuffer* cb, DipoleObject* dipoles, int uniformOffset);

    /**
     * Upload the network's streams if they changed and write the uniforms of its draws.
     *
     * @param[in] u          Resource update batch, committed before the pass.
     * @param[in] data       Scene uniforms.
     * @param[in] network    Pointer to NetworkObject.
     * @return Uniform buffer offset, or -1 when nothing is drawn.
     */
    int prepareNetworkDraw(QRhiResourceUpdateBatch* u, const SceneData& data, NetworkObject* network);

    /**
     * Draw a connectivity network: instanced node spheres and edge cylinders. Call inside a pass.
     *
     * @param[in] cb             Command buffer.
     * @param[in] network        Pointer to NetworkObject.
     * @param[in] uniformOffset  Offset returned by prepareNetworkDraw().
     */
    void issueNetworkDraw(QRhiCommandBuffer* cb, NetworkObject* network, int uniformOffset);

    /**
     * Upload the polyline's streams if they changed and write the uniforms of its draw.
     *
     * @param[in] u          Resource update batch, committed before the pass.
     * @param[in] data       Scene uniforms.
     * @param[in] polyline   Pointer to PolylineObject.
     * @return Uniform buffer offset, or -1 when nothing is drawn.
     */
    int preparePolylineDraw(QRhiResourceUpdateBatch* u, const SceneData& data, PolylineObject* polyline);

    /**
     * Draw a polyline as connected segments. Call inside a pass.
     *
     * @param[in] cb             Command buffer.
     * @param[in] polyline       Pointer to PolylineObject.
     * @param[in] uniformOffset  Offset returned by preparePolylineDraw().
     */
    void issuePolylineDraw(QRhiCommandBuffer* cb, PolylineObject* polyline, int uniformOffset);

    //=========================================================================================================
    /**
     * Prepare live video overlay GPU resources and upload the latest frame.
     *
     * Must be called before a render pass starts. Texture uploads require a
     * blit encoder on Metal and must not be recorded while a render encoder is
     * active.
     *
     * @param[in] rhi        QRhi pointer.
     * @param[in] u          Resource update batch owned by the caller.
     * @param[in] overlay    Pointer to VideoOverlay; ignored when null/disabled.
     */
    void prepareVideoOverlay(QRhi* rhi,
                             QRhiResourceUpdateBatch* u,
                             DISP3DLIB::VideoOverlay* overlay);

    //=========================================================================================================
    /**
     * Write the uniforms of one video overlay draw: a camera-facing quad at the focus position,
     * or, with a target surface, a decal projected onto that mesh. Call after prepareVideoOverlay().
     *
     * @param[in] u          Resource update batch, committed before the pass.
     * @param[in] data       Scene uniforms (MVP, camera position).
     * @param[in] overlay    Pointer to VideoOverlay; ignored when null/disabled.
     * @param[in] surface    Target surface mesh (typically scalp/head), or nullptr for the quad.
     * @return Uniform buffer offset for this draw, or -1 when nothing is drawn.
     */
    int prepareVideoOverlayDraw(QRhiResourceUpdateBatch* u,
                                const SceneData& data,
                                DISP3DLIB::VideoOverlay* overlay,
                                BrainSurface* surface);

    /**
     * Draw a video overlay prepared with prepareVideoOverlayDraw(). Call inside a pass.
     *
     * @param[in] cb             Command buffer.
     * @param[in] overlay        Pointer to VideoOverlay.
     * @param[in] surface        Same target surface as in prepareVideoOverlayDraw().
     * @param[in] uniformOffset  Offset returned by prepareVideoOverlayDraw().
     */
    void issueVideoOverlayDraw(QRhiCommandBuffer* cb,
                               DISP3DLIB::VideoOverlay* overlay,
                               BrainSurface* surface,
                               int uniformOffset);

    //=========================================================================================================
    /**
     * Prepare MRI slice GPU resources and upload texture data.
     *
     * Must be called before a render pass starts. Creates the slice
     * pipeline lazily on first use and uploads the slice texture.
     *
     * @param[in] rhi        QRhi pointer.
     * @param[in] u          Resource update batch owned by the caller.
     * @param[in] slice      Pointer to SliceObject; ignored when null.
     * @param[in] slotIndex  Slot index (0=axial, 1=coronal, 2=sagittal).
     */
    void prepareSlice(QRhi* rhi,
                      QRhiResourceUpdateBatch* u,
                      DISP3DLIB::SliceObject* slice,
                      int slotIndex);

    //=========================================================================================================
    /**
     * Write the uniforms of one MRI slice draw into a resource update batch.
     *
     * @param[in] u          Resource update batch, committed before the pass.
     * @param[in] data       Scene uniforms (MVP, camera position).
     * @param[in] slotIndex  Slot index (0=axial, 1=coronal, 2=sagittal).
     * @return Uniform buffer offset for this draw, or -1 on failure.
     */
    int prepareSliceDraw(QRhiResourceUpdateBatch* u,
                         const SceneData& data,
                         int slotIndex);

    //=========================================================================================================
    /**
     * Draw an MRI slice prepared with prepareSliceDraw(). Call inside a pass.
     *
     * @param[in] cb             Command buffer (must be inside a render pass).
     * @param[in] slotIndex      Slot index matching the prepareSliceDraw call.
     * @param[in] uniformOffset  Offset returned by prepareSliceDraw().
     */
    void issueSliceDraw(QRhiCommandBuffer* cb,
                        int slotIndex,
                        int uniformOffset);

    //=========================================================================================================
    /**
     * End any render pass (clearing or preserving).
     *
     * @param[in] cb         Command buffer.
     */
    void endPass(QRhiCommandBuffer* cb);

private:
    /** @brief Private implementation holding QRhi pipelines, shader resources, and uniform buffers (PIMPL). */
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace DISP3DLIB

#endif // BRAINRENDERER_H
