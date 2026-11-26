//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brain_frame.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    BrainFrameBuilder describes a frame of the 3-D brain view as a GpuFrame.
 */

#ifndef SCENE_BRAIN_FRAME_H
#define SCENE_BRAIN_FRAME_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gpu_frame.h"
#include "scene_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <variant>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

/**
 * How a mesh of the brain view is shaded; the first six follow disp::ShaderMode.
 */
enum class BrainShader : std::uint8_t
{
    Standard,    /**< Lit surface (standard.vert/.frag). */
    Holographic, /**< Translucent additive shell, back faces then front faces (holographic.vert/.frag). */
    Anatomical,  /**< Tissue colours by tissue type (anatomical.vert/.frag). */
    Dipole,      /**< Translucent instanced glyphs (dipole.vert/.frag). */
    XRay,        /**< Holographic without depth test. */
    ShowNormals, /**< Normals as colours (shownormals.vert/.frag). */
    Electrode    /**< Opaque instanced glyphs (electrode.vert/.frag). */
};

/**
 * @brief The uniform block of the surface shaders (binding 0 of standard, holographic, anatomical, shownormals).
 *
 * @since 3.0.0
 */
struct BrainSurfaceUniforms
{
    float mvp[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f}; /**< Offset 0: projection * view * model, column-major. */
    float camera_pos[3] = {0.f, 0.f, 0.f};                                                            /**< Offset 64: camera position in model space. */
    float is_selected = 0.f;                                                                          /**< Offset 76: 1 to highlight the whole surface. */
    float light_dir[3] = {0.f, 0.f, 1.f};                                                             /**< Offset 80: direction towards the light. */
    float tissue_type = 0.f;                                                                          /**< Offset 92: 0 unknown, 1 brain, 2 skin, 3 outer skull, 4 inner skull. */
    float lighting_enabled = 1.f;                                                                     /**< Offset 96: 1 to light the surface. */
    float overlay_mode = 0.f;                                                                         /**< Offset 100: 0 curvature, 1 annotation, 2 scientific, 3 source estimate. */
    float label_active = 0.f;                                                                         /**< Offset 104: 1 to highlight the vertices of selected_label. */
    float padding = 0.f;                                                                              /**< Offset 108. */
    float selected_label[4] = {0.f, 0.f, 0.f, 0.f};                                                   /**< Offset 112: annotation colour of the highlighted region. */
};

static_assert(sizeof(BrainSurfaceUniforms) == 128, "BrainSurfaceUniforms must match the uniform block of standard.vert");

/**
 * @brief The uniform block of the instanced glyph shaders (binding 0 of dipole and electrode).
 *
 * @since 3.0.0
 */
struct BrainInstanceUniforms
{
    float mvp[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f}; /**< Offset 0: projection * view, column-major. */
    float camera_pos[3] = {0.f, 0.f, 0.f};                                                            /**< Offset 64: camera position. */
    float padding = 0.f;                                                                              /**< Offset 76. */
    float light_dir[3] = {0.f, 0.f, 1.f};                                                             /**< Offset 80: direction towards the light. */
    float lighting_enabled = 1.f;                                                                     /**< Offset 92: 1 to light the glyphs. */
};

static_assert(sizeof(BrainInstanceUniforms) == 96, "BrainInstanceUniforms must match the uniform block of dipole.vert");

/**
 * @brief Bytes of a mesh stream and a generation its owner changes with them.
 *
 * @since 3.0.0
 */
struct GpuStreamView
{
    std::span<const std::byte> bytes; /**< Contents; empty for no stream. */
    std::uint64_t generation = 0;     /**< Changes with the contents; equal generations are not uploaded again. */
};

/**
 * @brief The vertex and index streams of a surface in the layout of the surface shaders.
 *
 * @since 3.0.0
 */
struct BrainSurfaceStreams
{
    GpuStreamView geometry;        /**< Six floats per vertex: position and normal. */
    GpuStreamView colors;          /**< RGBA bytes per vertex; alpha is the curvature grey. */
    GpuStreamView annotations;     /**< RGBA bytes per vertex: annotation colour. */
    GpuStreamView indices;         /**< 32-bit triangle indices. */
    std::uint32_t index_count = 0; /**< Number of indices. */
};

/**
 * @brief The streams of an instanced glyph mesh: one shape drawn once per instance.
 *
 * @since 3.0.0
 */
struct BrainInstancedStreams
{
    GpuStreamView vertices;           /**< Six floats per shape vertex: position and normal. */
    GpuStreamView instances;          /**< 21 floats per instance: four model matrix columns, RGBA, selected flag. */
    GpuStreamView indices;            /**< 32-bit triangle indices of the shape. */
    std::uint32_t index_count = 0;    /**< Number of shape indices. */
    std::uint32_t instance_count = 0; /**< Number of instances. */
};

/**
 * @brief Describes the frames of the brain view as GpuFrame for any backend.
 *
 * A frame is begun with begin(), filled pane by pane with surfaces, instanced glyphs and the screen overlay in
 * drawing order, and taken with finish(). Meshes are identified by a key of the caller; their streams are copied
 * into resources() when their generation changes, and dropped when a frame does not draw them for a while or on
 * release().
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT BrainFrameBuilder
{
public:
    /** Frames a mesh may go undrawn before its buffers are dropped. */
    static constexpr std::uint64_t kUnusedFrames = 300;

    //=========================================================================================================
    /**
     * Creates a builder with the pipelines of every brain shader.
     */
    BrainFrameBuilder();

    //=========================================================================================================
    /**
     * Starts a frame.
     *
     * @param[in] target   Size and pixel ratio of the target.
     * @param[in] clear    Colour the frame clears to.
     */
    void begin(const GpuFrameTarget& target, Color clear = Color::black());

    //=========================================================================================================
    /**
     * Draws a surface; holographic and x-ray surfaces draw their back faces first.
     *
     * @param[in] key        Identity of the mesh, e.g. the address of its owner.
     * @param[in] streams    Vertex and index streams.
     * @param[in] shader     Surface shader: Standard, Holographic, Anatomical, XRay or ShowNormals.
     * @param[in] uniforms   Shader parameters.
     * @param[in] viewport   Pane in target pixels.
     *
     * @return Whether anything was drawn: false for empty streams or a glyph shader.
     */
    bool draw_surface(std::uint64_t key, const BrainSurfaceStreams& streams, BrainShader shader, const BrainSurfaceUniforms& uniforms,
                      const GpuRect& viewport);

    //=========================================================================================================
    /**
     * Draws instanced glyphs.
     *
     * @param[in] key        Identity of the mesh.
     * @param[in] streams    Shape and instance streams.
     * @param[in] shader     Dipole (translucent) or Electrode (opaque).
     * @param[in] uniforms   Shader parameters.
     * @param[in] viewport   Pane in target pixels.
     *
     * @return Whether anything was drawn: false for empty streams or a surface shader.
     */
    bool draw_instanced(std::uint64_t key, const BrainInstancedStreams& streams, BrainShader shader, const BrainInstanceUniforms& uniforms,
                        const GpuRect& viewport);

    //=========================================================================================================
    /**
     * Blends a screen-sized overlay over everything drawn so far, e.g. labels and pane separators.
     *
     * @param[in] overlay   RGBA pixels (not premultiplied) of the target size, or a figure of the logical size.
     * @param[in] scale     Texture pixels per figure pixel of a figure.
     */
    void draw_overlay(std::variant<Image, DisplayList> overlay, double scale = 1.0);

    //=========================================================================================================
    /**
     * Ends the frame and drops the buffers of meshes undrawn for kUnusedFrames frames.
     *
     * @return The frame, referring to resources().
     */
    GpuFrame finish();

    //=========================================================================================================
    /**
     * Drops the buffers of a mesh.
     *
     * @param[in] key   Identity of the mesh.
     */
    void release(std::uint64_t key);

    /** @return The number of meshes with buffers. */
    int mesh_count() const
    {
        return static_cast<int>(m_meshes.size());
    }

    /** @return The buffers, textures and pipelines of the frames. */
    const GpuResources& resources() const
    {
        return m_resources;
    }

private:
    struct Mesh
    {
        GpuHandle buffers[4] = {0, 0, 0, 0};
        std::uint64_t generations[4] = {0, 0, 0, 0};
        bool uploaded[4] = {false, false, false, false};
        std::uint64_t last_frame = 0;
    };

    GpuHandle stream_buffer(Mesh& mesh, int slot, GpuBufferUsage usage, const GpuStreamView& stream);

    GpuResources m_resources;
    std::unordered_map<std::uint64_t, Mesh> m_meshes;
    GpuHandle m_pipelines[7] = {0, 0, 0, 0, 0, 0, 0};
    GpuHandle m_backFacePipelines[2] = {0, 0}; /**< Holographic, XRay. */
    GpuHandle m_overlayPipeline = 0;
    GpuHandle m_quad = 0;
    GpuHandle m_overlayTexture = 0;
    GpuFrame m_frame;
    std::uint64_t m_frameIndex = 0;
};

} // namespace mne::scene

#endif // SCENE_BRAIN_FRAME_H
