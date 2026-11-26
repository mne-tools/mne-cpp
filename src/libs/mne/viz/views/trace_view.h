//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_view.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TraceView holds the viewport and interaction state of a raw-data trace view and draws its overlays.
 */

#ifndef VIZ_TRACE_VIEW_H
#define VIZ_TRACE_VIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../figures/canvas.h"
#include "input_event.h"
#include "../viz_global.h"
#include "../figures/style.h"
#include "trace_source.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class TextMetrics;

/**
 * @brief Viewport and interaction state of a raw-data trace view, without a GUI toolkit.
 *
 * TraceView knows which samples and channel rows a view of a given size shows, handles pointer and wheel
 * input (wheel zoom and scrolling, drag scrolling with inertia, the right-button ruler, annotation range
 * selection and boundary resizing, the crosshair) and draws the overlays on top of the traces (crosshair,
 * scalebars, ruler and annotation selection) into a @ref Canvas. It does not animate or render traces: what
 * the host has to do is reported through @ref Effects, e.g. animate a scroll, change the cursor or redraw.
 *
 * Positions are logical (device-independent) pixels with x = 0 at the left edge of the traces.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT TraceView
{
public:
    /** A time-span annotation. */
    struct Annotation
    {
        int start_sample = 0; /**< First covered absolute sample. */
        int end_sample = 0;   /**< Last covered absolute sample. */
        Color color;          /**< Fill and border colour. */
        std::string label;    /**< Label near the top edge. */

        friend bool operator==(const Annotation&, const Annotation&) = default;
    };

    /** A scroll the host should animate (cubic ease-out) by setting the scroll sample over time. */
    struct ScrollAnimation
    {
        double from = 0.0;     /**< Start position in samples. */
        double to = 0.0;       /**< End position in samples. */
        int duration_ms = 0;   /**< Duration in milliseconds. */
        bool inertial = false; /**< Whether this is the glide after a drag rather than a requested scroll. */
    };

    /** A zoom the host should animate (cubic ease-out) by setting the samples per pixel over time. */
    struct ZoomAnimation
    {
        double from = 0.0;   /**< Start samples per pixel. */
        double to = 0.0;     /**< End samples per pixel. */
        int duration_ms = 0; /**< Duration in milliseconds. */
    };

    /** What lies under the crosshair. */
    struct CursorData
    {
        float time_sec = 0.f;     /**< Time relative to the first file sample in seconds. */
        float amplitude = 0.f;    /**< Raw value under the cursor (0 in butterfly mode). */
        std::string channel_name; /**< Channel name, or the channel type in butterfly mode. */
        std::string unit_label;   /**< Short unit, e.g. "T", "V", "AU". */
    };

    /** How the ruler is locked to an axis. */
    enum class RulerSnap
    {
        Free,       /**< Both time and amplitude are measured. */
        Horizontal, /**< Only time: the end point keeps the start's y. */
        Vertical    /**< Only amplitude: the end point keeps the start's x. */
    };

    /** The measurement drawn while the right button is held. */
    struct Ruler
    {
        bool active = false;              /**< Whether the ruler is shown. */
        RulerSnap snap = RulerSnap::Free; /**< Current axis lock. */
        int x0 = 0;                       /**< Press position. */
        int y0 = 0;                       /**< Press position. */
        int x1 = 0;                       /**< End position after snapping. */
        int y1 = 0;                       /**< End position after snapping. */
    };

    /** The range drawn while an annotation is selected with the right button. */
    struct Selection
    {
        bool active = false; /**< Whether a range is being selected. */
        int x0 = 0;          /**< Press position. */
        int x1 = 0;          /**< Current position. */
    };

    /** What the host does in reaction to the view; every callback may be empty. */
    struct Effects
    {
        std::function<void(double)> scroll_changed;                    /**< The scroll sample changed. */
        std::function<void(double)> zoom_changed;                      /**< The samples per pixel changed. */
        std::function<void(int)> channel_offset_changed;               /**< The first visible channel changed. */
        std::function<void(const ScrollAnimation&)> animate_scroll;    /**< Start a scroll animation. */
        std::function<void(const ZoomAnimation&)> animate_zoom;        /**< Start a zoom animation. */
        std::function<void()> stop_animations;                         /**< Stop all scroll animations. */
        std::function<void(int)> sample_clicked;                       /**< A click at an absolute sample. */
        std::function<void(int, int)> range_selected;                  /**< A new annotation range. */
        std::function<void(int, bool, int)> annotation_boundary_moved; /**< Index, start boundary, new sample. */
        std::function<void()> annotations_changed;                     /**< A span changed during a drag. */
        std::function<void(int)> channel_double_clicked;               /**< Double click on a logical channel. */
        std::function<void(CursorShape)> cursor_changed;               /**< Show another pointer shape. */
        std::function<void(const CursorData&)> cursor_data_changed;    /**< The crosshair moved. */
        std::function<void()> redraw;                                  /**< The overlays changed. */
    };

    //=========================================================================================================
    /**
     * Sets the reactions of the host.
     *
     * @param[in] effects   Callbacks; empty ones are skipped.
     */
    void set_effects(Effects effects);

    //=========================================================================================================
    /**
     * Sets the channels shown.
     *
     * @param[in] source   Channel source, not owned; nullptr for none.
     */
    void set_source(const TraceSource* source);

    /** @return The channel source or nullptr. */
    const TraceSource* source() const
    {
        return m_source;
    }

    // ── Viewport ──────────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Sets the size of the view in logical pixels.
     *
     * @param[in] width    Width.
     * @param[in] height   Height.
     */
    void set_view_size(int width, int height);

    /** @return The width of the view. */
    int view_width() const
    {
        return m_width;
    }

    /** @return The height of the view. */
    int view_height() const
    {
        return m_height;
    }

    /** @return The absolute sample at the left edge. */
    double scroll_sample() const
    {
        return m_scrollSample;
    }

    /** @return Samples per logical pixel. */
    double samples_per_pixel() const
    {
        return m_samplesPerPixel;
    }

    //=========================================================================================================
    /**
     * Sets the sample at the left edge, clamped with clamp_scroll_sample(); reports scroll_changed.
     *
     * @param[in] sample   Absolute sample.
     *
     * @return Whether the position changed.
     */
    bool set_scroll_sample(double sample);

    //=========================================================================================================
    /**
     * Sets the horizontal zoom (at least 1e-4); reports zoom_changed.
     *
     * @param[in] spp   Samples per pixel.
     *
     * @return Whether the zoom changed.
     */
    bool set_samples_per_pixel(double spp);

    //=========================================================================================================
    /**
     * @param[in] sample   Wanted left-edge sample.
     *
     * @return @p sample, not before the first loaded sample (or 0) and, if the last file sample is known, not so
     *         far that the view passes the end of the file.
     */
    double clamp_scroll_sample(double sample) const;

    //=========================================================================================================
    /**
     * Scrolls to a sample, animated by the host if @p duration_ms is positive.
     *
     * @param[in] target        Left-edge sample.
     * @param[in] duration_ms   Animation duration; 0 sets the position at once.
     */
    void scroll_to(double target, int duration_ms = 200);

    //=========================================================================================================
    /**
     * Scrolls by a number of samples; steps during a running scroll animation add up.
     *
     * @param[in] delta         Samples (negative = earlier).
     * @param[in] duration_ms   Animation duration; 0 scrolls at once.
     */
    void scroll_by(double delta, int duration_ms = 200);

    //=========================================================================================================
    /**
     * Zooms to a number of samples per pixel, animated by the host if @p duration_ms is positive.
     *
     * @param[in] spp           Target samples per pixel.
     * @param[in] duration_ms   Animation duration; 0 zooms at once.
     */
    void zoom_to(double spp, int duration_ms = 200);

    //=========================================================================================================
    /**
     * Asks the host to stop the scroll animations (stop_animations).
     */
    void stop_scroll_animations();

    //=========================================================================================================
    /**
     * Must be called by the host when a scroll animation started by scroll_to() reached its end.
     */
    void scroll_animation_finished();

    /** @return Whether a scroll animation of scroll_to() runs. */
    bool scroll_animation_running() const
    {
        return m_scrollAnimating;
    }

    /** @return The end of the running scroll animation. */
    double scroll_target() const
    {
        return m_scrollTarget;
    }

    /** @return The first visible sample, the scroll sample truncated. */
    int visible_first_sample() const;

    /** @return The number of visible samples, the width times the samples per pixel truncated. */
    int visible_sample_count() const;

    //=========================================================================================================
    /**
     * @param[in] x   Horizontal position.
     *
     * @return The absolute sample under @p x.
     */
    double sample_at(double x) const;

    //=========================================================================================================
    /**
     * Sets the sampling frequency used for time labels.
     *
     * @param[in] sfreq   Sampling frequency in Hz (negative values become 0).
     */
    void set_sfreq(double sfreq);

    /** @return The sampling frequency in Hz. */
    double sfreq() const
    {
        return m_sfreq;
    }

    //=========================================================================================================
    /**
     * Sets the absolute index of the first sample of the file (time 0).
     *
     * @param[in] first   Absolute sample.
     *
     * @return Whether the value changed.
     */
    bool set_first_file_sample(int first);

    /** @return The first file sample. */
    int first_file_sample() const
    {
        return m_firstFileSample;
    }

    //=========================================================================================================
    /**
     * Sets the absolute index of the last sample of the file.
     *
     * @param[in] last   Absolute sample, or -1 if unknown (scrolling is then unlimited).
     */
    void set_last_file_sample(int last);

    /** @return The last file sample or -1. */
    int last_file_sample() const
    {
        return m_lastFileSample;
    }

    // ── Channel window ────────────────────────────────────────────────

    /** @return The number of logical channels of the source (0 without a source). */
    int total_channels() const;

    //=========================================================================================================
    /**
     * Sets the first visible channel row, clamped so the window stays filled; reports channel_offset_changed.
     *
     * @param[in] channel   Logical channel index.
     *
     * @return Whether the first visible channel changed.
     */
    bool set_first_visible_channel(int channel);

    /** @return The first visible logical channel. */
    int first_visible_channel() const
    {
        return m_firstVisibleChannel;
    }

    //=========================================================================================================
    /**
     * Sets the number of channel rows shown at once.
     *
     * @param[in] count   Rows (at least 1).
     *
     * @return Whether the count changed.
     */
    bool set_visible_channel_count(int count);

    /** @return The number of channel rows shown at once. */
    int visible_channel_count() const
    {
        return m_visibleChannelCount;
    }

    /** @return The number of rows that show a channel: the row count limited by the remaining channels. */
    int visible_rows() const;

    //=========================================================================================================
    /**
     * Overlays all channels of a type in one lane (the groups of the source).
     *
     * @param[in] enabled   Butterfly mode on or off.
     *
     * @return Whether the mode changed.
     */
    bool set_butterfly_mode(bool enabled);

    /** @return Whether butterfly mode is on. */
    bool butterfly_mode() const
    {
        return m_butterflyMode;
    }

    // ── Behaviour ─────────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Freezes scrolling by wheel and drag; freezing stops running scroll animations. Clicks still report their
     * sample.
     *
     * @param[in] frozen   Frozen or not.
     */
    void set_frozen(bool frozen);

    /** @return Whether scrolling is frozen. */
    bool frozen() const
    {
        return m_frozen;
    }

    //=========================================================================================================
    /**
     * Sets whether the vertical wheel scrolls channel rows (true) or time (false).
     *
     * @param[in] channels   Wheel mode.
     */
    void set_wheel_scrolls_channels(bool channels);

    /** @return Whether the vertical wheel scrolls channels. */
    bool wheel_scrolls_channels() const
    {
        return m_wheelScrollsChannels;
    }

    //=========================================================================================================
    /**
     * Sets the multiplier of wheel scrolling in time.
     *
     * @param[in] factor   Factor, clamped to 0.25 .. 4.
     */
    void set_scroll_speed_factor(double factor);

    /** @return The wheel scroll multiplier. */
    double scroll_speed_factor() const
    {
        return m_scrollSpeedFactor;
    }

    // ── Annotations ───────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Sets the annotation spans.
     *
     * @param[in] annotations   Spans.
     */
    void set_annotations(std::vector<Annotation> annotations);

    /** @return The annotation spans, including a boundary that is being dragged. */
    const std::vector<Annotation>& annotations() const
    {
        return m_annotations;
    }

    //=========================================================================================================
    /**
     * Enables annotation editing: the right button selects a new range, the left button drags boundaries.
     *
     * @param[in] enabled   Editing on or off.
     */
    void set_annotation_selection_enabled(bool enabled);

    /** @return Whether annotation editing is enabled. */
    bool annotation_selection_enabled() const
    {
        return m_annotationSelectionEnabled;
    }

    //=========================================================================================================
    /**
     * @param[in] x          Horizontal position.
     * @param[out] is_start  Whether the start boundary was hit.
     *
     * @return The index of the first annotation with a boundary within 5 pixels of @p x, or -1.
     */
    int hit_test_annotation_boundary(double x, bool& is_start) const;

    /** @return Whether an annotation boundary is being dragged. */
    bool annotation_dragging() const
    {
        return m_annDragging;
    }

    /** @return The annotation range being selected. */
    const Selection& selection() const
    {
        return m_selection;
    }

    // ── Overlays ──────────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Enables the crosshair; disabling it forgets the pointer position.
     *
     * @param[in] enabled   Crosshair on or off.
     *
     * @return Whether the setting changed.
     */
    bool set_crosshair_enabled(bool enabled);

    /** @return Whether the crosshair is enabled. */
    bool crosshair_enabled() const
    {
        return m_crosshairEnabled;
    }

    /** @return The crosshair x, -1 while the pointer was not tracked. */
    int crosshair_x() const
    {
        return m_crosshairX;
    }

    /** @return The crosshair y, -1 while the pointer was not tracked. */
    int crosshair_y() const
    {
        return m_crosshairY;
    }

    //=========================================================================================================
    /**
     * Sets whether the crosshair label shows clock time (mm:ss.zzz) or seconds.
     *
     * @param[in] clock   Clock time or seconds.
     */
    void set_clock_time_format(bool clock);

    /** @return Whether the crosshair label shows clock time. */
    bool clock_time_format() const
    {
        return m_clockTime;
    }

    //=========================================================================================================
    /**
     * Shows a scalebar per channel type in the bottom-right corner.
     *
     * @param[in] visible   Scalebars on or off.
     *
     * @return Whether the setting changed.
     */
    bool set_scalebars_visible(bool visible);

    /** @return Whether the scalebars are shown. */
    bool scalebars_visible() const
    {
        return m_scalebarsVisible;
    }

    /** @return The ruler. */
    const Ruler& ruler() const
    {
        return m_ruler;
    }

    //=========================================================================================================
    /**
     * @param[in] dx   Horizontal distance from the press position.
     * @param[in] dy   Vertical distance from the press position.
     *
     * @return The axis lock of the ruler: free below 8 pixels, horizontal or vertical if one distance is more
     *         than twice the other.
     */
    static RulerSnap ruler_snap(int dx, int dy);

    /** @return Whether draw_overlays() draws anything: crosshair, scalebars, ruler or annotation selection. */
    bool overlays_visible() const;

    /** @return What lies under the crosshair, or nothing without a crosshair position or channel. */
    std::optional<CursorData> cursor_data() const;

    //=========================================================================================================
    /**
     * Draws the crosshair, scalebars, ruler and annotation selection into (0, 0, width, height) of @p canvas.
     *
     * @param[in] canvas    Canvas.
     * @param[in] metrics   Measures the labels.
     * @param[in] font      Base font of the labels; its size is replaced by the overlay sizes (8 to 9 points).
     */
    void draw_overlays(Canvas& canvas, const TextMetrics& metrics, const Font& font = {}) const;

    //=========================================================================================================
    /**
     * @param[in] metrics   Measures the labels.
     * @param[in] font      Base font of the labels.
     *
     * @return The overlays as a transparent figure of the view size.
     */
    DisplayList render_overlays(const TextMetrics& metrics, const Font& font = {}) const;

    // ── Input ─────────────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Handles a wheel event: Ctrl zooms, horizontal swipes scroll time, the vertical wheel scrolls channels or
     * time.
     *
     * @param[in] event   Wheel event.
     *
     * @return True (the event is always used).
     */
    bool handle_wheel(const WheelEvent& event);

    //=========================================================================================================
    /**
     * Handles a pointer event: ruler and annotation selection (right button), drag scrolling with inertia (left
     * button, or middle / Alt+left without inertia), annotation boundary resizing, clicks, crosshair tracking
     * and double clicks on channel rows.
     *
     * @param[in] event   Pointer event.
     *
     * @return Whether the event was used; a host may pass unused events on.
     */
    bool handle_pointer(const PointerEvent& event);

private:
    bool handle_press(const PointerEvent& event);
    bool handle_move(const PointerEvent& event);
    bool handle_release(const PointerEvent& event);
    bool handle_double_click(const PointerEvent& event);
    int clamped_boundary_sample(int x) const;
    void apply_ruler_end(int x, int y);
    void redraw() const;

    void draw_crosshair(Canvas& canvas, const TextMetrics& metrics, const Font& font) const;
    void draw_scalebars(Canvas& canvas, const TextMetrics& metrics, const Font& font) const;
    void draw_ruler(Canvas& canvas, const TextMetrics& metrics, const Font& font) const;
    void draw_selection(Canvas& canvas, const TextMetrics& metrics, const Font& font) const;

    struct VelocitySample
    {
        int x = 0;
        std::int64_t t = 0;
    };

    Effects m_effects;
    const TraceSource* m_source = nullptr;

    int m_width = 0;
    int m_height = 0;
    double m_scrollSample = 0.0;
    double m_samplesPerPixel = 1.0;
    double m_sfreq = 1000.0;
    int m_firstFileSample = 0;
    int m_lastFileSample = -1;
    bool m_scrollAnimating = false;
    double m_scrollTarget = 0.0;

    int m_firstVisibleChannel = 0;
    int m_visibleChannelCount = 12;
    bool m_butterflyMode = false;

    bool m_frozen = false;
    bool m_wheelScrollsChannels = true;
    double m_scrollSpeedFactor = 1.0;

    // Middle-button or Alt+left drag: scrolls without inertia
    bool m_dragging = false;
    int m_dragStartX = 0;
    double m_dragStartScroll = 0.0;

    // Left-button drag: activated past a threshold, glides on with the release velocity
    bool m_leftButtonDown = false;
    bool m_leftDragActivated = false;
    int m_leftDownX = 0;
    double m_leftDownScroll = 0.0;
    std::int64_t m_dragStartTime = 0;
    std::vector<VelocitySample> m_velocityHistory;

    std::vector<Annotation> m_annotations;
    bool m_annotationSelectionEnabled = false;
    bool m_annDragging = false;
    int m_annDragIndex = -1;
    bool m_annDragIsStart = true;
    int m_annHoverIndex = -1;
    bool m_annHoverIsStart = true;
    Selection m_selection;

    Ruler m_ruler;

    bool m_crosshairEnabled = false;
    int m_crosshairX = -1;
    int m_crosshairY = -1;
    bool m_clockTime = false;
    bool m_scalebarsVisible = false;
};

} // namespace mne::viz

#endif // VIZ_TRACE_VIEW_H
