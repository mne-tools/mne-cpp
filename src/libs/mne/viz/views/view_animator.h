//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     view_animator.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ViewAnimator animates the scroll position and zoom of a TraceView over explicit timestamps.
 */

#ifndef VIZ_VIEW_ANIMATOR_H
#define VIZ_VIEW_ANIMATOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "trace_view.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <optional>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * @brief Runs the scroll and zoom animations a @ref TraceView asks for, without timers.
 *
 * The host starts the animations from the view's animate_scroll and animate_zoom effects, stops the scroll
 * animation on stop_animations, and calls tick() with the current time for every frame (or from a timer)
 * while it returns true. Animations use the cubic ease-out curve f(t) = 1 - (1 - t)^3, whose initial speed is
 * three times the mean speed: an inertial glide of duration T over a distance v T / 3 starts at speed v.
 *
 * There is at most one scroll animation (a requested scroll or an inertial glide) and one zoom animation;
 * starting one replaces the running one of its kind.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT ViewAnimator
{
public:
    /** Progress curve. */
    enum class Easing
    {
        Linear,  /**< f(t) = t. */
        OutCubic /**< f(t) = 1 - (1 - t)^3, as QEasingCurve::OutCubic. */
    };

    /** One animated value. */
    struct Animation
    {
        double from = 0.0;                /**< Start value. */
        double to = 0.0;                  /**< End value. */
        std::int64_t start_ms = 0;        /**< Start time. */
        int duration_ms = 0;              /**< Duration; 0 jumps to the end on the first tick. */
        Easing easing = Easing::OutCubic; /**< Progress curve. */
        bool inertial = false;            /**< Whether this is a glide after a drag (scroll only). */

        /**
         * @param[in] now_ms   Time.
         *
         * @return The value at @p now_ms: @c from before the start, exactly @c to from the end on.
         */
        double value_at(std::int64_t now_ms) const;

        /**
         * @param[in] now_ms   Time.
         *
         * @return Whether the animation has reached its end at @p now_ms.
         */
        bool finished_at(std::int64_t now_ms) const;
    };

    /**
     * @param[in] easing   Curve.
     * @param[in] t        Progress, clamped to 0 .. 1.
     *
     * @return The eased progress.
     */
    static double ease(Easing easing, double t);

    //=========================================================================================================
    /**
     * Starts a scroll animation, replacing a running one.
     *
     * @param[in] animation   The scroll the view asked for.
     * @param[in] now_ms      Start time.
     */
    void start_scroll(const TraceView::ScrollAnimation& animation, std::int64_t now_ms);

    //=========================================================================================================
    /**
     * Starts a zoom animation, replacing a running one.
     *
     * @param[in] animation   The zoom the view asked for.
     * @param[in] now_ms      Start time.
     */
    void start_zoom(const TraceView::ZoomAnimation& animation, std::int64_t now_ms);

    /** Stops the scroll animation where it is. */
    void stop_scroll()
    {
        m_scroll.reset();
    }

    /** Stops the zoom animation where it is. */
    void stop_zoom()
    {
        m_zoom.reset();
    }

    /** @return The running scroll animation. */
    const std::optional<Animation>& scroll() const
    {
        return m_scroll;
    }

    /** @return The running zoom animation. */
    const std::optional<Animation>& zoom() const
    {
        return m_zoom;
    }

    /** @return Whether an animation runs. */
    bool running() const
    {
        return m_scroll.has_value() || m_zoom.has_value();
    }

    //=========================================================================================================
    /**
     * Sets the animated values of @p view for @p now_ms and ends the animations that reached their end; a
     * finished requested scroll (not a glide) is reported with TraceView::scroll_animation_finished().
     *
     * @param[in] now_ms   Time.
     * @param[in] view     The animated view.
     *
     * @return Whether an animation still runs, i.e. more frames are needed.
     */
    bool tick(std::int64_t now_ms, TraceView& view);

private:
    std::optional<Animation> m_scroll;
    std::optional<Animation> m_zoom;
};

} // namespace mne::viz

#endif // VIZ_VIEW_ANIMATOR_H
