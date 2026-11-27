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

#ifndef VIZ_BRAIN_FRAME_H
#define VIZ_BRAIN_FRAME_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gpu_frame.h"
#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <variant>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * How a mesh of the brain view is shaded; the first six follow viz::qt::ShaderMode.
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
 * @brief The uniform block of the MRI slice shader (binding 0 of slice.vert/.frag).
 *
 * @since 3.0.0
 */
struct BrainSliceUniforms
{
    float mvp[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};            /**< Offset 0: projection * view, column-major. */
    float slice_to_world[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f}; /**< Offset 64: slice quad to world. */
    float opacity = 1.f;                                                                                         /**< Offset 128: opacity. */
    float window_center = 0.5f;                                                                                  /**< Offset 132: intensity window centre, 0 to 1. */
    float window_width = 1.f;                                                                                    /**< Offset 136: intensity window width, 0 to 1. */
    float padding = 0.f;                                                                                         /**< Offset 140. */
};

static_assert(sizeof(BrainSliceUniforms) == 144, "BrainSliceUniforms must match the uniform block of slice.vert");

/**
 * @brief The uniform block of the camera-facing video disk (binding 0 of video_overlay.vert/.frag).
 *
 * @since 3.0.0
 */
struct BrainVideoBillboardUniforms
{
    float mvp[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f}; /**< Offset 0: projection * view, column-major. */
    float border_color[4] = {0.f, 0.85f, 1.f, 1.f};                                                   /**< Offset 64: colour of the rim. */
    float opacity = 1.f;                                                                              /**< Offset 80: opacity. */
    float padding[3] = {0.f, 0.f, 0.f};                                                               /**< Offset 84. */
};

static_assert(sizeof(BrainVideoBillboardUniforms) == 96, "BrainVideoBillboardUniforms must match the uniform block of video_overlay.vert");

/**
 * @brief The uniform block of the video decal on a surface (binding 0 of video_decal and video_decal_depth).
 *
 * The depth shader reads depth_params after the block of video_decal; the builder passes the bytes each shader
 * declares.
 *
 * @since 3.0.0
 */
struct BrainVideoDecalUniforms
{
    float mvp[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f}; /**< Offset 0: projection * view, column-major. */
    float focus_and_size[4] = {0.f, 0.f, 0.f, 0.f};                                                   /**< Offset 64: decal centre, side length. */
    float axis_u_and_opacity[4] = {1.f, 0.f, 0.f, 1.f};                                               /**< Offset 80: decal U axis, opacity. */
    float axis_v_and_offset[4] = {0.f, 1.f, 0.f, 0.f};                                                /**< Offset 96: decal V axis, offset along the normal. */
    float axis_n_and_depth[4] = {0.f, 0.f, 1.f, 0.f};                                                 /**< Offset 112: decal normal, half the depth it reaches. */
    float camera_pos_and_aspect[4] = {0.f, 0.f, 0.f, 1.f};                                            /**< Offset 128: camera position, frame width / height. */
    float border_color[4] = {0.f, 0.85f, 1.f, 1.f};                                                   /**< Offset 144: colour of the rim. */
    float depth_params[4] = {0.f, 0.f, 0.f, 0.f};                                                     /**< Offset 160: displacement scale, steps, 1 to displace. */
};

static_assert(sizeof(BrainVideoDecalUniforms) == 176, "BrainVideoDecalUniforms must match the uniform block of video_decal_depth.vert");

/**
 * @brief Pixels of a texture and a revision its owner changes with them.
 *
 * @since 3.0.0
 */
struct BrainTextureView
{
    const Image* image = nullptr; /**< RGBA pixels, not premultiplied; nullptr for none. */
    std::uint64_t revision = 0;   /**< Changes with the pixels; equal revisions are not uploaded again. */
};

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
class VIZSHARED_EXPORT BrainFrameBuilder
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
     * Draws an MRI slice: a textured quad whose dark voxels are discarded, blended behind opaque geometry.
     *
     * @param[in] key        Identity of the slice; also identifies its texture.
     * @param[in] image      Slice image; the shader reads the red channel as intensity.
     * @param[in] quad       Four corners, each position (3 floats) and texture coordinates (2 floats), as a strip.
     * @param[in] uniforms   Shader parameters.
     * @param[in] viewport   Pane in target pixels.
     *
     * @return Whether anything was drawn: false without an image.
     */
    bool draw_slice(std::uint64_t key, const BrainTextureView& image, std::span<const float, 20> quad, const BrainSliceUniforms& uniforms,
                    const GpuRect& viewport);

    //=========================================================================================================
    /**
     * Draws a video frame as a round disk facing the camera, over everything drawn before.
     *
     * @param[in] key        Identity of the disk; also identifies its texture. Panes that each face their own
     *                       camera need keys of their own, since a key holds one quad per frame.
     * @param[in] frame      Video frame.
     * @param[in] quad       Four corners, each position (3 floats) and texture coordinates (2 floats), as a strip.
     * @param[in] uniforms   Shader parameters.
     * @param[in] viewport   Pane in target pixels.
     *
     * @return Whether anything was drawn: false without a frame.
     */
    bool draw_video_billboard(std::uint64_t key, const BrainTextureView& frame, std::span<const float, 20> quad,
                              const BrainVideoBillboardUniforms& uniforms, const GpuRect& viewport);

    //=========================================================================================================
    /**
     * Projects a video frame onto a surface as a decal; with a depth frame the decal follows its relief.
     *
     * @param[in] key        Identity of the surface mesh, as for draw_surface().
     * @param[in] streams    Vertex and index streams of the surface.
     * @param[in] textureKey Identity of the video textures.
     * @param[in] frame      Video frame.
     * @param[in] depth      Depth frame, or none for a flat decal.
     * @param[in] uniforms   Shader parameters.
     * @param[in] viewport   Pane in target pixels.
     *
     * @return Whether anything was drawn: false without streams or frame.
     */
    bool draw_video_decal(std::uint64_t key, const BrainSurfaceStreams& streams, std::uint64_t textureKey, const BrainTextureView& frame,
                          const BrainTextureView& depth, const BrainVideoDecalUniforms& uniforms, const GpuRect& viewport);

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
     * Drops the buffers of a mesh and the textures of that key.
     *
     * @param[in] key   Identity of the mesh or textured draw.
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

    struct Texture
    {
        GpuHandle handle = 0;
        std::uint64_t revision = 0;
        std::uint64_t last_frame = 0;
    };

    GpuHandle stream_buffer(Mesh& mesh, int slot, GpuBufferUsage usage, const GpuStreamView& stream);
    GpuHandle texture(std::uint64_t key, const BrainTextureView& view, bool mipmaps);
    bool surface_buffers(std::uint64_t key, const BrainSurfaceStreams& streams, GpuDraw& draw);
    GpuHandle quad_buffer(std::uint64_t key, std::span<const float, 20> quad);

    GpuResources m_resources;
    std::unordered_map<std::uint64_t, Mesh> m_meshes;
    std::unordered_map<std::uint64_t, Texture> m_textures; /**< By key; a key's second texture (decal depth) at ~key. */
    GpuHandle m_pipelines[7] = {0, 0, 0, 0, 0, 0, 0};
    GpuHandle m_backFacePipelines[2] = {0, 0}; /**< Holographic, XRay. */
    GpuHandle m_overlayPipeline = 0;
    GpuHandle m_slicePipeline = 0;
    GpuHandle m_videoBillboardPipeline = 0;
    GpuHandle m_videoDecalPipeline = 0;
    GpuHandle m_videoDecalDepthPipeline = 0;
    GpuHandle m_quad = 0;
    GpuHandle m_overlayTexture = 0;
    GpuFrame m_frame;
    std::uint64_t m_frameIndex = 0;
};

} // namespace mne::viz

#endif // VIZ_BRAIN_FRAME_H
