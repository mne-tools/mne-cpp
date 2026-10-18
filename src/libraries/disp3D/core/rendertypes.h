//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rendertypes.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Lightweight render-related enums (ShaderMode, VisualizationMode) shared across disp3D.
 *
 * Kept QRhi-free and free of heavy Qt includes so it can be pulled in
 * from any disp3D translation unit without forcing a recompile of
 * every shader pipeline when a new render mode is added.
 *
 * @ref DISP3DLIB::ShaderMode selects between the Standard Phong lighting model,
 * Holographic (translucent fresnel), Anatomical (matte tissue),
 * Dipole (instanced arrow shader), XRay (additive front-faces) and
 * ShowNormals (debug visualisation of vertex normals). @ref
 * DISP3DLIB::VisualizationMode selects how per-vertex colour is computed in the
 * fragment shader: from the base surface tint, from FsAnnotation
 * parcellation, from a scientific (curvature-shaded) palette, or
 * from a source-time-course overlay.
 */

#ifndef RENDERTYPES_H
#define RENDERTYPES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <QtGlobal>

#include <cstdint>
#include <memory>

namespace DISP3DLIB
{

//=============================================================================================================
/**
 * Pack RGBA colour components into a single ABGR uint32_t suitable for
 * GPU vertex attributes declared as UNormByte4.
 *
 * @param[in] r  Red   channel (0–255).
 * @param[in] g  Green channel (0–255).
 * @param[in] b  Blue  channel (0–255).
 * @param[in] a  Alpha channel (0–255, default 255).
 * @return Packed ABGR colour.
 */
inline uint32_t packABGR(uint32_t r, uint32_t g, uint32_t b, uint32_t a = 0xFF)
{
    return (a << 24) | (b << 16) | (g << 8) | r;
}

//=============================================================================================================
/**
 * Shader pipeline modes supported by the renderer.
 *
 * Each mode corresponds to a different GPU shader program and visual style.
 * Values are kept compatible with the original BrainRenderer::ShaderMode enum.
 */
enum ShaderMode
{
    Standard,    /**< Default Phong-style shading. */
    Holographic, /**< Two-sided holographic effect. */
    Anatomical,  /**< Anatomical / curvature-based coloring. */
    Dipole,      /**< Specialized dipole rendering. */
    XRay,        /**< Semi-transparent X-ray effect. */
    ShowNormals  /**< Visualise surface normals as colour. */
};

//=============================================================================================================
/**
 * Overlay visualisation modes for brain surfaces.
 *
 * Controls which vertex-colour channel drives the final appearance.
 * Values are kept compatible with the original BrainSurface::VisualizationMode enum.
 */
enum VisualizationMode
{
    ModeSurface,       /**< Plain surface colours (curvature-derived). */
    ModeAnnotation,    /**< Atlas / parcellation annotation colours. */
    ModeScientific,    /**< Scientific colourmap (curvature). */
    ModeSourceEstimate /**< Source-estimate overlay colours. */
};

//=============================================================================================================
/**
 * @brief CPU-side contents of one GPU buffer of a renderable.
 *
 * Renderables own only CPU data; the renderer keeps the GPU copies and uploads a stream again
 * when its generation changes. The data must stay valid until the next change of the renderable.
 */
struct GpuStream
{
    const void* data = nullptr; /**< First byte, or nullptr when the stream is empty. */
    quint32 bytes = 0;          /**< Size in bytes. */
    quint64 generation = 0;     /**< Bumped by the owner on every change of the data. */
};

//=============================================================================================================
/**
 * @brief CPU streams of an instanced mesh: one shape (vertices, indices) drawn once per instance.
 */
struct InstancedStreams
{
    GpuStream vertices;    /**< Shape vertices. */
    GpuStream indices;     /**< Shape triangle indices (uint32). */
    GpuStream instances;   /**< Per-instance data. */
    int indexCount = 0;    /**< Number of indices of the shape. */
    int instanceCount = 0; /**< Number of instances. */
};

//=============================================================================================================
/**
 * @brief Identity of a renderable in the renderer's GPU resource cache.
 *
 * The renderer drops the GPU copies of a renderable once its key is destroyed. A copied
 * renderable gets a key of its own, so it never shares GPU buffers with the original.
 */
class RenderKey
{
public:
    RenderKey()
    : m_token(std::make_shared<char>())
    {
    }
    RenderKey(const RenderKey&)
    : RenderKey()
    {
    }
    RenderKey& operator=(const RenderKey&)
    {
        return *this;
    }

    /** @return Address that identifies the renderable while it lives. */
    const void* id() const
    {
        return m_token.get();
    }

    /** @return Token that expires when the renderable is destroyed. */
    std::weak_ptr<const void> token() const
    {
        return m_token;
    }

private:
    std::shared_ptr<char> m_token;
};

} // namespace DISP3DLIB

#endif // RENDERTYPES_H
