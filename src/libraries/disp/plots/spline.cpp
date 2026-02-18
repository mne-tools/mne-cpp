//=============================================================================================================
/**
 * @file     spline.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of spline class
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spline.h"

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
    Q_UNUSED(title)
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
    const double maxY = (m_iMaximumFrequency > 0) ? static_cast<double>(m_iMaximumFrequency) : 1.0;
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

void Spline::paintEvent(QPaintEvent * /*event*/)
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

    Q_UNUSED(h)

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
    const int maxFreq = (m_iMaximumFrequency > 0) ? m_iMaximumFrequency : 1;
    const int nYTicks = 5;
    for (int i = 0; i <= nYTicks; ++i) {
        int yVal = maxFreq * i / nYTicks;
        int y = mTop + plotH - (plotH * i / nYTicks);
        painter.drawLine(mLeft - 5, y, mLeft, y);
        painter.drawText(QRect(0, y - 10, mLeft - 8, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(yVal));
    }

    // X-axis ticks
    const int nXTicks = 5;
    for (int i = 0; i <= nXTicks; ++i) {
        double dataVal = m_dMinAxisX + (m_dMaxAxisX - m_dMinAxisX) * i / nXTicks;
        int x = mLeft + plotW * i / nXTicks;
        painter.drawLine(x, mTop + plotH, x, mTop + plotH + 5);
        painter.drawText(QRect(x - 30, mTop + plotH + 6, 60, 20), Qt::AlignCenter, QString::number(dataVal, 'g', 3));
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
            if (idx < 0) idx = 0;
            if (idx >= count) idx = count - 1;
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

void Spline::mousePressEvent(QMouseEvent *event)
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

    double emitLeft = m_dLeftThreshold * pow(10, m_vecResultExponentValues[0]);
    double emitMiddle = m_dMiddleThreshold * pow(10, m_vecResultExponentValues[0]);
    double emitRight = m_dRightThreshold * pow(10, m_vecResultExponentValues[0]);
    emit borderChanged(emitLeft, emitMiddle, emitRight);

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

    QVector3D correctedVectorThreshold = correctionDisplayTrueValue(vecThresholdValues, "up");

    // Check if all values are within range
    if (correctedVectorThreshold.x() < m_dMinAxisX || correctedVectorThreshold.y() < m_dMinAxisX || correctedVectorThreshold.z() < m_dMinAxisX ||
        correctedVectorThreshold.x() > m_dMaxAxisX || correctedVectorThreshold.y() > m_dMaxAxisX || correctedVectorThreshold.z() > m_dMaxAxisX) {
        qDebug() << "One or more of the values given are out of the minimum and maximum range. Changed to default thresholds.";
        m_dLeftThreshold = 1.01 * m_dMinAxisX;
        m_dMiddleThreshold = (m_dMinAxisX + m_dMaxAxisX) / 2.0;
        m_dRightThreshold = 0.99 * m_dMaxAxisX;
    } else {
        // Sort the three values into left < middle < right
        double vals[3] = { static_cast<double>(correctedVectorThreshold.x()),
                           static_cast<double>(correctedVectorThreshold.y()),
                           static_cast<double>(correctedVectorThreshold.z()) };
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
    QVector3D originalVector;
    originalVector.setX(static_cast<float>(m_dLeftThreshold));
    originalVector.setY(static_cast<float>(m_dMiddleThreshold));
    originalVector.setZ(static_cast<float>(m_dRightThreshold));
    m_vecReturnVector = correctionDisplayTrueValue(originalVector, "down");
    return m_vecReturnVector;
}

//=============================================================================================================

QVector3D Spline::correctionDisplayTrueValue(QVector3D vecOriginalValues, QString upOrDown)
{
    QVector3D returnCorrectedVector;

    if(m_vecResultExponentValues.rows() > 0) {
        int exponent = 0;
        if (upOrDown == "up")
        {
            if (m_vecResultExponentValues[0] < 0)
            {
                exponent = std::abs(m_vecResultExponentValues[0]);
            }
            else if (m_vecResultExponentValues[0] > 0)
            {
                exponent = -(std::abs(m_vecResultExponentValues[0]));
            }
        }
        else if (upOrDown == "down")
        {
            if (m_vecResultExponentValues[0] < 0)
            {
                exponent = -(std::abs(m_vecResultExponentValues[0]));
            }
            else if (m_vecResultExponentValues[0] > 0)
            {
                exponent = std::abs(m_vecResultExponentValues[0]);
            }
        }
        else
        {
            qDebug() << "Spline::correctionDisplayTrueValue error.";
        }

        returnCorrectedVector.setX(vecOriginalValues.x() * static_cast<float>(pow(10, exponent)));
        returnCorrectedVector.setY(vecOriginalValues.y() * static_cast<float>(pow(10, exponent)));
        returnCorrectedVector.setZ(vecOriginalValues.z() * static_cast<float>(pow(10, exponent)));
    }

    return returnCorrectedVector;
}
