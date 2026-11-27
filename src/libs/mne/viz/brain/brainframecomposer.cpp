//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainframecomposer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     May 2026
 * @brief    BrainFrameComposer class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainframecomposer.h"
#include "brainsurface.h"
#include "dipoleobject.h"
#include "electrodeobject.h"
#include "networkobject.h"
#include "polylineobject.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstddef>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

// Keys of the textured draws, apart from the counted keys of meshes
constexpr std::uint64_t kSliceKey = std::uint64_t{1} << 62;
constexpr std::uint64_t kVideoTextureKey = kSliceKey + 0x1000;
constexpr std::uint64_t kVideoDiskKey = kSliceKey + 0x2000;

constexpr float kVideoDecalOffset = 0.00045f;

void copy_matrix(const Eigen::Matrix4f& matrix, float* out)
{
    std::copy(matrix.data(), matrix.data() + 16, out);
}

void copy_vector(const Eigen::Vector3f& vector, float* out)
{
    std::copy(vector.data(), vector.data() + 3, out);
}

GpuStreamView stream_view(const GpuStream& stream)
{
    return {std::span(static_cast<const std::byte*>(stream.data), stream.data ? stream.bytes : 0), stream.generation};
}

BrainSurfaceStreams surface_streams(const BrainSurface& surface)
{
    BrainSurfaceStreams streams;
    streams.geometry = stream_view(surface.geometryStream());
    streams.colors = stream_view(surface.colorStream());
    streams.annotations = stream_view(surface.annotationStream());
    streams.indices = stream_view(surface.indexStream());
    streams.index_count = surface.indexCount();
    return streams;
}

BrainInstancedStreams instanced_streams(const InstancedStreams& source)
{
    BrainInstancedStreams streams;
    streams.vertices = stream_view(source.vertices);
    streams.instances = stream_view(source.instances);
    streams.indices = stream_view(source.indices);
    streams.index_count = static_cast<std::uint32_t>(std::max(source.indexCount, 0));
    streams.instance_count = static_cast<std::uint32_t>(std::max(source.instanceCount, 0));
    return streams;
}

float aspect_of(const BrainTextureView& view)
{
    return (view.image && view.image->height > 0) ? static_cast<float>(view.image->width) / static_cast<float>(view.image->height) : 1.f;
}

// Camera, light and the projection of a pane, shared by its draws
struct PaneFrame
{
    Eigen::Matrix4f mvp;
    Eigen::Vector3f eye;
    Eigen::Vector3f light;
    bool lighting;
};

BrainSurfaceUniforms surface_uniforms(const PaneFrame& frame, const BrainSurface& surface, float overlayMode)
{
    BrainSurfaceUniforms uniforms;
    copy_matrix(frame.mvp, uniforms.mvp);
    copy_vector(frame.eye, uniforms.camera_pos);
    copy_vector(frame.light, uniforms.light_dir);
    // A highlighted region or vertex range replaces the whole-surface glow
    const std::optional<std::uint32_t> label = surface.selectedLabel();
    uniforms.is_selected = (surface.isSelected() && !label) ? 1.f : 0.f;
    uniforms.tissue_type = static_cast<float>(surface.tissueType());
    uniforms.lighting_enabled = frame.lighting ? 1.f : 0.f;
    uniforms.overlay_mode = overlayMode;
    uniforms.label_active = label ? 1.f : 0.f;
    const std::uint32_t key = label.value_or(0u);
    for (int c = 0; c < 4; ++c) {
        uniforms.selected_label[c] = static_cast<float>((key >> (8 * c)) & 0xFFu) / 255.f;
    }
    return uniforms;
}

BrainInstanceUniforms instance_uniforms(const PaneFrame& frame)
{
    BrainInstanceUniforms uniforms;
    copy_matrix(frame.mvp, uniforms.mvp);
    copy_vector(frame.eye, uniforms.camera_pos);
    copy_vector(frame.light, uniforms.light_dir);
    uniforms.lighting_enabled = frame.lighting ? 1.f : 0.f;
    return uniforms;
}

// The video disk faces the camera; its long edge is perpendicular to the up hint (the optical axis) when known
std::array<float, 20> video_disk_quad(const BrainVideoLayer& video, const Eigen::Vector3f& eye)
{
    const float aspect = aspect_of(video.frame);
    const float halfWidth = 0.5f * video.size;
    const float halfHeight = aspect > 0.f ? halfWidth / aspect : halfWidth;

    Eigen::Vector3f viewDir = video.focus - eye;
    viewDir = viewDir.squaredNorm() < 1e-12f ? Eigen::Vector3f(0.f, 0.f, -1.f) : viewDir.normalized();
    Eigen::Vector3f up;
    if (video.up_hint.squaredNorm() > 1e-8f) {
        up = video.up_hint - video.up_hint.dot(viewDir) * viewDir;
        up = up.squaredNorm() < 1e-8f ? Eigen::Vector3f(0.f, 0.f, 1.f) : up.normalized();
    } else {
        Eigen::Vector3f worldUp(0.f, 0.f, 1.f);
        if (std::abs(viewDir.dot(worldUp)) > 0.95f) {
            worldUp = Eigen::Vector3f(0.f, 1.f, 0.f);
        }
        up = viewDir.cross(worldUp).cross(viewDir).normalized();
    }
    const Eigen::Vector3f right = viewDir.cross(up).normalized();

    // Image rows run downwards, texture v upwards
    const std::array<Eigen::Vector3f, 4> corners = {video.focus - right * halfWidth - up * halfHeight, video.focus + right * halfWidth - up * halfHeight,
                                                    video.focus - right * halfWidth + up * halfHeight, video.focus + right * halfWidth + up * halfHeight};
    const std::array<float, 8> uv = {0.f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f};
    std::array<float, 20> quad{};
    for (std::size_t c = 0; c < corners.size(); ++c) {
        copy_vector(corners[c], quad.data() + 5 * c);
        quad[5 * c + 3] = uv[2 * c];
        quad[5 * c + 4] = uv[2 * c + 1];
    }
    return quad;
}

// The decal frame is tangent to the head at the focus; the up hint keeps it locked to the microscope
BrainVideoDecalUniforms video_decal_uniforms(const BrainVideoLayer& video, const PaneFrame& frame, bool relief)
{
    Eigen::Vector3f normal = video.focus;
    normal = normal.squaredNorm() < 1e-12f ? Eigen::Vector3f(0.f, 0.f, 1.f) : normal.normalized();
    Eigen::Vector3f up = video.up_hint;
    if (up.squaredNorm() < 1e-6f) {
        up = std::abs(normal.z()) > 0.95f ? Eigen::Vector3f(1.f, 0.f, 0.f) : Eigen::Vector3f(0.f, 0.f, 1.f);
    }
    up -= up.dot(normal) * normal;
    const Eigen::Vector3f axisV = up.squaredNorm() < 1e-12f ? Eigen::Vector3f(0.f, 0.f, 1.f) : up.normalized();
    const Eigen::Vector3f axisU = axisV.cross(normal).normalized();

    BrainVideoDecalUniforms uniforms;
    copy_matrix(frame.mvp, uniforms.mvp);
    copy_vector(video.focus, uniforms.focus_and_size);
    uniforms.focus_and_size[3] = video.size;
    copy_vector(axisU, uniforms.axis_u_and_opacity);
    uniforms.axis_u_and_opacity[3] = video.opacity;
    copy_vector(axisV, uniforms.axis_v_and_offset);
    uniforms.axis_v_and_offset[3] = kVideoDecalOffset;
    copy_vector(normal, uniforms.axis_n_and_depth);
    uniforms.axis_n_and_depth[3] = std::max(0.015f, video.size * 0.45f);
    copy_vector(frame.eye, uniforms.camera_pos_and_aspect);
    uniforms.camera_pos_and_aspect[3] = aspect_of(video.frame);
    uniforms.depth_params[0] = relief ? video.depth_scale : 0.f;
    uniforms.depth_params[1] = relief ? static_cast<float>(video.depth_steps) : 0.f;
    uniforms.depth_params[2] = relief ? 1.f : 0.f;
    return uniforms;
}

// The head surface carries the video decal: the BEM head, or else the first skin surface
const BrainSurface* video_target(const std::map<std::string, std::shared_ptr<BrainSurface>>& surfaces)
{
    if (const auto head = surfaces.find("bem_head"); head != surfaces.end() && head->second) {
        return head->second.get();
    }
    for (const auto& [key, surface] : surfaces) {
        if (surface && surface->tissueType() == BrainSurface::TissueSkin) {
            return surface.get();
        }
    }
    return nullptr;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void BrainFrameComposer::begin(const GpuFrameTarget& target, Color clear)
{
    // Renderables destroyed since the last frame release their buffers
    for (auto it = m_keys.begin(); it != m_keys.end();) {
        if (it->second.owner.expired()) {
            m_builder.release(it->second.id);
            it = m_keys.erase(it);
        } else {
            ++it;
        }
    }
    m_clipCorrection = target.clip_correction;
    m_builder.begin(target, clear);
}

//=============================================================================================================

std::vector<std::vector<std::string>> BrainFrameComposer::draw(const BrainScene& scene, std::span<const BrainScenePane> panes)
{
    std::vector<std::vector<std::string>> drawn(panes.size());
    for (std::size_t p = 0; p < panes.size(); ++p) {
        if (panes[p].view) {
            draw_pane(scene, panes[p], p, drawn[p]);
        }
    }
    return drawn;
}

//=============================================================================================================

GpuFrame BrainFrameComposer::finish(std::optional<std::variant<Image, DisplayList>> overlay, double scale)
{
    if (overlay) {
        m_builder.draw_overlay(std::move(*overlay), scale);
    }
    return m_builder.finish();
}

//=============================================================================================================

std::uint64_t BrainFrameComposer::key_of(const RenderKey& key)
{
    Key& entry = m_keys[key.id()];
    // An address may be reused by a new renderable once the old one is gone
    if (entry.id == 0 || entry.owner.expired()) {
        if (entry.id != 0) {
            m_builder.release(entry.id);
        }
        entry.owner = key.token();
        entry.id = m_nextKey++;
    }
    return entry.id;
}

//=============================================================================================================

void BrainFrameComposer::draw_pane(const BrainScene& scene, const BrainScenePane& pane, std::size_t index, std::vector<std::string>& drawn)
{
    const SubView& view = *pane.view;
    const GpuRect& viewport = pane.viewport;
    PaneFrame frame;
    const Eigen::Matrix4f pvm = pane.camera.pvm();
    frame.mvp = m_clipCorrection * pvm;
    frame.eye = pane.camera.eye;
    frame.light = pane.camera.eye.normalized();
    frame.lighting = scene.lighting;

    static const std::map<std::string, std::shared_ptr<BrainSurface>> noSurfaces;
    const auto& surfaces = scene.surfaces ? *scene.surfaces : noSurfaces;

    const auto drawSurface = [this, &frame, &viewport](const BrainSurface& surface, ShaderMode shader, float overlayMode) {
        if (surface.isVisible()) {
            m_builder.draw_surface(key_of(surface.renderKey()), surface_streams(surface), static_cast<BrainShader>(shader),
                                   surface_uniforms(frame, surface, overlayMode), viewport);
        }
    };
    const auto drawInstanced = [this, &frame, &viewport](const RenderKey& key, const InstancedStreams& streams, BrainShader shader) {
        m_builder.draw_instanced(key_of(key), instanced_streams(streams), shader, instance_uniforms(frame), viewport);
    };

    // Brain surfaces of the pane's surface type, with the pane's overlay
    const float paneOverlay = static_cast<float>(view.overlayMode);
    for (const auto& [key, surface] : surfaces) {
        if (surface && view.matchesSurfaceType(key) && view.shouldRenderSurface(key)) {
            drawn.push_back(key);
            drawSurface(*surface, view.brainShader, paneOverlay);
        }
    }

    // The other surfaces: source spaces and digitizers are opaque except for the tracked markers; BEM and sensors
    // are translucent
    struct Translucent
    {
        const BrainSurface* surface;
        ShaderMode shader;
        float overlayMode;
        float distance;
    };
    std::vector<Translucent> translucent;
    const auto distanceTo = [&frame](const BrainSurface& surface) {
        Eigen::Vector3f low;
        Eigen::Vector3f high;
        surface.boundingBox(low, high);
        return (frame.eye - (low + high) * 0.5f).squaredNorm();
    };
    const float scientific = static_cast<float>(ModeScientific);
    for (const auto& [key, surface] : surfaces) {
        if (!surface || SubView::isBrainSurfaceKey(key) || !view.shouldRenderSurface(key) || !surface->isVisible()) {
            continue;
        }
        if (key.starts_with("srcsp_") || key.starts_with("dig_")) {
            if (key.starts_with("dig_live_t_") || key.starts_with("dig_ray_") || key.starts_with("dig_probe_")) {
                translucent.push_back({surface.get(), Holographic, scientific, distanceTo(*surface)});
            } else {
                drawSurface(*surface, view.brainShader, scientific);
            }
        } else if (key.starts_with("sens_") || key.starts_with("bem_")) {
            // A field map surface shows plain colours while its field map is hidden
            float overlay = scientific;
            if ((key == scene.meg_field_key && !view.visibility.megFieldMap) || (key == scene.eeg_field_key && !view.visibility.eegFieldMap)) {
                overlay = static_cast<float>(ModeSurface);
            }
            translucent.push_back({surface.get(), key.starts_with("bem_") ? view.bemShader : Holographic, overlay, distanceTo(*surface)});
        }
    }

    if (scene.electrodes && scene.electrodes->isVisible()) {
        drawInstanced(scene.electrodes->bodyRenderKey(), scene.electrodes->bodyStreams(), BrainShader::Electrode);
        drawInstanced(scene.electrodes->contactRenderKey(), scene.electrodes->contactStreams(), BrainShader::Electrode);
    }

    // Slices go after the opaque surfaces and before the translucent ones, which add their glow on top
    if (view.visibility.mriSlices) {
        for (std::size_t s = 0; s < scene.slices.size(); ++s) {
            const BrainSliceLayer& slice = scene.slices[s];
            BrainSliceUniforms uniforms;
            copy_matrix(frame.mvp, uniforms.mvp);
            uniforms.opacity = slice.opacity;
            uniforms.window_center = slice.window_center;
            uniforms.window_width = slice.window_width;
            m_builder.draw_slice(kSliceKey + s, slice.image, slice.quad, uniforms, viewport);
        }
    }

    // Translucent surfaces back to front; the video decal right after the head it lies on
    std::stable_sort(translucent.begin(), translucent.end(), [](const Translucent& a, const Translucent& b) { return a.distance > b.distance; });
    const BrainSurface* videoTarget = scene.video ? video_target(surfaces) : nullptr;
    bool videoDrawn = !scene.video;
    const auto drawVideo = [&]() {
        const BrainVideoLayer& video = *scene.video;
        if (videoTarget) {
            const bool relief = video.depth.image != nullptr;
            m_builder.draw_video_decal(key_of(videoTarget->renderKey()), surface_streams(*videoTarget), kVideoTextureKey, video.frame,
                                       relief ? video.depth : BrainTextureView{}, video_decal_uniforms(video, frame, relief), viewport);
        } else {
            BrainVideoBillboardUniforms uniforms;
            copy_matrix(frame.mvp, uniforms.mvp);
            uniforms.opacity = video.opacity;
            m_builder.draw_video_billboard(kVideoDiskKey + index, kVideoTextureKey, video.frame, video_disk_quad(video, frame.eye), uniforms, viewport);
        }
        videoDrawn = true;
    };
    for (const Translucent& item : translucent) {
        drawSurface(*item.surface, item.shader, item.overlayMode);
        if (!videoDrawn && item.surface == videoTarget) {
            drawVideo();
        }
    }
    if (!videoDrawn) {
        drawVideo();
    }

    if (view.visibility.dipoles) {
        for (const DipoleObject* dipoles : scene.dipoles) {
            if (dipoles && dipoles->isVisible()) {
                drawInstanced(dipoles->renderKey(), dipoles->streams(), BrainShader::Dipole);
            }
        }
    }
    if (view.visibility.network && scene.network && scene.network->isVisible() && scene.network->hasData()) {
        drawInstanced(scene.network->nodeRenderKey(), scene.network->nodeStreams(), BrainShader::Dipole);
        drawInstanced(scene.network->edgeRenderKey(), scene.network->edgeStreams(), BrainShader::Dipole);
    }
    // The head movement path is not connectivity data, so it does not follow the network visibility
    if (scene.head_path && scene.head_path->isVisible() && scene.head_path->hasData()) {
        drawInstanced(scene.head_path->renderKey(), scene.head_path->streams(), BrainShader::Dipole);
    }

    if (scene.pointer && scene.pointer->isVisible()) {
        PaneFrame pointerFrame = frame;
        pointerFrame.mvp = m_clipCorrection * pvm * translation_matrix(scene.pointer_position);
        m_builder.draw_surface(key_of(scene.pointer->renderKey()), surface_streams(*scene.pointer), BrainShader::Holographic,
                               surface_uniforms(pointerFrame, *scene.pointer, 0.f), viewport);
    }
}
