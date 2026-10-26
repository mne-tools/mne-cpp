//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     lineplot.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the LinePlot widget (auto-range computation and polyline painting).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lineplot.h"
#include "detail/niceticks.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFontMetrics>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include <algorithm>
#include <cmath>
#include <limits>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace DISPLIB::detail;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

const QVector<QColor> kPalette = {QColor(70, 130, 180), QColor(221, 132, 82), QColor(85, 168, 104),
                                  QColor(196, 78, 82), QColor(129, 114, 179), QColor(147, 120, 96),
                                  QColor(218, 139, 195), QColor(140, 140, 140), QColor(204, 185, 116),
                                  QColor(100, 181, 205)};

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LinePlot::LinePlot(QWidget* parent)
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

LinePlot::LinePlot(const QVector<double>& y,
                   const QString& title,
                   QWidget* parent)
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

LinePlot::LinePlot(const QVector<double>& x,
                   const QVector<double>& y,
                   const QString& title,
                   QWidget* parent)
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

void LinePlot::setTitle(const QString& p_sTitle)
{
    m_sTitle = p_sTitle;
    QWidget::update();
}

//=============================================================================================================

void LinePlot::setXLabel(const QString& p_sXLabel)
{
    m_sXLabel = p_sXLabel;
    QWidget::update();
}

//=============================================================================================================

void LinePlot::setYLabel(const QString& p_sYLabel)
{
    m_sYLabel = p_sYLabel;
    QWidget::update();
}

//=============================================================================================================

void LinePlot::updateData(const QVector<double>& y)
{
    QVector<double> x(y.size());
    for (int i = 0; i < x.size(); ++i) {
        x[i] = static_cast<double>(i);
    }
    updateData(x, y);
}

//=============================================================================================================

void LinePlot::updateData(const QVector<double>& x,
                          const QVector<double>& y)
{
    m_series.clear();
    addSeries(x, y);
}

//=============================================================================================================

void LinePlot::addSeries(const QVector<double>& x,
                         const QVector<double>& y,
                         const QString& label,
                         const QColor& color,
                         Qt::PenStyle style)
{
    const int n = qMin(x.size(), y.size());
    const bool empty = std::all_of(m_series.cbegin(), m_series.cend(), [](const Series& s) { return s.x.isEmpty(); });
    for (int i = 0; i < n; ++i) {
        const bool first = empty && i == 0;
        m_dMinX = first ? x[i] : qMin(m_dMinX, x[i]);
        m_dMaxX = first ? x[i] : qMax(m_dMaxX, x[i]);
        m_dMinY = first ? y[i] : qMin(m_dMinY, y[i]);
        m_dMaxY = first ? y[i] : qMax(m_dMaxY, y[i]);
    }
    QColor lineColor = color;
    if (!lineColor.isValid()) {
        lineColor = kPalette[m_series.size() % kPalette.size()];
        for (const QColor& candidate : kPalette) {
            if (std::none_of(m_series.cbegin(), m_series.cend(), [&](const Series& s) { return s.color == candidate; })) {
                lineColor = candidate;
                break;
            }
        }
    }
    m_series.append({x.mid(0, n), y.mid(0, n), label, lineColor, style});

    setWindowTitle(m_sTitle);
    QWidget::update();
}

//=============================================================================================================

int LinePlot::seriesCount() const
{
    return m_series.size();
}

//=============================================================================================================

void LinePlot::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const QColor text(51, 51, 51);
    const QColor grid(230, 230, 230);
    const QFont tickFont = font();
    const QFontMetrics tickMetrics(tickFont);

    double minY = m_dMinY;
    double maxY = m_dMaxY;
    const double padY = (maxY - minY) * 0.05;
    minY -= padY == 0.0 ? 1.0 : padY;
    maxY += padY == 0.0 ? 1.0 : padY;
    const double minX = m_dMinX;
    const double maxX = m_dMaxX > m_dMinX ? m_dMaxX : m_dMinX + 1.0;
    const QVector<double> yTicks = niceTicks(minY, maxY, 6);
    const QVector<double> xTicks = niceTicks(minX, maxX, 8);

    int tickWidth = 0;
    for (double v : yTicks) {
        tickWidth = qMax(tickWidth, tickMetrics.horizontalAdvance(tickLabel(v)));
    }
    const int line = tickMetrics.height();
    const int mLeft = tickWidth + 14 + (m_sYLabel.isEmpty() ? 0 : line + 6);
    const int mRight = 24;
    const int mTop = m_sTitle.isEmpty() ? 16 : 2 * line + 8;
    const int mBottom = line + 14 + (m_sXLabel.isEmpty() ? 0 : line + 6);
    QRectF area(mLeft, mTop, width() - mLeft - mRight, height() - mTop - mBottom);
    if (area.width() <= 0 || area.height() <= 0) {
        return;
    }

    auto toPixelX = [&](double dx) {
        return area.left() + (dx - minX) / (maxX - minX) * area.width();
    };
    auto toPixelY = [&](double dy) {
        return area.bottom() - (dy - minY) / (maxY - minY) * area.height();
    };

    // Legend: in the corner of the axes that hides the least of the lines, else to the right of them
    int labelWidth = 0;
    int labelCount = 0;
    for (const Series& series : m_series) {
        if (!series.label.isEmpty()) {
            labelWidth = qMax(labelWidth, tickMetrics.horizontalAdvance(series.label));
            ++labelCount;
        }
    }
    const QSizeF legendSize(labelWidth + 38, labelCount * (line + 2) + 10);
    QRectF legend;
    if (labelCount > 0) {
        const QList<QRectF> corners = {
            QRectF(QPointF(area.right() - legendSize.width() - 8, area.top() + 8), legendSize),
            QRectF(QPointF(area.left() + 8, area.top() + 8), legendSize),
            QRectF(QPointF(area.right() - legendSize.width() - 8, area.bottom() - legendSize.height() - 8), legendSize),
            QRectF(QPointF(area.left() + 8, area.bottom() - legendSize.height() - 8), legendSize)};
        double fewest = std::numeric_limits<double>::max();
        for (const QRectF& corner : corners) {
            const double hidden = hiddenLength(corner, toPixelX, toPixelY);
            if (hidden < fewest) {
                fewest = hidden;
                legend = corner;
            }
        }
        if (fewest > 0.0 && area.width() > 2 * legendSize.width()) {
            area.setRight(area.right() - legendSize.width() - 12);
            legend = QRectF(QPointF(area.right() + 12, area.top()), legendSize);
        }
    }

    if (!m_sTitle.isEmpty()) {
        QFont titleFont = font();
        titleFont.setBold(true);
        titleFont.setPointSizeF(titleFont.pointSizeF() * 1.25);
        painter.setFont(titleFont);
        painter.setPen(text);
        painter.drawText(QRectF(0, 4, width(), mTop - 4), Qt::AlignCenter, m_sTitle);
    }

    painter.setFont(tickFont);
    for (double v : yTicks) {
        const double py = toPixelY(v);
        painter.setPen(QPen(grid, 1));
        painter.drawLine(QPointF(area.left(), py), QPointF(area.right(), py));
        painter.setPen(text);
        painter.drawText(QRectF(0, py - line, area.left() - 8, 2 * line), Qt::AlignRight | Qt::AlignVCenter, tickLabel(v));
    }
    for (double v : xTicks) {
        const double px = toPixelX(v);
        painter.setPen(QPen(grid, 1));
        painter.drawLine(QPointF(px, area.top()), QPointF(px, area.bottom()));
        painter.setPen(text);
        painter.drawText(QRectF(px - 50, area.bottom() + 6, 100, line), Qt::AlignHCenter | Qt::AlignTop, tickLabel(v));
    }
    if (!m_sXLabel.isEmpty()) {
        painter.drawText(QRectF(area.left(), height() - line - 6, area.width(), line), Qt::AlignCenter, m_sXLabel);
    }
    if (!m_sYLabel.isEmpty()) {
        painter.save();
        painter.translate(6, area.center().y());
        painter.rotate(-90);
        painter.drawText(QRectF(-area.height() / 2, 0, area.height(), line), Qt::AlignCenter, m_sYLabel);
        painter.restore();
    }

    painter.save();
    painter.setClipRect(area);
    const qreal penWidth = m_series.size() > 20 ? 1.0 : 1.75;
    for (const Series& series : m_series) {
        if (series.x.isEmpty()) {
            continue;
        }
        QPainterPath path;
        path.moveTo(toPixelX(series.x[0]), toPixelY(series.y[0]));
        for (int i = 1; i < series.x.size(); ++i) {
            path.lineTo(toPixelX(series.x[i]), toPixelY(series.y[i]));
        }
        painter.setPen(QPen(series.color, penWidth, series.style, series.style == Qt::SolidLine ? Qt::RoundCap : Qt::FlatCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }
    painter.restore();

    painter.setPen(QPen(QColor(136, 136, 136), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(area);

    if (labelCount > 0) {
        painter.setPen(QPen(QColor(204, 204, 204), 1));
        painter.setBrush(QColor(255, 255, 255, 225));
        painter.drawRoundedRect(legend, 4, 4);
        int row = 0;
        for (const Series& series : m_series) {
            if (series.label.isEmpty()) {
                continue;
            }
            const double y = legend.top() + 5 + row++ * (line + 2) + line / 2.0;
            painter.setPen(QPen(series.color, 2.5, series.style, series.style == Qt::SolidLine ? Qt::RoundCap : Qt::FlatCap));
            painter.drawLine(QPointF(legend.left() + 8, y), QPointF(legend.left() + 26, y));
            painter.setPen(text);
            painter.drawText(QRectF(legend.left() + 32, y - line / 2.0, labelWidth + 4, line), Qt::AlignLeft | Qt::AlignVCenter, series.label);
        }
    }
}

//=============================================================================================================

double LinePlot::hiddenLength(const QRectF& rect,
                              const std::function<double(double)>& toPixelX,
                              const std::function<double(double)>& toPixelY) const
{
    double hidden = 0.0;
    for (const Series& series : m_series) {
        for (int i = 1; i < series.x.size(); ++i) {
            const QPointF a(toPixelX(series.x[i - 1]), toPixelY(series.y[i - 1]));
            const QPointF b(toPixelX(series.x[i]), toPixelY(series.y[i]));
            const double length = QLineF(a, b).length();
            const int steps = qMax(1, static_cast<int>(length / 2.0));
            for (int s = 0; s <= steps; ++s) {
                hidden += rect.contains(a + (b - a) * (static_cast<double>(s) / steps)) ? length / (steps + 1) : 0.0;
            }
        }
    }
    return hidden;
}
