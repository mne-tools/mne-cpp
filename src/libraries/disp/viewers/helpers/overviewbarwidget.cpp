//=============================================================================================================
/**
 * @file     overviewbarwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 *       the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *       and the following disclaimer in the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or
 *       promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the OverviewBarWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "overviewbarwidget.h"
#include "channeldatamodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QMouseEvent>
#include <QtMath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

OverviewBarWidget::OverviewBarWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(kBarHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
}

//=============================================================================================================

QSize OverviewBarWidget::sizeHint() const
{
    return QSize(400, kBarHeight);
}

//=============================================================================================================

QSize OverviewBarWidget::minimumSizeHint() const
{
    return QSize(100, kBarHeight);
}

//=============================================================================================================

void OverviewBarWidget::setModel(ChannelDataModel *model)
{
    m_model = model;
    m_envelopeDirty = true;
    update();
}

//=============================================================================================================

void OverviewBarWidget::setFirstFileSample(int first)
{
    m_firstFileSample = first;
    m_envelopeDirty = true;
    update();
}

//=============================================================================================================

void OverviewBarWidget::setLastFileSample(int last)
{
    m_lastFileSample = last;
    m_envelopeDirty = true;
    update();
}

//=============================================================================================================

void OverviewBarWidget::setSfreq(float sfreq)
{
    m_sfreq = sfreq;
    update();
}

//=============================================================================================================

void OverviewBarWidget::setViewport(float scrollSample, float visibleSamples)
{
    m_scrollSample = scrollSample;
    m_visibleSamples = visibleSamples;
    update();
}

//=============================================================================================================

void OverviewBarWidget::setEvents(const QVector<ChannelRhiView::EventMarker> &events)
{
    m_events = events;
    update();
}

//=============================================================================================================

void OverviewBarWidget::setAnnotations(const QVector<ChannelRhiView::AnnotationSpan> &annotations)
{
    m_annotations = annotations;
    update();
}

//=============================================================================================================

float OverviewBarWidget::xToSample(int x) const
{
    int totalSamples = m_lastFileSample - m_firstFileSample;
    if (totalSamples <= 0 || width() <= 0)
        return static_cast<float>(m_firstFileSample);
    float frac = static_cast<float>(x) / static_cast<float>(width());
    frac = qBound(0.f, frac, 1.f);
    return static_cast<float>(m_firstFileSample) + frac * static_cast<float>(totalSamples);
}

//=============================================================================================================

void OverviewBarWidget::rebuildEnvelope()
{
    const int pw = width();
    const int ph = kBarHeight;
    if (pw <= 0 || !m_model || m_lastFileSample <= m_firstFileSample) {
        m_envelopeImage = QImage();
        m_envelopeDirty = false;
        return;
    }

    m_envelopeImage = QImage(pw, ph, QImage::Format_RGB32);
    m_envelopeImage.fill(QColor(38, 38, 42).rgb()); // dark background

    QPainter p(&m_envelopeImage);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int totalSamples = m_lastFileSample - m_firstFileSample;
    const float samplesPerPx = static_cast<float>(totalSamples) / static_cast<float>(pw);
    const int nChannels = m_model->channelCount();
    if (nChannels <= 0 || totalSamples <= 0) {
        m_envelopeDirty = false;
        return;
    }

    // Group channels by type and compute a per-pixel RMS envelope for each type
    struct TypeEnvelope {
        QString typeLabel;
        QColor  color;
        QVector<float> envelope; // per-pixel RMS, size = pw
    };
    QVector<TypeEnvelope> typeEnvelopes;
    QMap<QString, int> typeIndex;

    for (int ch = 0; ch < nChannels; ++ch) {
        auto info = m_model->channelInfo(ch);
        if (!typeIndex.contains(info.typeLabel)) {
            int idx = typeEnvelopes.size();
            typeIndex[info.typeLabel] = idx;
            TypeEnvelope te;
            te.typeLabel = info.typeLabel;
            te.color = info.color;
            te.envelope.resize(pw, 0.f);
            typeEnvelopes.append(te);
        }
    }

    // For each pixel column, compute max absolute value across channels of each type
    for (int px_col = 0; px_col < pw; ++px_col) {
        int sampleStart = m_firstFileSample + static_cast<int>(px_col * samplesPerPx);
        int sampleEnd = m_firstFileSample + static_cast<int>((px_col + 1) * samplesPerPx);
        sampleEnd = qMin(sampleEnd, m_lastFileSample);
        if (sampleEnd <= sampleStart)
            continue;

        // Sample a few points in this range for speed
        int step = qMax(1, (sampleEnd - sampleStart) / 4);
        for (int ch = 0; ch < nChannels; ++ch) {
            auto info = m_model->channelInfo(ch);
            int ti = typeIndex.value(info.typeLabel, -1);
            if (ti < 0) continue;

            float maxAbs = 0.f;
            for (int s = sampleStart; s < sampleEnd; s += step) {
                float v = qAbs(m_model->sampleValueAt(ch, s));
                if (v > maxAbs) maxAbs = v;
            }
            // Normalise by amplitude scale
            float norm = (info.amplitudeMax > 0.f) ? maxAbs / info.amplitudeMax : 0.f;
            if (norm > typeEnvelopes[ti].envelope[px_col])
                typeEnvelopes[ti].envelope[px_col] = norm;
        }
    }

    // Draw each type's envelope as a filled area from the bottom
    const int nTypes = typeEnvelopes.size();
    const float laneH = static_cast<float>(ph) / qMax(nTypes, 1);

    for (int ti = 0; ti < nTypes; ++ti) {
        const auto &te = typeEnvelopes[ti];
        QColor fillColor = te.color;
        fillColor.setAlpha(160);
        p.setPen(Qt::NoPen);

        float yBase = (ti + 1) * laneH;
        for (int x = 0; x < pw; ++x) {
            float level = qBound(0.f, te.envelope[x], 1.f);
            float barH = level * laneH * 0.9f;
            if (barH < 0.5f) continue;
            p.fillRect(QRectF(x, yBase - barH, 1.f, barH), fillColor);
        }

        // Type label
        QFont f = font();
        f.setPointSizeF(7.0);
        p.setFont(f);
        p.setPen(QColor(200, 200, 200, 180));
        p.drawText(QRectF(2, ti * laneH, 60, laneH * 0.5f),
                   Qt::AlignLeft | Qt::AlignTop, te.typeLabel);
    }

    m_envelopeDirty = false;
}

//=============================================================================================================

void OverviewBarWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int pw = width();
    const int ph = height();

    if (m_envelopeDirty || m_envelopeImage.width() != pw)
        rebuildEnvelope();

    // Draw the envelope image or a plain dark background
    if (!m_envelopeImage.isNull()) {
        p.drawImage(0, 0, m_envelopeImage);
    } else {
        p.fillRect(rect(), QColor(38, 38, 42));
    }

    int totalSamples = m_lastFileSample - m_firstFileSample;
    if (totalSamples <= 0) {
        // No data loaded yet — show placeholder text
        p.setPen(QColor(120, 120, 130));
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("No data"));
        return;
    }

    float samplesPerPx = static_cast<float>(totalSamples) / static_cast<float>(pw);

    // ── Annotation spans ────────────────────────────────────────────
    for (const auto &ann : m_annotations) {
        float xStart = static_cast<float>(ann.startSample - m_firstFileSample) / samplesPerPx;
        float xEnd = static_cast<float>(ann.endSample - m_firstFileSample) / samplesPerPx;
        xStart = qBound(0.f, xStart, static_cast<float>(pw));
        xEnd = qBound(0.f, xEnd, static_cast<float>(pw));
        if (xEnd > xStart) {
            QColor c = ann.color;
            c.setAlpha(60);
            p.fillRect(QRectF(xStart, 0, xEnd - xStart, ph), c);
        }
    }

    // ── Event markers ───────────────────────────────────────────────
    for (const auto &ev : m_events) {
        float xF = static_cast<float>(ev.sample - m_firstFileSample) / samplesPerPx;
        if (xF < 0.f || xF > pw)
            continue;
        QColor c = ev.color;
        c.setAlpha(200);
        p.setPen(QPen(c, 1));
        int ix = static_cast<int>(xF);
        p.drawLine(ix, ph - 6, ix, ph);
    }

    // ── Viewport rectangle ──────────────────────────────────────────
    float vpX = (m_scrollSample - static_cast<float>(m_firstFileSample)) / samplesPerPx;
    float vpW = m_visibleSamples / samplesPerPx;
    vpX = qBound(0.f, vpX, static_cast<float>(pw));
    vpW = qBound(2.f, vpW, static_cast<float>(pw) - vpX);

    // Semi-transparent overlay outside the viewport
    QColor dimColor(0, 0, 0, 100);
    if (vpX > 0.f)
        p.fillRect(QRectF(0, 0, vpX, ph), dimColor);
    if (vpX + vpW < pw)
        p.fillRect(QRectF(vpX + vpW, 0, pw - vpX - vpW, ph), dimColor);

    // Viewport border
    QPen vpPen(QColor(255, 255, 255, 200), 1.5);
    p.setPen(vpPen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(vpX, 0.5f, vpW, ph - 1.f));
}

//=============================================================================================================

void OverviewBarWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        float targetSample = xToSample(event->position().toPoint().x()) - m_visibleSamples * 0.5f;
        emit scrollRequested(targetSample);
        event->accept();
    }
}

//=============================================================================================================

void OverviewBarWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        float targetSample = xToSample(event->position().toPoint().x()) - m_visibleSamples * 0.5f;
        emit scrollRequested(targetSample);
        event->accept();
    }
}

//=============================================================================================================

void OverviewBarWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}
