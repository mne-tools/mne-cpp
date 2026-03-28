//=============================================================================================================
/**
 * @file     timerulerwidget.h
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
 * @brief    Declaration of the TimeRulerWidget class.
 *
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
 * @brief TimeRulerWidget – a thin horizontal time axis ruler for ChannelDataView.
 *
 * Displays time ticks and labels beneath the signal viewport.  Auto-computes a
 * "nice" tick interval so approximately 7 major ticks are visible regardless of
 * zoom level.  Labels show elapsed seconds from the start of the file or, for
 * longer ranges, a MM:SS clock format.
 *
 * The ruler is driven by two Q_PROPERTY-compatible slots that mirror the
 * ChannelRhiView properties:
 *   - setScrollSample(float)      – left-edge sample of the current view
 *   - setSamplesPerPixel(float)   – current horizontal zoom
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

protected:
    void paintEvent(QPaintEvent *event) override;

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

    double m_sfreq           = 1.0;
    int    m_firstFileSample = 0;
    float  m_scrollSample    = 0.f;
    float  m_spp             = 1.f;     // samples per pixel

    QVector<TimeRulerEventMark> m_events;
};

} // namespace DISPLIB

#endif // TIMERULERWIDGET_H
