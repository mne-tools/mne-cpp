//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     view_animator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of ViewAnimator.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "view_animator.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

double ViewAnimator::ease(Easing easing, double t)
{
    t = std::clamp(t, 0.0, 1.0);
    if (easing == Easing::Linear) {
        return t;
    }
    const double r = 1.0 - t;
    return 1.0 - r * r * r;
}

//=============================================================================================================

double ViewAnimator::Animation::value_at(std::int64_t now_ms) const
{
    if (finished_at(now_ms)) {
        return to;
    }
    const double t = static_cast<double>(now_ms - start_ms) / duration_ms;
    return from + (to - from) * ease(easing, t);
}

//=============================================================================================================

bool ViewAnimator::Animation::finished_at(std::int64_t now_ms) const
{
    return now_ms - start_ms >= duration_ms;
}

//=============================================================================================================

void ViewAnimator::start_scroll(const TraceView::ScrollAnimation& animation, std::int64_t now_ms)
{
    m_scroll = Animation{animation.from, animation.to, now_ms, std::max(animation.duration_ms, 0), Easing::OutCubic, animation.inertial};
}

//=============================================================================================================

void ViewAnimator::start_zoom(const TraceView::ZoomAnimation& animation, std::int64_t now_ms)
{
    m_zoom = Animation{animation.from, animation.to, now_ms, std::max(animation.duration_ms, 0), Easing::OutCubic, false};
}

//=============================================================================================================

bool ViewAnimator::tick(std::int64_t now_ms, TraceView& view)
{
    if (m_zoom) {
        const Animation zoom = *m_zoom;
        if (zoom.finished_at(now_ms)) {
            m_zoom.reset();
        }
        view.set_samples_per_pixel(zoom.value_at(now_ms));
    }
    if (m_scroll) {
        const Animation scroll = *m_scroll;
        const bool finished = scroll.finished_at(now_ms);
        if (finished) {
            m_scroll.reset();
        }
        // Setting the value may report effects that start a new animation; it then stays
        view.set_scroll_sample(scroll.value_at(now_ms));
        if (finished && !scroll.inertial && !m_scroll) {
            view.scroll_animation_finished();
        }
    }
    return running();
}
