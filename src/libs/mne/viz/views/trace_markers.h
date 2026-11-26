//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_markers.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TraceMarkers draws the annotation spans, event lines and epoch markers of a raw-data trace view.
 */

#ifndef VIZ_TRACE_MARKERS_H
#define VIZ_TRACE_MARKERS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../figures/canvas.h"
#include "../viz_global.h"
#include "../figures/style.h"
#include "trace_view.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class TextMetrics;

/**
 * @brief Event and epoch markers of a trace view, and the drawing of all time markers.
 *
 * Draws, on a time axis given by a first sample and a scale, the annotation spans (a translucent band with
 * border lines and a label pill), the event lines and the dashed epoch trigger lines. The GPU trace view
 * paints this into a texture wider than the view; the image export draws it between the grid and the traces
 * (annotations) and on top of the traces (events and epochs).
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT TraceMarkers
{
public:
    /** A stimulus event: a coloured vertical line at a sample. */
    struct Event
    {
        int sample = 0;    /**< Absolute sample of the onset. */
        int type = 1;      /**< Event code. */
        Color color;       /**< Line colour. */
        std::string label; /**< Short label. */

        friend bool operator==(const Event&, const Event&) = default;
    };

    //=========================================================================================================
    /**
     * Sets the events.
     *
     * @param[in] events   Events.
     */
    void set_events(std::vector<Event> events);

    /** @return The events. */
    const std::vector<Event>& events() const
    {
        return m_events;
    }

    //=========================================================================================================
    /**
     * Sets the epoch trigger samples, drawn as dashed lines.
     *
     * @param[in] samples   Absolute samples.
     */
    void set_epoch_markers(std::vector<int> samples);

    /** @return The epoch trigger samples. */
    const std::vector<int>& epoch_markers() const
    {
        return m_epochMarkers;
    }

    //=========================================================================================================
    /**
     * Shows or hides the event lines.
     *
     * @param[in] visible   Visibility.
     *
     * @return Whether the setting changed.
     */
    bool set_events_visible(bool visible);

    /** @return Whether the event lines are drawn. */
    bool events_visible() const
    {
        return m_eventsVisible;
    }

    //=========================================================================================================
    /**
     * Shows or hides the annotation spans.
     *
     * @param[in] visible   Visibility.
     *
     * @return Whether the setting changed.
     */
    bool set_annotations_visible(bool visible);

    /** @return Whether the annotation spans are drawn. */
    bool annotations_visible() const
    {
        return m_annotationsVisible;
    }

    //=========================================================================================================
    /**
     * Shows or hides the epoch markers.
     *
     * @param[in] visible   Visibility.
     *
     * @return Whether the setting changed.
     */
    bool set_epoch_markers_visible(bool visible);

    /** @return Whether the epoch markers are drawn. */
    bool epoch_markers_visible() const
    {
        return m_epochMarkersVisible;
    }

    //=========================================================================================================
    /**
     * Draws the visible annotation spans into (0, 0, width, height) of @p canvas; sample s lies at
     * x = (s - first_sample) * pixels_per_sample.
     *
     * @param[in] canvas              Canvas.
     * @param[in] metrics             Measures the labels.
     * @param[in] font                Base font of the labels (drawn bold at 8 points).
     * @param[in] annotations         Annotation spans.
     * @param[in] first_sample        Sample at x = 0.
     * @param[in] pixels_per_sample   Horizontal scale.
     */
    void draw_annotations(Canvas& canvas, const TextMetrics& metrics, const Font& font,
                          std::span<const TraceView::Annotation> annotations, double first_sample,
                          double pixels_per_sample) const;

    //=========================================================================================================
    /**
     * Draws the visible event lines and epoch markers, mapped like draw_annotations().
     *
     * @param[in] canvas              Canvas.
     * @param[in] first_sample        Sample at x = 0.
     * @param[in] pixels_per_sample   Horizontal scale.
     */
    void draw_lines(Canvas& canvas, double first_sample, double pixels_per_sample) const;

    //=========================================================================================================
    /**
     * Draws annotations, then event lines and epoch markers.
     *
     * @param[in] canvas              Canvas.
     * @param[in] metrics             Measures the labels.
     * @param[in] font                Base font of the labels.
     * @param[in] annotations         Annotation spans.
     * @param[in] first_sample        Sample at x = 0.
     * @param[in] pixels_per_sample   Horizontal scale.
     */
    void draw(Canvas& canvas, const TextMetrics& metrics, const Font& font,
              std::span<const TraceView::Annotation> annotations, double first_sample, double pixels_per_sample) const;

private:
    std::vector<Event> m_events;
    std::vector<int> m_epochMarkers;
    bool m_eventsVisible = true;
    bool m_annotationsVisible = true;
    bool m_epochMarkersVisible = true;
};

} // namespace mne::viz

#endif // VIZ_TRACE_MARKERS_H
