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
#include <QPointer>
#include <QPropertyAnimation>
#include <QtConcurrent>

#include <memory>
#include <vector>

#ifdef MNE_DISP_RHIWIDGET
#  include <QRhiWidget>
#  include <rhi/qrhi.h>
#endif

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
/**
 * @brief ChannelRhiView – GPU-accelerated (or QPainter-fallback) channel signal renderer.
 *
 * When compiled with Qt6 + GuiPrivate (MNE_DISP_RHIWIDGET defined) this widget
 * subclasses QRhiWidget and renders all channel traces via a custom GLSL pipeline
 * with min/max decimation.  Smooth scrolling and zoom animations are driven by
 * Q_PROPERTY so that QPropertyAnimation can be used directly.
 *
 * When MNE_DISP_RHIWIDGET is not defined the widget falls back to a QPainter
 * implementation that still applies min/max decimation for fast rendering.
 *
 * In both cases the public interface is identical.
 */
#ifdef MNE_DISP_RHIWIDGET
class DISPSHARED_EXPORT ChannelRhiView : public QRhiWidget
#else
class DISPSHARED_EXPORT ChannelRhiView : public QWidget
#endif
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
     * When @p hide is true the bad-channel traces are suppressed (rendered as flat
     * centre lines) so the renderer gives immediate visual feedback when the flag
     * is toggled.  The label panel collapses those rows simultaneously.
     *
     * @param[in] hide  true = suppress bad-channel traces.
     */
    void setHideBadChannels(bool hide);
    bool hideBadChannels() const { return m_hideBadChannels; }

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

protected:
#ifdef MNE_DISP_RHIWIDGET
    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;
    void releaseResources() override;
    void paintEvent(QPaintEvent *event) override; // QPainter fallback when RHI fails
#else
    void paintEvent(QPaintEvent *event) override;
#endif

    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    // ── GPU resource management (QRhi path only) ──────────────────────
#ifdef MNE_DISP_RHIWIDGET
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
#endif

    // ── Tile cache (QPainter paths) ────────────────────────────────────

    // Result produced by the async tile worker
    struct TileResult {
        QImage image;
        float  sampleFirst     = 0.f;
        float  samplesPerPixel = 0.f;
        int    firstChannel    = 0;
        int    visibleCount    = 0;
    };

    void scheduleTileRebuild();          // kick off async rebuild (non-blocking)
    static TileResult buildTile(        // pure function, runs on worker thread
        ChannelDataModel *model,
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
        const QVector<int> &channelIndices, // empty = identity (all channels)
        const QVector<EventMarker> &events,    // empty = no markers
        const QVector<AnnotationSpan> &annotations); // empty = no annotations
    bool isTileFresh() const;

    QImage m_tileImage;
    float  m_tileSampleFirst     = 0.f;
    float  m_tileSamplesPerPixel = 0.f;
    int    m_tileFirstChannel    = -1;
    int    m_tileVisibleCount    = 0;
    bool   m_tileDirty           = true;

    QFutureWatcher<TileResult>  m_tileWatcher;
    bool                        m_tileRebuildPending = false;

#ifdef MNE_DISP_RHIWIDGET

    std::unique_ptr<QRhiBuffer>                  m_ubo;
    std::unique_ptr<QRhiShaderResourceBindings>  m_srb;
    std::unique_ptr<QRhiGraphicsPipeline>        m_pipeline;
    std::vector<ChannelGpuData>                  m_gpuChannels;
    int                                          m_uboStride = 256;
    bool                                         m_pipelineDirty = true;
    bool                                         m_vboDirty      = true;
    bool                                         m_rhiFailed     = false; // set on renderFailed
#endif

    // ── State shared by both paths ─────────────────────────────────────
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

    // Helper — use instead of firstCh+i for actual model channel index
    int actualChannelAt(int logicalIdx) const; // maps logical → model channel index

    // ── Prefetch window tracking ───────────────────────────────────────
    int   m_vboWindowFirst  = 0;
    int   m_vboWindowLast   = 0;

    // ── Ruler / measurement overlay ───────────────────────────────────
    // Active while Shift+Left-button is held.
    bool  m_rulerActive   = false;
    int   m_rulerX0       = 0;   // press position (screen px)
    int   m_rulerY0       = 0;
    int   m_rulerX1       = 0;   // current cursor position
    int   m_rulerY1       = 0;

    void drawOverlays();   // QPainter-based overlays (bands + ruler) on top of both rendering paths
};

} // namespace DISPLIB

#endif // CHANNELRHIVIEW_H
