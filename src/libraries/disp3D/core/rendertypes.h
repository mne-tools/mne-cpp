//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rendertypes.h
 * @since 2026
 * @date  March 2026
 * @brief Lightweight render-related enums (ShaderMode, VisualizationMode) shared across disp3D.
 *
 * Kept QRhi-free and free of heavy Qt includes so it can be pulled in
 * from any disp3D translation unit without forcing a recompile of
 * every shader pipeline when a new render mode is added.
 *
 * @ref ShaderMode selects between the Standard Phong lighting model,
 * Holographic (translucent fresnel), Anatomical (matte tissue),
 * Dipole (instanced arrow shader), XRay (additive front-faces) and
 * ShowNormals (debug visualisation of vertex normals). @ref
 * VisualizationMode selects how per-vertex colour is computed in the
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

#include <cstdint>

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
    Standard,       /**< Default Phong-style shading. */
    Holographic,    /**< Two-sided holographic effect. */
    Anatomical,     /**< Anatomical / curvature-based coloring. */
    Dipole,         /**< Specialized dipole rendering. */
    XRay,           /**< Semi-transparent X-ray effect. */
    ShowNormals     /**< Visualise surface normals as colour. */
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
    ModeSurface,         /**< Plain surface colours (curvature-derived). */
    ModeAnnotation,      /**< Atlas / parcellation annotation colours. */
    ModeScientific,      /**< Scientific colourmap (curvature). */
    ModeSourceEstimate   /**< Source-estimate overlay colours. */
};

#endif // RENDERTYPES_H
