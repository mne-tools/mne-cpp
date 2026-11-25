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
 * ChannelRhiRenderer holds the complete state of the GPU raw-trace view (scroll, zoom, channel window,
 * events, annotations, crosshair, ruler, scalebars), its QRhi pipelines and buffers, and records a frame
 * into any QRhi render target. It only depends on QtCore and QtGui: a host (the @c QRhiWidget
 * @ref mne::disp::ChannelRhiView or the @c QWindow @ref mne::disp::ChannelWindow) implements
 * @ref mne::disp::ChannelRhiHost, forwards its input events and calls renderFrame().
 */

#ifndef DISP_CHANNELRHIRENDERER_H
#define DISP_CHANNELRHIRENDERER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include "channeldatamodel.h"

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

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

class QMouseEvent;
class QPainter;
class QPropertyAnimation;
class QRhi;
class QRhiBuffer;
class QRhiCommandBuffer;
class QRhiGraphicsPipeline;
class QRhiRenderTarget;
class QRhiResourceUpdateBatch;
class QRhiSampler;
class QRhiShaderResourceBindings;
class QRhiTexture;
class QWheelEvent;

namespace mne::disp
{

//=============================================================================================================
/**
 * @brief Services a host surface provides to a @ref ChannelRhiRenderer.
 *
 * Implemented by the widget host @ref ChannelRhiView and the window host @ref ChannelWindow.
 */
class DISPSHARED_EXPORT ChannelRhiHost
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
class DISPSHARED_EXPORT ChannelRhiRenderer : public QObject
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
    const QImage& hudImage() const
    {
        return m_hudImage;
    }

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
        return m_bShowEpochMarkers;
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
        return m_bShowEvents;
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
        return m_bShowAnnotations;
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
        return m_scrollSample;
    }

    //=========================================================================================================
    /**
     * Returns the horizontal zoom.
     *
     * @return Samples per logical pixel.
     */
    float samplesPerPixel() const
    {
        return m_samplesPerPixel;
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
        return m_firstVisibleChannel;
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
        return m_visibleChannelCount;
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
        return m_frozen;
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
        return m_lastFileSample;
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
        return m_wheelScrollsChannels;
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
        return m_scrollSpeedFactor;
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
        return m_crosshairEnabled;
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
        return m_useClockTime;
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
        return m_scalebarsVisible;
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
        return m_butterflyMode;
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

private:
    int viewWidth() const;
    int viewHeight() const;
    QFont hostFont() const;
    void requestUpdate();

    float clampScrollSample(float sample) const;
    void stopScrollAnimations();

    void drawCrosshair(QPainter& p) const;
    void drawScalebars(QPainter& p) const;
    void drawRulerOverlay(QPainter& p) const;
    void drawAnnotationSelectionOverlay(QPainter& p) const;
    void emitCursorData();

    // ── GPU resource management ────────────────────────────────────────
    struct ChannelGpuData
    {
        std::unique_ptr<QRhiBuffer> vbo;
        int vertexCount = 0;
        int vboFirstSample = 0; // absolute first sample in VBO
    };

    void ensurePipeline(QRhi* rhi, QRhiRenderTarget* target);
    void rebuildVBOs(QRhi* rhi, QRhiResourceUpdateBatch* batch);
    void updateUBO(QRhiResourceUpdateBatch* batch);
    bool isVboDirty() const;

    // ── Overlay blit (annotations/events baked into texture, bands in shader) ──
    void ensureOverlayPipeline(QRhi* rhi, QRhiRenderTarget* target);
    bool ensureOverlayResources(QRhi* rhi, QRhiRenderTarget* target, QRhiResourceUpdateBatch* batch);
    void rebuildOverlayImage(int logicalWidth, int logicalHeight, qreal devicePixelRatio);
    bool prepareHud(QRhi* rhi, QRhiResourceUpdateBatch* batch, int logicalW, int logicalH, qreal dpr);

    ChannelRhiHost* m_pHost = nullptr;

    std::unique_ptr<QRhiBuffer> m_overlayVbo; // Static quad
    std::unique_ptr<QRhiTexture> m_overlayTex;
    std::unique_ptr<QRhiSampler> m_overlaySampler;
    std::unique_ptr<QRhiShaderResourceBindings> m_overlaySrb;
    std::unique_ptr<QRhiGraphicsPipeline> m_overlayPipeline;
    std::unique_ptr<QRhiBuffer> m_overlayUbo; // OverlayParams UBO
    bool m_overlayVboNeedsUpload = false;
    QImage m_overlayImage;
    bool m_overlayDirty = true;
    QSize m_overlayTexSize;

    // Overlay prefetch: the texture covers a wider sample range than the viewport.
    static constexpr float kOverlayPrefetchFactor = 1.0f; // extra viewport widths each side
    float m_overlayFirstSample = 0.f;                     // first sample covered by overlay tex
    float m_overlayTotalSamples = 0.f;                    // total sample span of overlay tex

    // ── HUD overlay (crosshair, scalebars, ruler, annotation selection) ──
    // Screen-space QImage drawn with the overlay pipeline as a second quad after the traces.
    std::unique_ptr<QRhiTexture> m_hudTex;
    std::unique_ptr<QRhiShaderResourceBindings> m_hudSrb;
    std::unique_ptr<QRhiBuffer> m_hudUbo;
    QImage m_hudImage;
    QSize m_hudTexSize;
    bool m_hudDirty = true;

    std::unique_ptr<QRhiBuffer> m_ubo;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
    std::vector<ChannelGpuData> m_gpuChannels;
    int m_uboStride = 256;
    bool m_pipelineDirty = true;
    bool m_vboDirty = true;

    // ── Shared browser state ───────────────────────────────────────────
    QPointer<ChannelDataModel> m_model;
    float m_scrollSample = 0.f;
    float m_samplesPerPixel = 1.f;
    float m_prefetchFactor = 1.0f;
    QColor m_bgColor{250, 250, 250}; // light default

    bool m_frozen = false;
    bool m_gridVisible = true;
    float m_sfreq = 1000.f;
    int m_firstFileSample = 0;
    int m_lastFileSample = -1;          // -1 = no limit (file not yet known)
    bool m_wheelScrollsChannels = true; // default: vertical wheel → channels
    float m_scrollSpeedFactor = 1.0f;   // multiplier for wheel/keyboard scroll
    bool m_hideBadChannels = false;

    // ── Vertical channel windowing ────────────────────────────────────
    int m_firstVisibleChannel = 0;
    int m_visibleChannelCount = 12;

    // ── Drag scroll support ────────────────────────────────────────────
    bool m_dragging = false;
    int m_dragStartX = 0;
    float m_dragStartScroll = 0.f;

    // ── Left-button drag (panning) + inertial scroll ──────────────────
    bool m_leftButtonDown = false;
    int m_leftDownX = 0;
    float m_leftDownScroll = 0.f;
    bool m_leftDragActivated = false;

    struct VelocitySample
    {
        int x;
        std::int64_t t;
    };
    std::vector<VelocitySample> m_velocityHistory;
    std::chrono::steady_clock::time_point m_dragStart;
    QPropertyAnimation* m_pInertialAnim = nullptr;
    QPointer<QPropertyAnimation> m_pScrollAnim;
    float m_scrollTarget = 0.f;

    // ── Channel index filter ──────────────────────────────────────────
    std::vector<int> m_filteredChannels;

    // ── Event / stimulus markers ──────────────────────────────────────
    std::vector<EventMarker> m_events;
    std::vector<int> m_epochTriggerSamples;
    std::vector<AnnotationSpan> m_annotations;
    bool m_annotationSelectionEnabled = false;
    bool m_bShowEvents = true;
    bool m_bShowAnnotations = true;
    bool m_bShowEpochMarkers = true;
    bool m_bShowClipping = true;
    bool m_bZScoreMode = false;

    // ── Annotation boundary drag-resize ───────────────────────────────
    bool m_annDragging = false;                 ///< True while dragging an annotation boundary.
    int m_annDragIndex = -1;                    ///< Index of the annotation being resized.
    bool m_annDragIsStart = true;               ///< True if dragging the start boundary.
    int m_annHoverIndex = -1;                   ///< Index of annotation whose boundary is under cursor.
    bool m_annHoverIsStart = true;              ///< True if cursor is near the start boundary.
    static constexpr int kAnnBoundaryHitPx = 5; ///< Hit-test tolerance in pixels.

    // ── Annotation range selection (right-drag when annotation mode ON) ─
    bool m_annSelecting = false; ///< True while right-dragging to select a new annotation range.
    int m_annSelX0 = 0;          ///< Press position (screen px).
    int m_annSelX1 = 0;          ///< Current cursor position (screen px).

    int hitTestAnnotationBoundary(int px, bool& isStart) const;

    int actualChannelAt(int logicalIdx) const; // maps logical → model channel index
    std::vector<int> effectiveChannelIndices() const;

    // ── Prefetch window tracking ───────────────────────────────────────
    int m_vboWindowFirst = 0;
    int m_vboWindowLast = 0;

    // ── Ruler / measurement overlay (active while the right button is held) ──
    enum class RulerSnap
    {
        Free,
        Horizontal,
        Vertical
    };
    bool m_rulerActive = false;
    RulerSnap m_rulerSnap = RulerSnap::Free;
    int m_rulerX0 = 0; // press position (screen px)
    int m_rulerY0 = 0;
    int m_rulerX1 = 0; // current cursor position (may be snapped)
    int m_rulerY1 = 0;
    int m_rulerRawX1 = 0; // raw cursor position (unsnapped)
    int m_rulerRawY1 = 0;

    // ── Crosshair cursor ──────────────────────────────────────────────
    bool m_crosshairEnabled = false;
    int m_crosshairX = -1; // screen px, -1 = not tracking
    int m_crosshairY = -1;
    bool m_useClockTime = false; // mirror of TimeRulerWidget time format

    // ── Scalebars ─────────────────────────────────────────────────────
    bool m_scalebarsVisible = false;

    // ── Butterfly mode ────────────────────────────────────────────────
    bool m_butterflyMode = false;

    struct ButterflyTypeGroup
    {
        std::string typeLabel;           // e.g. "MEG", "EEG"
        QColor color;                    // representative type colour
        float amplitudeMax;              // per-type amplitude scale
        std::vector<int> channelIndices; // model channel indices in this group
    };

    std::vector<ButterflyTypeGroup> butterflyTypeGroups() const;
    int butterflyLaneCount() const;
};

} // namespace mne::disp

#endif // DISP_CHANNELRHIRENDERER_H
