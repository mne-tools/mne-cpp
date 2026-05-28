//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Andreas Griesshammer <ag@fieldlineinc.com>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file bar.cpp
 * @since 2022
 * @date  February 2026
 * @brief Implementation of the Bar histogram widget (axis layout and bar drawing).
 *
 * Computes the per-bin pixel rectangles from the class limits, formats
 * the tick labels via @ref Bar::splitCoefficientAndExponent() so very
 * small or very large class-limit values render compactly, and paints
 * the bars and axes in @c paintEvent().
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bar.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QPaintEvent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Bar::Bar(const QString& title,
         QWidget* parent)
: QWidget(parent)
, m_sTitle(title)
, m_iMaxFrequency(0)
{
    setMinimumSize(200, 150);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

//=============================================================================================================

void Bar::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int marginLeft = 60;
    const int marginRight = 20;
    const int marginTop = 40;
    const int marginBottom = 100;
    const int plotW = w - marginLeft - marginRight;
    const int plotH = h - marginTop - marginBottom;

    if (plotW <= 0 || plotH <= 0)
        return;

    // Title
    if (!m_sTitle.isEmpty()) {
        painter.setPen(Qt::black);
        QFont titleFont = painter.font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 2);
        painter.setFont(titleFont);
        painter.drawText(QRect(0, 0, w, marginTop), Qt::AlignCenter, m_sTitle);
        painter.setFont(QFont());
    }

    if (m_frequencies.isEmpty())
        return;

    const int nBars = m_frequencies.size();
    const int maxFreq = (m_iMaxFrequency > 0) ? m_iMaxFrequency : 1;

    // Draw axes
    painter.setPen(QPen(Qt::black, 1));
    painter.drawLine(marginLeft, marginTop, marginLeft, marginTop + plotH);
    painter.drawLine(marginLeft, marginTop + plotH, marginLeft + plotW, marginTop + plotH);

    // Y-axis ticks
    const int nYTicks = 5;
    for (int i = 0; i <= nYTicks; ++i) {
        int yVal = maxFreq * i / nYTicks;
        int y = marginTop + plotH - (plotH * i / nYTicks);
        painter.drawLine(marginLeft - 5, y, marginLeft, y);
        painter.drawText(QRect(0, y - 10, marginLeft - 8, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(yVal));
    }

    // Draw bars
    double barWidth = static_cast<double>(plotW) / nBars;
    double barPadding = barWidth * 0.1;

    for (int i = 0; i < nBars; ++i) {
        double barH = (static_cast<double>(m_frequencies[i]) / maxFreq) * plotH;
        double x = marginLeft + i * barWidth + barPadding;
        double bw = barWidth - 2.0 * barPadding;
        double y = marginTop + plotH - barH;

        painter.setBrush(QColor(70, 130, 180));
        painter.setPen(QPen(QColor(50, 100, 150), 1));
        painter.drawRect(QRectF(x, y, bw, barH));
    }

    // X-axis labels (rotated)
    painter.setPen(Qt::black);
    QFont smallFont = painter.font();
    smallFont.setPointSize(smallFont.pointSize() - 1);
    painter.setFont(smallFont);

    for (int i = 0; i < nBars && i < m_categories.size(); ++i) {
        double cx = marginLeft + (i + 0.5) * barWidth;
        painter.save();
        painter.translate(cx, marginTop + plotH + 5);
        painter.rotate(45);
        painter.drawText(0, 0, m_categories[i]);
        painter.restore();
    }

    // Legend
    if (!m_sLegend.isEmpty()) {
        painter.setFont(QFont());
        painter.setPen(Qt::black);
        painter.drawText(QRect(0, h - 20, w, 20), Qt::AlignCenter, m_sLegend);
    }
}
