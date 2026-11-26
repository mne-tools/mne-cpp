//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     software_renderer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    SoftwareRenderer definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "software_renderer.h"
#include "detail/software_shaders.h"
#include "raster_renderer.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

//=============================================================================================================
// DEFINE STATIC HELPERS
//=============================================================================================================

namespace
{

/** A vertex after the vertex shader. */
struct ShadedVertex
{
    Eigen::Vector4f clip = Eigen::Vector4f::Zero();
    SoftwareVaryings varyings{};
};

/** A vertex in target pixels. */
struct ScreenVertex
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float inv_w = 1.f;
    const SoftwareVaryings* varyings = nullptr;
};

/** Pixel rectangle [x0, x1) x [y0, y1). */
struct PixelRect
{
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
};

ShadedVertex mix(const ShadedVertex& a, const ShadedVertex& b, float t, int varyingCount)
{
    ShadedVertex out;
    out.clip = a.clip + t * (b.clip - a.clip);
    for (int i = 0; i < varyingCount; ++i) {
        out.varyings[static_cast<std::size_t>(i)] = a.varyings[static_cast<std::size_t>(i)] + t * (b.varyings[static_cast<std::size_t>(i)] - a.varyings[static_cast<std::size_t>(i)]);
    }
    return out;
}

/** Signed distance of a clip-space vertex to the near (z = -w) or far (z = w) plane; inside where >= 0. */
float plane_distance(const Eigen::Vector4f& clip, bool nearPlane)
{
    return nearPlane ? clip.w() + clip.z() : clip.w() - clip.z();
}

/** Clips a convex polygon against the near and far planes (Sutherland-Hodgman). */
std::vector<ShadedVertex> clip_polygon(std::vector<ShadedVertex> polygon, int varyingCount)
{
    for (const bool nearPlane : {true, false}) {
        std::vector<ShadedVertex> out;
        for (std::size_t i = 0; i < polygon.size(); ++i) {
            const ShadedVertex& a = polygon[i];
            const ShadedVertex& b = polygon[(i + 1) % polygon.size()];
            const float da = plane_distance(a.clip, nearPlane);
            const float db = plane_distance(b.clip, nearPlane);
            if (da >= 0.f) {
                out.push_back(a);
            }
            if ((da >= 0.f) != (db >= 0.f)) {
                out.push_back(mix(a, b, da / (da - db), varyingCount));
            }
        }
        polygon = std::move(out);
        if (polygon.size() < 3) {
            return {};
        }
    }
    return polygon;
}

float edge(float ax, float ay, float bx, float by, float px, float py)
{
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

/** Whether an edge of a triangle with positive edge() area is a top or left edge (the fill rule's tie-break). */
bool top_left(const ScreenVertex& a, const ScreenVertex& b)
{
    return (a.y == b.y && b.x > a.x) || b.y < a.y;
}

Eigen::Vector4f fetch(const GpuBuffer& buffer, std::size_t offset, GpuVertexFormat format)
{
    Eigen::Vector4f value(0.f, 0.f, 0.f, 1.f);
    const auto read = [&](std::size_t count) {
        if (offset + count * sizeof(float) > buffer.data.size()) {
            return;
        }
        float components[4] = {};
        std::memcpy(components, buffer.data.data() + offset, count * sizeof(float));
        for (std::size_t i = 0; i < count; ++i) {
            value[static_cast<Eigen::Index>(i)] = components[i];
        }
    };
    switch (format) {
        case GpuVertexFormat::Float:
            read(1);
            break;
        case GpuVertexFormat::Float2:
            read(2);
            break;
        case GpuVertexFormat::Float3:
            read(3);
            break;
        case GpuVertexFormat::Float4:
            read(4);
            break;
        case GpuVertexFormat::UNormByte4:
            if (offset + 4 <= buffer.data.size()) {
                for (Eigen::Index i = 0; i < 4; ++i) {
                    value[i] = static_cast<float>(std::to_integer<int>(buffer.data[offset + static_cast<std::size_t>(i)])) / 255.f;
                }
            }
            break;
    }
    return value;
}

std::uint32_t index_at(const GpuBuffer* indices, std::uint32_t position)
{
    if (!indices) {
        return position;
    }
    if (indices->usage == GpuBufferUsage::Index16) {
        std::uint16_t index = 0;
        if (std::size_t{position} * 2 + 2 <= indices->data.size()) {
            std::memcpy(&index, indices->data.data() + std::size_t{position} * 2, 2);
        }
        return index;
    }
    std::uint32_t index = 0;
    if (std::size_t{position} * 4 + 4 <= indices->data.size()) {
        std::memcpy(&index, indices->data.data() + std::size_t{position} * 4, 4);
    }
    return index;
}

std::uint8_t to_byte(float value)
{
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.f, 1.f) * 255.f));
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void SoftwareBindings::copy_uniforms(int binding, void* target, std::size_t size) const
{
    if (!m_draw) {
        return;
    }
    for (const GpuUniformBlock& block : m_draw->uniforms) {
        if (block.binding == binding) {
            std::memcpy(target, block.data.data(), std::min(size, block.data.size()));
            return;
        }
    }
}

//=============================================================================================================

Eigen::Vector4f SoftwareBindings::sample(int binding, const Eigen::Vector2f& uv) const
{
    const auto bound = std::find_if(m_textures.begin(), m_textures.end(), [binding](const BoundTexture& t) {
        return t.binding == binding;
    });
    if (bound == m_textures.end() || !bound->image || bound->image->width <= 0 || bound->image->height <= 0) {
        return Eigen::Vector4f::Zero();
    }
    const Image& image = *bound->image;
    const auto texel = [&image](int x, int y) -> Eigen::Vector4f {
        const Color c = image.pixel(std::clamp(x, 0, image.width - 1), std::clamp(y, 0, image.height - 1));
        return Eigen::Vector4f(c.r, c.g, c.b, c.a) / 255.f;
    };
    const float u = uv.x() * static_cast<float>(image.width);
    const float v = uv.y() * static_cast<float>(image.height);
    if (!bound->linear) {
        return texel(static_cast<int>(std::floor(u)), static_cast<int>(std::floor(v)));
    }
    const float x = u - 0.5f;
    const float y = v - 0.5f;
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const float fx = x - static_cast<float>(x0);
    const float fy = y - static_cast<float>(y0);
    const Eigen::Vector4f top = (1.f - fx) * texel(x0, y0) + fx * texel(x0 + 1, y0);
    const Eigen::Vector4f bottom = (1.f - fx) * texel(x0, y0 + 1) + fx * texel(x0 + 1, y0 + 1);
    return (1.f - fy) * top + fy * bottom;
}

//=============================================================================================================

struct SoftwareRenderer::Target
{
    int width = 0;
    int height = 0;
    std::vector<Eigen::Vector4f> color;
    std::vector<float> depth;
};

//=============================================================================================================

SoftwareRenderer::SoftwareRenderer()
{
    detail::register_builtin_software_shaders(*this);
}

//=============================================================================================================

void SoftwareRenderer::set_shader(const std::string& id, SoftwareShader shader)
{
    m_shaders[id] = std::move(shader);
}

//=============================================================================================================

bool SoftwareRenderer::has_shader(const std::string& id) const
{
    return m_shaders.contains(id);
}

//=============================================================================================================

const Image* SoftwareRenderer::texture_image(GpuHandle handle, const GpuTexture& texture)
{
    CachedTexture& cached = m_textures[handle];
    if (cached.revision != texture.revision || cached.image.width != texture.pixel_width()) {
        if (const Image* image = std::get_if<Image>(&texture.content)) {
            cached.image = *image;
        } else {
            cached.image = to_rgba_image(std::get<DisplayList>(texture.content), texture.scale);
        }
        cached.revision = texture.revision;
    }
    return &cached.image;
}

//=============================================================================================================

Image SoftwareRenderer::render(const GpuFrame& frame, const GpuResources& resources)
{
    Target target;
    target.width = std::max(frame.pixel_width, 0);
    target.height = std::max(frame.pixel_height, 0);
    const std::size_t pixels = static_cast<std::size_t>(target.width) * static_cast<std::size_t>(target.height);
    target.color.assign(pixels, Eigen::Vector4f::Zero());
    target.depth.assign(pixels, 1.f);

    for (const GpuPass& pass : frame.passes) {
        if (pass.clear_color) {
            const Color c = *pass.clear_color;
            std::fill(target.color.begin(), target.color.end(), Eigen::Vector4f(c.r, c.g, c.b, c.a) / 255.f);
        }
        if (pass.clear_depth) {
            std::fill(target.depth.begin(), target.depth.end(), 1.f);
        }
        for (const GpuDraw& d : pass.draws) {
            draw(target, d, resources);
        }
    }

    Image image = Image::transparent(target.width, target.height);
    for (int y = 0; y < target.height; ++y) {
        for (int x = 0; x < target.width; ++x) {
            const Eigen::Vector4f& c = target.color[static_cast<std::size_t>(y) * static_cast<std::size_t>(target.width) + static_cast<std::size_t>(x)];
            image.set_pixel(x, y, Color{to_byte(c.x()), to_byte(c.y()), to_byte(c.z()), to_byte(c.w())});
        }
    }
    return image;
}

//=============================================================================================================

void SoftwareRenderer::draw(Target& target, const GpuDraw& draw, const GpuResources& resources)
{
    const GpuPipeline* pipeline = resources.pipeline(draw.pipeline);
    if (!pipeline || draw.count == 0) {
        return;
    }
    const auto shaderIt = m_shaders.find(pipeline->shader);
    if (shaderIt == m_shaders.end() || !shaderIt->second.vertex || !shaderIt->second.fragment) {
        return;
    }
    const SoftwareShader& shader = shaderIt->second;
    const int varyingCount = std::clamp(shader.varying_count, 0, SoftwareBindings::kMaxVaryings);

    SoftwareBindings bindings;
    bindings.m_draw = &draw;
    for (const GpuTextureBinding& binding : draw.textures) {
        if (const GpuTexture* texture = resources.texture(binding.texture)) {
            bindings.m_textures.push_back({binding.binding, texture_image(binding.texture, *texture), texture->linear_filter});
        }
    }

    // Viewport and the pixels that may be written
    const GpuRect viewport = draw.viewport.value_or(GpuRect{0, 0, target.width, target.height});
    PixelRect clipRect{std::max(viewport.x, 0), std::max(viewport.y, 0), std::min(viewport.x + viewport.width, target.width),
                       std::min(viewport.y + viewport.height, target.height)};
    if (draw.scissor) {
        clipRect.x0 = std::max(clipRect.x0, draw.scissor->x);
        clipRect.y0 = std::max(clipRect.y0, draw.scissor->y);
        clipRect.x1 = std::min(clipRect.x1, draw.scissor->x + draw.scissor->width);
        clipRect.y1 = std::min(clipRect.y1, draw.scissor->y + draw.scissor->height);
    }
    if (clipRect.x0 >= clipRect.x1 || clipRect.y0 >= clipRect.y1) {
        return;
    }

    std::vector<const GpuBuffer*> vertexBuffers;
    vertexBuffers.reserve(draw.vertex_buffers.size());
    for (const GpuHandle handle : draw.vertex_buffers) {
        vertexBuffers.push_back(resources.buffer(handle));
    }
    const GpuBuffer* indices = draw.index_buffer != 0 ? resources.buffer(draw.index_buffer) : nullptr;
    if (draw.index_buffer != 0 && !indices) {
        return;
    }

    const auto toScreen = [&viewport](const ShadedVertex& v) {
        ScreenVertex s;
        s.inv_w = 1.f / v.clip.w();
        s.x = static_cast<float>(viewport.x) + (v.clip.x() * s.inv_w + 1.f) * 0.5f * static_cast<float>(viewport.width);
        s.y = static_cast<float>(viewport.y) + (1.f - v.clip.y() * s.inv_w) * 0.5f * static_cast<float>(viewport.height);
        s.z = v.clip.z() * s.inv_w * 0.5f + 0.5f;
        s.varyings = &v.varyings;
        return s;
    };

    // Depth test, fragment shader and blending of one fragment
    const auto shade = [&](int x, int y, float depth, const SoftwareVaryings& varyings, bool frontFacing) {
        const std::size_t pixel = static_cast<std::size_t>(y) * static_cast<std::size_t>(target.width) + static_cast<std::size_t>(x);
        if (pipeline->depth_test) {
            const float stored = target.depth[pixel];
            const bool pass = pipeline->depth_compare == GpuDepthCompare::Always || (pipeline->depth_compare == GpuDepthCompare::Less && depth < stored) || (pipeline->depth_compare == GpuDepthCompare::LessOrEqual && depth <= stored);
            if (!pass) {
                return;
            }
        }
        SoftwareFragment fragment;
        fragment.position = Eigen::Vector2f(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
        fragment.depth = depth;
        fragment.front_facing = frontFacing;
        const std::optional<Eigen::Vector4f> color = shader.fragment(varyings, fragment, bindings);
        if (!color) {
            return;
        }
        if (pipeline->depth_write) {
            target.depth[pixel] = depth;
        }
        if (!pipeline->color_write) {
            return;
        }
        Eigen::Vector4f& dst = target.color[pixel];
        const float a = std::clamp(color->w(), 0.f, 1.f);
        switch (pipeline->blend) {
            case GpuBlend::None:
                dst = *color;
                break;
            case GpuBlend::Alpha:
                dst.head<3>() = color->head<3>() * a + dst.head<3>() * (1.f - a);
                dst.w() = a + dst.w() * (1.f - a);
                break;
            case GpuBlend::Additive:
                dst.head<3>() += color->head<3>() * a;
                dst.w() += a;
                break;
        }
        dst = dst.cwiseMax(0.f).cwiseMin(1.f);
    };

    const auto drawTriangle = [&](const ShadedVertex& a, const ShadedVertex& b, const ShadedVertex& c) {
        const std::vector<ShadedVertex> polygon = clip_polygon({a, b, c}, varyingCount);
        for (std::size_t k = 1; k + 1 < polygon.size(); ++k) {
            ScreenVertex v0 = toScreen(polygon[0]);
            ScreenVertex v1 = toScreen(polygon[k]);
            ScreenVertex v2 = toScreen(polygon[k + 1]);
            float area = edge(v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);
            if (area == 0.f || !std::isfinite(area)) {
                continue;
            }
            // Counter-clockwise in clip space (y up) is clockwise on the target (y down), i.e. a negative area
            const bool frontFacing = pipeline->front_counter_clockwise ? area < 0.f : area > 0.f;
            if ((pipeline->cull == GpuCull::Back && !frontFacing) || (pipeline->cull == GpuCull::Front && frontFacing)) {
                continue;
            }
            if (area < 0.f) {
                std::swap(v1, v2);
                area = -area;
            }
            const int x0 = std::max(clipRect.x0, static_cast<int>(std::floor(std::min({v0.x, v1.x, v2.x}))));
            const int x1 = std::min(clipRect.x1, static_cast<int>(std::ceil(std::max({v0.x, v1.x, v2.x}))) + 1);
            const int y0 = std::max(clipRect.y0, static_cast<int>(std::floor(std::min({v0.y, v1.y, v2.y}))));
            const int y1 = std::min(clipRect.y1, static_cast<int>(std::ceil(std::max({v0.y, v1.y, v2.y}))) + 1);
            const bool topLeft0 = top_left(v1, v2);
            const bool topLeft1 = top_left(v2, v0);
            const bool topLeft2 = top_left(v0, v1);
            SoftwareVaryings varyings{};
            for (int y = y0; y < y1; ++y) {
                const float py = static_cast<float>(y) + 0.5f;
                for (int x = x0; x < x1; ++x) {
                    const float px = static_cast<float>(x) + 0.5f;
                    const float w0 = edge(v1.x, v1.y, v2.x, v2.y, px, py);
                    const float w1 = edge(v2.x, v2.y, v0.x, v0.y, px, py);
                    const float w2 = edge(v0.x, v0.y, v1.x, v1.y, px, py);
                    if (w0 < 0.f || w1 < 0.f || w2 < 0.f || (w0 == 0.f && !topLeft0) || (w1 == 0.f && !topLeft1) || (w2 == 0.f && !topLeft2)) {
                        continue;
                    }
                    const float l0 = w0 / area;
                    const float l1 = w1 / area;
                    const float l2 = w2 / area;
                    const float depth = l0 * v0.z + l1 * v1.z + l2 * v2.z;
                    // Perspective-correct varyings
                    const float p0 = l0 * v0.inv_w;
                    const float p1 = l1 * v1.inv_w;
                    const float p2 = l2 * v2.inv_w;
                    const float norm = 1.f / (p0 + p1 + p2);
                    for (std::size_t i = 0; i < static_cast<std::size_t>(varyingCount); ++i) {
                        varyings[i] = ((*v0.varyings)[i] * p0 + (*v1.varyings)[i] * p1 + (*v2.varyings)[i] * p2) * norm;
                    }
                    shade(x, y, depth, varyings, frontFacing);
                }
            }
        }
    };

    const auto drawLine = [&](ShadedVertex a, ShadedVertex b) {
        // Clip against the near and far planes
        for (const bool nearPlane : {true, false}) {
            const float da = plane_distance(a.clip, nearPlane);
            const float db = plane_distance(b.clip, nearPlane);
            if (da < 0.f && db < 0.f) {
                return;
            }
            if (da < 0.f) {
                a = mix(a, b, da / (da - db), varyingCount);
            } else if (db < 0.f) {
                b = mix(a, b, da / (da - db), varyingCount);
            }
        }
        const ScreenVertex s0 = toScreen(a);
        const ScreenVertex s1 = toScreen(b);
        const float dx = s1.x - s0.x;
        const float dy = s1.y - s0.y;
        // One fragment per pixel along the major axis; the last pixel is left to the next segment
        const int steps = static_cast<int>(std::lround(std::max(std::abs(dx), std::abs(dy))));
        SoftwareVaryings varyings{};
        for (int step = 0; step < std::max(steps, 1); ++step) {
            const float t = steps > 0 ? (static_cast<float>(step) + 0.5f) / static_cast<float>(steps) : 0.f;
            const int x = static_cast<int>(std::floor(s0.x + t * dx));
            const int y = static_cast<int>(std::floor(s0.y + t * dy));
            if (x < clipRect.x0 || x >= clipRect.x1 || y < clipRect.y0 || y >= clipRect.y1) {
                continue;
            }
            for (std::size_t i = 0; i < static_cast<std::size_t>(varyingCount); ++i) {
                varyings[i] = (*s0.varyings)[i] + t * ((*s1.varyings)[i] - (*s0.varyings)[i]);
            }
            shade(x, y, s0.z + t * (s1.z - s0.z), varyings, true);
        }
    };

    const auto drawPoint = [&](const ShadedVertex& v) {
        if (plane_distance(v.clip, true) < 0.f || plane_distance(v.clip, false) < 0.f) {
            return;
        }
        const ScreenVertex s = toScreen(v);
        const int x = static_cast<int>(std::floor(s.x));
        const int y = static_cast<int>(std::floor(s.y));
        if (x >= clipRect.x0 && x < clipRect.x1 && y >= clipRect.y0 && y < clipRect.y1) {
            shade(x, y, s.z, *s.varyings, true);
        }
    };

    for (std::uint32_t instance = 0; instance < draw.instance_count; ++instance) {
        // Run the vertex shader once per referenced vertex
        std::vector<ShadedVertex> shaded(draw.count);
        for (std::uint32_t k = 0; k < draw.count; ++k) {
            const std::uint32_t vertex = index_at(indices, draw.first + k);
            SoftwareAttributes attributes;
            attributes.fill(Eigen::Vector4f(0.f, 0.f, 0.f, 1.f));
            for (const GpuVertexAttribute& attribute : pipeline->attributes) {
                if (attribute.location < 0 || attribute.location >= SoftwareBindings::kMaxAttributes || attribute.binding < 0 || attribute.binding >= std::ssize(vertexBuffers) || attribute.binding >= std::ssize(pipeline->bindings)) {
                    continue;
                }
                const GpuBuffer* buffer = vertexBuffers[static_cast<std::size_t>(attribute.binding)];
                if (!buffer) {
                    continue;
                }
                const GpuVertexBinding& layout = pipeline->bindings[static_cast<std::size_t>(attribute.binding)];
                const std::size_t element = layout.per_instance ? instance : vertex;
                attributes[static_cast<std::size_t>(attribute.location)] = fetch(*buffer, element * layout.stride + attribute.offset, attribute.format);
            }
            ShadedVertex& out = shaded[k];
            out.clip = shader.vertex(attributes, bindings, out.varyings);
        }

        switch (pipeline->topology) {
            case GpuTopology::Triangles:
                for (std::size_t k = 0; k + 2 < shaded.size(); k += 3) {
                    drawTriangle(shaded[k], shaded[k + 1], shaded[k + 2]);
                }
                break;
            case GpuTopology::TriangleStrip:
                for (std::size_t k = 0; k + 2 < shaded.size(); ++k) {
                    // Every other triangle is reversed to keep the strip's winding
                    if (k % 2 == 0) {
                        drawTriangle(shaded[k], shaded[k + 1], shaded[k + 2]);
                    } else {
                        drawTriangle(shaded[k + 1], shaded[k], shaded[k + 2]);
                    }
                }
                break;
            case GpuTopology::Lines:
                for (std::size_t k = 0; k + 1 < shaded.size(); k += 2) {
                    drawLine(shaded[k], shaded[k + 1]);
                }
                break;
            case GpuTopology::LineStrip:
                for (std::size_t k = 0; k + 1 < shaded.size(); ++k) {
                    drawLine(shaded[k], shaded[k + 1]);
                }
                break;
            case GpuTopology::Points:
                for (const ShadedVertex& v : shaded) {
                    drawPoint(v);
                }
                break;
        }
    }
}

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

Image mne::scene::to_rgba_image(const DisplayList& list, double scale)
{
    RasterOptions options;
    options.scale = scale;
    options.background = Color::black();
    const RasterImage onBlack = to_raster(list, options);
    options.background = Color::white();
    const RasterImage onWhite = to_raster(list, options);

    Image image = Image::transparent(onBlack.width(), onBlack.height());
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const Rgb black = onBlack.pixel(x, y);
            const Rgb white = onWhite.pixel(x, y);
            // On black a pixel is c * a, on white c * a + 255 * (1 - a)
            const int alpha = std::clamp(255 - ((white.r - black.r) + (white.g - black.g) + (white.b - black.b)) / 3, 0, 255);
            if (alpha == 0) {
                continue;
            }
            const auto unmix = [alpha](int premultiplied) {
                return static_cast<std::uint8_t>(std::clamp((premultiplied * 255 + alpha / 2) / alpha, 0, 255));
            };
            image.set_pixel(x, y, Color{unmix(black.r), unmix(black.g), unmix(black.b), static_cast<std::uint8_t>(alpha)});
        }
    }
    return image;
}
