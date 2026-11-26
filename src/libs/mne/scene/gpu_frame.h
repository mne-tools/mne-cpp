//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     gpu_frame.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    GpuFrame describes what a view draws on a GPU (buffers, pipelines, passes, draws) without a graphics API.
 *
 * A view keeps its buffers, textures and pipelines in a GpuResources store across frames and builds one GpuFrame
 * per frame that lists the passes and draws referring to them. A backend executes the frame: the QRhi backend of
 * mne_disp uploads changed resources and records the draws, SoftwareRenderer rasterizes them into an image.
 * Shaders are named by an id; every backend implements the id (precompiled .qsb for QRhi, C++ for software).
 */

#ifndef SCENE_GPU_FRAME_H
#define SCENE_GPU_FRAME_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "display_list.h"
#include "scene_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

/** Identifies a buffer, texture or pipeline in a @ref GpuResources store; 0 is no resource. */
using GpuHandle = std::uint32_t;

/** Format of a vertex attribute. */
enum class GpuVertexFormat : std::uint8_t
{
    Float,     /**< One 32-bit float. */
    Float2,    /**< Two 32-bit floats. */
    Float3,    /**< Three 32-bit floats. */
    Float4,    /**< Four 32-bit floats. */
    UNormByte4 /**< Four bytes mapped to 0..1. */
};

/**
 * @brief One vertex attribute: a shader input location read from a vertex buffer.
 *
 * @since 3.0.0
 */
struct GpuVertexAttribute
{
    int location = 0;                                /**< Shader input location. */
    int binding = 0;                                 /**< Index of the vertex buffer in GpuDraw::vertex_buffers. */
    GpuVertexFormat format = GpuVertexFormat::Float; /**< Format. */
    std::uint32_t offset = 0;                        /**< Byte offset in the vertex. */

    friend bool operator==(const GpuVertexAttribute&, const GpuVertexAttribute&) = default;
};

/**
 * @brief How the vertices of one vertex buffer are laid out.
 *
 * @since 3.0.0
 */
struct GpuVertexBinding
{
    std::uint32_t stride = 0;  /**< Bytes from one vertex (or instance) to the next. */
    bool per_instance = false; /**< Whether the buffer advances per instance rather than per vertex. */

    friend bool operator==(const GpuVertexBinding&, const GpuVertexBinding&) = default;
};

/** Primitive topology. */
enum class GpuTopology : std::uint8_t
{
    Triangles,     /**< Independent triangles. */
    TriangleStrip, /**< A triangle strip. */
    Lines,         /**< Independent lines, one pixel wide. */
    LineStrip,     /**< A line strip, one pixel wide. */
    Points         /**< Points, one pixel each. */
};

/** Colour blending of a fragment with the target. */
enum class GpuBlend : std::uint8_t
{
    None,    /**< The fragment replaces the target. */
    Alpha,   /**< Colour src * a + dst * (1 - a), alpha a + dst_a * (1 - a). */
    Additive /**< Colour src * a + dst, alpha a + dst_a. */
};

/** Which triangle faces are skipped. */
enum class GpuCull : std::uint8_t
{
    None, /**< None. */
    Back, /**< Back faces. */
    Front /**< Front faces. */
};

/** When a fragment passes the depth test. */
enum class GpuDepthCompare : std::uint8_t
{
    Less,        /**< When it is closer than the stored depth. */
    LessOrEqual, /**< When it is closer or equally close. */
    Always       /**< Always. */
};

/**
 * @brief A graphics pipeline: shader, vertex layout and fixed-function state.
 *
 * The uniform blocks and textures the shader reads are given per draw (@ref GpuDraw) by the binding numbers of
 * the shader.
 *
 * @since 3.0.0
 */
struct GpuPipeline
{
    std::string shader;                                    /**< Shader id, e.g. "channeldata" for channeldata.vert/.frag. */
    std::vector<GpuVertexBinding> bindings;                /**< Vertex buffer layouts. */
    std::vector<GpuVertexAttribute> attributes;            /**< Vertex attributes. */
    GpuTopology topology = GpuTopology::Triangles;         /**< Primitive topology. */
    GpuBlend blend = GpuBlend::None;                       /**< Colour blending. */
    GpuCull cull = GpuCull::None;                          /**< Face culling. */
    bool front_counter_clockwise = true;                   /**< Whether counter-clockwise triangles (y up) face the viewer. */
    bool depth_test = false;                               /**< Whether fragments are depth tested. */
    bool depth_write = false;                              /**< Whether fragments write their depth. */
    GpuDepthCompare depth_compare = GpuDepthCompare::Less; /**< Depth comparison of the test. */
    bool color_write = true;                               /**< Whether fragments write colour. */

    friend bool operator==(const GpuPipeline&, const GpuPipeline&) = default;
};

/** What a buffer holds. */
enum class GpuBufferUsage : std::uint8_t
{
    Vertex,  /**< Vertex or instance data. */
    Index16, /**< 16-bit indices. */
    Index32  /**< 32-bit indices. */
};

/**
 * @brief The contents of a buffer and a revision that changes with them.
 *
 * @since 3.0.0
 */
struct GpuBuffer
{
    GpuBufferUsage usage = GpuBufferUsage::Vertex; /**< What the buffer holds. */
    std::vector<std::byte> data;                   /**< Contents. */
    std::uint64_t revision = 0;                    /**< Changes with the contents; backends upload again on change. */
};

/**
 * @brief A texture: RGBA pixels or a figure the backend rasterizes, and a revision that changes with it.
 *
 * A figure lets every backend draw text and shapes with its own painter, e.g. the QRhi backend with QPainter and
 * the toolkit's fonts, at @ref scale texture pixels per figure pixel on a transparent ground.
 *
 * @since 3.0.0
 */
struct GpuTexture
{
    std::variant<Image, DisplayList> content; /**< Pixels (not premultiplied) or a figure. */
    double scale = 1.0;                       /**< Texture pixels per figure pixel of a figure. */
    bool linear_filter = true;                /**< Linear rather than nearest sampling. */
    std::uint64_t revision = 0;               /**< Changes with the contents; backends upload again on change. */

    /** @return The width in pixels. */
    int pixel_width() const
    {
        if (const Image* image = std::get_if<Image>(&content)) {
            return image->width;
        }
        return static_cast<int>(std::ceil(std::get<DisplayList>(content).width * scale));
    }

    /** @return The height in pixels. */
    int pixel_height() const
    {
        if (const Image* image = std::get_if<Image>(&content)) {
            return image->height;
        }
        return static_cast<int>(std::ceil(std::get<DisplayList>(content).height * scale));
    }
};

/**
 * @brief Buffers, textures and pipelines a view keeps across frames.
 *
 * A handle stays valid until its resource is released and is not reused by the store, so a backend can keep one
 * GPU object per handle and upload again when the revision changed.
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT GpuResources
{
public:
    //=========================================================================================================
    /**
     * Adds an empty buffer.
     *
     * @param[in] usage   What the buffer holds.
     *
     * @return The handle.
     */
    GpuHandle create_buffer(GpuBufferUsage usage);

    //=========================================================================================================
    /**
     * Replaces the contents of a buffer.
     *
     * @param[in] buffer   Handle of a buffer of this store; others are ignored.
     * @param[in] bytes    New contents.
     */
    void set_buffer_data(GpuHandle buffer, std::span<const std::byte> bytes);

    //=========================================================================================================
    /**
     * Replaces the contents of a buffer with trivially copyable values.
     *
     * @param[in] buffer   Handle of a buffer of this store; others are ignored.
     * @param[in] values   New contents.
     */
    template<typename T>
    void set_buffer_values(GpuHandle buffer, std::span<const T> values)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        set_buffer_data(buffer, std::as_bytes(values));
    }

    //=========================================================================================================
    /**
     * Adds a texture.
     *
     * @param[in] texture   Texture; the store sets its revision.
     *
     * @return The handle.
     */
    GpuHandle create_texture(GpuTexture texture);

    //=========================================================================================================
    /**
     * Replaces the contents of a texture and keeps its filter.
     *
     * @param[in] texture   Handle of a texture of this store; others are ignored.
     * @param[in] content   Pixels or a figure.
     * @param[in] scale     Texture pixels per figure pixel of a figure.
     */
    void set_texture(GpuHandle texture, std::variant<Image, DisplayList> content, double scale = 1.0);

    //=========================================================================================================
    /**
     * Adds a pipeline.
     *
     * @param[in] pipeline   Description.
     *
     * @return The handle.
     */
    GpuHandle create_pipeline(GpuPipeline pipeline);

    //=========================================================================================================
    /**
     * Releases a buffer, texture or pipeline; its handle becomes invalid.
     *
     * @param[in] handle   Handle; 0 and unknown handles are ignored.
     */
    void release(GpuHandle handle);

    //=========================================================================================================
    /**
     * @param[in] handle   Handle.
     *
     * @return The buffer, or nullptr if @p handle is no buffer of this store.
     */
    const GpuBuffer* buffer(GpuHandle handle) const;

    //=========================================================================================================
    /**
     * @param[in] handle   Handle.
     *
     * @return The texture, or nullptr if @p handle is no texture of this store.
     */
    const GpuTexture* texture(GpuHandle handle) const;

    //=========================================================================================================
    /**
     * @param[in] handle   Handle.
     *
     * @return The pipeline, or nullptr if @p handle is no pipeline of this store.
     */
    const GpuPipeline* pipeline(GpuHandle handle) const;

private:
    using Resource = std::variant<std::monostate, GpuBuffer, GpuTexture, GpuPipeline>;

    std::vector<Resource> m_resources; /**< Resource of handle h at index h - 1. */
    std::uint64_t m_revision = 0;      /**< Last revision handed out. */
};

/**
 * @brief The bytes of one uniform block of a draw, laid out as the shader declares it (std140).
 *
 * @since 3.0.0
 */
struct GpuUniformBlock
{
    int binding = 0;             /**< Binding number in the shader. */
    std::vector<std::byte> data; /**< Contents. */
};

/**
 * @brief A texture bound to a sampler of the shader.
 *
 * @since 3.0.0
 */
struct GpuTextureBinding
{
    int binding = 0;       /**< Binding number in the shader. */
    GpuHandle texture = 0; /**< Texture. */
};

/**
 * @brief A rectangle in target pixels with the origin at the top left.
 *
 * @since 3.0.0
 */
struct GpuRect
{
    int x = 0;      /**< Left edge. */
    int y = 0;      /**< Top edge. */
    int width = 0;  /**< Width. */
    int height = 0; /**< Height. */

    friend bool operator==(const GpuRect&, const GpuRect&) = default;
};

/**
 * @brief One draw call.
 *
 * @since 3.0.0
 */
struct GpuDraw
{
    GpuHandle pipeline = 0;                  /**< Pipeline. */
    std::vector<GpuHandle> vertex_buffers;   /**< Vertex buffers in the order of the pipeline's bindings. */
    GpuHandle index_buffer = 0;              /**< Index buffer, or 0 to draw without indices. */
    std::uint32_t first = 0;                 /**< First vertex or index. */
    std::uint32_t count = 0;                 /**< Number of vertices or indices. */
    std::uint32_t instance_count = 1;        /**< Number of instances. */
    std::vector<GpuUniformBlock> uniforms;   /**< Uniform blocks. */
    std::vector<GpuTextureBinding> textures; /**< Textures. */
    std::optional<GpuRect> viewport;         /**< Viewport; the whole target if absent. */
    std::optional<GpuRect> scissor;          /**< Scissor rectangle, if any. */

    //=========================================================================================================
    /**
     * Adds a uniform block from a trivially copyable struct with the shader's std140 layout.
     *
     * @param[in] binding   Binding number in the shader.
     * @param[in] value     Contents.
     */
    template<typename T>
    void set_uniforms(int binding, const T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        GpuUniformBlock block;
        block.binding = binding;
        block.data.resize(sizeof(T));
        std::memcpy(block.data.data(), &value, sizeof(T));
        uniforms.push_back(std::move(block));
    }
};

/**
 * @brief A render pass: what it clears and its draws in order.
 *
 * @since 3.0.0
 */
struct GpuPass
{
    std::optional<Color> clear_color = Color::black(); /**< Colour the pass clears to, or none to keep the target. */
    bool clear_depth = true;                           /**< Whether the depth buffer is cleared to the far plane. */
    std::vector<GpuDraw> draws;                        /**< Draws in order. */
};

/**
 * @brief Everything drawn in one frame.
 *
 * Clip space is that of OpenGL: x and y from -1 to 1 with y up, depth from -1 (near) to 1 (far). A backend with
 * another convention tells the producer through @ref GpuFrameTarget::clip_correction, which the producer applies
 * to its projections, as the shaders take the final positions from their uniforms.
 *
 * @since 3.0.0
 */
struct GpuFrame
{
    int pixel_width = 0;             /**< Width of the target in pixels. */
    int pixel_height = 0;            /**< Height of the target in pixels. */
    double device_pixel_ratio = 1.0; /**< Pixels per logical pixel. */
    std::vector<GpuPass> passes;     /**< Passes in order. */
};

/**
 * @brief What a producer needs to know about the target before it builds a frame.
 *
 * @since 3.0.0
 */
struct GpuFrameTarget
{
    int pixel_width = 0;                                           /**< Width in pixels. */
    int pixel_height = 0;                                          /**< Height in pixels. */
    double device_pixel_ratio = 1.0;                               /**< Pixels per logical pixel. */
    Eigen::Matrix4f clip_correction = Eigen::Matrix4f::Identity(); /**< Applied to projections by the producer. */
};

} // namespace mne::scene

#endif // SCENE_GPU_FRAME_H
