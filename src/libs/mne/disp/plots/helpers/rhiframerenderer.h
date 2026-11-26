//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rhiframerenderer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    RhiFrameRenderer executes a scene::GpuFrame with QRhi.
 *
 * The QRhi backend of the GPU frame description of mne_scene: it keeps one QRhi buffer, texture and pipeline per
 * resource handle, uploads what changed, packs the uniform blocks of all draws into one dynamic uniform buffer and
 * records the passes. Shaders are the precompiled .qsb files of qt_add_shaders, found by their id under the
 * registered resource prefixes; figure textures are painted with QPainter.
 */

#ifndef DISP_RHIFRAMERENDERER_H
#define DISP_RHIFRAMERENDERER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include <mne/scene/gpu_frame.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <string>
#include <vector>

class QFont;
class QImage;
class QRhi;
class QRhiCommandBuffer;
class QRhiRenderTarget;

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

/**
 * @brief Executes scene::GpuFrame descriptions with QRhi.
 *
 * Use one renderer per QRhi and resource store; call releaseResources() before the QRhi goes away.
 *
 * @since 3.0.0
 */
class DISPSHARED_EXPORT RhiFrameRenderer
{
public:
    RhiFrameRenderer();
    ~RhiFrameRenderer();

    RhiFrameRenderer(const RhiFrameRenderer&) = delete;
    RhiFrameRenderer& operator=(const RhiFrameRenderer&) = delete;

    //=========================================================================================================
    /**
     * Adds a resource directory where the shaders of an id are looked up: the files are the prefix followed by the
     * id and ".vert.qsb" or ".frag.qsb".
     *
     * @param[in] prefix   Resource path ending with '/', e.g. ":/disp/shaders/viewers/helpers/shaders/".
     */
    void addShaderPrefix(const std::string& prefix);

    //=========================================================================================================
    /**
     * Sets the font that fills in what the fonts of figure textures leave unset.
     *
     * @param[in] font   Base font.
     */
    void setFont(const QFont& font);

    //=========================================================================================================
    /**
     * @param[in] rhi                The QRhi.
     * @param[in] target             Render target.
     * @param[in] devicePixelRatio   Pixels per logical pixel.
     *
     * @return What a producer needs to know about the target, with QRhi's clip-space correction.
     */
    static scene::GpuFrameTarget frameTarget(QRhi* rhi, QRhiRenderTarget* target, double devicePixelRatio);

    //=========================================================================================================
    /**
     * Uploads what changed and records the frame into @p cb.
     *
     * Passes after the first that clear no colour continue the previous QRhi pass; their depth is not cleared
     * then.
     *
     * @param[in] rhi         The QRhi.
     * @param[in] target      Render target.
     * @param[in] cb          Command buffer of the current frame.
     * @param[in] frame       Passes and draws.
     * @param[in] resources   Buffers, textures and pipelines the frame refers to.
     */
    void render(QRhi* rhi, QRhiRenderTarget* target, QRhiCommandBuffer* cb, const scene::GpuFrame& frame,
                const scene::GpuResources& resources);

    //=========================================================================================================
    /**
     * Compares the draws of a frame with the reflection of their shaders: every uniform block a draw sets has a
     * block of the same size at its binding, every texture a sampler, and every vertex attribute of the pipeline
     * an input of the same location and number of components; every block, sampler and input of a shader is set.
     *
     * @param[in] frame       Passes and draws.
     * @param[in] resources   Pipelines the frame refers to.
     *
     * @return The mismatches, empty if the frame fits its shaders.
     */
    std::vector<std::string> checkShaders(const scene::GpuFrame& frame, const scene::GpuResources& resources) const;

    //=========================================================================================================
    /**
     * @param[in] texture   Handle of a texture of the last rendered frame.
     *
     * @return The image last uploaded for the texture, with the figure's device pixel ratio, or a null image.
     */
    QImage textureImage(scene::GpuHandle texture) const;

    //=========================================================================================================
    /**
     * Releases every QRhi object.
     */
    void releaseResources();

private:
    struct Impl;
    std::unique_ptr<Impl> m_pImpl;
};

} // namespace mne::disp

#endif // DISP_RHIFRAMERENDERER_H
