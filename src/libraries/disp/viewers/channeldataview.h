//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     channeldataview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Composite real-time multi-channel time-series scroller (label panel, table view, time ruler, scrollbars).
 *
 * ChannelDataView assembles the four widgets that make up the rolling
 * raw-data browser: a @ref ChannelLabelPanel column on the left, a
 * central @c QTableView driven by @ref ChannelDataModel and
 * @ref RtFiffRawViewDelegate, a @ref TimeRulerWidget at the bottom
 * and synchronised vertical / horizontal @c QScrollBars. It exposes
 * playback controls, zoom and channel-selection signals that the
 * hosting Quick-Control panels feed into.
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

enum class DetrendMode;
class ChannelDataModel;
class ChannelLabelPanel;
class TimeRulerWidget;

//=============================================================================================================
/**
 * @brief Composite real-time multi-channel raw scroller assembling label panel, table view, time ruler and scrollbars.
 *
 * Acts as the user-facing host of the @ref ChannelDataModel and
 * @ref RtFiffRawViewDelegate pair, exposing playback / zoom /
 * channel-selection signals consumed by the surrounding plugin GUI.
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
     * Set the detrending mode for on-the-fly trend removal during rendering.
     * None = raw, Mean = DC offset, Linear = least-squares linear fit.
     *
     * @param[in] mode  The DetrendMode to use.
     */
    void setDetrendMode(DetrendMode mode);
    DetrendMode detrendMode() const;

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

    void setEpochMarkers(const QVector<int> &triggerSamples);
    void setEpochMarkersVisible(bool visible);
    bool epochMarkersVisible() const;

    void setClippingVisible(bool visible);
    bool clippingVisible() const;

    void setZScoreMode(bool enabled);
    bool zScoreMode() const;

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

    void setEventsVisible(bool visible);
    bool eventsVisible() const;

    void setAnnotationsVisible(bool visible);
    bool annotationsVisible() const;

    void setOverviewBarVisible(bool visible);
    bool overviewBarVisible() const;

    void setScrollSpeedFactor(float factor);
    float scrollSpeedFactor() const;

    void sortChannelsByType();
    void resetChannelOrder();

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
    void eventsVisibleToggled(bool on);
    void annotationsVisibleToggled(bool on);
    void overviewBarToggled(bool on);
    void scrollSpeedChanged(float factor);
    void epochMarkersToggled(bool on);
    void clippingToggled(bool on);
    void zScoreModeToggled(bool on);

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
