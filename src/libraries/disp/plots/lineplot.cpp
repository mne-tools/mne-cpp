//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Andreas Griesshammer <ag@fieldlineinc.com>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file lineplot.cpp
 * @since July 2018
 * @brief Implementation of the LinePlot widget (auto-range computation and polyline painting).
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
