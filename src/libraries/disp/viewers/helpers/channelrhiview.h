//=============================================================================================================
/**
 * @file     channelrhiview.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Declaration of the ChannelRhiView class.
 *
 */

#ifndef CHANNELRHIVIEW_H
#define CHANNELRHIVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include "channeldatamodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPointer>
#include <QPropertyAnimation>
#include <QRhiWidget>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QtConcurrent>

#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

class QRhi;
class QRhiBuffer;
class QRhiCommandBuffer;
class QRhiGraphicsPipeline;
class QRhiResourceUpdateBatch;
class QRhiSampler;
class QRhiShaderResourceBindings;
class QRhiTexture;

class CrosshairOverlay;  // defined in .cpp

namespace DISPLIB
{

//=============================================================================================================
/**
 * @brief ChannelRhiView – QRhiWidget-based channel signal renderer.
 *
 * The raw browser uses QRhiWidget as its only supported rendering backend.
 * It renders channel traces via a custom GLSL pipeline with min/max decimation,
 * while smooth scrolling and zoom animations are driven by Q_PROPERTY so that
 * QPropertyAnimation can be used directly.
 */
class DISPSHARED_EXPORT ChannelRhiView : public QRhiWidget
{
    Q_OBJECT

    Q_PROPERTY(float scrollSample    READ scrollSample    WRITE setScrollSample    NOTIFY scrollSampleChanged)
    Q_PROPERTY(float samplesPerPixel READ samplesPerPixel WRITE setSamplesPerPixel NOTIFY samplesPerPixelChanged)

public:
    //=========================================================================================================
    /**
     * @brief Stimulus / event marker — a coloured vertical line at a given sample.
     */
    struct EventMarker {
        int     sample = 0;   ///< Absolute sample index of the event onset.
        int     type   = 1;   ///< Numeric event type (stimulus code).
        QColor  color;        ///< Display colour of the line and label chip.
        QString label;        ///< Short text label (shown at the bottom strip); defaults to type number.
    };

    //=========================================================================================================
    /**
     * @brief Time-span annotation overlay.
     */
    struct AnnotationSpan {
        int     startSample = 0; ///< Absolute first sample covered by the annotation.
        int     endSample   = 0; ///< Absolute last sample covered by the annotation.
        QColor  color;           ///< Fill / border colour for the highlighted span.
        QString label;           ///< Annotation label shown near the top edge.
    };

    explicit ChannelRhiView(QWidget *parent = nullptr);
    ~ChannelRhiView() override;

    //=========================================================================================================
    /**
     * Attach the data model.  The view does NOT take ownership.
     *
     * @param[in] model  The channel data model to render.
     */
    void setModel(ChannelDataModel *model);

    //=========================================================================================================
    /**
     * Set the list of stimulus / event markers to overlay on the traces.
     * Each marker is drawn as a coloured vertical line spanning all channel rows,
     * with a small colour-coded label chip at the bottom of the view.
     * Pass an empty vector to clear all markers.
     *
     * @param[in] events  List of EventMarker objects.
     */
    void setEvents(const QVector<EventMarker> &events);

    //=========================================================================================================
    /**
     * Set the list of time-span annotations to overlay on the traces.
     *
     * @param[in] annotations  List of AnnotationSpan objects.
     */
    void setAnnotations(const QVector<AnnotationSpan> &annotations);

    //=========================================================================================================
    /**
     * Enable or disable annotation range selection. When enabled, Shift+drag emits
     * sampleRangeSelected on mouse release instead of acting as a pure measurement tool.
     */
    void setAnnotationSelectionEnabled(bool enabled);

    void setEventsVisible(bool visible);
    bool eventsVisible() const;

    void setAnnotationsVisible(bool visible);
    bool annotationsVisible() const;

    // ── Scroll / zoom ─────────────────────────────────────────────────

    float scrollSample()    const { return m_scrollSample; }
    float samplesPerPixel() const { return m_samplesPerPixel; }

    //=========================================================================================================
    /**
     * Set the left-edge scroll position.  Updates only the GPU uniform —
     * no VBO rebuild unless the prefetch window is exhausted.
     *
     * @param[in] sample  Absolute sample index for the left viewport edge.
     */
    void setScrollSample(float sample);

    //=========================================================================================================
    /**
     * Set the horizontal zoom (samples per screen pixel).
     * Triggers a VBO rebuild with new decimation.
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
    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const { return m_bgColor; }

    //=========================================================================================================
    /**
     * How many samples to prefetch beyond the visible window on each side
     * (in multiples of the visible window).  Default = 1.0.
     *
     * @param[in] factor  Prefetch factor.
     */
    void setPrefetchFactor(float factor);

    //=========================================================================================================
    /**
     * Returns the absolute sample index currently shown at x = 0 (left edge).
     * (Same as scrollSample rounded to int.)
     */
    int visibleFirstSample() const;

    //=========================================================================================================
    /**
     * Returns the number of samples currently visible.
     */
    int visibleSampleCount() const;

    //=========================================================================================================
    /**
     * Restrict rendering to a specific subset of channel indices.
     * When @p indices is empty all channels in the model are shown (no filter).
     * When set, scrolling and visible-channel-count are relative to this subset.
     *
     * @param[in] indices  Ordered list of model channel indices to display.
     *                     Pass an empty vector to clear the filter.
     */
    void setChannelIndices(const QVector<int> &indices);

    //=========================================================================================================
    /**
     * Total number of logical channels available for scrolling (respects active filter).
     */
    int totalLogicalChannels() const;

    //=========================================================================================================
    /**
     * Set the index of the first visible channel row.
     *
     * @param[in] ch  Zero-based channel index (within the active filter if any).
     */
    void setFirstVisibleChannel(int ch);
    int  firstVisibleChannel() const { return m_firstVisibleChannel; }

    //=========================================================================================================
    /**
     * Set how many channel rows are displayed simultaneously.
     *
     * @param[in] count  Number of channels to show (clamped to 1–model count).
     */
    void setVisibleChannelCount(int count);
    int  visibleChannelCount() const { return m_visibleChannelCount; }

    //=========================================================================================================
    /**
     * Freeze or unfreeze drag-panning (mouse drag and inertia).
     * Wheel scroll and the scrollbar remain active when frozen.
     */
    void setFrozen(bool frozen);
    bool isFrozen() const { return m_frozen; }

    //=========================================================================================================
    /**
     * Show or hide the time and amplitude grid overlay.
     */
    void setGridVisible(bool visible);
    bool gridVisible() const { return m_gridVisible; }

    //=========================================================================================================
    /**
     * Set the sampling frequency (Hz) used to place time-grid tick lines.
     * Call this from ChannelDataView::init() after FiffInfo is available.
     */
    void setSfreq(float sfreq);

    //=========================================================================================================
    /**
     * Set the absolute sample index of the first sample in the file.
     * Used to align grid tick lines with the time ruler labels.
     *
     * @param[in] first  Absolute sample index of the file's first sample.
     */
    void setFirstFileSample(int first);

    //=========================================================================================================
    /**
     * Set the absolute sample index of the last sample in the file.
     * Used to clamp scrolling so mouse pan cannot exceed the file end.
     * Pass -1 (default) for unlimited.
     *
     * @param[in] last  Absolute sample index of the file's last sample.
     */
    void setLastFileSample(int last);
    int  lastFileSample() const { return m_lastFileSample; }

    //=========================================================================================================
    /**
     * Control what the vertical mouse wheel scrolls.
     * When @p channelsMode is true the vertical wheel scrolls through channels.
     * When false the vertical wheel scrolls through time (same as horizontal).
     *
     * @param[in] channelsMode  true = vertical wheel → channels, false → time.
     */
    void setWheelScrollsChannels(bool channelsMode);
    bool wheelScrollsChannels() const { return m_wheelScrollsChannels; }

    //=========================================================================================================
    /**
     * Show or hide waveform traces of channels marked bad.
     * When @p hide is true bad channels are removed from the logical channel list
     * used for scrolling and rendering so the trace lanes collapse upward in sync
     * with the label panel.
     *
     * @param[in] hide  true = remove bad channels from the visible trace list.
     */
    void setHideBadChannels(bool hide);
    bool hideBadChannels() const { return m_hideBadChannels; }

    // ── Crosshair ─────────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Enable or disable the crosshair cursor overlay.
     * When enabled, a vertical + horizontal cross follows the mouse and the view
     * continuously emits cursorDataChanged() with the time/amplitude under the cursor.
     */
    void setCrosshairEnabled(bool enabled);
    bool crosshairEnabled() const { return m_crosshairEnabled; }

    /**
     * Set whether the crosshair label uses clock time (mm:ss.ms) or seconds.
     */
    void setClockTimeFormat(bool useClock) { m_useClockTime = useClock; update(); }
    bool clockTimeFormat() const { return m_useClockTime; }

    // ── Scalebars ─────────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Show or hide per-channel-type scalebars in the bottom-right corner.
     */
    void setScalebarsVisible(bool visible);
    bool scalebarsVisible() const { return m_scalebarsVisible; }

    // ── Butterfly mode ────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Toggle butterfly mode: all channels of the same type are overlaid in a single lane.
     */
    void setButterflyMode(bool enabled);
    bool butterflyMode() const { return m_butterflyMode; }

signals:
    void scrollSampleChanged(float sample);
    void samplesPerPixelChanged(float spp);
    void viewResized(int newWidth, int newHeight);

    //=========================================================================================================
    /**
     * Emitted whenever the first visible channel index changes.
     */
    void channelOffsetChanged(int firstChannel);

    //=========================================================================================================
    /**
     * Emitted whenever the user clicks; provides the sample index under the cursor.
     */
    void sampleClicked(int sample);

    //=========================================================================================================
    /**
     * Emitted when the user selects a time span with Shift+drag in annotation mode.
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
    void cursorDataChanged(float timeSec, float amplitude,
                           const QString &channelName, const QString &unitLabel);

protected:
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;
    void releaseResources() override;
    void paintEvent(QPaintEvent *event) override;

    // Overlay access — called by CrosshairOverlay::paintEvent
    friend class ::CrosshairOverlay;
    void drawCrosshair(QPainter &p);
    void drawScalebars(QPainter &p);
    void drawRulerOverlay(QPainter &p);
    void emitCursorData();
    bool rulerActive() const { return m_rulerActive; }

    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    // ── GPU resource management ────────────────────────────────────────
    struct ChannelGpuData {
        std::unique_ptr<QRhiBuffer> vbo;
        int                         vertexCount  = 0;
        int                         vboFirstSample = 0; // absolute first sample in VBO
    };

    void ensurePipeline();
    void rebuildVBOs(QRhiResourceUpdateBatch *batch);
    void updateUBO(QRhiResourceUpdateBatch *batch);
    bool isVboDirty() const;

    // ── Overlay blit (bands + event lines baked into Metal texture) ───
    void ensureOverlayPipeline();
    void rebuildOverlayImage(int logicalWidth, int logicalHeight, qreal devicePixelRatio);

    std::unique_ptr<QRhiBuffer>                  m_overlayVbo;
    std::unique_ptr<QRhiTexture>                 m_overlayTex;
    std::unique_ptr<QRhiSampler>                 m_overlaySampler;
    std::unique_ptr<QRhiShaderResourceBindings>  m_overlaySrb;
    std::unique_ptr<QRhiGraphicsPipeline>        m_overlayPipeline;
    QImage                                       m_overlayImage;
    bool                                         m_overlayDirty = true;
    QSize                                        m_overlayTexSize;

    // ── Legacy async tile helpers retained for staging/reuse ──────────
    struct TileResult {
        QImage image;
        float  sampleFirst     = 0.f;
        float  samplesPerPixel = 0.f;
        int    firstChannel    = 0;
        int    visibleCount    = 0;
    };

    void scheduleTileRebuild();
    static TileResult buildTile(ChannelDataModel *model,
                                float scrollSample,
                                float samplesPerPixel,
                                int   firstVisibleChannel,
                                int   visibleChannelCount,
                                int   viewWidth, int viewHeight,
                                QColor bgColor,
                                bool  gridVisible,
                                float sfreq,
                                int   firstFileSample,
                                bool  hideBadChannels,
                                const QVector<int> &channelIndices,
                                const QVector<EventMarker> &events,
                                const QVector<AnnotationSpan> &annotations);
    bool isTileFresh() const;

    QImage m_tileImage;
    float  m_tileSampleFirst     = 0.f;
    float  m_tileSamplesPerPixel = 0.f;
    int    m_tileFirstChannel    = -1;
    int    m_tileVisibleCount    = 0;
    bool   m_tileDirty           = true;

    QFutureWatcher<TileResult>  m_tileWatcher;
    bool                        m_tileRebuildPending = false;

    std::unique_ptr<QRhiBuffer>                  m_ubo;
    std::unique_ptr<QRhiShaderResourceBindings>  m_srb;
    std::unique_ptr<QRhiGraphicsPipeline>        m_pipeline;
    std::vector<ChannelGpuData>                  m_gpuChannels;
    int                                          m_uboStride = 256;
    bool                                         m_pipelineDirty = true;
    bool                                         m_vboDirty      = true;

    // ── Shared browser state ───────────────────────────────────────────
    QPointer<ChannelDataModel> m_model;
    float                      m_scrollSample     = 0.f;
    float                      m_samplesPerPixel  = 1.f;
    float                      m_prefetchFactor   = 1.0f;
    QColor                     m_bgColor          { 250, 250, 250 }; // light default

    bool   m_frozen               = false;
    bool   m_gridVisible          = true;
    float  m_sfreq                = 1000.f;
    int    m_firstFileSample      = 0;
    int    m_lastFileSample       = -1;   // -1 = no limit (file not yet known)
    bool   m_wheelScrollsChannels = true; // default: vertical wheel → channels
    bool   m_hideBadChannels      = false;

    // ── Vertical channel windowing ────────────────────────────────────
    int   m_firstVisibleChannel  = 0;
    int   m_visibleChannelCount  = 12;

    // ── Drag scroll support ────────────────────────────────────────────
    bool  m_dragging        = false;
    int   m_dragStartX      = 0;
    float m_dragStartScroll = 0.f;

    // ── Left-button drag (panning) + inertial scroll ──────────────────
    bool  m_leftButtonDown    = false;
    int   m_leftDownX         = 0;
    float m_leftDownScroll    = 0.f;
    bool  m_leftDragActivated = false;

    struct VelocitySample { int x; qint64 t; };
    QVector<VelocitySample> m_velocityHistory;
    QElapsedTimer           m_dragTimer;
    QPropertyAnimation*     m_pInertialAnim = nullptr;

    // ── Channel index filter ──────────────────────────────────────────
    // When non-empty, only these model channel indices are rendered/scrolled.
    QVector<int> m_filteredChannels;

    // ── Event / stimulus markers ──────────────────────────────────────
    QVector<EventMarker> m_events;
    QVector<AnnotationSpan> m_annotations;
    bool m_annotationSelectionEnabled = false;
    bool m_bShowEvents      = true;
    bool m_bShowAnnotations = true;

    // ── Annotation boundary drag-resize ───────────────────────────────
    bool m_annDragging         = false;   ///< True while dragging an annotation boundary.
    int  m_annDragIndex        = -1;      ///< Index of the annotation being resized.
    bool m_annDragIsStart      = true;    ///< True if dragging the start boundary.
    int  m_annHoverIndex       = -1;      ///< Index of annotation whose boundary is under cursor.
    bool m_annHoverIsStart     = true;    ///< True if cursor is near the start boundary.
    static constexpr int kAnnBoundaryHitPx = 5; ///< Hit-test tolerance in pixels.

    int hitTestAnnotationBoundary(int px, bool &isStart) const;

    // Helper — use instead of firstCh+i for actual model channel index
    int actualChannelAt(int logicalIdx) const; // maps logical → model channel index
    QVector<int> effectiveChannelIndices() const;

    // ── Prefetch window tracking ───────────────────────────────────────
    int   m_vboWindowFirst  = 0;
    int   m_vboWindowLast   = 0;

    // ── Ruler / measurement overlay ───────────────────────────────────
    // Active while right-button is held.
    enum class RulerSnap { Free, Horizontal, Vertical };
    bool       m_rulerActive = false;
    RulerSnap  m_rulerSnap   = RulerSnap::Free;
    int   m_rulerX0       = 0;   // press position (screen px)
    int   m_rulerY0       = 0;
    int   m_rulerX1       = 0;   // current cursor position (may be snapped)
    int   m_rulerY1       = 0;
    int   m_rulerRawX1    = 0;   // raw cursor position (unsnapped)
    int   m_rulerRawY1    = 0;

    // ── Crosshair cursor ──────────────────────────────────────────────
    bool  m_crosshairEnabled = false;
    int   m_crosshairX       = -1;  // screen px, -1 = not tracking
    int   m_crosshairY       = -1;
    bool  m_useClockTime     = false;  // mirror of TimeRulerWidget time format
    CrosshairOverlay* m_overlay = nullptr;  // lightweight overlay for crosshair/scalebars

    // ── Scalebars ─────────────────────────────────────────────────────
    bool  m_scalebarsVisible = false;

    // ── Butterfly mode ────────────────────────────────────────────────
    bool  m_butterflyMode = false;

    struct ButterflyTypeGroup {
        QString typeLabel;         // e.g. "MEG", "EEG"
        QColor  color;             // representative type colour
        float   amplitudeMax;      // per-type amplitude scale
        QVector<int> channelIndices; // model channel indices in this group
    };

    QVector<ButterflyTypeGroup> butterflyTypeGroups() const;
    int butterflyLaneCount() const;

    void drawOverlays();   // QPainter-based overlays on top of the QRHI-rendered traces
};

} // namespace DISPLIB

#endif // CHANNELRHIVIEW_H
