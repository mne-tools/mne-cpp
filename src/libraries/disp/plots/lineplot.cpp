//=============================================================================================================
/**
 * @file     lineplot.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    LinePlot class implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lineplot.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LinePlot::LinePlot(QWidget *parent)
: QWidget(parent)
, m_dMinX(0)
, m_dMaxX(0)
, m_dMinY(0)
, m_dMaxY(0)
{
    setMinimumSize(200, 150);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

//=============================================================================================================

LinePlot::LinePlot(const QVector<double> &y,
                   const QString& title,
                   QWidget *parent)
: QWidget(parent)
, m_sTitle(title)
, m_dMinX(0)
, m_dMaxX(0)
, m_dMinY(0)
, m_dMaxY(0)
{
    setMinimumSize(200, 150);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    updateData(y);
}

//=============================================================================================================

LinePlot::LinePlot(const QVector<double> &x,
                   const QVector<double> &y,
                   const QString& title,
                   QWidget *parent)
: QWidget(parent)
, m_sTitle(title)
, m_dMinX(0)
, m_dMaxX(0)
, m_dMinY(0)
, m_dMaxY(0)
{
    setMinimumSize(200, 150);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    updateData(x, y);
}

//=============================================================================================================

LinePlot::~LinePlot()
{
}

//=============================================================================================================

void LinePlot::setTitle(const QString &p_sTitle)
{
    m_sTitle = p_sTitle;
    QWidget::update();
}

//=============================================================================================================

void LinePlot::setXLabel(const QString &p_sXLabel)
{
    m_sXLabel = p_sXLabel;
    QWidget::update();
}

//=============================================================================================================

void LinePlot::setYLabel(const QString &p_sYLabel)
{
    m_sYLabel = p_sYLabel;
    QWidget::update();
}

//=============================================================================================================

void LinePlot::updateData(const QVector<double> &y)
{
    QVector<double> x(y.size());
    for(int i = 0; i < x.size(); ++i) {
        x[i] = static_cast<double>(i);
    }
    updateData(x, y);
}

//=============================================================================================================

void LinePlot::updateData(const QVector<double> &x,
                          const QVector<double> &y)
{
    m_vecXData = x;
    m_vecYData = y;

    if (!x.isEmpty()) {
        m_dMinX = x[0];
        m_dMaxX = x[0];
        for (int i = 1; i < x.size(); ++i) {
            if (x[i] < m_dMinX) m_dMinX = x[i];
            if (x[i] > m_dMaxX) m_dMaxX = x[i];
        }
    }

    if (!y.isEmpty()) {
        m_dMinY = y[0];
        m_dMaxY = y[0];
        for (int i = 1; i < y.size(); ++i) {
            if (y[i] < m_dMinY) m_dMinY = y[i];
            if (y[i] > m_dMaxY) m_dMaxY = y[i];
        }
    }

    setWindowTitle(m_sTitle);
    QWidget::update();
}

//=============================================================================================================

void LinePlot::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int mLeft = 60;
    const int mRight = 20;
    const int mTop = 35;
    const int mBottom = 40;
    const int plotW = w - mLeft - mRight;
    const int plotH = h - mTop - mBottom;

    if (plotW <= 0 || plotH <= 0)
        return;

    // Title
    if (!m_sTitle.isEmpty()) {
        QFont titleFont = painter.font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 2);
        painter.setFont(titleFont);
        painter.setPen(Qt::black);
        painter.drawText(QRect(0, 0, w, mTop), Qt::AlignCenter, m_sTitle);
        painter.setFont(QFont());
    }

    // Draw axes
    painter.setPen(QPen(Qt::black, 1));
    painter.drawLine(mLeft, mTop, mLeft, mTop + plotH);
    painter.drawLine(mLeft, mTop + plotH, mLeft + plotW, mTop + plotH);

    // Y-axis label
    if (!m_sYLabel.isEmpty()) {
        painter.save();
        painter.translate(12, mTop + plotH / 2);
        painter.rotate(-90);
        painter.drawText(QRect(-plotH / 2, 0, plotH, 15), Qt::AlignCenter, m_sYLabel);
        painter.restore();
    }

    // X-axis label
    if (!m_sXLabel.isEmpty()) {
        painter.drawText(QRect(mLeft, h - 15, plotW, 15), Qt::AlignCenter, m_sXLabel);
    }

    double rangeX = m_dMaxX - m_dMinX;
    double rangeY = m_dMaxY - m_dMinY;
    if (rangeX == 0) rangeX = 1.0;
    if (rangeY == 0) rangeY = 1.0;

    // Y-axis ticks
    const int nYTicks = 5;
    for (int i = 0; i <= nYTicks; ++i) {
        double yVal = m_dMinY + rangeY * i / nYTicks;
        int y = mTop + plotH - static_cast<int>(plotH * static_cast<double>(i) / nYTicks);
        painter.drawLine(mLeft - 5, y, mLeft, y);
        painter.drawText(QRect(0, y - 10, mLeft - 8, 20), Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(yVal, 'g', 4));
    }

    // X-axis ticks
    const int nXTicks = 5;
    for (int i = 0; i <= nXTicks; ++i) {
        double xVal = m_dMinX + rangeX * i / nXTicks;
        int x = mLeft + static_cast<int>(plotW * static_cast<double>(i) / nXTicks);
        painter.drawLine(x, mTop + plotH, x, mTop + plotH + 5);
        painter.drawText(QRect(x - 30, mTop + plotH + 6, 60, 20), Qt::AlignCenter,
                         QString::number(xVal, 'g', 4));
    }

    if (m_vecXData.isEmpty() || m_vecYData.isEmpty())
        return;

    // Draw line series
    QPainterPath path;
    auto toPixelX = [&](double dx) -> double {
        return mLeft + ((dx - m_dMinX) / rangeX) * plotW;
    };
    auto toPixelY = [&](double dy) -> double {
        return mTop + plotH - ((dy - m_dMinY) / rangeY) * plotH;
    };

    int n = qMin(m_vecXData.size(), m_vecYData.size());
    path.moveTo(toPixelX(m_vecXData[0]), toPixelY(m_vecYData[0]));
    for (int i = 1; i < n; ++i) {
        path.lineTo(toPixelX(m_vecXData[i]), toPixelY(m_vecYData[i]));
    }

    painter.setPen(QPen(QColor(70, 130, 180), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}
