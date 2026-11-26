//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_view_client.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TraceViewClient runs the raw-data trace view in any backend's window.
 *
 * The trace view needs more than TraceView itself: an animator for its scroll and zoom animations, markers, a
 * style, text metrics and a frame builder that turns it into GPU frames, all wired so that zooming invalidates
 * the vertex cache and animations ask the host for frames. TraceViewClient does that wiring once, as a
 * SurfaceClient, so a backend only forwards its window's input and executes the frames.
 */

#ifndef VIZ_TRACE_VIEW_CLIENT_H
#define VIZ_TRACE_VIEW_CLIENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../figures/text_metrics.h"
#include "../gpu/trace_frame.h"
#include "../views/trace_markers.h"
#include "../views/trace_view.h"
#include "../views/view_animator.h"
#include "../viz_global.h"
#include "surface.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class TraceSource;

//=============================================================================================================
/**
 * @brief The trace view with its animator, markers, style and frame builder, hosted through a Surface.
 *
 * @snippet ex_scene_sdl/main.cpp trace_view_client_usage
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT TraceViewClient : public SurfaceClient
{
public:
    //=========================================================================================================
    /**
     * Creates the client with the default style and approximate text metrics.
     *
     * @since 3.0.0
     */
    TraceViewClient();

    TraceViewClient(const TraceViewClient&) = delete;
    TraceViewClient& operator=(const TraceViewClient&) = delete;

    //=========================================================================================================
    /**
     * Sets the channels and samples to show.
     *
     * @param[in] source    The data; it outlives the client or is replaced first.
     *
     * @since 3.0.0
     */
    void set_source(const TraceSource* source);

    //=========================================================================================================
    /**
     * Tells the client that the samples of the source changed, e.g. new data streamed in.
     *
     * @since 3.0.0
     */
    void data_changed();

    //=========================================================================================================
    /**
     * Sets the callbacks of the application. The client handles animations, caches and redraws itself and
     * calls these afterwards, e.g. to update a scroll bar or a status line.
     *
     * @param[in] effects   Callbacks; the animation entries are not used.
     *
     * @since 3.0.0
     */
    void set_effects(TraceView::Effects effects);

    //=========================================================================================================
    /**
     * Sets the text metrics used to lay out labels.
     *
     * @param[in] metrics   Metrics of the host's font; nullptr restores the approximate metrics.
     *
     * @since 3.0.0
     */
    void set_text_metrics(const TextMetrics* metrics);

    //=========================================================================================================
    /**
     * @return The view: viewport, input handling and overlays.
     *
     * @since 3.0.0
     */
    TraceView& view()
    {
        return m_view;
    }

    //=========================================================================================================
    /**
     * @return Event and epoch markers drawn over the traces.
     *
     * @since 3.0.0
     */
    TraceMarkers& markers()
    {
        return m_markers;
    }

    //=========================================================================================================
    /**
     * @return Colours and line widths of the frames.
     *
     * @since 3.0.0
     */
    TraceStyle& style()
    {
        return m_style;
    }

    //=========================================================================================================
    /**
     * @return The frame builder with its vertex cache.
     *
     * @since 3.0.0
     */
    TraceFrameBuilder& builder()
    {
        return m_builder;
    }

    //=========================================================================================================
    /**
     * @return The scroll and zoom animations.
     *
     * @since 3.0.0
     */
    ViewAnimator& animator()
    {
        return m_animator;
    }

    void resize(const SurfaceSize& size) override;
    bool handle_pointer(const PointerEvent& event) override;
    bool handle_wheel(const WheelEvent& event) override;
    bool tick(std::int64_t now_ms) override;
    GpuFrame frame(const GpuFrameTarget& target) override;
    const GpuResources& resources() const override;

private:
    void install_effects();
    std::int64_t now() const;
    void update() const;

    TraceView m_view;
    ViewAnimator m_animator;
    TraceFrameBuilder m_builder;
    TraceMarkers m_markers;
    TraceStyle m_style;
    ApproximateTextMetrics m_approximateMetrics;
    const TextMetrics* m_metrics = &m_approximateMetrics;
    const TraceSource* m_source = nullptr;
    TraceView::Effects m_effects;
};

} // namespace mne::viz

#endif // VIZ_TRACE_VIEW_CLIENT_H
