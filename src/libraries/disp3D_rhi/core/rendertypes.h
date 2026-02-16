//=============================================================================================================
/**
 * @file     rendertypes.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    Lightweight render-related enums shared across the disp3D_rhi library.
 *
 *           This header deliberately avoids pulling in any Qt-private or QRhi
 *           headers so that it can be included from public API headers without
 *           leaking implementation details to downstream consumers.
 */

#ifndef RENDERTYPES_H
#define RENDERTYPES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

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
