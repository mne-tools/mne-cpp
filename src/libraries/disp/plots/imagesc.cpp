//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     imagesc.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the ImageSc heat-map widget (matrix-to-image colour mapping and painting).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "imagesc.h"
#include "detail/niceticks.h"
#include "helpers/colormap.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFontMetrics>
#include <QPainter>

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace DISPLIB::detail;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ImageSc::ImageSc(QWidget* parent)
: Graph(parent)
{
    init();
}

//=============================================================================================================

ImageSc::ImageSc(MatrixXd& p_dMat, QWidget* parent)
: Graph(parent)
{
    init();
    updateData(p_dMat);
}

//=============================================================================================================

ImageSc::ImageSc(MatrixXf& p_fMat, QWidget* parent)
: Graph(parent)
{
    init();
    updateData(p_fMat);
}

//=============================================================================================================

ImageSc::ImageSc(MatrixXi& p_iMat, QWidget* parent)
: Graph(parent)
{
    init();
    updateData(p_iMat);
}

//=============================================================================================================

ImageSc::~ImageSc() = default;

//=============================================================================================================

void ImageSc::init()
{
    //Colormap
    pColorMapper = ColorMap::valueToColor;
    m_sColorMap = "Hot";

    //Parent init
    Graph::init();

    m_dMinValue = 0.0;
    m_dMaxValue = 0.0;
    m_bColorRangeSet = false;
    m_dLeft = m_dRight = m_dBottom = m_dTop = 0.0;
    m_bExtentSet = false;

    //Colorbar
    m_bColorbar = true;
    m_iColorbarWidth = 14;
    m_iColorbarGradSteps = 256;

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

//=============================================================================================================

void ImageSc::updateData(MatrixXd& p_dMat)
{
    if (p_dMat.rows() > 0 && p_dMat.cols() > 0) {
        m_matData = p_dMat;
        if (!m_bColorRangeSet) {
            m_dMinValue = p_dMat.minCoeff();
            m_dMaxValue = p_dMat.maxCoeff();
        }
        normalize();
        updateMaps();
    }
}

//=============================================================================================================

void ImageSc::updateData(MatrixXf& p_fMat)
{
    MatrixXd t_dMat = p_fMat.cast<double>();
    updateData(t_dMat);
}

//=============================================================================================================

void ImageSc::updateData(MatrixXi& p_iMat)
{
    MatrixXd t_dMat = p_iMat.cast<double>();
    updateData(t_dMat);
}

//=============================================================================================================

void ImageSc::normalize()
{
    m_matCentNormData = m_matData;
    m_matCentNormData.array() -= m_dMinValue;
    // A constant matrix has no range: keep it at 0 instead of dividing 0 by 0
    if (m_dMaxValue > m_dMinValue) {
        m_matCentNormData.array() /= (m_dMaxValue - m_dMinValue);
    }
    m_matCentNormData = m_matCentNormData.cwiseMax(0.0).cwiseMin(1.0);
}

//=============================================================================================================

void ImageSc::updateMaps()
{
    m_pixmapData = QPixmap();
    m_pixmapColorbar = QPixmap();

    if (m_matCentNormData.rows() > 0 && m_matCentNormData.cols() > 0) {
        const qint32 cols = m_matCentNormData.cols();
        const qint32 rows = m_matCentNormData.rows();
        QImage imageData(cols, rows, QImage::Format_RGB32);
        for (qint32 i = 0; i < cols; ++i) {
            for (qint32 j = 0; j < rows; ++j) {
                imageData.setPixel(i, j, pColorMapper(m_matCentNormData(j, i), m_sColorMap));
            }
        }
        m_pixmapData = QPixmap::fromImage(imageData);

        QImage imageColorbar(1, m_iColorbarGradSteps, QImage::Format_RGB32);
        for (qint32 j = 0; j < m_iColorbarGradSteps; ++j) {
            imageColorbar.setPixel(0, j, pColorMapper(static_cast<double>(m_iColorbarGradSteps - 1 - j) / (m_iColorbarGradSteps - 1), m_sColorMap));
        }
        m_pixmapColorbar = QPixmap::fromImage(imageColorbar);
    }
    update();
}

//=============================================================================================================

void ImageSc::setColorMap(const QString& p_sColorMap)
{
    m_sColorMap = p_sColorMap;

    updateMaps();
}

//=============================================================================================================

void ImageSc::setExtent(double left, double right, double bottom, double top)
{
    m_dLeft = left;
    m_dRight = right;
    m_dBottom = bottom;
    m_dTop = top;
    m_bExtentSet = true;
    update();
}

//=============================================================================================================

void ImageSc::setColorRange(double min, double max)
{
    m_dMinValue = min;
    m_dMaxValue = max;
    m_bColorRangeSet = true;
    if (m_matData.size() > 0) {
        normalize();
        updateMaps();
    }
}

//=============================================================================================================

void ImageSc::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    if (m_pixmapData.isNull()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing);
    const QColor text(51, 51, 51);
    const QFontMetrics metrics(font());
    const int line = metrics.height();

    // Column and row centres in data units: without an extent, column and row indices
    const int cols = m_matCentNormData.cols();
    const int rows = m_matCentNormData.rows();
    const double left = m_bExtentSet ? m_dLeft : 0.0;
    const double right = m_bExtentSet ? m_dRight : cols - 1;
    const double top = m_bExtentSet ? m_dTop : 0.0;
    const double bottom = m_bExtentSet ? m_dBottom : rows - 1;
    // Half a cell beyond the first and last centre, so each cell is centred on its value
    const double halfX = cols > 1 ? (right - left) / (cols - 1) / 2.0 : 0.5;
    const double halfY = rows > 1 ? (bottom - top) / (rows - 1) / 2.0 : 0.5;
    const double x0 = left - halfX;
    const double x1 = right + halfX;
    const double y0 = top - halfY;
    const double y1 = bottom + halfY;

    // Column and row indices are whole numbers
    const double minStep = m_bExtentSet ? 0.0 : 1.0;
    const QVector<double> xTicks = niceTicks(std::min(x0, x1), std::max(x0, x1), 8, minStep);
    const QVector<double> yTicks = niceTicks(std::min(y0, y1), std::max(y0, y1), 6, minStep);
    const QVector<double> cTicks = niceTicks(m_dMinValue, m_dMaxValue, 6);

    int yTickWidth = 0;
    for (double v : yTicks) {
        yTickWidth = std::max(yTickWidth, metrics.horizontalAdvance(tickLabel(v)));
    }
    int cTickWidth = 0;
    for (double v : cTicks) {
        cTickWidth = std::max(cTickWidth, metrics.horizontalAdvance(tickLabel(v)));
    }

    const int mLeft = yTickWidth + 14 + (m_sYLabel.isEmpty() ? 0 : line + 6);
    const int mRight = 16 + (m_bColorbar ? m_iColorbarWidth + 14 + cTickWidth : 0);
    const int mTop = m_sTitle.isEmpty() ? 12 : 2 * line + 8;
    const int mBottom = line + 14 + (m_sXLabel.isEmpty() ? 0 : line + 6);
    const QRect area(mLeft, mTop, width() - mLeft - mRight, height() - mTop - mBottom);
    if (area.width() <= 0 || area.height() <= 0) {
        return;
    }

    const auto toPixelX = [&](double x) {
        return area.left() + (x - x0) / (x1 - x0) * area.width();
    };
    const auto toPixelY = [&](double y) {
        return area.top() + (y - y0) / (y1 - y0) * area.height();
    };

    painter.drawPixmap(area, m_pixmapData);
    painter.setPen(QPen(QColor(136, 136, 136), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(area.adjusted(0, 0, -1, -1));

    painter.setPen(text);
    for (double v : xTicks) {
        const double px = toPixelX(v);
        painter.drawLine(QPointF(px, area.bottom()), QPointF(px, area.bottom() + 4));
        painter.drawText(QRectF(px - 50, area.bottom() + 6, 100, line), Qt::AlignHCenter | Qt::AlignTop, tickLabel(v));
    }
    for (double v : yTicks) {
        const double py = toPixelY(v);
        painter.drawLine(QPointF(area.left() - 4, py), QPointF(area.left(), py));
        painter.drawText(QRectF(0, py - line, area.left() - 8, 2 * line), Qt::AlignRight | Qt::AlignVCenter, tickLabel(v));
    }

    if (m_bColorbar && !m_pixmapColorbar.isNull()) {
        const QRect bar(area.right() + 16, area.top(), m_iColorbarWidth, area.height());
        painter.drawPixmap(bar, m_pixmapColorbar);
        painter.setPen(QPen(QColor(136, 136, 136), 1));
        painter.drawRect(bar.adjusted(0, 0, -1, -1));
        painter.setPen(text);
        if (m_dMaxValue > m_dMinValue) {
            for (double v : cTicks) {
                const double py = bar.bottom() - (v - m_dMinValue) / (m_dMaxValue - m_dMinValue) * bar.height();
                painter.drawLine(QPointF(bar.right(), py), QPointF(bar.right() + 4, py));
                painter.drawText(QRectF(bar.right() + 7, py - line, cTickWidth + 4, 2 * line), Qt::AlignLeft | Qt::AlignVCenter, tickLabel(v));
            }
        }
    }

    if (!m_sTitle.isEmpty()) {
        QFont titleFont = font();
        titleFont.setBold(true);
        titleFont.setPointSizeF(titleFont.pointSizeF() * 1.25);
        painter.setFont(titleFont);
        painter.drawText(QRectF(0, 4, width(), mTop - 4), Qt::AlignCenter, m_sTitle);
        painter.setFont(font());
    }
    if (!m_sXLabel.isEmpty()) {
        painter.drawText(QRectF(area.left(), height() - line - 6, area.width(), line), Qt::AlignCenter, m_sXLabel);
    }
    if (!m_sYLabel.isEmpty()) {
        painter.save();
        painter.translate(6, area.center().y());
        painter.rotate(-90);
        painter.drawText(QRectF(-area.height() / 2.0, 0, area.height(), line), Qt::AlignCenter, m_sYLabel);
        painter.restore();
    }
}
