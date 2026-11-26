//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_frame.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TraceFrameBuilder describes a frame of the trace view as a GpuFrame.
 */

#ifndef SCENE_TRACE_FRAME_H
#define SCENE_TRACE_FRAME_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gpu_frame.h"
#include "scene_global.h"
#include "trace_geometry.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::scene
//=============================================================================================================

namespace mne::scene
{

class TextMetrics;
class TraceMarkers;

/**
 * @brief The uniform block of overlay.frag (binding 2): maps the screen into the overlay texture and draws the
 * per-second bands.
 *
 * @since 3.0.0
 */
struct TraceOverlayUniforms
{
    float scroll_sample = 0.f;         /**< Offset 0: sample at the left edge. */
    float samples_per_pixel = 1.f;     /**< Offset 4: horizontal scale. */
    float view_width = 0.f;            /**< Offset 8: view width in logical pixels. */
    float sfreq = 0.f;                 /**< Offset 12: sampling frequency; 0 draws no bands. */
    float first_file_sample = 0.f;     /**< Offset 16: first sample of the file, where the first second starts. */
    float grid_enabled = 0.f;          /**< Offset 20: 1 to darken every other second. */
    float overlay_first_sample = 0.f;  /**< Offset 24: sample at the left edge of the texture. */
    float overlay_total_samples = 0.f; /**< Offset 28: samples the texture spans. */
};

static_assert(sizeof(TraceOverlayUniforms) == 32, "TraceOverlayUniforms must match the uniform block of overlay.frag");

/**
 * @brief Describes the frames of a trace view as GpuFrame for any backend.
 *
 * One pass clears to the background and draws one line strip per trace (shader "channeldata"), then the
 * annotations, events and epoch markers with the per-second bands (shader "overlay" over a texture that spans
 * overlay_prefetch() view widths on each side, so scrolling does not redraw it), then the crosshair, scalebars,
 * ruler and annotation selection (shader "overlay" over a screen-sized texture). The builder keeps the vertex
 * buffers, textures and pipelines in resources() and streams vertices through its TraceGeometry.
 *
 * @since 3.0.0
 */
class SCENESHARED_EXPORT TraceFrameBuilder
{
public:
    //=========================================================================================================
    /**
     * Creates a builder with its pipelines and the vertex buffer of the overlay quad.
     */
    TraceFrameBuilder();

    /** @return The trace vertices; invalidate() them after a zoom or a data change. */
    TraceGeometry& geometry()
    {
        return m_geometry;
    }

    /** @return The trace vertices. */
    const TraceGeometry& geometry() const
    {
        return m_geometry;
    }

    //=========================================================================================================
    /**
     * Marks the annotation, event and epoch overlay as outdated, e.g. after the annotations changed.
     */
    void invalidate_overlay()
    {
        m_overlayDirty = true;
    }

    /** @return How many view widths the overlay texture spans on each side of the view. */
    static constexpr double overlay_prefetch()
    {
        return 1.0;
    }

    //=========================================================================================================
    /**
     * Describes the next frame; streams vertices and redraws the textures as needed.
     *
     * @param[in] view      Viewport, overlays and annotations.
     * @param[in] source    The channels.
     * @param[in] style     Colours and scales; the background is the clear colour, grid the per-second bands.
     * @param[in] markers   Events and epoch markers.
     * @param[in] metrics   Measures the text of the overlays.
     * @param[in] target    Size and pixel ratio of the target.
     *
     * @return The frame, referring to resources().
     */
    GpuFrame build(const TraceView& view, const TraceSource& source, const TraceStyle& style, const TraceMarkers& markers,
                   const TextMetrics& metrics, const GpuFrameTarget& target);

    /** @return The texture of the crosshair, scalebars, ruler and annotation selection; 0 until first drawn. */
    GpuHandle hud_texture() const
    {
        return m_hudTexture;
    }

    /** @return The buffers, textures and pipelines of the frames. */
    const GpuResources& resources() const
    {
        return m_resources;
    }

private:
    void upload_vertices();
    GpuDraw overlay_draw(GpuHandle texture, const TraceOverlayUniforms& uniforms) const;

    GpuResources m_resources;
    TraceGeometry m_geometry;
    GpuHandle m_tracePipeline = 0;
    GpuHandle m_overlayPipeline = 0;
    GpuHandle m_quad = 0;
    GpuHandle m_overlayTexture = 0;
    GpuHandle m_hudTexture = 0;
    std::vector<GpuHandle> m_channelBuffers; /**< Vertex buffer by logical channel. */
    bool m_overlayDirty = true;
    double m_overlayFirstSample = 0.0;
    double m_overlayTotalSamples = 0.0;
    int m_overlayWidth = 0;
    int m_overlayHeight = 0;
    double m_overlayScale = 0.0;
    double m_overlaySpp = 0.0;
};

} // namespace mne::scene

#endif // SCENE_TRACE_FRAME_H
