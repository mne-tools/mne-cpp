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

void ChannelLabelPanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    // Background — slightly more grey than the render surface
    p.fillRect(rect(), QColor(242, 242, 242));

    if (!m_model)
        return;

    int totalCh      = m_model->channelCount();
    int visibleCount = qMin(m_visibleChannelCount, totalCh - m_firstVisibleChannel);
    if (visibleCount <= 0)
        return;

    const int   pw    = width();
    const float laneH = static_cast<float>(height()) / visibleCount;

    // Font sizes proportional to lane height, clamped for readability
    QFont nameFont = font();
    nameFont.setPointSizeF(qBound(7.0, static_cast<double>(laneH) * 0.22, 11.0));
    nameFont.setBold(true);

    QFont typeFont = nameFont;
    typeFont.setPointSizeF(qBound(6.0, static_cast<double>(laneH) * 0.15, 8.5));
    typeFont.setBold(false);

    for (int i = 0; i < visibleCount; ++i) {
        int ch = m_firstVisibleChannel + i;
        auto info = m_model->channelInfo(ch);

        float yTop = i * laneH;
        float yBot = yTop + laneH;

        // Bad channel: tinted background
        if (info.bad)
            p.fillRect(QRectF(0, yTop, pw, laneH), QColor(255, 225, 225));

        // Type-colour strip
        p.fillRect(QRectF(0, yTop, kStripWidth, laneH), info.color);

        // Lane separator (top edge of every row except the first)
        p.setPen(QPen(QColor(200, 200, 200), 1));
        if (i > 0)
            p.drawLine(QPointF(0, yTop), QPointF(pw, yTop));

        // BAD badge (top-right corner)
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

        // Channel name (bold, elided)
        p.setFont(nameFont);
        p.setPen(info.bad ? QColor(180, 20, 20) : QColor(25, 25, 25));
        int nameBadgeGap = info.bad ? (kBadBadgeW + 4) : 4;
        QRectF nameRect(kStripWidth + 4, yTop + 1,
                        pw - kStripWidth - nameBadgeGap - 4,
                        laneH * 0.55f);
        QString elidedName = p.fontMetrics().elidedText(
            info.name, Qt::ElideRight, static_cast<int>(nameRect.width()));
        p.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

        // Type label (small, dimmed)
        p.setFont(typeFont);
        p.setPen(QColor(130, 130, 130));
        QRectF typeRect(kStripWidth + 4, yTop + laneH * 0.54f,
                        pw - kStripWidth - 8, laneH * 0.42f);
        p.drawText(typeRect, Qt::AlignLeft | Qt::AlignVCenter, info.typeLabel);
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

    const int totalCh    = m_model ? m_model->channelCount() : 0;
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

bool ChannelLabelPanel::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip && m_model) {
        auto *he = static_cast<QHelpEvent *>(e);
        int totalCh      = m_model->channelCount();
        int visibleCount = qMin(m_visibleChannelCount, totalCh - m_firstVisibleChannel);
        if (visibleCount > 0) {
            float laneH = static_cast<float>(height()) / visibleCount;
            int   i     = static_cast<int>(he->pos().y() / laneH);
            if (i >= 0 && i < visibleCount) {
                int ch = m_firstVisibleChannel + i;
                auto info = m_model->channelInfo(ch);
                QString tip = QStringLiteral("<b>%1</b><br>"
                                              "Type: %2<br>"
                                              "Scale: %3<br>"
                                              "Bad: %4")
                    .arg(info.name)
                    .arg(info.typeLabel)
                    .arg(info.amplitudeMax, 0, 'e', 2)
                    .arg(info.bad ? QStringLiteral("yes") : QStringLiteral("no"));
                QToolTip::showText(he->globalPos(), tip, this);
                return true;
            }
        }
    }
    return QWidget::event(e);
}
