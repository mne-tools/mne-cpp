//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channelrhiview.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    QRhi-based GPU-accelerated channel time-series renderer used by the modern raw browser.
 *
 * ChannelRhiView is the @c QRhiWidget host of the Widgets-free @ref mne::disp::ChannelRhiRenderer, which
 * holds the view state, uploads channel data as vertex buffers, draws every visible trace and paints the
 * crosshair, scalebar and ruler overlay in the same GPU pass. The widget forwards its input events and
 * frames to the renderer and exposes the API used by @ref mne::disp::ChannelDataView to push new samples,
 * change scaling or update the visible window. Applications without Qt Widgets can use
 * @ref mne::disp::ChannelWindow instead.
 */

#ifndef CHANNELRHIVIEW_H
#define CHANNELRHIVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_widgets_global.h"
#include <mne/disp/viewers/helpers/channeldatamodel.h>
#include <mne/disp/viewers/helpers/channelrhirenderer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QColor>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QRhiWidget>
#include <QResizeEvent>
#include <QWheelEvent>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

class QRhi;
class QRhiCommandBuffer;
class QRhiRenderTarget;

namespace mne::disp
{

//=============================================================================================================
/**
 * @brief QRhi-based GPU-accelerated channel time-series renderer used by the modern raw browser.
 *
 * Thin @c QRhiWidget host of a @ref ChannelRhiRenderer; frames and input events are forwarded to the
 * renderer and its signals are re-emitted, so the widget keeps the API used by @ref ChannelDataView.
 */
class DISPWIDGETSSHARED_EXPORT ChannelRhiView : public QRhiWidget
{
    Q_OBJECT

    Q_PROPERTY(float scrollSample READ scrollSample WRITE setScrollSample NOTIFY scrollSampleChanged)
    Q_PROPERTY(float samplesPerPixel READ samplesPerPixel WRITE setSamplesPerPixel NOTIFY samplesPerPixelChanged)

public:
    using EventMarker = ChannelRhiRenderer::EventMarker;       ///< Stimulus / event marker.
    using AnnotationSpan = ChannelRhiRenderer::AnnotationSpan; ///< Time-span annotation.

    explicit ChannelRhiView(QWidget* parent = nullptr);
    ~ChannelRhiView() override;

    //=========================================================================================================
    /**
     * Returns the Widgets-free renderer that holds the view state, the GPU resources and the input handling.
     *
     * @since 3.0.0
     *
     * @return The renderer owned by this widget.
     */
    ChannelRhiRenderer* renderer() const
    {
        return m_pRenderer.get();
    }

    //=========================================================================================================
    /**
     * Attach the data model.  The view does NOT take ownership.
     *
     * @param[in] model  The channel data model to render.
     */
    void setModel(ChannelDataModel* model)
    {
        m_pRenderer->setModel(model);
    }

    //=========================================================================================================
    /**
     * Set the list of stimulus / event markers to overlay on the traces.
     * Each marker is drawn as a coloured vertical line spanning all channel rows,
     * with a small colour-coded label chip at the bottom of the view.
     * Pass an empty vector to clear all markers.
     *
     * @param[in] events  List of EventMarker objects.
     */
    void setEvents(const std::vector<EventMarker>& events)
    {
        m_pRenderer->setEvents(events);
    }

    void setEpochMarkers(const std::vector<int>& triggerSamples)
    {
        m_pRenderer->setEpochMarkers(triggerSamples);
    }
    void setEpochMarkersVisible(bool visible)
    {
        m_pRenderer->setEpochMarkersVisible(visible);
    }
    bool epochMarkersVisible() const
    {
        return m_pRenderer->epochMarkersVisible();
    }

    void setClippingVisible(bool visible)
    {
        m_pRenderer->setClippingVisible(visible);
    }
    bool clippingVisible() const
    {
        return m_pRenderer->clippingVisible();
    }

    void setZScoreMode(bool enabled)
    {
        m_pRenderer->setZScoreMode(enabled);
    }
    bool zScoreMode() const
    {
        return m_pRenderer->zScoreMode();
    }

    //=========================================================================================================
    /**
     * Set the list of time-span annotations to overlay on the traces.
     *
     * @param[in] annotations  List of AnnotationSpan objects.
     */
    void setAnnotations(const std::vector<AnnotationSpan>& annotations)
    {
        m_pRenderer->setAnnotations(annotations);
    }

    //=========================================================================================================
    /**
     * Enable or disable annotation range selection. When enabled, Shift+drag emits
     * sampleRangeSelected on mouse release instead of acting as a pure measurement tool.
     *
     * @param[in] enabled  True to make Shift+drag select annotation ranges.
     */
    void setAnnotationSelectionEnabled(bool enabled)
    {
        m_pRenderer->setAnnotationSelectionEnabled(enabled);
    }

    void setEventsVisible(bool visible)
    {
        m_pRenderer->setEventsVisible(visible);
    }
    bool eventsVisible() const
    {
        return m_pRenderer->eventsVisible();
    }

    void setAnnotationsVisible(bool visible)
    {
        m_pRenderer->setAnnotationsVisible(visible);
    }
    bool annotationsVisible() const
    {
        return m_pRenderer->annotationsVisible();
    }

    // ── Scroll / zoom ─────────────────────────────────────────────────

    float scrollSample() const
    {
        return m_pRenderer->scrollSample();
    }
    float samplesPerPixel() const
    {
        return m_pRenderer->samplesPerPixel();
    }

    //=========================================================================================================
    /**
     * Set the left-edge scroll position.  Updates only the GPU uniform —
     * no VBO rebuild unless the prefetch window is exhausted.
     *
     * @param[in] sample  Absolute sample index for the left viewport edge.
     */
    void setScrollSample(float sample)
    {
        m_pRenderer->setScrollSample(sample);
    }

    //=========================================================================================================
    /**
     * Set the horizontal zoom (samples per screen pixel).
     * Triggers a VBO rebuild with new decimation.
     *
     * @param[in] spp  Samples per pixel (> 1 = zoomed out, < 1 = zoomed in).
     */
    void setSamplesPerPixel(float spp)
    {
        m_pRenderer->setSamplesPerPixel(spp);
    }

    //=========================================================================================================
    /**
     * Smoothly animate the scroll position.
     *
     * @param[in] targetSample  Target left-edge sample index.
     * @param[in] durationMs    Animation duration in milliseconds (0 = instant).
     */
    void scrollTo(float targetSample, int durationMs = 200)
    {
        m_pRenderer->scrollTo(targetSample, durationMs);
    }

    //=========================================================================================================
    /**
     * Smoothly scroll by a number of samples. While a scroll animation runs, the step continues from its
     * target, so repeated wheel ticks or key presses add up.
     *
     * @param[in] deltaSamples  Samples to scroll (negative = earlier).
     * @param[in] durationMs    Animation duration in milliseconds (0 = instant).
     */
    void scrollBy(float deltaSamples, int durationMs = 200)
    {
        m_pRenderer->scrollBy(deltaSamples, durationMs);
    }

    //=========================================================================================================
    /**
     * Smoothly animate the zoom level.
     *
     * @param[in] targetSpp   Target samples-per-pixel.
     * @param[in] durationMs  Animation duration in milliseconds (0 = instant).
     */
    void zoomTo(float targetSpp, int durationMs = 200)
    {
        m_pRenderer->zoomTo(targetSpp, durationMs);
    }

    //=========================================================================================================
    /**
     * Background colour of the render surface.
     *
     * @param[in] color  The new background colour.
     */
    void setBackgroundColor(const QColor& color)
    {
        m_pRenderer->setBackgroundColor(color);
    }
    QColor backgroundColor() const
    {
        return m_pRenderer->backgroundColor();
    }

    //=========================================================================================================
    /**
     * How many samples to prefetch beyond the visible window on each side
     * (in multiples of the visible window).  Default = 1.0.
     *
     * @param[in] factor  Prefetch factor.
     */
    void setPrefetchFactor(float factor)
    {
        m_pRenderer->setPrefetchFactor(factor);
    }

    //=========================================================================================================
    /**
     * Returns the absolute sample index currently shown at x = 0 (left edge).
     * (Same as scrollSample rounded to int.)
     *
     * @return Absolute sample index at the left viewport edge.
     */
    int visibleFirstSample() const
    {
        return m_pRenderer->visibleFirstSample();
    }

    //=========================================================================================================
    /**
     * Returns the number of samples currently visible.
     *
     * @return Widget width in pixels times samples per pixel, truncated to int.
     */
    int visibleSampleCount() const
    {
        return m_pRenderer->visibleSampleCount();
    }

    //=========================================================================================================
    /**
     * Restrict rendering to a specific subset of channel indices.
     * When @p indices is empty all channels in the model are shown (no filter).
     * When set, scrolling and visible-channel-count are relative to this subset.
     *
     * @param[in] indices  Ordered list of model channel indices to display.
     *                     Pass an empty vector to clear the filter.
     */
    void setChannelIndices(const std::vector<int>& indices)
    {
        m_pRenderer->setChannelIndices(indices);
    }

    //=========================================================================================================
    /**
     * Total number of logical channels available for scrolling (respects active filter).
     *
     * @return Number of channels after applying the index filter and bad-channel hiding.
     */
    int totalLogicalChannels() const
    {
        return m_pRenderer->totalLogicalChannels();
    }

    //=========================================================================================================
    /**
     * Set the index of the first visible channel row.
     *
     * @param[in] ch  Zero-based channel index (within the active filter if any).
     */
    void setFirstVisibleChannel(int ch)
    {
        m_pRenderer->setFirstVisibleChannel(ch);
    }
    int firstVisibleChannel() const
    {
        return m_pRenderer->firstVisibleChannel();
    }

    //=========================================================================================================
    /**
     * Set how many channel rows are displayed simultaneously.
     *
     * @param[in] count  Number of channels to show (clamped to 1–model count).
     */
    void setVisibleChannelCount(int count)
    {
        m_pRenderer->setVisibleChannelCount(count);
    }
    int visibleChannelCount() const
    {
        return m_pRenderer->visibleChannelCount();
    }

    //=========================================================================================================
    /**
     * Freeze or unfreeze drag-panning (mouse drag and inertia).
     * Wheel scroll and the scrollbar remain active when frozen.
     *
     * @param[in] frozen  True to disable drag-panning.
     */
    void setFrozen(bool frozen)
    {
        m_pRenderer->setFrozen(frozen);
    }
    bool isFrozen() const
    {
        return m_pRenderer->isFrozen();
    }

    //=========================================================================================================
    /**
     * Show or hide the time and amplitude grid overlay.
     *
     * @param[in] visible  True to draw the grid.
     */
    void setGridVisible(bool visible)
    {
        m_pRenderer->setGridVisible(visible);
    }
    bool gridVisible() const
    {
        return m_pRenderer->gridVisible();
    }

    //=========================================================================================================
    /**
     * Set the sampling frequency (Hz) used to place time-grid tick lines.
     * Call this from ChannelDataView::init() after FiffInfo is available.
     *
     * @param[in] sfreq  Sampling frequency in Hz.
     */
    void setSfreq(float sfreq)
    {
        m_pRenderer->setSfreq(sfreq);
    }

    //=========================================================================================================
    /**
     * Set the absolute sample index of the first sample in the file.
     * Used to align grid tick lines with the time ruler labels.
     *
     * @param[in] first  Absolute sample index of the file's first sample.
     */
    void setFirstFileSample(int first)
    {
        m_pRenderer->setFirstFileSample(first);
    }

    //=========================================================================================================
    /**
     * Set the absolute sample index of the last sample in the file.
     * Used to clamp scrolling so mouse pan cannot exceed the file end.
     * Pass -1 (default) for unlimited.
     *
     * @param[in] last  Absolute sample index of the file's last sample.
     */
    void setLastFileSample(int last)
    {
        m_pRenderer->setLastFileSample(last);
    }
    int lastFileSample() const
    {
        return m_pRenderer->lastFileSample();
    }

    //=========================================================================================================
    /**
     * Control what the vertical mouse wheel scrolls.
     * When @p channelsMode is true the vertical wheel scrolls through channels.
     * When false the vertical wheel scrolls through time (same as horizontal).
     *
     * @param[in] channelsMode  true = vertical wheel → channels, false → time.
     */
    void setWheelScrollsChannels(bool channelsMode)
    {
        m_pRenderer->setWheelScrollsChannels(channelsMode);
    }
    bool wheelScrollsChannels() const
    {
        return m_pRenderer->wheelScrollsChannels();
    }

    void setScrollSpeedFactor(float factor)
    {
        m_pRenderer->setScrollSpeedFactor(factor);
    }
    float scrollSpeedFactor() const
    {
        return m_pRenderer->scrollSpeedFactor();
    }

    //=========================================================================================================
    /**
     * Show or hide waveform traces of channels marked bad.
     * When @p hide is true bad channels are removed from the logical channel list
     * used for scrolling and rendering so the trace lanes collapse upward in sync
     * with the label panel.
     *
     * @param[in] hide  true = remove bad channels from the visible trace list.
     */
    void setHideBadChannels(bool hide)
    {
        m_pRenderer->setHideBadChannels(hide);
    }
    bool hideBadChannels() const
    {
        return m_pRenderer->hideBadChannels();
    }

    // ── Crosshair ─────────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Enable or disable the crosshair cursor overlay.
     * When enabled, a vertical + horizontal cross follows the mouse and the view
     * continuously emits cursorDataChanged() with the time/amplitude under the cursor.
     *
     * @param[in] enabled  True to show the crosshair overlay.
     */
    void setCrosshairEnabled(bool enabled)
    {
        m_pRenderer->setCrosshairEnabled(enabled);
    }
    bool crosshairEnabled() const
    {
        return m_pRenderer->crosshairEnabled();
    }

    /**
     * Set whether the crosshair label uses clock time (mm:ss.ms) or seconds.
     *
     * @param[in] useClock  True for clock time, false for float seconds.
     */
    void setClockTimeFormat(bool useClock)
    {
        m_pRenderer->setClockTimeFormat(useClock);
    }
    bool clockTimeFormat() const
    {
        return m_pRenderer->clockTimeFormat();
    }

    // ── Export ────────────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Render the current view (background bands, grid, annotations, traces, event and epoch
     * markers) into an image with QPainter, without the GPU. Rows, scaling, butterfly
     * layout, z-score mode and clipping highlight match the on-screen rendering, so this
     * also works offscreen, e.g. for reports.
     *
     * @param[in] size  Output size in pixels; the visible time span is stretched to fit.
     *                  An invalid size (default) uses the widget size.
     *
     * @return The rendered image (only the background if no channel is shown), or a null
     *         image if there is no model or the size is empty.
     */
    QImage renderToImage(const QSize& size = QSize()) const
    {
        return m_pRenderer->renderToImage(size);
    }

    // ── Scalebars ─────────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Show or hide per-channel-type scalebars in the bottom-right corner.
     *
     * @param[in] visible  True to draw the scalebars.
     */
    void setScalebarsVisible(bool visible)
    {
        m_pRenderer->setScalebarsVisible(visible);
    }
    bool scalebarsVisible() const
    {
        return m_pRenderer->scalebarsVisible();
    }

    // ── Butterfly mode ────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Toggle butterfly mode: all channels of the same type are overlaid in a single lane.
     *
     * @param[in] enabled  True to enable butterfly mode.
     */
    void setButterflyMode(bool enabled)
    {
        m_pRenderer->setButterflyMode(enabled);
    }
    bool butterflyMode() const
    {
        return m_pRenderer->butterflyMode();
    }

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
                           const QString& channelName, const QString& unitLabel);

protected:
    void initialize(QRhiCommandBuffer* cb) override;
    void render(QRhiCommandBuffer* cb) override;
    void releaseResources() override;
    void paintEvent(QPaintEvent* event) override;

    //=========================================================================================================
    /**
     * Records one frame into @p target; render() calls it with the widget's own QRhi and render target.
     *
     * @param[in] rhi     The QRhi that owns @p target and the view's GPU resources.
     * @param[in] target  Render target sized like the widget times its device pixel ratio.
     * @param[in] cb      Command buffer of the current frame.
     */
    void renderFrame(QRhi* rhi, QRhiRenderTarget* target, QRhiCommandBuffer* cb);

    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    std::unique_ptr<ChannelRhiHost> m_pHost;         // widget services for the renderer
    std::unique_ptr<ChannelRhiRenderer> m_pRenderer; // view state, GPU resources and input handling
};

} // namespace mne::disp

#endif // CHANNELRHIVIEW_H
