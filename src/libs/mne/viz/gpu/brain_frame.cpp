//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brain_frame.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    BrainFrameBuilder definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brain_frame.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC HELPERS
//=============================================================================================================

namespace
{

// Full-target quad as a strip: (x, y) in clip space, (u, v) with v = 0 at the top of the texture
constexpr std::array<float, 16> kQuad = {-1.f, 1.f, 0.f, 0.f, -1.f, -1.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f, 1.f, -1.f, 1.f, 1.f};

GpuPipeline surface_pipeline(const char* shader)
{
    GpuPipeline pipeline;
    pipeline.shader = shader;
    pipeline.bindings = {{6 * sizeof(float), false}, {4, false}, {4, false}};
    pipeline.attributes = {{0, 0, GpuVertexFormat::Float3, 0},
                           {1, 0, GpuVertexFormat::Float3, 3 * sizeof(float)},
                           {2, 1, GpuVertexFormat::UNormByte4, 0},
                           {3, 2, GpuVertexFormat::UNormByte4, 0}};
    pipeline.depth_test = true;
    pipeline.depth_write = true;
    return pipeline;
}

GpuPipeline instanced_pipeline(const char* shader)
{
    GpuPipeline pipeline;
    pipeline.shader = shader;
    pipeline.bindings = {{6 * sizeof(float), false}, {21 * sizeof(float), true}};
    pipeline.attributes = {{0, 0, GpuVertexFormat::Float3, 0}, {1, 0, GpuVertexFormat::Float3, 3 * sizeof(float)}};
    for (int column = 0; column < 5; ++column) {
        pipeline.attributes.push_back({2 + column, 1, GpuVertexFormat::Float4, static_cast<std::uint32_t>(4 * column * sizeof(float))});
    }
    pipeline.attributes.push_back({7, 1, GpuVertexFormat::Float, 20 * sizeof(float)});
    pipeline.depth_test = true;
    pipeline.depth_write = true;
    return pipeline;
}

GpuPipeline textured_quad_pipeline(const char* shader)
{
    GpuPipeline pipeline;
    pipeline.shader = shader;
    pipeline.bindings = {{5 * sizeof(float), false}};
    pipeline.attributes = {{0, 0, GpuVertexFormat::Float3, 0}, {1, 0, GpuVertexFormat::Float2, 3 * sizeof(float)}};
    pipeline.topology = GpuTopology::TriangleStrip;
    pipeline.blend = GpuBlend::Alpha;
    return pipeline;
}

// Bytes of the uniform block video_decal declares: everything but depth_params
constexpr std::size_t kVideoDecalBlockSize = offsetof(BrainVideoDecalUniforms, depth_params);

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainFrameBuilder::BrainFrameBuilder()
{
    const auto index = [](BrainShader shader) {
        return static_cast<std::size_t>(shader);
    };
    m_pipelines[index(BrainShader::Standard)] = m_resources.create_pipeline(surface_pipeline("standard"));
    m_pipelines[index(BrainShader::Anatomical)] = m_resources.create_pipeline(surface_pipeline("anatomical"));
    m_pipelines[index(BrainShader::ShowNormals)] = m_resources.create_pipeline(surface_pipeline("shownormals"));

    // Holographic shells add their glow, back faces first; x-ray ones also show through other surfaces
    GpuPipeline holographic = surface_pipeline("holographic");
    holographic.blend = GpuBlend::Additive;
    holographic.depth_write = false;
    holographic.cull = GpuCull::Back;
    GpuPipeline xray = holographic;
    xray.depth_test = false;
    m_pipelines[index(BrainShader::Holographic)] = m_resources.create_pipeline(holographic);
    m_pipelines[index(BrainShader::XRay)] = m_resources.create_pipeline(xray);
    holographic.cull = GpuCull::Front;
    xray.cull = GpuCull::Front;
    m_backFacePipelines[0] = m_resources.create_pipeline(std::move(holographic));
    m_backFacePipelines[1] = m_resources.create_pipeline(std::move(xray));

    GpuPipeline dipole = instanced_pipeline("dipole");
    dipole.blend = GpuBlend::Alpha;
    dipole.depth_write = false;
    m_pipelines[index(BrainShader::Dipole)] = m_resources.create_pipeline(std::move(dipole));
    m_pipelines[index(BrainShader::Electrode)] = m_resources.create_pipeline(instanced_pipeline("electrode"));

    // MRI slices blend behind opaque geometry; the video disk lies on top of everything
    GpuPipeline slice = textured_quad_pipeline("slice");
    slice.depth_test = true;
    m_slicePipeline = m_resources.create_pipeline(std::move(slice));
    m_videoBillboardPipeline = m_resources.create_pipeline(textured_quad_pipeline("video_overlay"));

    // The decal lies on its surface; the relief-following decal moves vertices inwards, so it is not depth tested
    GpuPipeline decal = surface_pipeline("video_decal");
    decal.blend = GpuBlend::Alpha;
    decal.depth_write = false;
    m_videoDecalPipeline = m_resources.create_pipeline(decal);
    decal.shader = "video_decal_depth";
    decal.depth_test = false;
    m_videoDecalDepthPipeline = m_resources.create_pipeline(std::move(decal));

    GpuPipeline overlay;
    overlay.shader = "screen_overlay";
    overlay.bindings = {{4 * sizeof(float), false}};
    overlay.attributes = {{0, 0, GpuVertexFormat::Float2, 0}, {1, 0, GpuVertexFormat::Float2, 2 * sizeof(float)}};
    overlay.topology = GpuTopology::TriangleStrip;
    overlay.blend = GpuBlend::Alpha;
    m_overlayPipeline = m_resources.create_pipeline(std::move(overlay));
    m_quad = m_resources.create_buffer(GpuBufferUsage::Vertex);
    m_resources.set_buffer_values(m_quad, std::span<const float>(kQuad));
}

//=============================================================================================================

void BrainFrameBuilder::begin(const GpuFrameTarget& target, Color clear)
{
    ++m_frameIndex;
    m_frame = GpuFrame{};
    m_frame.pixel_width = target.pixel_width;
    m_frame.pixel_height = target.pixel_height;
    m_frame.device_pixel_ratio = target.device_pixel_ratio;
    GpuPass pass;
    pass.clear_color = clear;
    m_frame.passes.push_back(std::move(pass));
}

//=============================================================================================================

GpuHandle BrainFrameBuilder::stream_buffer(Mesh& mesh, int slot, GpuBufferUsage usage, const GpuStreamView& stream)
{
    const auto at = static_cast<std::size_t>(slot);
    if (stream.bytes.empty()) {
        return 0;
    }
    if (mesh.buffers[at] == 0) {
        mesh.buffers[at] = m_resources.create_buffer(usage);
    }
    if (!mesh.uploaded[at] || mesh.generations[at] != stream.generation) {
        m_resources.set_buffer_data(mesh.buffers[at], stream.bytes);
        mesh.generations[at] = stream.generation;
        mesh.uploaded[at] = true;
    }
    return mesh.buffers[at];
}

//=============================================================================================================

bool BrainFrameBuilder::surface_buffers(std::uint64_t key, const BrainSurfaceStreams& streams, GpuDraw& draw)
{
    Mesh& mesh = m_meshes[key];
    mesh.last_frame = m_frameIndex;
    const GpuHandle geometry = stream_buffer(mesh, 0, GpuBufferUsage::Vertex, streams.geometry);
    const GpuHandle colors = stream_buffer(mesh, 1, GpuBufferUsage::Vertex, streams.colors);
    const GpuHandle annotations = stream_buffer(mesh, 2, GpuBufferUsage::Vertex, streams.annotations);
    const GpuHandle indices = stream_buffer(mesh, 3, GpuBufferUsage::Index32, streams.indices);
    if (geometry == 0 || colors == 0 || annotations == 0 || indices == 0) {
        return false;
    }
    draw.vertex_buffers = {geometry, colors, annotations};
    draw.index_buffer = indices;
    draw.count = streams.index_count;
    return true;
}

//=============================================================================================================

GpuHandle BrainFrameBuilder::texture(std::uint64_t key, const BrainTextureView& view, bool mipmaps)
{
    if (!view.image || view.image->width <= 0 || view.image->height <= 0) {
        return 0;
    }
    Texture& texture = m_textures[key];
    texture.last_frame = m_frameIndex;
    if (texture.handle == 0) {
        GpuTexture created;
        created.content = *view.image;
        created.mipmaps = mipmaps;
        texture.handle = m_resources.create_texture(std::move(created));
        texture.revision = view.revision;
    } else if (texture.revision != view.revision) {
        m_resources.set_texture(texture.handle, *view.image);
        texture.revision = view.revision;
    }
    return texture.handle;
}

//=============================================================================================================

GpuHandle BrainFrameBuilder::quad_buffer(std::uint64_t key, std::span<const float, 20> quad)
{
    Mesh& mesh = m_meshes[key];
    mesh.last_frame = m_frameIndex;
    if (mesh.buffers[0] == 0) {
        mesh.buffers[0] = m_resources.create_buffer(GpuBufferUsage::Vertex);
    }
    const GpuBuffer* current = m_resources.buffer(mesh.buffers[0]);
    const std::span<const std::byte> bytes = std::as_bytes(quad);
    if (!current || current->data.size() != bytes.size() || !std::equal(bytes.begin(), bytes.end(), current->data.begin())) {
        m_resources.set_buffer_data(mesh.buffers[0], bytes);
    }
    return mesh.buffers[0];
}

//=============================================================================================================

bool BrainFrameBuilder::draw_slice(std::uint64_t key, const BrainTextureView& image, std::span<const float, 20> quad, const BrainSliceUniforms& uniforms,
                                   const GpuRect& viewport)
{
    if (m_frame.passes.empty()) {
        return false;
    }
    const GpuHandle tex = texture(key, image, false);
    if (tex == 0) {
        return false;
    }
    GpuDraw draw;
    draw.pipeline = m_slicePipeline;
    draw.vertex_buffers = {quad_buffer(key, quad)};
    draw.count = 4;
    draw.set_uniforms(0, uniforms);
    draw.textures = {{1, tex}};
    draw.viewport = viewport;
    draw.scissor = viewport;
    m_frame.passes.back().draws.push_back(std::move(draw));
    return true;
}

//=============================================================================================================

bool BrainFrameBuilder::draw_video_billboard(std::uint64_t key, std::uint64_t textureKey, const BrainTextureView& frame, std::span<const float, 20> quad,
                                             const BrainVideoBillboardUniforms& uniforms, const GpuRect& viewport)
{
    if (m_frame.passes.empty()) {
        return false;
    }
    const GpuHandle tex = texture(textureKey, frame, false);
    if (tex == 0) {
        return false;
    }
    GpuDraw draw;
    draw.pipeline = m_videoBillboardPipeline;
    draw.vertex_buffers = {quad_buffer(key, quad)};
    draw.count = 4;
    draw.set_uniforms(0, uniforms);
    draw.textures = {{1, tex}};
    draw.viewport = viewport;
    draw.scissor = viewport;
    m_frame.passes.back().draws.push_back(std::move(draw));
    return true;
}

//=============================================================================================================

bool BrainFrameBuilder::draw_video_decal(std::uint64_t key, const BrainSurfaceStreams& streams, std::uint64_t textureKey, const BrainTextureView& frame,
                                         const BrainTextureView& depth, const BrainVideoDecalUniforms& uniforms, const GpuRect& viewport)
{
    if (streams.index_count == 0 || m_frame.passes.empty()) {
        return false;
    }
    const GpuHandle video = texture(textureKey, frame, false);
    if (video == 0) {
        return false;
    }
    GpuDraw draw;
    if (!surface_buffers(key, streams, draw)) {
        return false;
    }
    // The depth texture of a key sits at its complement; the vertex shader reads a blurred mip level of it
    const GpuHandle relief = texture(~textureKey, depth, true);
    GpuUniformBlock block;
    block.binding = 0;
    const std::size_t size = relief != 0 ? sizeof(BrainVideoDecalUniforms) : kVideoDecalBlockSize;
    block.data.resize(size);
    std::memcpy(block.data.data(), &uniforms, size);
    draw.uniforms.push_back(std::move(block));
    draw.textures = {{1, video}};
    if (relief != 0) {
        draw.textures.push_back({2, relief});
    }
    draw.pipeline = relief != 0 ? m_videoDecalDepthPipeline : m_videoDecalPipeline;
    draw.viewport = viewport;
    draw.scissor = viewport;
    m_frame.passes.back().draws.push_back(std::move(draw));
    return true;
}

//=============================================================================================================

bool BrainFrameBuilder::draw_surface(std::uint64_t key, const BrainSurfaceStreams& streams, BrainShader shader, const BrainSurfaceUniforms& uniforms,
                                     const GpuRect& viewport)
{
    if (shader == BrainShader::Dipole || shader == BrainShader::Electrode || streams.index_count == 0 || m_frame.passes.empty()) {
        return false;
    }
    GpuDraw draw;
    if (!surface_buffers(key, streams, draw)) {
        return false;
    }
    draw.set_uniforms(0, uniforms);
    draw.viewport = viewport;
    draw.scissor = viewport;
    std::vector<GpuDraw>& draws = m_frame.passes.back().draws;
    if (shader == BrainShader::Holographic || shader == BrainShader::XRay) {
        GpuDraw back = draw;
        back.pipeline = m_backFacePipelines[shader == BrainShader::Holographic ? 0 : 1];
        draws.push_back(std::move(back));
    }
    draw.pipeline = m_pipelines[static_cast<std::size_t>(shader)];
    draws.push_back(std::move(draw));
    return true;
}

//=============================================================================================================

bool BrainFrameBuilder::draw_instanced(std::uint64_t key, const BrainInstancedStreams& streams, BrainShader shader, const BrainInstanceUniforms& uniforms,
                                       const GpuRect& viewport)
{
    if ((shader != BrainShader::Dipole && shader != BrainShader::Electrode) || streams.index_count == 0 || streams.instance_count == 0 || m_frame.passes.empty()) {
        return false;
    }
    Mesh& mesh = m_meshes[key];
    mesh.last_frame = m_frameIndex;
    const GpuHandle vertices = stream_buffer(mesh, 0, GpuBufferUsage::Vertex, streams.vertices);
    const GpuHandle instances = stream_buffer(mesh, 1, GpuBufferUsage::Vertex, streams.instances);
    const GpuHandle indices = stream_buffer(mesh, 2, GpuBufferUsage::Index32, streams.indices);
    if (vertices == 0 || instances == 0 || indices == 0) {
        return false;
    }
    GpuDraw draw;
    draw.pipeline = m_pipelines[static_cast<std::size_t>(shader)];
    draw.vertex_buffers = {vertices, instances};
    draw.index_buffer = indices;
    draw.count = streams.index_count;
    draw.instance_count = streams.instance_count;
    draw.set_uniforms(0, uniforms);
    draw.viewport = viewport;
    draw.scissor = viewport;
    m_frame.passes.back().draws.push_back(std::move(draw));
    return true;
}

//=============================================================================================================

void BrainFrameBuilder::draw_overlay(std::variant<Image, DisplayList> overlay, double scale)
{
    if (m_frame.passes.empty()) {
        return;
    }
    if (m_overlayTexture == 0) {
        GpuTexture texture;
        texture.content = std::move(overlay);
        texture.scale = scale;
        texture.linear_filter = false;
        m_overlayTexture = m_resources.create_texture(std::move(texture));
    } else {
        m_resources.set_texture(m_overlayTexture, std::move(overlay), scale);
    }
    redraw_overlay();
}

//=============================================================================================================

bool BrainFrameBuilder::redraw_overlay()
{
    if (m_frame.passes.empty() || m_overlayTexture == 0) {
        return false;
    }
    GpuDraw draw;
    draw.pipeline = m_overlayPipeline;
    draw.vertex_buffers = {m_quad};
    draw.count = 4;
    draw.textures = {{0, m_overlayTexture}};
    m_frame.passes.back().draws.push_back(std::move(draw));
    return true;
}

//=============================================================================================================

GpuFrame BrainFrameBuilder::finish()
{
    for (auto it = m_meshes.begin(); it != m_meshes.end();) {
        if (m_frameIndex - it->second.last_frame > kUnusedFrames) {
            for (const GpuHandle buffer : it->second.buffers) {
                m_resources.release(buffer);
            }
            it = m_meshes.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_textures.begin(); it != m_textures.end();) {
        if (m_frameIndex - it->second.last_frame > kUnusedFrames) {
            m_resources.release(it->second.handle);
            it = m_textures.erase(it);
        } else {
            ++it;
        }
    }
    return std::exchange(m_frame, GpuFrame{});
}

//=============================================================================================================

void BrainFrameBuilder::release(std::uint64_t key)
{
    if (const auto found = m_meshes.find(key); found != m_meshes.end()) {
        for (const GpuHandle buffer : found->second.buffers) {
            m_resources.release(buffer);
        }
        m_meshes.erase(found);
    }
    for (const std::uint64_t textureKey : {key, ~key}) {
        if (const auto found = m_textures.find(textureKey); found != m_textures.end()) {
            m_resources.release(found->second.handle);
            m_textures.erase(found);
        }
    }
}
