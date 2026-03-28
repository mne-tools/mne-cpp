//=============================================================================================================
/**
 * @file     timerulerwidget.cpp
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
 * @brief    Definition of the TimeRulerWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timerulerwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QPaintEvent>
#include <QFontDatabase>
#include <QtMath>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

namespace {
// Same nice-interval table used by the render grid — guarantees perfect alignment.
static const double kNiceIntervals[] = { 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 30.0, 60.0 };
constexpr double kMinMajorPx = 80.0; // minimum px spacing between major ticks
constexpr int    kMajorH     = 10;   // tick mark height in px (downward from bottom border)
constexpr int    kMinorH     =  5;
constexpr int    kLabelGap   =  3;   // gap between label bottom and tick top
} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TimeRulerWidget::TimeRulerWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(kTotalH);
}

//=============================================================================================================

void TimeRulerWidget::setSfreq(double sfreq)
{
    m_sfreq = (sfreq > 0.0) ? sfreq : 1.0;
    update();
}

//=============================================================================================================

void TimeRulerWidget::setFirstFileSample(int firstFileSample)
{
    m_firstFileSample = firstFileSample;
    update();
}

//=============================================================================================================

void TimeRulerWidget::setEvents(const QVector<TimeRulerEventMark> &events)
{
    m_events = events;
    update();
}

//=============================================================================================================

void TimeRulerWidget::setScrollSample(float sample)
{
    if (qFuzzyCompare(m_scrollSample, sample))
        return;
    m_scrollSample = sample;
    update();
}

//=============================================================================================================

void TimeRulerWidget::setSamplesPerPixel(float spp)
{
    if (qFuzzyCompare(m_spp, spp))
        return;
    m_spp = spp;
    update();
}

//=============================================================================================================

QString TimeRulerWidget::formatTime(double seconds)
{
    if (seconds < 0.0)
        seconds = 0.0;

    if (seconds >= 3600.0) {
        int h = static_cast<int>(seconds) / 3600;
        int m = (static_cast<int>(seconds) % 3600) / 60;
        int s = static_cast<int>(seconds) % 60;
        return QString("%1:%2:%3")
            .arg(h)
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));
    } else if (seconds >= 60.0) {
        int    m = static_cast<int>(seconds) / 60;
        double s = seconds - m * 60.0;
        return QString("%1:%2").arg(m).arg(s, 4, 'f', 1, QChar('0'));
    } else if (seconds >= 10.0) {
        return QString("%1 s").arg(static_cast<int>(std::round(seconds)));
    } else if (seconds >= 1.0) {
        return QString("%1 s").arg(seconds, 0, 'f', 1);
    } else if (seconds >= 0.1) {
        return QString("%1 s").arg(seconds, 0, 'f', 2);
    } else if (seconds >= 0.001) {
        return QString("%1 ms").arg(seconds * 1e3, 0, 'f', 1);
    } else {
        return QString("%1 ms").arg(seconds * 1e3, 0, 'f', 2);
    }
}

//=============================================================================================================

void TimeRulerWidget::paintEvent(QPaintEvent */*event*/)
{
    const int    W   = width();
    const int    H   = height();   // == kTotalH == kStimZoneH + kTimeZoneH
    const double spp = static_cast<double>(m_spp);

    if (W <= 0 || spp <= 0.0 || m_sfreq <= 0.0)
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    // ── Stim lane background (top kStimZoneH px) ─────────────────────
    p.fillRect(QRect(0, 0, W, kStimZoneH), QColor(238, 238, 246));

    // ── Time zone background (bottom kTimeZoneH px) ───────────────────
    p.fillRect(QRect(0, kStimZoneH, W, kTimeZoneH), QColor(245, 245, 247));

    // Separator line between the two zones
    p.setPen(QPen(QColor(190, 190, 205), 1));
    p.drawLine(0, kStimZoneH, W, kStimZoneH);

    // Bottom border
    p.setPen(QPen(QColor(185, 185, 195), 1));
    p.drawLine(0, H - 1, W, H - 1);

    // ── Stim event chips ─────────────────────────────────────────────
    if (!m_events.isEmpty()) {
        QFont evFont;
        evFont.setPixelSize(9);
        evFont.setBold(true);
        p.setFont(evFont);

        constexpr int kChipW = 26;
        constexpr int kChipH = 11;
        constexpr int kChipY = (kStimZoneH - kChipH) / 2;

        // Track the rightmost x edge drawn so we can nudge a duplicate label
        // slightly right rather than drawing it on top — but we do NOT loop,
        // which was the source of the earlier crash.
        int lastChipRight = -kChipW;

        for (const TimeRulerEventMark &ev : m_events) {
            float xF = (static_cast<float>(ev.sample) - m_scrollSample) / static_cast<float>(spp);
            if (xF < -2.f || xF > W + 2.f)
                continue;
            int ix = static_cast<int>(xF);

            // Tick mark at the bottom of the stim zone (points toward time area)
            QColor col = ev.color;
            col.setAlpha(200);
            p.setPen(QPen(col, 1));
            p.drawLine(ix, kStimZoneH - 3, ix, kStimZoneH);

            // Chip: shift right by one chip width if it would overlap the previous one
            int chipX = ix - kChipW / 2;
            if (chipX < lastChipRight + 2)
                chipX = lastChipRight + 2;
            chipX = qBound(0, chipX, qMax(0, W - kChipW));

            QRectF chip(chipX, kChipY, kChipW, kChipH);
            QColor fill = ev.color;
            fill.setAlpha(215);
            p.fillRect(chip, fill);
            p.setPen(Qt::white);
            QString lbl = ev.label.isEmpty() ? QStringLiteral("?") : ev.label;
            p.drawText(chip, Qt::AlignCenter, lbl);

            lastChipRight = chipX + kChipW;
        }
    }

    // ── Choose tick interval ──────────────────────────────────────────
    const double pxPerSec = m_sfreq / spp;
    double tickIntervalS = kNiceIntervals[0];
    for (double iv : kNiceIntervals) {
        tickIntervalS = iv;
        if (iv * pxPerSec >= kMinMajorPx)
            break;
    }

    const double tickSamples   = tickIntervalS * m_sfreq;
    const double minorSamples  = tickSamples / 5.0;
    const double origin        = static_cast<double>(m_firstFileSample);

    // ── Font ─────────────────────────────────────────────────────────
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSizeF(8.0);
    p.setFont(font);
    const QFontMetrics fm(font);

    // ── Minor ticks (in time zone) ────────────────────────────────────
    {
        double firstMinorS = std::ceil((m_scrollSample - origin - minorSamples) / minorSamples)
                             * minorSamples + origin;
        p.setPen(QPen(QColor(165, 165, 175), 1));
        for (double s = firstMinorS; ; s += minorSamples) {
            double xPx = (s - m_scrollSample) / spp;
            if (xPx > W + 2) break;
            if (xPx < -2)    continue;
            int xi = static_cast<int>(std::round(xPx));
            p.drawLine(xi, H - 1 - kMinorH, xi, H - 2);
        }
    }

    // ── Major ticks + labels ─────────────────────────────────────────
    {
        double firstMajorS = std::ceil((m_scrollSample - origin - tickSamples) / tickSamples)
                             * tickSamples + origin;
        for (double s = firstMajorS; ; s += tickSamples) {
            double xPx = (s - m_scrollSample) / spp;
            if (xPx > W + 2) break;
            if (xPx < -2)    continue;

            int xi = static_cast<int>(std::round(xPx));

            p.setPen(QPen(QColor(100, 100, 115), 1));
            p.drawLine(xi, H - 1 - kMajorH, xi, H - 2);

            double elapsedSec = (s - origin) / m_sfreq;
            if (elapsedSec >= -tickIntervalS * 0.5) {
                QString label = formatTime(elapsedSec);
                const int lw  = fm.horizontalAdvance(label);

                int lx = xi - lw / 2;
                lx = qBound(2, lx, W - lw - 2);
                int ly = H - 1 - kMajorH - kLabelGap;

                p.setPen(QColor(65, 65, 80));
                p.drawText(lx, ly, label);
            }
        }
    }
}
