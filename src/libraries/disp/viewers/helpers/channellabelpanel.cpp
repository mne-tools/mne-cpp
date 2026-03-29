//=============================================================================================================
/**
 * @file     channellabelpanel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Definition of the ChannelLabelPanel class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channellabelpanel.h"
#include "channeldatamodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QMouseEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QtMath>

#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

namespace {
constexpr int kPanelWidth  = 120;
constexpr int kStripWidth  =   5;   // type-colour strip on left edge
constexpr int kBadBadgeW   =  28;
constexpr int kBadBadgeH   =  11;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelLabelPanel::ChannelLabelPanel(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(kPanelWidth);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setCursor(Qt::SizeVerCursor);
    setMouseTracking(true);
}

//=============================================================================================================

QSize ChannelLabelPanel::sizeHint() const
{
    return QSize(kPanelWidth, 400);
}

//=============================================================================================================

QSize ChannelLabelPanel::minimumSizeHint() const
{
    return QSize(kPanelWidth, 50);
}

//=============================================================================================================

void ChannelLabelPanel::setModel(ChannelDataModel *model)
{
    m_model = model;
    update();
}

//=============================================================================================================

void ChannelLabelPanel::setChannelIndices(const QVector<int> &indices)
{
    m_channelIndices = indices;
    update();
}

//=============================================================================================================

void ChannelLabelPanel::setFirstVisibleChannel(int ch)
{
    if (ch == m_firstVisibleChannel)
        return;
    m_firstVisibleChannel = ch;
    update();
}

//=============================================================================================================

void ChannelLabelPanel::setVisibleChannelCount(int count)
{
    if (count == m_visibleChannelCount)
        return;
    m_visibleChannelCount = count;
    update();
}

//=============================================================================================================

void ChannelLabelPanel::setHideBadChannels(bool hide)
{
    if (m_hideBadChannels == hide)
        return;
    m_hideBadChannels = hide;
    update();
}

//=============================================================================================================

void ChannelLabelPanel::setVisibleSampleRange(int firstSample, int lastSample)
{
    m_visSampleFirst = firstSample;
    m_visSampleLast  = lastSample;
    update();
}

//=============================================================================================================

void ChannelLabelPanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    // Background — slightly more grey than the render surface
    p.fillRect(rect(), QColor(242, 242, 242));

    if (!m_model)
        return;

    const QVector<int> displayChannels = effectiveChannelIndices();
    const int totalCh = displayChannels.size();
    int visibleCount = qMin(m_visibleChannelCount, totalCh - m_firstVisibleChannel);
    if (visibleCount <= 0)
        return;

    const int   pw         = width();
    const float laneH      = static_cast<float>(height()) / visibleCount;

    // Whether DC removal is active (shown as a pill in the status line)
    const bool dcActive = m_model->removeDC();

    // Font sizes proportional to lane height, clamped for readability
    QFont nameFont = font();
    nameFont.setPointSizeF(qBound(7.0, static_cast<double>(laneH) * 0.22, 11.0));
    nameFont.setBold(true);

    QFont typeFont = nameFont;
    typeFont.setPointSizeF(qBound(6.0, static_cast<double>(laneH) * 0.15, 8.5));
    typeFont.setBold(false);

    float yTop = 0.f;
    for (int i = 0; i < visibleCount; ++i) {
        int logIdx = m_firstVisibleChannel + i;
        int ch = (logIdx >= 0 && logIdx < displayChannels.size()) ? displayChannels.at(logIdx) : -1;
        if (ch < 0) {
            yTop += laneH;
            continue;
        }
        auto info = m_model->channelInfo(ch);
        float yBot  = yTop + laneH;

        // Lane separator
        p.setPen(QPen(QColor(200, 200, 200), 1));
        if (i > 0)
            p.drawLine(QPointF(0, yTop), QPointF(pw, yTop));

        // Bad channel background
        if (info.bad)
            p.fillRect(QRectF(0, yTop, pw, laneH), QColor(255, 225, 225));

        // Type-colour strip
        p.fillRect(QRectF(0, yTop, kStripWidth, laneH), info.color);

        // ── BAD badge (top-right corner) ─────────────────────────────────
        if (info.bad) {
            QRectF badRect(pw - kBadBadgeW - 2, yTop + 2, kBadBadgeW, kBadBadgeH);
            p.fillRect(badRect, QColor(210, 30, 30));
            QFont badFont = typeFont;
            badFont.setPointSizeF(6.0);
            badFont.setBold(true);
            p.setFont(badFont);
            p.setPen(Qt::white);
            p.drawText(badRect, Qt::AlignCenter, QStringLiteral("BAD"));
        }

        // ── Channel name (bold, elided) ───────────────────────────────────
        p.setFont(nameFont);
        p.setPen(info.bad ? QColor(180, 20, 20) : QColor(25, 25, 25));
        int nameBadgeGap = info.bad ? (kBadBadgeW + 4) : 4;
        QRectF nameRect(kStripWidth + 4, yTop + 1,
                        pw - kStripWidth - nameBadgeGap - 4,
                        laneH * 0.48f);
        QString elidedName = p.fontMetrics().elidedText(
            info.name, Qt::ElideRight, static_cast<int>(nameRect.width()));
        p.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

        // ── Status line: type · bad · dc ─────────────────────────────────
        QString statusLine = info.typeLabel;
        if (info.bad)
            statusLine += QStringLiteral(" \u00B7 bad");
        if (info.isVirtualChannel)
            statusLine += QStringLiteral(" \u00B7 deriv");
        if (dcActive)
            statusLine += QStringLiteral(" \u00B7 dc");

        p.setFont(typeFont);
        p.setPen(info.bad ? QColor(190, 60, 60) : QColor(110, 110, 120));
        QRectF statusRect(kStripWidth + 4, yTop + laneH * 0.50f,
                          pw - kStripWidth - 8, laneH * 0.28f);
        p.drawText(statusRect, Qt::AlignLeft | Qt::AlignVCenter, statusLine);

        // ── RMS level bar ─────────────────────────────────────────────────
        float rms   = 0.f;
        if (m_visSampleFirst < m_visSampleLast)
            rms = m_model->channelRms(ch, m_visSampleFirst, m_visSampleLast);
        float level = (info.amplitudeMax > 0.f)
                      ? qBound(0.f, rms / info.amplitudeMax, 1.f)
                      : 0.f;

        const float barY  = yTop + laneH * 0.80f;
        const float barH  = qMax(2.f, laneH * 0.14f);
        const float barX0 = kStripWidth + 4.f;
        const float barW  = pw - kStripWidth - 8.f;

        p.fillRect(QRectF(barX0, barY, barW, barH), QColor(215, 215, 215));

        if (level > 0.f) {
            QColor barColor;
            if (level < 0.5f)
                barColor = QColor(50, 180, 80);
            else if (level < 0.85f)
                barColor = QColor(220, 160, 30);
            else
                barColor = QColor(210, 50, 40);
            if (info.bad)
                barColor = barColor.darker(115);
            p.fillRect(QRectF(barX0, barY, barW * level, barH), barColor);
        }

        yTop = yBot;
    }
}

//=============================================================================================================

void ChannelLabelPanel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging       = true;
        m_dragStartY     = event->position().toPoint().y();
        m_dragStartFirst = m_firstVisibleChannel;
        event->accept();
    }
}

//=============================================================================================================

void ChannelLabelPanel::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging) {
        event->ignore();
        return;
    }

    const int totalCh    = effectiveChannelIndices().size();
    const int maxFirst   = qMax(0, totalCh - m_visibleChannelCount);
    const float laneH    = (m_visibleChannelCount > 0 && height() > 0)
                           ? static_cast<float>(height()) / m_visibleChannelCount
                           : 30.f;

    int dy = event->position().toPoint().y() - m_dragStartY;
    // Dragging DOWN means earlier channels (positive dy → lower first index)
    int targetFirst = qBound(0,
                             m_dragStartFirst - static_cast<int>(dy / laneH),
                             maxFirst);

    emit channelScrollRequested(targetFirst);
    event->accept();
}

//=============================================================================================================

void ChannelLabelPanel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}

//=============================================================================================================

QVector<int> ChannelLabelPanel::effectiveChannelIndices() const
{
    QVector<int> indices;

    if (!m_model) {
        return indices;
    }

    if (m_channelIndices.isEmpty()) {
        indices.reserve(m_model->channelCount());
        for (int channelIndex = 0; channelIndex < m_model->channelCount(); ++channelIndex) {
            indices.append(channelIndex);
        }
    } else {
        indices = m_channelIndices;
    }

    if (!m_hideBadChannels) {
        return indices;
    }

    QVector<int> visibleIndices;
    visibleIndices.reserve(indices.size());
    for (int channelIndex : std::as_const(indices)) {
        if (channelIndex < 0) {
            continue;
        }

        const ChannelDisplayInfo info = m_model->channelInfo(channelIndex);
        if (!info.bad) {
            visibleIndices.append(channelIndex);
        }
    }

    return visibleIndices;
}

bool ChannelLabelPanel::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip && m_model) {
        auto *he = static_cast<QHelpEvent *>(e);
        const QVector<int> displayChannels = effectiveChannelIndices();
        int totalCh = displayChannels.size();
        int visibleCount = qMin(m_visibleChannelCount, totalCh - m_firstVisibleChannel);
        if (visibleCount > 0) {
            const float laneH = static_cast<float>(height()) / visibleCount;
            const int i = qBound(0, static_cast<int>(he->pos().y() / laneH), visibleCount - 1);
            if (i >= 0 && i < visibleCount) {
                const int logIdx = m_firstVisibleChannel + i;
                const int ch = (logIdx >= 0 && logIdx < displayChannels.size())
                    ? displayChannels.at(logIdx)
                    : -1;
                if (ch < 0) return QWidget::event(e);
                auto info = m_model->channelInfo(ch);
                float rms = (m_visSampleFirst < m_visSampleLast)
                            ? m_model->channelRms(ch, m_visSampleFirst, m_visSampleLast)
                            : 0.f;
                float level = (info.amplitudeMax > 0.f)
                              ? qBound(0.f, rms / info.amplitudeMax * 100.f, 100.f)
                              : 0.f;
                QString tip = QStringLiteral("<b>%1</b><br>"
                                              "Type: %2<br>"
                                              "Scale: %3<br>"
                                              "Level: %4 %<br>"
                                              "Bad: %5<br>"
                                              "Virtual: %6")
                    .arg(info.name)
                    .arg(info.typeLabel)
                    .arg(info.amplitudeMax, 0, 'e', 2)
                    .arg(static_cast<int>(level))
                    .arg(info.bad ? QStringLiteral("yes") : QStringLiteral("no"))
                    .arg(info.isVirtualChannel ? QStringLiteral("yes") : QStringLiteral("no"));
                QToolTip::showText(he->globalPos(), tip, this);
                return true;
            }
        }
    }
    return QWidget::event(e);
}
