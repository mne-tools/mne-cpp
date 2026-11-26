//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_scene_gpu_frame.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests the GPU frame description and the software backend of mne_scene, without Qt or a GPU.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/figures/canvas.h>
#include <mne/viz/gpu/gpu_frame.h>
#include <mne/viz/gpu/software_renderer.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <span>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

namespace
{

int g_failures = 0;

#define CHECK(condition)                                                                       \
    do {                                                                                       \
        if (!(condition)) {                                                                    \
            std::fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #condition); \
            ++g_failures;                                                                      \
        }                                                                                      \
    } while (false)

/** Uniform block of the "flat" test shader: a colour and a depth. */
struct FlatUniforms
{
    float color[4] = {0.f, 0.f, 0.f, 1.f};
    float depth = 0.f;
    float padding[3] = {0.f, 0.f, 0.f};
};

/** Positions (attribute 0, xy), an instance offset (attribute 1, xy) and a uniform colour and depth. */
SoftwareShader flat_shader()
{
    SoftwareShader shader;
    shader.varying_count = 1;
    shader.vertex = [](const SoftwareAttributes& in, const SoftwareBindings& bindings, SoftwareVaryings& out) {
        const auto u = bindings.uniforms<FlatUniforms>(0);
        out[0] = in[0].x();
        return Eigen::Vector4f(in[0].x() + in[1].x(), in[0].y() + in[1].y(), u.depth, 1.f);
    };
    shader.fragment = [](const SoftwareVaryings&, const SoftwareFragment&, const SoftwareBindings& bindings) -> std::optional<Eigen::Vector4f> {
        const auto u = bindings.uniforms<FlatUniforms>(0);
        return Eigen::Vector4f(u.color[0], u.color[1], u.color[2], u.color[3]);
    };
    return shader;
}

/** A store with one vertex buffer of (x, y) positions and a "flat" pipeline. */
struct Scene
{
    GpuResources resources;
    GpuHandle vertices = 0;
    GpuHandle offsets = 0;
    GpuHandle pipeline = 0;

    explicit Scene(std::span<const float> xy, GpuPipeline state = {})
    {
        vertices = resources.create_buffer(GpuBufferUsage::Vertex);
        resources.set_buffer_values(vertices, xy);
        offsets = resources.create_buffer(GpuBufferUsage::Vertex);
        const std::array<float, 2> none = {0.f, 0.f};
        resources.set_buffer_values(offsets, std::span<const float>(none));
        state.shader = "flat";
        state.bindings = {{2 * sizeof(float), false}, {2 * sizeof(float), true}};
        state.attributes = {{0, 0, GpuVertexFormat::Float2, 0}, {1, 1, GpuVertexFormat::Float2, 0}};
        pipeline = resources.create_pipeline(std::move(state));
    }

    GpuDraw draw(std::uint32_t count, Color color = Color{255, 0, 0}, float depth = 0.f) const
    {
        GpuDraw draw;
        draw.pipeline = pipeline;
        draw.vertex_buffers = {vertices, offsets};
        draw.count = count;
        FlatUniforms uniforms;
        uniforms.color[0] = color.r / 255.f;
        uniforms.color[1] = color.g / 255.f;
        uniforms.color[2] = color.b / 255.f;
        uniforms.color[3] = color.a / 255.f;
        uniforms.depth = depth;
        draw.set_uniforms(0, uniforms);
        return draw;
    }
};

GpuFrame frame_of(int width, int height, std::vector<GpuDraw> draws, std::optional<Color> clear = Color::black())
{
    GpuFrame frame;
    frame.pixel_width = width;
    frame.pixel_height = height;
    GpuPass pass;
    pass.clear_color = clear;
    pass.draws = std::move(draws);
    frame.passes.push_back(std::move(pass));
    return frame;
}

Image render(const GpuFrame& frame, const GpuResources& resources)
{
    SoftwareRenderer renderer;
    renderer.set_shader("flat", flat_shader());
    return renderer.render(frame, resources);
}

int count_pixels(const Image& image, Color color)
{
    int count = 0;
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            count += image.pixel(x, y) == color ? 1 : 0;
        }
    }
    return count;
}

// Two triangles covering the whole target, counter-clockwise
constexpr std::array<float, 12> kQuad = {-1.f, -1.f, 1.f, -1.f, 1.f, 1.f, -1.f, -1.f, 1.f, 1.f, -1.f, 1.f};

//=============================================================================================================

void test_resources()
{
    GpuResources resources;
    const GpuHandle buffer = resources.create_buffer(GpuBufferUsage::Index16);
    const GpuHandle texture = resources.create_texture(GpuTexture{Image::transparent(3, 2)});
    const GpuHandle pipeline = resources.create_pipeline(GpuPipeline{});
    CHECK(buffer != 0 && texture != 0 && pipeline != 0 && buffer != texture && texture != pipeline);
    CHECK(resources.buffer(buffer) && resources.buffer(buffer)->usage == GpuBufferUsage::Index16);
    CHECK(!resources.buffer(texture) && !resources.texture(pipeline) && !resources.pipeline(buffer) && !resources.buffer(0));
    CHECK(resources.texture(texture)->pixel_width() == 3 && resources.texture(texture)->pixel_height() == 2);

    // Every change gets a new revision
    const std::uint64_t before = resources.buffer(buffer)->revision;
    const std::array<std::uint16_t, 3> indices = {0, 1, 2};
    resources.set_buffer_values(buffer, std::span<const std::uint16_t>(indices));
    CHECK(resources.buffer(buffer)->revision > before && resources.buffer(buffer)->data.size() == 6);
    const std::uint64_t textureBefore = resources.texture(texture)->revision;
    Canvas figure(10, 4);
    resources.set_texture(texture, figure.display_list(), 2.0);
    CHECK(resources.texture(texture)->revision > textureBefore);
    CHECK(resources.texture(texture)->pixel_width() == 20 && resources.texture(texture)->pixel_height() == 8);

    // Released handles are gone and not reused
    resources.release(buffer);
    CHECK(!resources.buffer(buffer));
    CHECK(resources.create_buffer(GpuBufferUsage::Vertex) != buffer);
}

void test_coverage_and_blending()
{
    // Additive blending at half opacity: a pixel covered twice would be brighter, an uncovered one black
    GpuPipeline state;
    state.blend = GpuBlend::Additive;
    Scene scene(kQuad, state);
    const Image image = render(frame_of(16, 9, {scene.draw(6, Color{255, 0, 0, 128})}), scene.resources);
    CHECK(image.width == 16 && image.height == 9);
    CHECK(count_pixels(image, Color{128, 0, 0, 255}) == 16 * 9);

    // The clear colour stays where nothing is drawn; a viewport limits the draw
    Scene opaque(kQuad);
    GpuDraw draw = opaque.draw(6, Color{0, 255, 0});
    draw.viewport = GpuRect{2, 1, 4, 3};
    const Image partial = render(frame_of(16, 9, {draw}, Color{0, 0, 255}), opaque.resources);
    CHECK(count_pixels(partial, Color{0, 255, 0}) == 12);
    CHECK(partial.pixel(2, 1) == (Color{0, 255, 0}) && partial.pixel(5, 3) == (Color{0, 255, 0}) && partial.pixel(6, 3) == (Color{0, 0, 255}));
    // A scissor cuts it further
    draw.scissor = GpuRect{0, 0, 3, 9};
    CHECK(count_pixels(render(frame_of(16, 9, {draw}), opaque.resources), Color{0, 255, 0}) == 3);

    // Alpha blending over the clear colour
    GpuPipeline alpha;
    alpha.blend = GpuBlend::Alpha;
    Scene blended(kQuad, alpha);
    const Image mixed = render(frame_of(4, 4, {blended.draw(6, Color{255, 255, 255, 51})}, Color{0, 0, 0}), blended.resources);
    CHECK(mixed.pixel(1, 1) == (Color{51, 51, 51, 255}));
}

void test_culling()
{
    // A triangle in the lower left half, counter-clockwise, and the same clockwise
    const std::vector<float> triangles = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, -1.f, -1.f, -1.f, 1.f, 1.f, -1.f};
    GpuPipeline back;
    back.cull = GpuCull::Back;
    Scene scene(triangles, back);
    GpuDraw ccw = scene.draw(3);
    GpuDraw cw = scene.draw(3, Color{0, 255, 0});
    cw.first = 3;
    const Image image = render(frame_of(8, 8, {ccw, cw}), scene.resources);
    CHECK(count_pixels(image, Color{255, 0, 0}) > 20 && count_pixels(image, Color{0, 255, 0}) == 0);
    // The lower left corner (y up) is the last row on the target
    CHECK(image.pixel(0, 7) == (Color{255, 0, 0}) && image.pixel(7, 0) == Color::black());

    GpuPipeline front = back;
    front.front_counter_clockwise = false;
    Scene flipped(triangles, front);
    GpuDraw second = flipped.draw(3, Color{0, 255, 0});
    second.first = 3;
    const Image flippedImage = render(frame_of(8, 8, {flipped.draw(3), second}), flipped.resources);
    CHECK(count_pixels(flippedImage, Color{255, 0, 0}) == 0 && count_pixels(flippedImage, Color{0, 255, 0}) > 20);
}

void test_depth()
{
    GpuPipeline state;
    state.depth_test = true;
    state.depth_write = true;
    Scene scene(kQuad, state);
    // The nearer green quad wins whatever the order
    const Image farFirst = render(frame_of(4, 4, {scene.draw(6, Color{255, 0, 0}, 0.5f), scene.draw(6, Color{0, 255, 0}, -0.5f)}), scene.resources);
    const Image nearFirst = render(frame_of(4, 4, {scene.draw(6, Color{0, 255, 0}, -0.5f), scene.draw(6, Color{255, 0, 0}, 0.5f)}), scene.resources);
    CHECK(count_pixels(farFirst, Color{0, 255, 0}) == 16 && count_pixels(nearFirst, Color{0, 255, 0}) == 16);
    // Beyond the far plane nothing is drawn
    CHECK(count_pixels(render(frame_of(4, 4, {scene.draw(6, Color{255, 0, 0}, 1.5f)}), scene.resources), Color::black()) == 16);
}

void test_lines_points_and_strips()
{
    // A horizontal line through the middle of an 8 x 4 target: one pixel per column in row 2
    GpuPipeline lines;
    lines.topology = GpuTopology::LineStrip;
    Scene line(std::vector<float>{-1.f, 0.f, 0.f, 0.f, 1.f, 0.f}, lines);
    const Image image = render(frame_of(8, 4, {line.draw(3)}), line.resources);
    CHECK(count_pixels(image, Color{255, 0, 0}) == 8);
    for (int x = 0; x < 8; ++x) {
        CHECK(image.pixel(x, 2) == (Color{255, 0, 0}));
    }

    GpuPipeline points;
    points.topology = GpuTopology::Points;
    Scene point(std::vector<float>{-0.75f, 0.75f, 0.25f, -0.25f}, points);
    const Image dots = render(frame_of(8, 8, {point.draw(2)}), point.resources);
    CHECK(count_pixels(dots, Color{255, 0, 0}) == 2 && dots.pixel(1, 1) == (Color{255, 0, 0}) && dots.pixel(5, 5) == (Color{255, 0, 0}));

    // The full-screen quad as a triangle strip covers everything once
    GpuPipeline strip;
    strip.topology = GpuTopology::TriangleStrip;
    strip.blend = GpuBlend::Additive;
    strip.cull = GpuCull::Back;
    Scene quad(std::vector<float>{-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f}, strip);
    CHECK(count_pixels(render(frame_of(6, 6, {quad.draw(4, Color{255, 0, 0, 128})}), quad.resources), Color{128, 0, 0, 255}) == 36);
}

void test_indices_and_instances()
{
    // Two instances of an indexed lower-left triangle, the second moved to the upper right
    Scene scene(std::vector<float>{-1.f, -1.f, 0.f, -1.f, -1.f, 0.f});
    const std::array<float, 4> offsets = {0.f, 0.f, 1.f, 1.f};
    scene.resources.set_buffer_values(scene.offsets, std::span<const float>(offsets));
    const GpuHandle indices = scene.resources.create_buffer(GpuBufferUsage::Index16);
    const std::array<std::uint16_t, 3> order = {0, 1, 2};
    scene.resources.set_buffer_values(indices, std::span<const std::uint16_t>(order));
    GpuDraw draw = scene.draw(3);
    draw.index_buffer = indices;
    draw.instance_count = 2;
    const Image image = render(frame_of(8, 8, {draw}), scene.resources);
    CHECK(image.pixel(0, 7) == (Color{255, 0, 0}) && image.pixel(4, 3) == (Color{255, 0, 0}));
    CHECK(image.pixel(7, 0) == Color::black() && image.pixel(0, 0) == Color::black());
    // 32-bit indices, too
    const GpuHandle wide = scene.resources.create_buffer(GpuBufferUsage::Index32);
    const std::array<std::uint32_t, 3> wideOrder = {0, 1, 2};
    scene.resources.set_buffer_values(wide, std::span<const std::uint32_t>(wideOrder));
    draw.index_buffer = wide;
    CHECK(render(frame_of(8, 8, {draw}), scene.resources).pixel(4, 3) == (Color{255, 0, 0}));
    // Unknown shaders and missing pipelines draw nothing
    SoftwareRenderer plain;
    CHECK(!plain.has_shader("flat") && plain.has_shader("channeldata") && plain.has_shader("overlay"));
    CHECK(count_pixels(plain.render(frame_of(8, 8, {draw}), scene.resources), Color::black()) == 64);
}

void test_textures()
{
    // The overlay shader shows a texture over the screen; with grid_enabled = 0 it is a plain blit
    GpuResources resources;
    GpuPipeline state;
    state.shader = "overlay";
    state.bindings = {{4 * sizeof(float), false}};
    state.attributes = {{0, 0, GpuVertexFormat::Float2, 0}, {1, 0, GpuVertexFormat::Float2, 2 * sizeof(float)}};
    state.topology = GpuTopology::TriangleStrip;
    state.blend = GpuBlend::Alpha;
    const GpuHandle pipeline = resources.create_pipeline(state);
    const std::array<float, 16> quad = {-1.f, -1.f, 0.f, 1.f, -1.f, 1.f, 0.f, 0.f, 1.f, -1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 0.f};
    const GpuHandle vertices = resources.create_buffer(GpuBufferUsage::Vertex);
    resources.set_buffer_values(vertices, std::span<const float>(quad));
    Image pixels = Image::transparent(2, 2);
    pixels.set_pixel(0, 0, Color{255, 0, 0});
    pixels.set_pixel(1, 1, Color{0, 0, 255});
    GpuTexture texture{pixels};
    texture.linear_filter = false;
    const GpuHandle handle = resources.create_texture(texture);

    GpuDraw draw;
    draw.pipeline = pipeline;
    draw.vertex_buffers = {vertices};
    draw.count = 4;
    draw.textures = {{1, handle}};
    struct
    {
        float scroll_sample = 0.f, samples_per_pixel = 1.f, view_width = 4.f, sfreq = 0.f;
        float first_file_sample = 0.f, grid_enabled = 0.f, overlay_first_sample = 0.f, overlay_total_samples = 4.f;
    } uniforms;
    draw.set_uniforms(2, uniforms);
    SoftwareRenderer renderer;
    const Image image = renderer.render(frame_of(4, 4, {draw}, Color::white()), resources);
    CHECK(image.pixel(0, 0) == (Color{255, 0, 0}) && image.pixel(1, 1) == (Color{255, 0, 0}));
    CHECK(image.pixel(3, 3) == (Color{0, 0, 255}) && image.pixel(3, 0) == Color::white() && image.pixel(0, 3) == Color::white());

    // A figure texture is rasterized on a transparent ground
    Canvas figure(4, 4);
    figure.fill_rect(Rect{0, 0, 2, 4}, Brush::solid(Color{0, 128, 0}));
    resources.set_texture(handle, figure.display_list());
    const Image fromFigure = renderer.render(frame_of(4, 4, {draw}, Color::white()), resources);
    CHECK(fromFigure.pixel(0, 2) == (Color{0, 128, 0}) && fromFigure.pixel(3, 2) == Color::white());
}

void test_rgba_figures()
{
    // Opacity survives the rasterization; fully transparent pixels stay transparent
    Canvas figure(6, 2);
    figure.fill_rect(Rect{0, 0, 3, 2}, Brush::solid(Color{200, 100, 0, 128}));
    const Image image = to_rgba_image(figure.display_list(), 2.0);
    CHECK(image.width == 12 && image.height == 4);
    const Color c = image.pixel(1, 1);
    CHECK(std::abs(c.a - 128) <= 1 && std::abs(c.r - 200) <= 2 && std::abs(c.g - 100) <= 2 && c.b <= 2);
    CHECK(image.pixel(10, 1).a == 0);
}

void test_near_plane_clipping()
{
    // A quad tilted through the near plane: the part in front of it is drawn
    SoftwareShader shader;
    shader.vertex = [](const SoftwareAttributes& in, const SoftwareBindings&, SoftwareVaryings&) {
        // x from -1 to 1 maps depth from -3 (behind the near plane) to 0.5
        return Eigen::Vector4f(in[0].x(), in[0].y(), -1.25f + 1.75f * in[0].x(), 1.f);
    };
    shader.fragment = [](const SoftwareVaryings&, const SoftwareFragment&, const SoftwareBindings&) -> std::optional<Eigen::Vector4f> {
        return Eigen::Vector4f(1.f, 1.f, 1.f, 1.f);
    };
    GpuResources resources;
    GpuPipeline state;
    state.shader = "tilted";
    state.bindings = {{2 * sizeof(float), false}};
    state.attributes = {{0, 0, GpuVertexFormat::Float2, 0}};
    const GpuHandle pipeline = resources.create_pipeline(state);
    const GpuHandle vertices = resources.create_buffer(GpuBufferUsage::Vertex);
    resources.set_buffer_values(vertices, std::span<const float>(kQuad));
    GpuDraw draw;
    draw.pipeline = pipeline;
    draw.vertex_buffers = {vertices};
    draw.count = 6;
    SoftwareRenderer renderer;
    renderer.set_shader("tilted", shader);
    const Image image = renderer.render(frame_of(8, 2, {draw}), resources);
    // depth >= -1 for x >= 1/7: the right half and a bit more is drawn
    CHECK(image.pixel(7, 0) == Color::white() && image.pixel(5, 1) == Color::white());
    CHECK(image.pixel(0, 0) == Color::black() && image.pixel(3, 1) == Color::black());
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"resources", test_resources},
        {"coverage_and_blending", test_coverage_and_blending},
        {"culling", test_culling},
        {"depth", test_depth},
        {"lines_points_and_strips", test_lines_points_and_strips},
        {"indices_and_instances", test_indices_and_instances},
        {"textures", test_textures},
        {"rgba_figures", test_rgba_figures},
        {"near_plane_clipping", test_near_plane_clipping},
    };
    for (const auto& [name, test] : tests) {
        const int before = g_failures;
        test();
        std::printf("%s %s\n", g_failures == before ? "PASS" : "FAIL", name);
    }
    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
