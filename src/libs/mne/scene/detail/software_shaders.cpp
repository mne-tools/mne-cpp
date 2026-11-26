//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     software_shaders.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Software implementations of the shaders of the views in mne_scene.
 *
 * Each function mirrors the GLSL shader of the same name in mne_disp or mne_disp3D; keep them in step.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "software_shaders.h"
#include "../software_renderer.h"
#include "../trace_frame.h"
#include "../trace_geometry.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

//=============================================================================================================
// DEFINE STATIC HELPERS
//=============================================================================================================

namespace
{

/** channeldata.vert / channeldata.frag of mne_disp: one trace as a line strip. */
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

/** overlay.vert / overlay.frag of mne_disp: a texture over the screen with the per-second bands. */
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

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

void mne::scene::detail::register_builtin_software_shaders(SoftwareRenderer& renderer)
{
    renderer.set_shader("channeldata", channeldata_shader());
    renderer.set_shader("overlay", overlay_shader());
}
