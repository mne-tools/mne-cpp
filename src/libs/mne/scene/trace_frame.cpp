//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_frame.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TraceFrameBuilder definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "trace_frame.h"
#include "canvas.h"
#include "trace_markers.h"
#include "trace_view.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
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

// Full-screen quad as a triangle strip: (x, y) in clip space, (u, v) with v = 0 at the top of the texture
constexpr std::array<float, 16> kQuad = {-1.f, -1.f, 0.f, 1.f, -1.f, 1.f, 0.f, 0.f, 1.f, -1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 0.f};

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TraceFrameBuilder::TraceFrameBuilder()
{
    GpuPipeline traces;
    traces.shader = "channeldata";
    traces.bindings = {{2 * sizeof(float), false}};
    traces.attributes = {{0, 0, GpuVertexFormat::Float2, 0}};
    traces.topology = GpuTopology::LineStrip;
    traces.blend = GpuBlend::Alpha;
    m_tracePipeline = m_resources.create_pipeline(std::move(traces));

    GpuPipeline overlay;
    overlay.shader = "overlay";
    overlay.bindings = {{4 * sizeof(float), false}};
    overlay.attributes = {{0, 0, GpuVertexFormat::Float2, 0}, {1, 0, GpuVertexFormat::Float2, 2 * sizeof(float)}};
    overlay.topology = GpuTopology::TriangleStrip;
    overlay.blend = GpuBlend::Alpha;
    m_overlayPipeline = m_resources.create_pipeline(std::move(overlay));

    m_quad = m_resources.create_buffer(GpuBufferUsage::Vertex);
    m_resources.set_buffer_values(m_quad, std::span<const float>(kQuad));

    m_overlayTexture = m_resources.create_texture(GpuTexture{Image::transparent(1, 1)});
}

//=============================================================================================================

void TraceFrameBuilder::upload_vertices()
{
    const std::vector<TraceVertices>& channels = m_geometry.channels();
    while (m_channelBuffers.size() < channels.size()) {
        m_channelBuffers.push_back(m_resources.create_buffer(GpuBufferUsage::Vertex));
    }
    for (std::size_t channel = 0; channel < channels.size(); ++channel) {
        m_resources.set_buffer_values(m_channelBuffers[channel], std::span<const float>(channels[channel].xy));
    }
    for (std::size_t channel = channels.size(); channel < m_channelBuffers.size(); ++channel) {
        m_resources.set_buffer_data(m_channelBuffers[channel], {});
    }
}

//=============================================================================================================

GpuDraw TraceFrameBuilder::overlay_draw(GpuHandle texture, const TraceOverlayUniforms& uniforms) const
{
    GpuDraw draw;
    draw.pipeline = m_overlayPipeline;
    draw.vertex_buffers = {m_quad};
    draw.count = 4;
    draw.set_uniforms(2, uniforms);
    draw.textures = {{1, texture}};
    return draw;
}

//=============================================================================================================

GpuFrame TraceFrameBuilder::build(const TraceView& view, const TraceSource& source, const TraceStyle& style, const TraceMarkers& markers,
                                  const TextMetrics& metrics, const GpuFrameTarget& target)
{
    GpuFrame frame;
    frame.pixel_width = target.pixel_width;
    frame.pixel_height = target.pixel_height;
    frame.device_pixel_ratio = target.device_pixel_ratio;
    GpuPass& pass = frame.passes.emplace_back();
    pass.clear_color = style.background;
    pass.clear_depth = false;

    const int width = view.view_width();
    const int height = view.view_height();
    const double spp = view.samples_per_pixel();
    const double scale = target.device_pixel_ratio > 0.0 ? target.device_pixel_ratio : 1.0;
    if (target.pixel_width <= 0 || target.pixel_height <= 0 || width <= 0 || height <= 0) {
        return frame;
    }

    // Traces
    const bool hasTraces = source.channel_count() > 0 && spp > 0.0;
    if (hasTraces) {
        if (m_geometry.needs_stream(view) && m_geometry.stream(view, source, style.z_score)) {
            upload_vertices();
        }
        for (const TraceDraw& trace : m_geometry.draws(view, source, style)) {
            const auto channel = static_cast<std::size_t>(trace.channel);
            if (channel >= m_channelBuffers.size() || channel >= m_geometry.channels().size() || m_geometry.channels()[channel].vertex_count() < 2) {
                continue;
            }
            GpuDraw& draw = pass.draws.emplace_back();
            draw.pipeline = m_tracePipeline;
            draw.vertex_buffers = {m_channelBuffers[channel]};
            draw.count = static_cast<std::uint32_t>(m_geometry.channels()[channel].vertex_count());
            draw.set_uniforms(0, trace.uniforms);
        }
    }

    // Annotations, events and epoch markers over the prefetch window, with the per-second bands
    if (hasTraces && view.sfreq() > 0.0) {
        const double visible = width * spp;
        const double extra = overlay_prefetch() * visible;
        const int overlayWidth = static_cast<int>(std::ceil((1.0 + 2.0 * overlay_prefetch()) * width));
        const double scroll = view.scroll_sample();
        if (m_overlayDirty || overlayWidth != m_overlayWidth || height != m_overlayHeight || scale != m_overlayScale || spp != m_overlaySpp || scroll < m_overlayFirstSample || scroll + visible > m_overlayFirstSample + m_overlayTotalSamples) {
            m_overlayFirstSample = scroll - extra;
            m_overlayTotalSamples = visible + 2.0 * extra;
            m_overlayWidth = overlayWidth;
            m_overlayHeight = height;
            m_overlayScale = scale;
            m_overlaySpp = spp;
            m_overlayDirty = false;
            Canvas canvas(overlayWidth, height);
            markers.draw(canvas, metrics, Font{}, view.annotations(), m_overlayFirstSample, overlayWidth / m_overlayTotalSamples);
            m_resources.set_texture(m_overlayTexture, canvas.display_list(), scale);
        }
        TraceOverlayUniforms uniforms;
        uniforms.scroll_sample = static_cast<float>(scroll);
        uniforms.samples_per_pixel = static_cast<float>(spp);
        uniforms.view_width = static_cast<float>(width);
        uniforms.sfreq = static_cast<float>(view.sfreq());
        uniforms.first_file_sample = static_cast<float>(view.first_file_sample());
        uniforms.grid_enabled = style.grid ? 1.f : 0.f;
        uniforms.overlay_first_sample = static_cast<float>(m_overlayFirstSample);
        uniforms.overlay_total_samples = static_cast<float>(m_overlayTotalSamples);
        pass.draws.push_back(overlay_draw(m_overlayTexture, uniforms));
    }

    // Crosshair, scalebars, ruler and annotation selection, mapped 1:1 to the screen without bands
    if (view.overlays_visible()) {
        if (m_hudTexture == 0) {
            m_hudTexture = m_resources.create_texture(GpuTexture{view.render_overlays(metrics), scale});
        } else {
            m_resources.set_texture(m_hudTexture, view.render_overlays(metrics), scale);
        }
        TraceOverlayUniforms uniforms;
        uniforms.view_width = static_cast<float>(width);
        uniforms.overlay_total_samples = static_cast<float>(width);
        pass.draws.push_back(overlay_draw(m_hudTexture, uniforms));
    }
    return frame;
}
