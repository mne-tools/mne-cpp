//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainrenderer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Qt-RHI scene renderer: shader pipelines, lighting, dual render targets and per-frame draw orchestration for mne_viz_qt.
 *
 * BrainRenderer is the only object in mne_viz_qt that talks directly to
 * QRhi. It creates the shader pipelines for every @ref mne::viz::ShaderMode
 * (Standard Phong, Holographic, Anatomical, Dipole arrows, XRay,
 * ShowNormals), allocates the dual @c rtClear / @c rtPreserve render
 * targets needed for the multi-pass cortex + overlay composition, and
 * issues per-frame @c beginPass / @c drawIndexed calls for every
 * visible scene object.
 *
 * The renderer packs camera matrices, light direction and the active
 * overlay mode (@ref mne::viz::qt::BrainRenderer::SceneData) into one uniform
 * slot per draw. All slots and uploads of a frame are prepared before its
 * render pass, which then only binds and draws.
 *
 * All scene objects (@ref mne::viz::BrainSurface, @ref mne::viz::qt::DipoleObject,
 * @ref mne::viz::qt::NetworkObject, @ref mne::viz::qt::VideoOverlay) are accepted through opaque
 * pointers so the renderer never pulls in the heavy headers of the
 * modalities it draws.
 */

#ifndef BRAINRENDERER_H
#define BRAINRENDERER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"
#include "../../../../brain/rendertypes.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMatrix4x4>
#include <QVector3D>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

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
class QImage;

namespace mne::viz
{
class BrainSurface;
} // namespace mne::viz

namespace mne::viz::qt
{
class DipoleObject;
class ElectrodeObject;
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
class VIZQTSHARED_EXPORT BrainRenderer
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
     * @return The clearing render target (pass 1): the external target when one is set, else the own one.
     */
    QRhiRenderTarget* rtClear() const;

    //=========================================================================================================
    /**
     * Draw into a render target owned by the caller, e.g. the current target of a QRhiSwapChain, instead
     * of the own targets built by ensureRenderTargets(). beginFrame() then clears and draws into it.
     *
     * @param[in] rt         External render target, or nullptr to use the own targets again.
     * @since 3.0.0
     */
    void setExternalRenderTarget(QRhiRenderTarget* rt);

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

    /**
     * Upload the electrodes' streams if they changed and write the uniforms of their draws.
     *
     * @param[in] u           Resource update batch, committed before the pass.
     * @param[in] data        Scene uniforms.
     * @param[in] electrodes  Pointer to ElectrodeObject.
     * @return Uniform buffer offset, or -1 when nothing is drawn.
     */
    int prepareElectrodeDraw(QRhiResourceUpdateBatch* u, const SceneData& data, ElectrodeObject* electrodes);

    /**
     * Draw electrode lead bodies and contacts, opaque and depth-tested. Call inside a pass.
     *
     * @param[in] cb             Command buffer.
     * @param[in] electrodes     Pointer to ElectrodeObject.
     * @param[in] uniformOffset  Offset returned by prepareElectrodeDraw().
     */
    void issueElectrodeDraw(QRhiCommandBuffer* cb, ElectrodeObject* electrodes, int uniformOffset);

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
                             mne::viz::qt::VideoOverlay* overlay);

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
                                mne::viz::qt::VideoOverlay* overlay,
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
                               mne::viz::qt::VideoOverlay* overlay,
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
                      mne::viz::qt::SliceObject* slice,
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

    //=========================================================================================================
    /**
     * Upload a screen-space overlay (text labels, pane separators) that issueScreenOverlayDraw() blends
     * over the whole render target. The texture is re-uploaded only when the image changed.
     *
     * @param[in] rhi        QRhi instance.
     * @param[in] u          Resource update batch, committed before the pass.
     * @param[in] overlay    Premultiplied RGBA image of the target's pixel size; a null image draws nothing.
     * @since 3.0.0
     */
    void prepareScreenOverlay(QRhi* rhi, QRhiResourceUpdateBatch* u, const QImage& overlay);

    //=========================================================================================================
    /**
     * Alpha-blend the overlay uploaded by prepareScreenOverlay() over the whole render target.
     * Call inside the pass, after the 3-D scene.
     *
     * @param[in] cb         Command buffer.
     * @since 3.0.0
     */
    void issueScreenOverlayDraw(QRhiCommandBuffer* cb);

    //=========================================================================================================
    /**
     * @return Whether the last prepareScreenOverlay() call left an overlay to draw.
     * @since 3.0.0
     */
    bool hasScreenOverlay() const;

private:
    /** @brief Private implementation holding QRhi pipelines, shader resources, and uniform buffers (PIMPL). */
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace mne::viz::qt

#endif // BRAINRENDERER_H
