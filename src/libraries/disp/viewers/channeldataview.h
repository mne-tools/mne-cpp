//=============================================================================================================
/**
 * @file     channeldataview.h
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
 * @brief    Declaration of the ChannelDataView class.
 *
 */

#ifndef CHANNELDATAVIEW_H
#define CHANNELDATAVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>
#include <QRect>
#include <QSharedPointer>
#include <QStringList>
#include <QVector>

#include "helpers/channelrhiview.h"
#include "helpers/overviewbarwidget.h"
#include "helpers/timerulerwidget.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QScrollBar;
class QSplitter;
class QToolButton;

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class ChannelDataModel;
class ChannelLabelPanel;
class TimeRulerWidget;

//=============================================================================================================
/**
 * @brief ChannelDataView – high-performance, GPU-accelerated channel signal viewer.
 *
 * Drop-in companion to RtFiffRawView that replaces the QTableView + RawDelegate + QOpenGLWidget
 * stack with a single QRhiWidget-based rendering surface.
 *
 * Key features:
 *  - GPU rendering on all platforms (OpenGL/ES, Metal, Vulkan, D3D12, WebGL2 / WASM)
 *  - Min/max decimation: constant rendering cost regardless of zoom level or file size
 *  - Smooth scroll and zoom animations via QPropertyAnimation
 *  - Drag-to-scroll with middle mouse button or Alt + left drag
 *  - Ctrl + mouse wheel zoom
 *  - Channel-type-aware colours and amplitude scales
 *  - Compatible with RtFiffRawView's public API for easy migration
 *
 * Typical usage:
 * @code
 *   auto *view = new ChannelDataView(this);
 *   view->init(fiffInfo);
 *   view->addData(matrix);            // Eigen::MatrixXd, channels × samples
 *   view->setWindowSize(5);           // 5-second display window
 *   // ... or:
 *   view->scrollToSample(1000, true); // smooth animate
 * @endcode
 */
class DISPSHARED_EXPORT ChannelDataView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<ChannelDataView>       SPtr;
    typedef QSharedPointer<const ChannelDataView> ConstSPtr;

    //=========================================================================================================
    /**
     * Constructs a ChannelDataView.
     *
     * @param[in] sSettingsPath  QSettings prefix for persistent GUI settings.
     * @param[in] parent         Parent widget.
     * @param[in] f              Window flags.
     */
    explicit ChannelDataView(const QString &sSettingsPath = QString(),
                             QWidget *parent = nullptr,
                             Qt::WindowFlags f = Qt::Widget);

    ~ChannelDataView() override;

    // ── Initialisation ────────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Initialise the view from a FiffInfo (channel names, types, sampling rate).
     *
     * @param[in] pInfo  Shared pointer to FiffInfo.
     */
    void init(QSharedPointer<FIFFLIB::FiffInfo> pInfo);

    // ── Data management ───────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Replace all buffered data.
     *
     * @param[in] data         Channels × samples matrix.
     * @param[in] firstSample  Absolute sample index of column 0.
     */
    void setData(const Eigen::MatrixXd &data, int firstSample = 0);

    //=========================================================================================================
    /**
     * Append new samples (real-time streaming use-case).
     *
     * @param[in] data  Channels × new-samples matrix.
     */
    void addData(const Eigen::MatrixXd &data);

    // ── Scroll / zoom API ─────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Scroll to the given absolute sample index.
     *
     * @param[in] sample   Target left-edge sample.
     * @param[in] animate  If true, animate with a smooth cubic ease-out.
     */
    void scrollToSample(int sample, bool animate = true);

    //=========================================================================================================
    /**
     * Set the visible time window (in seconds).
     * This adjusts samplesPerPixel to fit the requested duration.
     *
     * @param[in] seconds  Duration of the visible window.
     */
    void setWindowSize(float seconds);

    //=========================================================================================================
    /**
     * Get the current visible time window in seconds.
     */
    float windowSize() const;

    //=========================================================================================================
    /**
     * Set the zoom factor (> 1 = zoom in, < 1 = zoom out relative to default).
     *
     * @param[in] factor  Zoom multiplier applied to the default samples-per-pixel.
     */
    void setZoom(double factor);

    //=========================================================================================================
    /**
     * Return the current zoom factor.
     */
    double zoom() const;

    // ── Visual properties ─────────────────────────────────────────────

    //=========================================================================================================
    /**
     * Set the background colour of the rendering surface.
     *
     * @param[in] color  New background colour.
     */
    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const;

    //=========================================================================================================
    /**
     * Set the default signal line colour (used for channel types without a type-specific colour).
     *
     * @param[in] color  New signal colour.
     */
    void setSignalColor(const QColor &color);
    QColor signalColor() const;

    //=========================================================================================================
    /**
     * Set the per-channel-type amplitude scale map.
     * Keys should be FIFF channel kind constants (FIFFV_MEG_CH, FIFFV_EEG_CH, …).
     *
     * @param[in] scaleMap  Map from FIFF kind to physical amplitude (e.g. 1.2e-12 for MEG).
     */
    void setScalingMap(const QMap<qint32, float> &scaleMap);
    QMap<qint32, float> scalingMap() const;

    //=========================================================================================================
    /**
     * Show or hide channels marked bad.
     *
     * @param[in] hide  If true, bad channels are collapsed to zero height.
     */
    void hideBadChannels(bool hide);
    bool badChannelsHidden() const;

    //=========================================================================================================
    /**
     * Restrict the view to a named subset of channels.
     * Channels not in @p names are hidden immediately.
     * Pass an empty list to restore all channels.
     *
     * @param[in] names  List of channel names to show (case-sensitive).
     */
    void setChannelFilter(const QStringList &names);

    //=========================================================================================================
    /**
     * Enable or disable DC (mean) removal applied at render time.
     * Takes effect immediately — the view redraws with the current data.
     *
     * @param[in] dc  true = subtract per-channel mean from each rendered window.
     */
    void setRemoveDC(bool dc);

    //=========================================================================================================
    /**
     * Set the absolute sample indices of the file's first and last samples.
     * The scrollbar range and mouse-pan clamp use these to prevent scrolling
     * outside the actual file boundaries regardless of the ring-buffer state.
     *
     * @param[in] first  Absolute sample index of the file's first sample.
     * @param[in] last   Absolute sample index of the file's last sample.
     */
    void setFileBounds(int first, int last);

    //=========================================================================================================
    /**
     * Set the list of event / stimulus markers to display as coloured vertical lines.
     * Pass an empty vector to clear all markers.
     *
     * @param[in] events  List of EventMarker objects.
     */
    void setEvents(const QVector<ChannelRhiView::EventMarker> &events);

    //=========================================================================================================
    /**
     * Set the list of persistent sample/reference markers shown in the time ruler.
     *
     * @param[in] markers  Reference markers to display in the ruler.
     */
    void setReferenceMarkers(const QVector<TimeRulerReferenceMark> &markers);

    //=========================================================================================================
    /**
     * Set the list of annotation spans to display as translucent overlays.
     *
     * @param[in] annotations  List of AnnotationSpan objects.
     */
    void setAnnotations(const QVector<ChannelRhiView::AnnotationSpan> &annotations);

    //=========================================================================================================
    /**
     * Enable or disable annotation span selection in the raw browser.
     */
    void setAnnotationSelectionEnabled(bool enabled);

    // ── Interactive inspection features ───────────────────────────────

    //=========================================================================================================
    /**
     * Enable or disable the crosshair cursor with coordinate readout.
     */
    void setCrosshairEnabled(bool enabled);
    bool crosshairEnabled() const;

    //=========================================================================================================
    /**
     * Show or hide per-channel-type amplitude scalebars.
     */
    void setScalebarsVisible(bool visible);
    bool scalebarsVisible() const;

    //=========================================================================================================
    /**
     * Toggle butterfly mode (overlay all same-type channels).
     */
    void setButterflyMode(bool enabled);
    bool butterflyMode() const;

    //=========================================================================================================
    /**
     * Toggle time format between float seconds and HH:MM:SS clock time.
     */
    void toggleTimeFormat();

    //=========================================================================================================
    /**
     * Set clock time format on the ruler.
     */
    void setClockTimeFormat(bool useClock);
    bool clockTimeFormat() const;

    // ── AbstractView overrides ────────────────────────────────────────
    void saveSettings() override;
    void loadSettings() override;
    void clearView()    override;

    //=========================================================================================================
    /**
     * Returns the first currently visible sample.
     */
    int firstVisibleSample() const;
    int visibleSampleCount() const;

    //=========================================================================================================
    /**
     * Returns the geometry of the actual QRHI signal viewport in ChannelDataView-local coordinates.
     */
    QRect signalViewportRect() const;

    //=========================================================================================================
    /**
     * Maps an absolute sample index to an x coordinate inside the signal viewport.
     *
     * @param[in] sample  Absolute sample index.
     * @return x position in ChannelDataView-local coordinates.
     */
    int sampleToViewportX(int sample) const;

    //=========================================================================================================
    /**
     * Maps a ChannelDataView-local x coordinate inside the signal viewport to an absolute sample index.
     *
     * @param[in] x  ChannelDataView-local x coordinate.
     * @return Absolute sample index at the requested x position.
     */
    int viewportXToSample(int x) const;

    //=========================================================================================================
    /**
     * Returns the underlying data model (non-owning pointer).
     * Use for advanced configuration such as setMaxStoredSamples().
     */
    ChannelDataModel* model() const { return m_pModel.data(); }

signals:
    //=========================================================================================================
    /**
     * Emitted when the user clicks on a sample position.
     *
     * @param[in] sample  Absolute sample index under the cursor.
     */
    void sampleClicked(int sample);

    //=========================================================================================================
    /**
     * Emitted whenever the scroll position changes.
     *
     * @param[in] sample  Current left-edge sample index.
     */
    void scrollPositionChanged(int sample);

    //=========================================================================================================
    /**
     * Emitted when the user selected a sample range for annotation creation.
     */
    void sampleRangeSelected(int startSample, int endSample);

    //=========================================================================================================
    /**
     * Emitted when the user drags an annotation boundary to a new position.
     */
    void annotationBoundaryMoved(int annotationIndex, bool isStartBoundary, int newSample);

    //=========================================================================================================
    /**
     * Emitted when the ruler context menu requests a new sample marker.
     */
    void referenceMarkerAddRequested(int sample);

    //=========================================================================================================
    /**
     * Emitted when the ruler context menu requests removal of a nearby sample marker.
     */
    void referenceMarkerRemoveRequested(int sample);

    //=========================================================================================================
    /**
     * Emitted when the ruler context menu requests clearing all sample markers.
     */
    void referenceMarkersClearRequested();

    void crosshairToggled(bool on);
    void butterflyToggled(bool on);
    void scalebarsToggled(bool on);

    //=========================================================================================================
    /**
     * Forwarded from the underlying ChannelRhiView when the crosshair is active.
     */
    void cursorDataChanged(float timeSec, float amplitude,
                           const QString &channelName, const QString &unitLabel);

protected:
    void updateGuiMode(GuiMode mode) override;
    void updateProcessingMode(ProcessingMode mode) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onScrollBarMoved(int value);
    void onRhiScrollChanged(float sample);
    void updateScrollBarRange();
    void onChannelScrollBarMoved(int value);
    void onChannelOffsetChanged(int firstChannel);
    void updateChannelScrollBarRange();

private:
    void setupLayout();
    void updateSamplesPerPixel();

    QString                          m_sSettingsPath;
    QSharedPointer<ChannelDataModel> m_pModel;
    ChannelLabelPanel*               m_pLabelPanel          = nullptr;
    ChannelRhiView*                  m_pRhiView             = nullptr;
    TimeRulerWidget*                 m_pTimeRuler           = nullptr;
    OverviewBarWidget*               m_pOverviewBar         = nullptr;
    QWidget*                         m_pRulerHeader         = nullptr;
    QScrollBar*                      m_pScrollBar           = nullptr;
    QScrollBar*                      m_pChannelScrollBar    = nullptr;
    QToolButton*                     m_pScrollModeButton    = nullptr;

    bool m_channelScrollBarUpdating = false;

    QSharedPointer<FIFFLIB::FiffInfo> m_pFiffInfo;
    QMap<qint32, float>               m_scaleMap;
    QColor                            m_signalColor { Qt::darkGreen };
    QColor                            m_bgColor     { 250, 250, 250 };

    float  m_windowSizeSeconds  = 10.f;
    double m_zoomFactor         = 1.0;
    bool   m_hideBadChannels    = false;
    bool   m_scrollBarUpdating  = false; // re-entrance guard

    int    m_firstFileSample    = -1;    // -1 = not yet set (fall back to model)
    int    m_lastFileSample     = -1;    // -1 = not yet set
};

} // namespace DISPLIB

#endif // CHANNELDATAVIEW_H
