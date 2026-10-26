//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     spline.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the Spline histogram widget (spline interpolation, threshold markers, click signals).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spline.h"

#include "detail/niceticks.h"
#include "helpers/colormap.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace DISPLIB::detail;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Spline::Spline(QWidget* parent, const QString& title)
: QWidget(parent)
, m_dMinAxisX(0)
, m_dMaxAxisX(0)
, m_dLeftThreshold(0)
, m_dMiddleThreshold(0)
, m_dRightThreshold(0)
, m_bHasData(false)
, m_bHasThresholds(false)
, m_iMaximumFrequency(0)
{
    m_sTitle = title;
    setMinimumSize(200, 150);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

//=============================================================================================================

double Spline::dataToPixelX(double dataX) const
{
    const int plotW = width() - leftMargin() - rightMargin();
    if (m_dMaxAxisX == m_dMinAxisX)
        return leftMargin();
    return leftMargin() + (dataX - m_dMinAxisX) / (m_dMaxAxisX - m_dMinAxisX) * plotW;
}

//=============================================================================================================

double Spline::dataToPixelY(double dataY) const
{
    const int plotH = height() - topMargin() - bottomMargin();
    // Headroom for the curve, which overshoots between the class marks
    const double maxY = (m_iMaximumFrequency > 0) ? 1.1 * m_iMaximumFrequency : 1.0;
    return topMargin() + plotH - (dataY / maxY) * plotH;
}

//=============================================================================================================

double Spline::pixelToDataX(double pixelX) const
{
    const int plotW = width() - leftMargin() - rightMargin();
    if (plotW <= 0)
        return m_dMinAxisX;
    return m_dMinAxisX + (pixelX - leftMargin()) / plotW * (m_dMaxAxisX - m_dMinAxisX);
}

//=============================================================================================================

void Spline::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int mLeft = leftMargin();
    const int mRight = rightMargin();
    const int mTop = topMargin();
    const int mBottom = bottomMargin();
    const int plotW = w - mLeft - mRight;
    const int plotH = h - mTop - mBottom;

    if (plotW <= 0 || plotH <= 0)
        return;

    // Draw color map background if thresholds are set
    if (m_bHasThresholds && !m_colorMap.isEmpty() && m_dMaxAxisX > m_dMinAxisX) {
        double leftNorm = (m_dLeftThreshold - m_dMinAxisX) / (m_dMaxAxisX - m_dMinAxisX);
        double middleNorm = (m_dMiddleThreshold - m_dMinAxisX) / (m_dMaxAxisX - m_dMinAxisX);
        double rightNorm = (m_dRightThreshold - m_dMinAxisX) / (m_dMaxAxisX - m_dMinAxisX);

        QLinearGradient gradient(mLeft, 0, mLeft + plotW, 0);
        const int steps = 25;
        double stepLeftMiddle = (middleNorm - leftNorm) / steps;
        double stepMiddleRight = (rightNorm - middleNorm) / steps;

        gradient.setColorAt(qBound(0.0, leftNorm, 1.0), ColorMap::valueToColor(0.0, m_colorMap));
        for (int i = 1; i < steps; ++i) {
            double pos1 = leftNorm + stepLeftMiddle * i;
            double val1 = static_cast<double>(i) * (0.5 / static_cast<double>(steps));
            gradient.setColorAt(qBound(0.0, pos1, 1.0), ColorMap::valueToColor(val1, m_colorMap));

            double pos2 = middleNorm + stepMiddleRight * i;
            double val2 = 0.5 + static_cast<double>(i) * (0.5 / static_cast<double>(steps));
            gradient.setColorAt(qBound(0.0, pos2, 1.0), ColorMap::valueToColor(val2, m_colorMap));
        }
        gradient.setColorAt(qBound(0.0, rightNorm, 1.0), ColorMap::valueToColor(1.0, m_colorMap));

        painter.fillRect(QRect(mLeft, mTop, plotW, plotH), gradient);
    }

    // Draw axes
    painter.setPen(QPen(Qt::black, 1));
    painter.drawLine(mLeft, mTop, mLeft, mTop + plotH);
    painter.drawLine(mLeft, mTop + plotH, mLeft + plotW, mTop + plotH);

    // Y-axis ticks
    const double maxFreq = (m_iMaximumFrequency > 0) ? 1.1 * m_iMaximumFrequency : 1.0;
    for (double yVal : niceTicks(0.0, maxFreq, 5, 1.0)) {
        const double y = dataToPixelY(yVal);
        painter.drawLine(QPointF(mLeft - 5, y), QPointF(mLeft, y));
        painter.drawText(QRectF(0, y - 10, mLeft - 8, 20), Qt::AlignRight | Qt::AlignVCenter, tickLabel(yVal));
    }

    // X-axis ticks
    for (double dataVal : niceTicks(m_dMinAxisX, m_dMaxAxisX, 6)) {
        const double x = dataToPixelX(dataVal);
        painter.drawLine(QPointF(x, mTop + plotH), QPointF(x, mTop + plotH + 5));
        painter.drawText(QRectF(x - 40, mTop + plotH + 6, 80, 20), Qt::AlignCenter, tickLabel(dataVal));
    }

    if (!m_sTitle.isEmpty()) {
        QFont titleFont = painter.font();
        titleFont.setBold(true);
        painter.setFont(titleFont);
        painter.drawText(QRect(0, 0, w, mTop), Qt::AlignCenter, m_sTitle);
        painter.setFont(font());
    }
    if (!m_sXLabel.isEmpty()) {
        painter.drawText(QRect(mLeft, h - 22, plotW, 20), Qt::AlignCenter, m_sXLabel);
    }
    if (!m_sYLabel.isEmpty()) {
        painter.save();
        painter.translate(4, mTop + plotH / 2.0);
        painter.rotate(-90);
        painter.drawText(QRectF(-plotH / 2.0, 0, plotH, 20), Qt::AlignCenter, m_sYLabel);
        painter.restore();
    }

    if (!m_bHasData || m_seriesData.isEmpty())
        return;

    // Draw spline curve using Catmull-Rom to cubic Bezier conversion
    QPainterPath path;
    const int n = m_seriesData.size();

    if (n == 1) {
        // Single point: draw a dot
        double px = dataToPixelX(m_seriesData[0].x());
        double py = dataToPixelY(m_seriesData[0].y());
        painter.setBrush(QColor(70, 130, 180));
        painter.drawEllipse(QPointF(px, py), 3, 3);
    } else {
        // Catmull-Rom spline
        auto getPoint = [this](int idx) -> QPointF {
            const int count = m_seriesData.size();
            if (idx < 0)
                idx = 0;
            if (idx >= count)
                idx = count - 1;
            return QPointF(dataToPixelX(m_seriesData[idx].x()),
                           dataToPixelY(m_seriesData[idx].y()));
        };

        path.moveTo(getPoint(0));

        for (int i = 0; i < n - 1; ++i) {
            QPointF p0 = getPoint(i - 1);
            QPointF p1 = getPoint(i);
            QPointF p2 = getPoint(i + 1);
            QPointF p3 = getPoint(i + 2);

            // Catmull-Rom to cubic Bezier control points
            QPointF cp1(p1.x() + (p2.x() - p0.x()) / 6.0,
                        p1.y() + (p2.y() - p0.y()) / 6.0);
            QPointF cp2(p2.x() - (p3.x() - p1.x()) / 6.0,
                        p2.y() - (p3.y() - p1.y()) / 6.0);

            path.cubicTo(cp1, cp2, p2);
        }

        painter.setPen(QPen(QColor(70, 130, 180), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

    // Draw threshold lines
    if (m_bHasThresholds) {
        auto drawThresholdLine = [&](double dataX, const QColor& color) {
            if (dataX >= m_dMinAxisX && dataX <= m_dMaxAxisX) {
                double px = dataToPixelX(dataX);
                painter.setPen(QPen(color, 2));
                painter.drawLine(QPointF(px, mTop), QPointF(px, mTop + plotH));
            }
        };

        drawThresholdLine(m_dLeftThreshold, Qt::red);
        drawThresholdLine(m_dMiddleThreshold, Qt::green);
        drawThresholdLine(m_dRightThreshold, Qt::blue);
    }
}

//=============================================================================================================

void Spline::mousePressEvent(QMouseEvent* event)
{
    if (!m_bHasData || m_seriesData.isEmpty()) {
        qDebug() << "Data set not found.";
        return;
    }

    double dataX = pixelToDataX(event->position().x());

    // Check bounds
    if (dataX < m_dMinAxisX || dataX > m_dMaxAxisX)
        return;

    if (event->buttons() == Qt::LeftButton) {
        if (dataX < m_dMiddleThreshold && dataX < m_dRightThreshold) {
            m_dLeftThreshold = dataX;
            m_bHasThresholds = true;
        }
    } else if (event->buttons() == Qt::MiddleButton) {
        if (dataX > m_dLeftThreshold && dataX < m_dRightThreshold) {
            m_dMiddleThreshold = dataX;
            m_bHasThresholds = true;
        }
    } else if (event->buttons() == Qt::RightButton) {
        if (dataX > m_dLeftThreshold && dataX > m_dMiddleThreshold) {
            m_dRightThreshold = dataX;
            m_bHasThresholds = true;
        }
    }

    emit borderChanged(m_dLeftThreshold, m_dMiddleThreshold, m_dRightThreshold);

    setColorMap(m_colorMap);
    QWidget::update();
}

//=============================================================================================================

void Spline::setThreshold(const QVector3D& vecThresholdValues)
{
    if (!m_bHasData || m_seriesData.isEmpty()) {
        qDebug() << "Data set not found.";
        return;
    }

    const QVector3D& correctedVectorThreshold = vecThresholdValues;

    // Check if all values are within range
    if (correctedVectorThreshold.x() < m_dMinAxisX || correctedVectorThreshold.y() < m_dMinAxisX || correctedVectorThreshold.z() < m_dMinAxisX ||
        correctedVectorThreshold.x() > m_dMaxAxisX || correctedVectorThreshold.y() > m_dMaxAxisX || correctedVectorThreshold.z() > m_dMaxAxisX) {
        qDebug() << "One or more of the values given are out of the minimum and maximum range. Changed to default thresholds.";
        // 1 % inside each end of the span; scaling the limits themselves leaves the axis for negative values
        const double span = m_dMaxAxisX - m_dMinAxisX;
        m_dLeftThreshold = m_dMinAxisX + 0.01 * span;
        m_dMiddleThreshold = m_dMinAxisX + 0.5 * span;
        m_dRightThreshold = m_dMaxAxisX - 0.01 * span;
    } else {
        // Sort the three values into left < middle < right
        double vals[3] = {static_cast<double>(correctedVectorThreshold.x()),
                          static_cast<double>(correctedVectorThreshold.y()),
                          static_cast<double>(correctedVectorThreshold.z())};
        std::sort(vals, vals + 3);
        m_dLeftThreshold = vals[0];
        m_dMiddleThreshold = vals[1];
        m_dRightThreshold = vals[2];
    }

    m_bHasThresholds = true;
    setColorMap(m_colorMap);
    QWidget::update();
}

//=============================================================================================================

void Spline::setColorMap(const QString& colorMap)
{
    m_colorMap = colorMap;
    QWidget::update();
}

//=============================================================================================================

const QVector3D& Spline::getThreshold()
{
    m_vecReturnVector = m_bHasData ? QVector3D(static_cast<float>(m_dLeftThreshold), static_cast<float>(m_dMiddleThreshold), static_cast<float>(m_dRightThreshold)) : QVector3D();
    return m_vecReturnVector;
}

//=============================================================================================================

void Spline::setXLabel(const QString& label)
{
    m_sXLabel = label;
    QWidget::update();
}

//=============================================================================================================

void Spline::setYLabel(const QString& label)
{
    m_sYLabel = label;
    QWidget::update();
}
