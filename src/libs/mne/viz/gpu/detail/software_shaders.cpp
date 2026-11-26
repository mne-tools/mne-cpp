//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     software_shaders.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Software implementations of the shaders of the views in mne_viz.
 *
 * Each function mirrors the GLSL shader of the same name in mne_viz_qt or mne_viz_qt; keep them in step.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "software_shaders.h"
#include "../brain_frame.h"
#include "../software_renderer.h"
#include "../trace_frame.h"
#include "../../views/trace_geometry.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <functional>
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

/** channeldata.vert / channeldata.frag of mne_viz_qt: one trace as a line strip. */
SoftwareShader channeldata_shader()
{
    SoftwareShader shader;
    shader.varying_count = 1;
    shader.vertex = [](const SoftwareAttributes& in, const SoftwareBindings& bindings, SoftwareVaryings& out) {
        const auto u = bindings.uniforms<TraceUniforms>(0);
        const float sample = u.first_sample + in[0].x();
        const float xPixel = (sample - u.scroll_sample) / std::max(u.samples_per_pixel, 1e-6f);
        const float x = (xPixel / u.view_width) * 2.f - 1.f;
        const float norm = u.amplitude_max > 0.f ? std::clamp(in[0].y() / u.amplitude_max, -2.f, 2.f) : 0.f;
        out[0] = norm;
        return Eigen::Vector4f(x, u.channel_y_center + norm * (u.channel_y_range * 0.45f), 0.f, 1.f);
    };
    shader.fragment = [](const SoftwareVaryings& in, const SoftwareFragment&, const SoftwareBindings& bindings) -> std::optional<Eigen::Vector4f> {
        const auto u = bindings.uniforms<TraceUniforms>(0);
        if (u.show_clipping > 0.5f && std::abs(in[0]) >= 0.95f) {
            return Eigen::Vector4f(1.f, 0.f, 0.f, 1.f);
        }
        return Eigen::Vector4f(u.color[0], u.color[1], u.color[2], u.color[3]);
    };
    return shader;
}

/** overlay.vert / overlay.frag of mne_viz_qt: a texture over the screen with the per-second bands. */
SoftwareShader overlay_shader()
{
    SoftwareShader shader;
    shader.varying_count = 2;
    shader.vertex = [](const SoftwareAttributes& in, const SoftwareBindings&, SoftwareVaryings& out) {
        out[0] = in[1].x();
        out[1] = in[1].y();
        return Eigen::Vector4f(in[0].x(), in[0].y(), 0.f, 1.f);
    };
    shader.fragment = [](const SoftwareVaryings& in, const SoftwareFragment&, const SoftwareBindings& bindings) -> std::optional<Eigen::Vector4f> {
        const auto u = bindings.uniforms<TraceOverlayUniforms>(2);
        const float screenSample = u.scroll_sample + in[0] * u.view_width * u.samples_per_pixel;
        const float overlayU = std::clamp((screenSample - u.overlay_first_sample) / std::max(u.overlay_total_samples, 1.f), 0.f, 1.f);
        Eigen::Vector4f color = bindings.sample(1, Eigen::Vector2f(overlayU, in[1]));
        if (u.grid_enabled > 0.5f && u.sfreq > 0.f) {
            const float second = std::floor((screenSample - u.first_file_sample) / u.sfreq);
            if (std::fmod(std::abs(second), 2.f) > 0.5f) {
                color.head<3>() *= 1.f - 0.08f;
                color.w() = std::max(color.w(), 0.08f);
            }
        }
        return color;
    };
    return shader;
}


using Vec3 = Eigen::Vector3f;
using Vec4 = Eigen::Vector4f;

Vec3 varying3(const SoftwareVaryings& v, std::size_t at)
{
    return {v[at], v[at + 1], v[at + 2]};
}

void set_varying3(SoftwareVaryings& v, std::size_t at, const Vec3& value)
{
    v[at] = value.x();
    v[at + 1] = value.y();
    v[at + 2] = value.z();
}

Vec3 mix3(const Vec3& a, const Vec3& b, float t)
{
    return a + t * (b - a);
}

float smoothstep(float edge0, float edge1, float x)
{
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

// Varyings of the surface shaders: world position, normal, colour, annotation colour, curvature grey, highlight, view
// direction
constexpr std::size_t kWorldPos = 0;
constexpr std::size_t kNormal = 3;
constexpr std::size_t kColor = 6;
constexpr std::size_t kAnnotation = 9;
constexpr std::size_t kCurvature = 12;
constexpr std::size_t kHighlight = 13;
constexpr std::size_t kViewDir = 14;

/** The vertex stage shared by standard, holographic and anatomical .vert. */
Vec4 surface_vertex(const SoftwareAttributes& in, const SoftwareBindings& bindings, SoftwareVaryings& out)
{
    const auto u = bindings.uniforms<BrainSurfaceUniforms>(0);
    const Vec3 position = in[0].head<3>();
    const Vec4& annotation = in[3];
    const Vec4 selected(u.selected_label[0], u.selected_label[1], u.selected_label[2], u.selected_label[3]);
    set_varying3(out, kWorldPos, position);
    set_varying3(out, kNormal, in[1].head<3>().normalized());
    set_varying3(out, kColor, in[2].head<3>());
    set_varying3(out, kAnnotation, annotation.head<3>());
    out[kCurvature] = in[2].w();
    out[kHighlight] = u.label_active > 0.5f && ((annotation - selected).cwiseAbs().array() < 0.5f / 255.f).all() ? 1.f : 0.f;
    set_varying3(out, kViewDir, (Vec3(u.camera_pos[0], u.camera_pos[1], u.camera_pos[2]) - position).normalized());
    return Eigen::Map<const Eigen::Matrix4f>(u.mvp) * Vec4(position.x(), position.y(), position.z(), 1.f);
}

template<std::optional<Vec4> (*Fragment)(const SoftwareVaryings&, const BrainSurfaceUniforms&)>
SoftwareShader surface_shader()
{
    SoftwareShader shader;
    shader.varying_count = 17;
    shader.vertex = surface_vertex;
    shader.fragment = [](const SoftwareVaryings& in, const SoftwareFragment&, const SoftwareBindings& bindings) -> std::optional<Vec4> {
        return Fragment(in, bindings.uniforms<BrainSurfaceUniforms>(0));
    };
    return shader;
}

/** standard.frag of mne_viz_qt. */
std::optional<Vec4> standard_fragment(const SoftwareVaryings& in, const BrainSurfaceUniforms& u)
{
    const Vec3 n = varying3(in, kNormal).normalized();
    const Vec3 l = Vec3(u.light_dir[0], u.light_dir[1], u.light_dir[2]).normalized();
    const Vec3 v = (Vec3(u.camera_pos[0], u.camera_pos[1], u.camera_pos[2]) - varying3(in, kWorldPos)).normalized();
    const Vec3 h = (l + v).normalized();
    const float diff = std::max(n.dot(l), 0.f);
    const float spec = std::pow(std::max(n.dot(h), 0.f), 32.f) * 0.3f;

    const Vec3 annotation = varying3(in, kAnnotation);
    Vec3 base;
    if (u.overlay_mode < 0.5f) {
        base = Vec3::Constant(in[kCurvature]);
    } else if (u.overlay_mode < 1.5f) {
        base = annotation.squaredNorm() > 0.001f ? annotation : Vec3(1.f, 0.97f, 0.94f);
    } else {
        base = varying3(in, kColor);
    }
    if (in[kHighlight] > 0.5f) {
        base = Vec3(1.f, 0.784f, 0.235f);
    }
    base = base.cwiseProduct(Vec3(1.f, 0.97f, 0.94f));
    const float fill = std::max(n.dot(Vec3(0.f, -0.5f, 0.3f).normalized()), 0.f) * 0.15f;
    Vec3 color = base * (0.35f + diff * 0.65f + fill) + Vec3(0.95f, 0.95f, 1.f) * spec;
    color = color.cwiseMax(0.f).cwiseMin(1.f);
    if (u.is_selected > 0.5f) {
        const float fresnel = std::pow(1.f - std::clamp(n.dot(v), 0.f, 1.f), 2.5f);
        const Vec3 glow(1.f, 0.85f, 0.3f);
        color = mix3(color, glow, 0.25f) + glow * fresnel * 0.6f;
    }
    return Vec4(color.x(), color.y(), color.z(), 1.f);
}

/** holographic.frag of mne_viz_qt: premultiplied output for additive blending. */
std::optional<Vec4> holographic_fragment(const SoftwareVaryings& in, const BrainSurfaceUniforms& u)
{
    const Vec3 n = varying3(in, kNormal).normalized();
    const Vec3 v = varying3(in, kViewDir).normalized();
    const Vec3 annotation = varying3(in, kAnnotation);
    Vec3 color;
    if (u.overlay_mode < 0.5f) {
        color = Vec3::Constant(in[kCurvature]);
    } else if (u.overlay_mode < 1.5f) {
        color = annotation.squaredNorm() > 0.001f ? annotation : Vec3::Ones();
    } else {
        color = varying3(in, kColor);
    }
    if (in[kHighlight] > 0.5f) {
        color = Vec3(1.f, 0.784f, 0.235f);
    }
    const float gray = color.dot(Vec3(0.299f, 0.587f, 0.114f));
    const float isData = smoothstep(0.0025f, 0.04f, (color - Vec3::Constant(gray)).squaredNorm());
    const float nDotV = std::abs(n.dot(v));
    const float fresnel = std::pow(1.f - nDotV, 2.5f);
    const float rim = std::pow(1.f - nDotV, 3.f);
    const Vec3 shell = Vec3(0.f, 0.4f, 0.8f) * (fresnel * 1.5f + rim * 0.5f);
    const Vec3 data = color * (1.f + fresnel * 2.f);
    Vec3 rgb = mix3(shell, data, isData);
    float alpha = (0.1f + 0.7f * fresnel) + isData * ((0.4f + 0.5f * fresnel) - (0.1f + 0.7f * fresnel));
    if (u.is_selected > 0.5f) {
        const Vec3 white(1.f, 1.1f, 1.2f);
        rgb += white * 0.2f + white * std::pow(1.f - nDotV, 3.f);
        alpha = std::clamp(alpha + 0.1f + std::pow(1.f - nDotV, 3.f) * 0.5f, 0.f, 1.f);
    }
    rgb *= alpha;
    return Vec4(rgb.x(), rgb.y(), rgb.z(), alpha);
}

/** anatomical.frag of mne_viz_qt: tissue colours with Blinn-Phong lighting. */
std::optional<Vec4> anatomical_fragment(const SoftwareVaryings& in, const BrainSurfaceUniforms& u)
{
    const Vec3 n = varying3(in, kNormal).normalized();
    const Vec3 v = varying3(in, kViewDir).normalized();
    const Vec3 l = Vec3(u.light_dir[0], u.light_dir[1], u.light_dir[2]).normalized();
    const Vec3 h = (l + v).normalized();
    const auto tissue = std::lround(u.tissue_type);
    const float curvature = in[kCurvature];
    const float selection = std::max(u.is_selected, in[kHighlight]);

    Vec3 base;
    float roughness = 0.5f;
    float specIntensity = 0.3f;
    if (tissue == 1) {
        base = mix3(Vec3(0.85f, 0.75f, 0.72f), Vec3(0.55f, 0.40f, 0.38f), std::clamp(curvature * 2.f + 0.5f, 0.f, 1.f));
        roughness = 0.7f;
        specIntensity = 0.15f;
    } else if (tissue == 2) {
        base = Vec3(0.92f, 0.78f, 0.68f) + Vec3(1.f, 0.4f, 0.3f) * std::max(n.dot(l) * 0.5f + 0.5f, 0.f) * 0.15f;
        roughness = 0.4f;
        specIntensity = 0.25f;
    } else if (tissue == 3) {
        base = Vec3(0.95f, 0.90f, 0.82f);
        roughness = 0.6f;
        specIntensity = 0.2f;
    } else if (tissue == 4) {
        base = Vec3(0.88f, 0.82f, 0.75f);
        roughness = 0.65f;
        specIntensity = 0.15f;
    } else {
        base = varying3(in, kColor);
    }
    const Vec3 annotation = varying3(in, kAnnotation);
    if (u.overlay_mode > 0.5f && u.overlay_mode < 1.5f) {
        if (annotation.squaredNorm() > 0.001f) {
            base = annotation;
        }
    } else if (u.overlay_mode > 2.5f) {
        base = varying3(in, kColor);
    }
    if (in[kHighlight] > 0.5f && ((u.overlay_mode > 0.5f && u.overlay_mode < 1.5f) || u.overlay_mode > 2.5f)) {
        base = Vec3(1.f, 0.784f, 0.235f);
    }

    const float ao = 0.7f + 0.3f * std::clamp(1.f - curvature * 0.5f, 0.f, 1.f);
    const float nDotH = std::max(n.dot(h), 0.f);
    const float specPower = 8.f + 56.f * (1.f - roughness);
    const float fill = std::max(n.dot(Vec3(0.2f, -0.6f, 0.3f).normalized()), 0.f) * 0.12f;
    Vec3 color = base * 0.3f * ao + base * std::max(n.dot(l), 0.f) * 0.6f + Vec3(0.95f, 0.95f, 1.f) * std::pow(nDotH, specPower) * specIntensity + base * fill;
    if (selection > 0.1f) {
        const float fresnel = std::pow(1.f - std::clamp(n.dot(v), 0.f, 1.f), 2.5f);
        color = mix3(color, Vec3(0.95f, 0.55f, 0.50f) * 0.9f, 0.25f * selection);
        color += Vec3(0.08f, 0.03f, 0.02f) * selection;
        color += Vec3(0.90f, 0.40f, 0.35f) * fresnel * 0.3f * selection;
        color += Vec3(0.95f, 0.90f, 0.90f) * std::pow(nDotH, 32.f) * 0.15f * selection;
    }
    color = color.cwiseMax(0.f).cwiseMin(1.f);
    return Vec4(color.x(), color.y(), color.z(), 1.f);
}

/** shownormals.vert / .frag of mne_viz_qt. */
SoftwareShader shownormals_shader()
{
    SoftwareShader shader;
    shader.varying_count = 3;
    shader.vertex = [](const SoftwareAttributes& in, const SoftwareBindings& bindings, SoftwareVaryings& out) {
        const auto u = bindings.uniforms<BrainSurfaceUniforms>(0);
        set_varying3(out, 0, in[1].head<3>().normalized());
        return Vec4(Eigen::Map<const Eigen::Matrix4f>(u.mvp) * Vec4(in[0].x(), in[0].y(), in[0].z(), 1.f));
    };
    shader.fragment = [](const SoftwareVaryings& in, const SoftwareFragment&, const SoftwareBindings&) -> std::optional<Vec4> {
        const Vec3 color = varying3(in, 0) * 0.5f + Vec3::Constant(0.5f);
        return Vec4(color.x(), color.y(), color.z(), 1.f);
    };
    return shader;
}

/** The vertex stage of dipole.vert and electrode.vert: the shape placed by the instance's model matrix. */
Vec4 instanced_vertex(const SoftwareAttributes& in, const SoftwareBindings& bindings, SoftwareVaryings& out)
{
    const auto u = bindings.uniforms<BrainInstanceUniforms>(0);
    Eigen::Matrix4f model;
    model << in[2], in[3], in[4], in[5];
    const Vec4 world = model * Vec4(in[0].x(), in[0].y(), in[0].z(), 1.f);
    set_varying3(out, 0, world.head<3>());
    set_varying3(out, 3, model.topLeftCorner<3, 3>() * in[1].head<3>());
    for (std::size_t i = 0; i < 4; ++i) {
        out[6 + i] = in[6][static_cast<Eigen::Index>(i)];
    }
    out[10] = in[7].x();
    return Vec4(Eigen::Map<const Eigen::Matrix4f>(u.mvp) * world);
}

/** dipole.frag of mne_viz_qt: translucent glyphs with a Fresnel glow. */
std::optional<Vec4> dipole_fragment(const SoftwareVaryings& in, const SoftwareFragment&, const SoftwareBindings& bindings)
{
    const auto u = bindings.uniforms<BrainInstanceUniforms>(0);
    const Vec3 n = varying3(in, 3).normalized();
    const Vec3 v = (Vec3(u.camera_pos[0], u.camera_pos[1], u.camera_pos[2]) - varying3(in, 0)).normalized();
    const float nDotV = std::abs(n.dot(v));
    const float fresnel = std::pow(1.f - nDotV, 2.5f);
    float alpha = std::clamp(0.4f + 0.6f * fresnel, 0.f, 1.f);
    Vec3 color = Vec3(in[6], in[7], in[8]) * (1.2f + fresnel * 0.8f);
    if (in[10] > 0.5f) {
        const float rim = std::pow(1.f - nDotV, 3.f);
        color += Vec3::Constant(0.15f + rim);
        alpha = std::clamp(alpha + 0.1f + rim * 0.5f, 0.f, 1.f);
    }
    return Vec4(color.x(), color.y(), color.z(), alpha);
}

/** electrode.frag of mne_viz_qt: opaque Blinn-Phong glyphs, gold when selected. */
std::optional<Vec4> electrode_fragment(const SoftwareVaryings& in, const SoftwareFragment&, const SoftwareBindings& bindings)
{
    const auto u = bindings.uniforms<BrainInstanceUniforms>(0);
    const Vec3 base = in[10] > 0.5f ? Vec3(1.f, 0.78f, 0.24f) : Vec3(in[6], in[7], in[8]);
    if (u.lighting_enabled < 0.5f) {
        return Vec4(base.x(), base.y(), base.z(), 1.f);
    }
    const Vec3 n = varying3(in, 3).normalized();
    const Vec3 v = (Vec3(u.camera_pos[0], u.camera_pos[1], u.camera_pos[2]) - varying3(in, 0)).normalized();
    const Vec3 l = Vec3(u.light_dir[0], u.light_dir[1], u.light_dir[2]).normalized();
    const float diffuse = std::abs(n.dot(l));
    const float specular = std::pow(std::max(n.dot((l + v).normalized()), 0.f), 48.f) * 0.35f;
    const Vec3 color = base * (0.3f + 0.7f * diffuse) + Vec3::Constant(specular);
    return Vec4(color.x(), color.y(), color.z(), 1.f);
}

SoftwareShader instanced_shader(std::optional<Vec4> (*fragment)(const SoftwareVaryings&, const SoftwareFragment&, const SoftwareBindings&))
{
    SoftwareShader shader;
    shader.varying_count = 11;
    shader.vertex = instanced_vertex;
    shader.fragment = fragment;
    return shader;
}

/** screen_overlay.vert / .frag of mne_viz_qt: a texture over the target. */
SoftwareShader screen_overlay_shader()
{
    SoftwareShader shader;
    shader.varying_count = 2;
    shader.vertex = [](const SoftwareAttributes& in, const SoftwareBindings&, SoftwareVaryings& out) {
        out[0] = in[1].x();
        out[1] = in[1].y();
        return Vec4(in[0].x(), in[0].y(), 0.f, 1.f);
    };
    shader.fragment = [](const SoftwareVaryings& in, const SoftwareFragment&, const SoftwareBindings& bindings) -> std::optional<Vec4> {
        return bindings.sample(0, Eigen::Vector2f(in[0], in[1]));
    };
    return shader;
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

void mne::viz::detail::register_builtin_software_shaders(SoftwareRenderer& renderer)
{
    renderer.set_shader("channeldata", channeldata_shader());
    renderer.set_shader("overlay", overlay_shader());
    renderer.set_shader("standard", surface_shader<standard_fragment>());
    renderer.set_shader("holographic", surface_shader<holographic_fragment>());
    renderer.set_shader("anatomical", surface_shader<anatomical_fragment>());
    renderer.set_shader("shownormals", shownormals_shader());
    renderer.set_shader("dipole", instanced_shader(dipole_fragment));
    renderer.set_shader("electrode", instanced_shader(electrode_fragment));
    renderer.set_shader("screen_overlay", screen_overlay_shader());
}
