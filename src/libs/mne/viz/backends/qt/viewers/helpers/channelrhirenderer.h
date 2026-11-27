//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channelrhirenderer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Widgets-free QRhi renderer and input handler of the raw-data channel trace view.
 *
 * ChannelRhiRenderer is the Qt side of the raw-trace view: it converts input events and records the frames of
 * @ref mne::viz::TraceFrameBuilder into any QRhi render target with @ref mne::viz::qt::RhiFrameRenderer.
 * The logic is Qt-free in mne_scene: @ref mne::viz::TraceView (viewport, interaction state, HUD overlays),
 * @ref mne::viz::TraceFrameBuilder (trace vertices, textures and draws of a frame),
 * @ref mne::viz::TraceMarkers (annotation, event and epoch overlay) and @ref mne::viz::ViewAnimator
 * (scroll and zoom animations). It only depends on QtCore and QtGui: a host (the @c QRhiWidget
 * @ref mne::viz::qt::ChannelRhiView or the @c QWindow @ref mne::viz::qt::ChannelWindow) implements
 * @ref mne::viz::qt::ChannelRhiHost, forwards its input events and calls renderFrame().
 */

#ifndef DISP_CHANNELRHIRENDERER_H
#define DISP_CHANNELRHIRENDERER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"
#include "channeldatamodel.h"
#include "../../plots/helpers/rhiframerenderer.h"
#include <mne/viz/gpu/trace_frame.h>
#include <mne/viz/host/trace_view_client.h>
#include <mne/viz/views/trace_geometry.h>
#include <mne/viz/views/trace_markers.h>
#include <mne/viz/views/trace_view.h>
#include <mne/viz/views/view_animator.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QFont>
#include <QImage>
#include <QObject>
#include <QPointer>
#include <QSize>
#include <QString>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

class QMouseEvent;
class QPainter;
class QRhi;
class QRhiCommandBuffer;
class QRhiRenderTarget;
class QTimerEvent;
class QWheelEvent;

namespace mne::viz::qt
{

class QtTextMetrics;

//=============================================================================================================
/**
 * @brief Services a host surface provides to a @ref ChannelRhiRenderer.
 *
 * Implemented by the widget host @ref ChannelRhiView and the window host @ref ChannelWindow.
 */
class VIZQTSHARED_EXPORT ChannelRhiHost
{
public:
    //=========================================================================================================
    /**
     * Destroys the host interface.
     */
    virtual ~ChannelRhiHost() = default;

    //=========================================================================================================
    /**
     * Schedules a new frame (asynchronously).
     */
    virtual void requestFrame() = 0;

    //=========================================================================================================
    /**
     * Returns the size of the render surface in logical (device-independent) pixels.
     *
     * @return The logical surface size.
     */
    virtual QSize logicalSize() const = 0;

    //=========================================================================================================
    /**
     * Returns the ratio between physical and logical pixels of the render surface.
     *
     * @return The device pixel ratio.
     */
    virtual qreal surfaceDevicePixelRatio() const = 0;

    //=========================================================================================================
    /**
     * Sets the mouse cursor shape shown over the surface.
     *
     * @param[in] shape  The cursor shape.
     */
    virtual void setCursorShape(Qt::CursorShape shape) = 0;

    //=========================================================================================================
    /**
     * Restores the default mouse cursor of the surface.
     */
    virtual void unsetCursorShape() = 0;

    //=========================================================================================================
    /**
     * Enables or disables delivery of mouse move events without a pressed button.
     *
     * @param[in] enabled  True to track the mouse.
     */
    virtual void setMouseTracking(bool enabled) = 0;

    //=========================================================================================================
    /**
     * Returns the font used for the overlay labels (crosshair, scalebars, ruler).
     *
     * @return The host font.
     */
    virtual QFont font() const = 0;
};

//=============================================================================================================
/**
 * @brief Widgets-free QRhi renderer and input handler of the raw-data channel trace view.
 *
 * Draws the channel traces, the per-second bands and grid, events, annotations and epoch markers, and,
 * as a textured alpha-blended quad on top, the crosshair, scalebars, ruler and annotation-selection
 * overlay. Mouse and wheel events of the host are handled by the handle*() functions.
 */
class VIZQTSHARED_EXPORT ChannelRhiRenderer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(float scrollSample READ scrollSample WRITE setScrollSample NOTIFY scrollSampleChanged)
    Q_PROPERTY(float samplesPerPixel READ samplesPerPixel WRITE setSamplesPerPixel NOTIFY samplesPerPixelChanged)

public:
    //=========================================================================================================
    /**
     * @brief Stimulus / event marker — a coloured vertical line at a given sample.
     */
    struct EventMarker
    {
        int sample = 0;    ///< Absolute sample index of the event onset.
        int type = 1;      ///< Numeric event type (stimulus code).
        QColor color;      ///< Display colour of the line and label chip.
        std::string label; ///< Short text label (shown at the bottom strip); defaults to type number.
    };

    //=========================================================================================================
    /**
     * @brief Time-span annotation overlay.
     */
    struct AnnotationSpan
    {
        int startSample = 0; ///< Absolute first sample covered by the annotation.
        int endSample = 0;   ///< Absolute last sample covered by the annotation.
        QColor color;        ///< Fill / border colour for the highlighted span.
        std::string label;   ///< Annotation label shown near the top edge.
    };

    //=========================================================================================================
    /**
     * Constructs the renderer.
     *
     * @param[in] host    Host surface services; not owned, may be nullptr (then the view size is empty).
     * @param[in] parent  Optional QObject parent.
     */
    explicit ChannelRhiRenderer(ChannelRhiHost* host = nullptr, QObject* parent = nullptr);

    //=========================================================================================================
    /**
     * Destroys the renderer and its GPU resources. Call releaseResources() before the QRhi goes away.
     */
    ~ChannelRhiRenderer() override;

    //=========================================================================================================
    /**
     * Sets the host surface services.
     *
     * @param[in] host  Host; not owned, may be nullptr.
     */
    void setHost(ChannelRhiHost* host);

    //=========================================================================================================
    /**
     * Returns the host surface services.
     *
     * @return The host or nullptr.
     */
    ChannelRhiHost* host() const
    {
        return m_pHost;
    }

    // ── GPU frame ─────────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Marks all GPU resources for (re)creation, e.g. after the host obtained a new QRhi.
     */
    void initializeResources();

    //=========================================================================================================
    /**
     * Releases all GPU resources. Must be called before the QRhi that created them is destroyed.
     */
    void releaseResources();

    //=========================================================================================================
    /**
     * Records one frame (traces, bands/events/annotations overlay and the crosshair/scalebar/ruler
     * overlay) into @p target.
     *
     * @param[in] rhi     The QRhi that owns @p target and the renderer's GPU resources.
     * @param[in] target  Render target sized like the host's logical size times its device pixel ratio.
     * @param[in] cb      Command buffer of the current frame.
     */
    void renderFrame(QRhi* rhi, QRhiRenderTarget* target, QRhiCommandBuffer* cb);

    //=========================================================================================================
    /**
     * Returns whether the crosshair, scalebar, ruler or annotation-selection overlay is currently shown,
     * i.e. whether renderFrame() draws the overlay quad.
     *
     * @return True if the overlay quad is drawn.
     */
    bool hudVisible() const;

    //=========================================================================================================
    /**
     * Paints the crosshair, scalebars, ruler and annotation-selection overlay in logical coordinates.
     *
     * @param[in] painter  Painter on a device of the host's logical size.
     */
    void paintHud(QPainter& painter) const;

    //=========================================================================================================
    /**
     * Returns the overlay image (crosshair, scalebars, ruler, annotation selection) uploaded by the
     * last renderFrame() that drew the overlay quad.
     *
     * @return The overlay image (premultiplied alpha is not used), or a null image.
     */
    QImage hudImage() const;

    // ── Input ─────────────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Handles a wheel event: Ctrl zooms, horizontal swipes scroll time, the vertical wheel scrolls
     * channels or time.
     *
     * @param[in] event  The wheel event in host coordinates.
     *
     * @return True if the event was handled.
     */
    bool handleWheel(QWheelEvent* event);

    //=========================================================================================================
    /**
     * Handles a mouse press (ruler, annotation selection, panning, annotation boundary drag, clicks).
     *
     * @param[in] event  The mouse event in host coordinates.
     *
     * @return True if the event was handled; otherwise the host passes it to its base class.
     */
    bool handleMousePress(QMouseEvent* event);

    //=========================================================================================================
    /**
     * Handles a mouse move (drags, ruler, crosshair tracking, annotation boundary hover).
     *
     * @param[in] event  The mouse event in host coordinates.
     *
     * @return True if the event was handled; otherwise the host passes it to its base class.
     */
    bool handleMouseMove(QMouseEvent* event);

    //=========================================================================================================
    /**
     * Handles a mouse release (ends drags, emits clicks and selections, starts inertial scrolling).
     *
     * @param[in] event  The mouse event in host coordinates.
     *
     * @return True if the event was handled; otherwise the host passes it to its base class.
     */
    bool handleMouseRelease(QMouseEvent* event);

    //=========================================================================================================
    /**
     * Handles a double click: toggles the bad state of the channel under the cursor.
     *
     * @param[in] event  The mouse event in host coordinates.
     *
     * @return True if the event was handled; otherwise the host passes it to its base class.
     */
    bool handleMouseDoubleClick(QMouseEvent* event);

    //=========================================================================================================
    /**
     * Must be called by the host after its surface was resized; emits viewResized().
     */
    void handleResize();

    // ── Data and overlays ─────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Attach the data model.  The renderer does NOT take ownership.
     *
     * @param[in] model  The channel data model to render.
     */
    void setModel(ChannelDataModel* model);

    //=========================================================================================================
    /**
     * Returns the attached data model.
     *
     * @return The model or nullptr.
     */
    ChannelDataModel* model() const
    {
        return m_model.data();
    }

    //=========================================================================================================
    /**
     * Set the list of stimulus / event markers to overlay on the traces.
     *
     * @param[in] events  List of EventMarker objects; empty to clear.
     */
    void setEvents(const std::vector<EventMarker>& events);

    //=========================================================================================================
    /**
     * Set the epoch trigger samples drawn as dashed vertical lines.
     *
     * @param[in] triggerSamples  Absolute trigger sample indices.
     */
    void setEpochMarkers(const std::vector<int>& triggerSamples);

    //=========================================================================================================
    /**
     * Show or hide the epoch trigger markers.
     *
     * @param[in] visible  True to draw them.
     */
    void setEpochMarkersVisible(bool visible);

    //=========================================================================================================
    /**
     * Returns whether the epoch trigger markers are drawn.
     *
     * @return True if visible.
     */
    bool epochMarkersVisible() const
    {
        return m_client.markers().epoch_markers_visible();
    }

    //=========================================================================================================
    /**
     * Show or hide the red highlight of clipped trace segments.
     *
     * @param[in] visible  True to highlight clipping.
     */
    void setClippingVisible(bool visible);

    //=========================================================================================================
    /**
     * Returns whether clipped segments are highlighted.
     *
     * @return True if highlighted.
     */
    bool clippingVisible() const
    {
        return m_bShowClipping;
    }

    //=========================================================================================================
    /**
     * Toggle z-score display: every trace is normalised to (y - mean) / std over the loaded window.
     *
     * @param[in] enabled  True to enable z-score mode.
     */
    void setZScoreMode(bool enabled);

    //=========================================================================================================
    /**
     * Returns whether z-score mode is on.
     *
     * @return True if enabled.
     */
    bool zScoreMode() const
    {
        return m_bZScoreMode;
    }

    //=========================================================================================================
    /**
     * Set the list of time-span annotations to overlay on the traces.
     *
     * @param[in] annotations  List of AnnotationSpan objects.
     */
    void setAnnotations(const std::vector<AnnotationSpan>& annotations);

    //=========================================================================================================
    /**
     * Enable or disable annotation range selection (right-drag) and boundary dragging.
     *
     * @param[in] enabled  True to enable annotation editing.
     */
    void setAnnotationSelectionEnabled(bool enabled);

    //=========================================================================================================
    /**
     * Show or hide the event markers.
     *
     * @param[in] visible  True to draw them.
     */
    void setEventsVisible(bool visible);

    //=========================================================================================================
    /**
     * Returns whether event markers are drawn.
     *
     * @return True if visible.
     */
    bool eventsVisible() const
    {
        return m_client.markers().events_visible();
    }

    //=========================================================================================================
    /**
     * Show or hide the annotation spans.
     *
     * @param[in] visible  True to draw them.
     */
    void setAnnotationsVisible(bool visible);

    //=========================================================================================================
    /**
     * Returns whether annotation spans are drawn.
     *
     * @return True if visible.
     */
    bool annotationsVisible() const
    {
        return m_client.markers().annotations_visible();
    }

    // ── Scroll / zoom ─────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Returns the absolute sample at the left edge.
     *
     * @return The scroll position in samples.
     */
    float scrollSample() const
    {
        return static_cast<float>(m_client.view().scroll_sample());
    }

    //=========================================================================================================
    /**
     * Returns the horizontal zoom.
     *
     * @return Samples per logical pixel.
     */
    float samplesPerPixel() const
    {
        return static_cast<float>(m_client.view().samples_per_pixel());
    }

    //=========================================================================================================
    /**
     * Set the left-edge scroll position (clamped to the file bounds).
     *
     * @param[in] sample  Absolute sample index for the left viewport edge.
     */
    void setScrollSample(float sample);

    //=========================================================================================================
    /**
     * Set the horizontal zoom (samples per screen pixel).
     *
     * @param[in] spp  Samples per pixel (> 1 = zoomed out, < 1 = zoomed in).
     */
    void setSamplesPerPixel(float spp);

    //=========================================================================================================
    /**
     * Smoothly animate the scroll position.
     *
     * @param[in] targetSample  Target left-edge sample index.
     * @param[in] durationMs    Animation duration in milliseconds (0 = instant).
     */
    void scrollTo(float targetSample, int durationMs = 200);

    //=========================================================================================================
    /**
     * Smoothly scroll by a number of samples; steps during a running animation add up.
     *
     * @param[in] deltaSamples  Samples to scroll (negative = earlier).
     * @param[in] durationMs    Animation duration in milliseconds (0 = instant).
     */
    void scrollBy(float deltaSamples, int durationMs = 200);

    //=========================================================================================================
    /**
     * Smoothly animate the zoom level.
     *
     * @param[in] targetSpp   Target samples-per-pixel.
     * @param[in] durationMs  Animation duration in milliseconds (0 = instant).
     */
    void zoomTo(float targetSpp, int durationMs = 200);

    //=========================================================================================================
    /**
     * Background colour of the render surface.
     *
     * @param[in] color  The new background colour.
     */
    void setBackgroundColor(const QColor& color);

    //=========================================================================================================
    /**
     * Returns the background colour.
     *
     * @return The background colour.
     */
    QColor backgroundColor() const
    {
        return m_bgColor;
    }

    //=========================================================================================================
    /**
     * How many visible windows to prefetch on each side of the viewport. Default = 1.0.
     *
     * @param[in] factor  Prefetch factor.
     */
    void setPrefetchFactor(float factor);

    //=========================================================================================================
    /**
     * Returns the absolute sample index shown at the left edge.
     *
     * @return scrollSample() truncated to int.
     */
    int visibleFirstSample() const;

    //=========================================================================================================
    /**
     * Returns the number of samples currently visible.
     *
     * @return Logical width times samples per pixel, truncated to int.
     */
    int visibleSampleCount() const;

    //=========================================================================================================
    /**
     * Restrict rendering to a subset of channel indices; empty shows all channels.
     *
     * @param[in] indices  Ordered list of model channel indices to display.
     */
    void setChannelIndices(const std::vector<int>& indices);

    //=========================================================================================================
    /**
     * Total number of logical channels available for scrolling (respects filter and bad hiding).
     *
     * @return Number of logical channels.
     */
    int totalLogicalChannels() const;

    //=========================================================================================================
    /**
     * Set the index of the first visible channel row.
     *
     * @param[in] ch  Zero-based logical channel index.
     */
    void setFirstVisibleChannel(int ch);

    //=========================================================================================================
    /**
     * Returns the first visible logical channel row.
     *
     * @return Zero-based logical channel index.
     */
    int firstVisibleChannel() const
    {
        return m_client.view().first_visible_channel();
    }

    //=========================================================================================================
    /**
     * Set how many channel rows are displayed simultaneously.
     *
     * @param[in] count  Number of rows (at least 1).
     */
    void setVisibleChannelCount(int count);

    //=========================================================================================================
    /**
     * Returns the number of displayed channel rows.
     *
     * @return Row count.
     */
    int visibleChannelCount() const
    {
        return m_client.view().visible_channel_count();
    }

    //=========================================================================================================
    /**
     * Freeze or unfreeze drag-panning (mouse drag and inertia).
     *
     * @param[in] frozen  True to disable drag-panning.
     */
    void setFrozen(bool frozen);

    //=========================================================================================================
    /**
     * Returns whether drag-panning is frozen.
     *
     * @return True if frozen.
     */
    bool isFrozen() const
    {
        return m_client.view().frozen();
    }

    //=========================================================================================================
    /**
     * Show or hide the time grid and alternating per-second bands.
     *
     * @param[in] visible  True to draw the grid.
     */
    void setGridVisible(bool visible);

    //=========================================================================================================
    /**
     * Returns whether the grid is drawn.
     *
     * @return True if visible.
     */
    bool gridVisible() const
    {
        return m_gridVisible;
    }

    //=========================================================================================================
    /**
     * Set the sampling frequency (Hz) used for time labels, bands and grid.
     *
     * @param[in] sfreq  Sampling frequency in Hz.
     */
    void setSfreq(float sfreq);

    //=========================================================================================================
    /**
     * Set the absolute sample index of the first sample in the file.
     *
     * @param[in] first  Absolute sample index of the file's first sample.
     */
    void setFirstFileSample(int first);

    //=========================================================================================================
    /**
     * Set the absolute sample index of the last sample in the file (-1 = unlimited).
     *
     * @param[in] last  Absolute sample index of the file's last sample.
     */
    void setLastFileSample(int last);

    //=========================================================================================================
    /**
     * Returns the last file sample used to clamp scrolling.
     *
     * @return Absolute sample index or -1.
     */
    int lastFileSample() const
    {
        return m_client.view().last_file_sample();
    }

    //=========================================================================================================
    /**
     * Control whether the vertical mouse wheel scrolls channels (true) or time (false).
     *
     * @param[in] channelsMode  true = vertical wheel → channels.
     */
    void setWheelScrollsChannels(bool channelsMode);

    //=========================================================================================================
    /**
     * Returns whether the vertical wheel scrolls channels.
     *
     * @return True for channel scrolling.
     */
    bool wheelScrollsChannels() const
    {
        return m_client.view().wheel_scrolls_channels();
    }

    //=========================================================================================================
    /**
     * Set the multiplier for wheel scrolling (clamped to 0.25–4).
     *
     * @param[in] factor  Scroll speed factor.
     */
    void setScrollSpeedFactor(float factor);

    //=========================================================================================================
    /**
     * Returns the wheel scroll multiplier.
     *
     * @return Scroll speed factor.
     */
    float scrollSpeedFactor() const
    {
        return static_cast<float>(m_client.view().scroll_speed_factor());
    }

    //=========================================================================================================
    /**
     * Remove bad channels from the logical channel list (true) or show them (false).
     *
     * @param[in] hide  True to hide bad channels.
     */
    void setHideBadChannels(bool hide);

    //=========================================================================================================
    /**
     * Returns whether bad channels are hidden.
     *
     * @return True if hidden.
     */
    bool hideBadChannels() const
    {
        return m_hideBadChannels;
    }

    //=========================================================================================================
    /**
     * Enable or disable the crosshair cursor; while enabled, mouse moves emit cursorDataChanged().
     *
     * @param[in] enabled  True to show the crosshair.
     */
    void setCrosshairEnabled(bool enabled);

    //=========================================================================================================
    /**
     * Returns whether the crosshair is enabled.
     *
     * @return True if enabled.
     */
    bool crosshairEnabled() const
    {
        return m_client.view().crosshair_enabled();
    }

    //=========================================================================================================
    /**
     * Set whether the crosshair label uses clock time (mm:ss.ms) or seconds.
     *
     * @param[in] useClock  True for clock time, false for float seconds.
     */
    void setClockTimeFormat(bool useClock);

    //=========================================================================================================
    /**
     * Returns whether the crosshair label uses clock time.
     *
     * @return True for clock time.
     */
    bool clockTimeFormat() const
    {
        return m_client.view().clock_time_format();
    }

    //=========================================================================================================
    /**
     * Render the current view into an image with QPainter, without the GPU.
     *
     * @param[in] size  Output size in pixels; an invalid size uses the host's logical size.
     *
     * @return The rendered image, or a null image if there is no model or the size is empty.
     */
    QImage renderToImage(const QSize& size = QSize()) const;

    //=========================================================================================================
    /**
     * Show or hide per-channel-type scalebars in the bottom-right corner.
     *
     * @param[in] visible  True to draw the scalebars.
     */
    void setScalebarsVisible(bool visible);

    //=========================================================================================================
    /**
     * Returns whether scalebars are drawn.
     *
     * @return True if visible.
     */
    bool scalebarsVisible() const
    {
        return m_client.view().scalebars_visible();
    }

    //=========================================================================================================
    /**
     * Toggle butterfly mode: all channels of the same type are overlaid in a single lane.
     *
     * @param[in] enabled  True to enable butterfly mode.
     */
    void setButterflyMode(bool enabled);

    //=========================================================================================================
    /**
     * Returns whether butterfly mode is on.
     *
     * @return True if enabled.
     */
    bool butterflyMode() const
    {
        return m_client.view().butterfly_mode();
    }

signals:
    //=========================================================================================================
    /**
     * Emitted when the left-edge scroll position changes.
     *
     * @param[in] sample  New scroll position in samples.
     */
    void scrollSampleChanged(float sample);

    //=========================================================================================================
    /**
     * Emitted when the horizontal zoom changes.
     *
     * @param[in] spp  New samples per pixel.
     */
    void samplesPerPixelChanged(float spp);

    //=========================================================================================================
    /**
     * Emitted after the host surface was resized.
     *
     * @param[in] newWidth   New logical width.
     * @param[in] newHeight  New logical height.
     */
    void viewResized(int newWidth, int newHeight);

    //=========================================================================================================
    /**
     * Emitted whenever the first visible channel index changes.
     *
     * @param[in] firstChannel  New first visible logical channel.
     */
    void channelOffsetChanged(int firstChannel);

    //=========================================================================================================
    /**
     * Emitted whenever the user clicks; provides the sample index under the cursor.
     *
     * @param[in] sample  Absolute sample index.
     */
    void sampleClicked(int sample);

    //=========================================================================================================
    /**
     * Emitted when the user selects a time span with a right-drag in annotation mode.
     *
     * @param[in] startSample  First selected sample.
     * @param[in] endSample    Last selected sample.
     */
    void sampleRangeSelected(int startSample, int endSample);

    //=========================================================================================================
    /**
     * Emitted when the user finishes dragging an annotation boundary.
     *
     * @param[in] annotationIndex  Index in the annotation span list.
     * @param[in] isStartBoundary  True if the start boundary was dragged.
     * @param[in] newSample        New absolute sample position of the boundary.
     */
    void annotationBoundaryMoved(int annotationIndex, bool isStartBoundary, int newSample);

    //=========================================================================================================
    /**
     * Emitted continuously when the crosshair is active and the mouse moves.
     *
     * @param[in] timeSec     Time at cursor in seconds relative to file start.
     * @param[in] amplitude   Raw amplitude value under the cursor in physical units.
     * @param[in] channelName Name of the channel under the cursor.
     * @param[in] unitLabel   Short unit label ("T", "V", "AU", …).
     */
    void cursorDataChanged(float timeSec, float amplitude, const QString& channelName, const QString& unitLabel);

protected:
    //=========================================================================================================
    /**
     * Advances the scroll and zoom animations.
     *
     * @param[in] event  The timer event.
     */
    void timerEvent(QTimerEvent* event) override;

private:
    int viewWidth() const;
    int viewHeight() const;
    QFont hostFont() const;
    void requestUpdate();

    class TraceSourceAdapter;
    class HostSurface;

    void connectView();
    viz::TraceView& view() const; // the TraceView, sized like the host
    bool forwardPointer(QMouseEvent* event, viz::PointerEvent::Type type);
    void tickAnimations();
    void startAnimationTimer();
    viz::TraceStyle traceStyle() const;
    void updateTextMetrics();

    ChannelRhiHost* m_pHost = nullptr;

    // ── Shared browser state ───────────────────────────────────────────
    QPointer<ChannelDataModel> m_model;
    std::unique_ptr<TraceSourceAdapter> m_traceSource; // the model's channels for the view
    std::unique_ptr<HostSurface> m_surface;            // the host as the client's viz::Surface
    mutable viz::TraceViewClient m_client;             // view, markers, animations and frames; sized by view()
    std::unique_ptr<QtTextMetrics> m_metrics;          // metrics of m_metricsFont for the client
    QFont m_metricsFont;
    RhiFrameRenderer m_frameRenderer; // executes the frames with QRhi
    int m_animationTimerId = 0;       // ticks the animations while the host renders no frames
    QColor m_bgColor{250, 250, 250};  // light default

    bool m_gridVisible = true;
    bool m_hideBadChannels = false;

    // ── Channel index filter ──────────────────────────────────────────
    std::vector<int> m_filteredChannels;

    // ── Trace style ───────────────────────────────────────────────────
    bool m_bShowClipping = true;
    bool m_bZScoreMode = false;

    int actualChannelAt(int logicalIdx) const; // maps logical → model channel index
    std::vector<int> effectiveChannelIndices() const;
};

} // namespace mne::viz::qt

#endif // DISP_CHANNELRHIRENDERER_H
