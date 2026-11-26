//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     software_renderer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SoftwareRenderer executes a GpuFrame on the CPU, without a GPU or a window system.
 *
 * The software backend of the GPU frame description: it rasterizes the draws of a GpuFrame into an RGBA image with
 * depth testing, culling and blending, and runs C++ implementations of the shaders, so views render headless, e.g.
 * for golden-image tests, command-line tools or a host without GPU access.
 */

#ifndef SCENE_SOFTWARE_RENDERER_H
#define SCENE_SOFTWARE_RENDERER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gpu_frame.h"
#include "scene_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

/**
 * @brief The uniform blocks and textures of a draw as a software shader reads them.
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT SoftwareBindings
{
public:
    /** Most vertex attributes a software shader reads. */
    static constexpr int kMaxAttributes = 8;

    /** Most floats a software vertex shader passes to the fragment shader. */
    static constexpr int kMaxVaryings = 24;

    //=========================================================================================================
    /**
     * @param[in] binding   Binding number of the uniform block.
     *
     * @return The block as @p T, the bytes missing from the draw's block (or of a missing block) zero.
     */
    template<typename T>
    T uniforms(int binding) const
    {
        static_assert(std::is_trivially_copyable_v<T>);
        T value{};
        copy_uniforms(binding, &value, sizeof(T));
        return value;
    }

    //=========================================================================================================
    /**
     * Samples a texture with clamp-to-edge addressing and the texture's filter.
     *
     * @param[in] binding   Binding number of the sampler.
     * @param[in] uv        Texture coordinates, (0, 0) at the top left of the image.
     *
     * @return RGBA from 0 to 1, or transparent black for a missing texture.
     */
    Eigen::Vector4f sample(int binding, const Eigen::Vector2f& uv) const;

private:
    friend class SoftwareRenderer;

    struct BoundTexture
    {
        int binding = 0;
        const Image* image = nullptr;
        bool linear = true;
    };

    void copy_uniforms(int binding, void* target, std::size_t size) const;

    const GpuDraw* m_draw = nullptr;
    std::vector<BoundTexture> m_textures;
};

/** The vertex attributes of one vertex, indexed by shader location; absent attributes are (0, 0, 0, 1). */
using SoftwareAttributes = std::array<Eigen::Vector4f, SoftwareBindings::kMaxAttributes>;

/** The values a software vertex shader passes to the fragment shader, interpolated across a primitive. */
using SoftwareVaryings = std::array<float, SoftwareBindings::kMaxVaryings>;

/**
 * @brief Where a fragment lies.
 *
 * @since 3.0.0
 */
struct SoftwareFragment
{
    Eigen::Vector2f position = Eigen::Vector2f::Zero(); /**< Pixel centre in target pixels, origin at the top left. */
    float depth = 0.f;                                  /**< Depth from 0 (near) to 1 (far). */
    bool front_facing = true;                           /**< Whether the triangle faces the viewer; true for lines. */
};

/**
 * @brief A shader of the software backend: the C++ counterpart of a GLSL vertex and fragment shader pair.
 *
 * @since 3.0.0
 */
struct SoftwareShader
{
    /** Number of varyings the vertex shader writes. */
    int varying_count = 0;

    /** Computes the clip-space position of a vertex and its varyings. */
    std::function<Eigen::Vector4f(const SoftwareAttributes& attributes, const SoftwareBindings& bindings, SoftwareVaryings& out)> vertex;

    /** Computes the RGBA colour (0 to 1, not premultiplied) of a fragment, or nothing to discard it. */
    std::function<std::optional<Eigen::Vector4f>(const SoftwareVaryings& in, const SoftwareFragment& fragment, const SoftwareBindings& bindings)>
        fragment;
};

/**
 * @brief Executes a GpuFrame on the CPU.
 *
 * Triangles are rasterized with perspective-correct varyings, near- and far-plane clipping, depth testing and face
 * culling; lines and points are one pixel wide, as on the GPU backends. Figure textures are rasterized with
 * to_raster(). The shaders of the views in mne_scene are registered on construction; set_shader() adds others.
 * Draws whose shader is unknown are skipped.
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT SoftwareRenderer
{
public:
    //=========================================================================================================
    /**
     * Creates a renderer with the shaders of the views in mne_scene.
     */
    SoftwareRenderer();

    //=========================================================================================================
    /**
     * Adds or replaces a shader.
     *
     * @param[in] id       Shader id the pipelines refer to.
     * @param[in] shader   Implementation.
     */
    void set_shader(const std::string& id, SoftwareShader shader);

    //=========================================================================================================
    /**
     * @param[in] id   Shader id.
     *
     * @return Whether a shader with the id is registered.
     */
    bool has_shader(const std::string& id) const;

    //=========================================================================================================
    /**
     * Renders a frame.
     *
     * @param[in] frame       Passes and draws.
     * @param[in] resources   Buffers, textures and pipelines the frame refers to.
     *
     * @return The target image, frame.pixel_width x frame.pixel_height, not premultiplied.
     */
    Image render(const GpuFrame& frame, const GpuResources& resources);

private:
    struct CachedTexture
    {
        std::uint64_t revision = 0;
        Image image;
    };

    struct Target;

    const Image* texture_image(GpuHandle handle, const GpuTexture& texture);
    void draw(Target& target, const GpuDraw& draw, const GpuResources& resources);

    std::map<std::string, SoftwareShader> m_shaders;
    std::map<GpuHandle, CachedTexture> m_textures;
};

//=============================================================================================================
/**
 * Rasterizes a figure on a transparent ground, with the opacity of every pixel.
 *
 * to_raster() draws on an opaque ground; the figure is drawn on black and on white and the opacity follows from
 * the difference.
 *
 * @param[in] list    Figure; its background, if any, is drawn too.
 * @param[in] scale   Image pixels per figure pixel.
 *
 * @return The RGBA image, not premultiplied.
 *
 * @since 3.0.0
 */
SCENESHARED_EXPORT Image to_rgba_image(const DisplayList& list, double scale = 1.0);

} // namespace mne::scene

#endif // SCENE_SOFTWARE_RENDERER_H
