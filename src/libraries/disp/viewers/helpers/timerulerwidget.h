//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file timerulerwidget.h
 * @since 2026
 * @date  April 2026
 * @brief Horizontal time-axis ruler displayed beneath @ref ChannelDataView with sample / second ticks.
 *
 * TimeRulerWidget paints the time axis aligned with the central raw
 * table, supports left-click drag for window scrolling and
 * right-click context actions (jump to event, set marker). Tick
 * spacing adapts to the current zoom so the labels stay readable
 * from milliseconds to minutes.
 */

#ifndef TIMERULERWIDGET_H
#define TIMERULERWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QColor>
#include <QVector>

class QContextMenuEvent;

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
/**
 * @brief Lightweight event mark passed to TimeRulerWidget for the stim lane.
 */
struct TimeRulerEventMark {
    int     sample = 0;   ///< Absolute sample index.
    QColor  color;        ///< Display colour.
    QString label;        ///< Short text (event type number).
};

//=============================================================================================================
/**
 * @brief Lightweight reference/sample marker passed to TimeRulerWidget.
 */
struct TimeRulerReferenceMark {
    int     sample = 0;   ///< Absolute sample index.
    QColor  color;        ///< Display colour.
    QString label;        ///< Short text identifier, e.g. M1.
};

//=============================================================================================================
/**
 * @brief Horizontal time-axis ruler displayed beneath @ref ChannelDataView.
 *
 * Tick spacing adapts to the current zoom so labels stay readable
 * from milliseconds to minutes; supports left-click drag for window
 * scrolling and right-click context actions (jump to event, set
 * marker).
 */
class DISPSHARED_EXPORT TimeRulerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimeRulerWidget(QWidget *parent = nullptr);

    //=========================================================================================================
    /**
     * Set the file sampling frequency. Must be called before the first paint.
     *
     * @param[in] sfreq  Sampling frequency in Hz.
     */
    void setSfreq(double sfreq);

    //=========================================================================================================
    /**
     * Set the absolute sample index of the left viewport edge (file-relative).
     * Also records the file's first sample so labels show elapsed time.
     *
     * @param[in] firstFileSample  The absolute sample index of the first sample in the file.
     */
    void setFirstFileSample(int firstFileSample);

    static constexpr int kStimZoneH = 16;  ///< Height of the stimulus lane (px).
    static constexpr int kTimeZoneH = 28;  ///< Height of the time-tick zone (px).
    static constexpr int kTotalH    = kStimZoneH + kTimeZoneH; ///< Total widget height (px).

    QSize sizeHint() const override { return QSize(100, kTotalH); }
    QSize minimumSizeHint() const override { return QSize(50, kTotalH); }

    //=========================================================================================================
    /**
     * Set the list of stimulus / event marks to display in the stim lane.
     * Pass an empty vector to clear all marks.
     *
     * @param[in] events  Event marks (sample position + colour + label).
     */
    void setEvents(const QVector<TimeRulerEventMark> &events);

    //=========================================================================================================
    /**
     * Set the list of persistent sample markers to display in the ruler.
     *
     * @param[in] markers  Reference/sample markers.
     */
    void setReferenceMarkers(const QVector<TimeRulerReferenceMark> &markers);

    //=========================================================================================================
    /**
     * Toggle the time format between float seconds and HH:MM:SS clock time.
     */
    void toggleTimeFormat();

    //=========================================================================================================
    /**
     * Set whether to use clock time (HH:MM:SS) or float seconds for labels.
     *
     * @param[in] useClock  true = HH:MM:SS, false = float seconds.
     */
    void setClockTimeFormat(bool useClock);
    bool clockTimeFormat() const { return m_useClockTime; }

public slots:
    //=========================================================================================================
    /**
     * Update the left-edge scroll position and trigger a repaint.
     *
     * @param[in] sample  Absolute sample index at the left edge of the view.
     */
    void setScrollSample(float sample);

    //=========================================================================================================
    /**
     * Update the horizontal zoom and trigger a repaint.
     *
     * @param[in] spp  Samples per pixel.
     */
    void setSamplesPerPixel(float spp);

signals:
    //=========================================================================================================
    /**
     * Emitted when the user requests a new sample marker from the ruler context menu.
     */
    void addReferenceMarkerRequested(int sample);

    //=========================================================================================================
    /**
     * Emitted when the user requests removal of the nearest sample marker from the ruler context menu.
     */
    void removeReferenceMarkerRequested(int sample);

    //=========================================================================================================
    /**
     * Emitted when the user requests that all sample markers be cleared.
     */
    void clearReferenceMarkersRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    //=========================================================================================================
    /**
     * Compute a "nice" tick interval (in seconds) for approximately @p targetTicks ticks.
     *
     * @param[in] visibleSeconds  Duration (s) of the current visible range.
     * @param[in] targetTicks     Desired number of major ticks.
     * @return   Nice interval in seconds.
     */
    static double niceTickInterval(double visibleSeconds, int targetTicks = 7);

    //=========================================================================================================
    /**
     * Format a time value in seconds as a human-readable label.
     *
     * @param[in] seconds  Elapsed time from file start.
     * @return   Formatted string.
     */
    static QString formatTime(double seconds);

    //=========================================================================================================
    /**
     * Map a local X position to the corresponding absolute sample index.
     *
     * @param[in] x  Local widget X position in pixels.
     * @return Absolute sample index.
     */
    int sampleAtX(int x) const;

    //=========================================================================================================
    /**
     * Find the nearest reference marker to a sample position.
     *
     * @param[in] sample              Absolute sample index to test against.
     * @param[in] tolerancePixels     Maximum X distance in pixels.
     * @return Index of the nearest marker, or -1 when none is close enough.
     */
    int nearestReferenceMarkerIndex(int sample, int tolerancePixels = 8) const;

    double m_sfreq           = 1.0;
    int    m_firstFileSample = 0;
    float  m_scrollSample    = 0.f;
    float  m_spp             = 1.f;     // samples per pixel
    bool   m_useClockTime    = false;   // false = float seconds, true = HH:MM:SS

    QVector<TimeRulerEventMark> m_events;
    QVector<TimeRulerReferenceMark> m_referenceMarkers;
};

} // namespace DISPLIB

#endif // TIMERULERWIDGET_H
