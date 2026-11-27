//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     spline.cpp
 * @author   Ricky Tjen <ricky270@student.sgu.ac.id>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April 2016
 * @brief    Implementation of the Spline histogram widget (spline interpolation, threshold markers, click signals).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spline.h"
#include "helpers/scenepainter.h"
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Spline::Spline(QWidget* parent, const QString& title)
: QWidget(parent)
{
    m_plot.set_title(title.toStdString());
    setMinimumSize(200, 150);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

//=============================================================================================================

double Spline::dataToPixelX(double dataX) const
{
    return m_plot.data_to_pixel_x(dataX, width());
}

//=============================================================================================================

double Spline::dataToPixelY(double dataY) const
{
    return m_plot.data_to_pixel_y(dataY, height());
}

//=============================================================================================================

double Spline::pixelToDataX(double pixelX) const
{
    return m_plot.pixel_to_data_x(pixelX, width());
}

//=============================================================================================================

void Spline::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    const QtTextMetrics metrics(font());
    viz::Canvas canvas(width(), height());
    m_plot.draw(canvas, metrics, toSceneFont(font()));
    paintScene(painter, canvas.display_list(), font());
}

//=============================================================================================================

void Spline::mousePressEvent(QMouseEvent* event)
{
    if (m_plot.class_marks().empty()) {
        mne::Log::debug("Data set not found.");
        return;
    }
    const double dataX = pixelToDataX(event->position().x());
    if (dataX < m_plot.min_x() || dataX > m_plot.max_x()) {
        return;
    }
    double left = m_plot.left_threshold();
    double middle = m_plot.middle_threshold();
    double right = m_plot.right_threshold();
    bool changed = false;
    if (event->buttons() == Qt::LeftButton) {
        if (dataX < middle && dataX < right) {
            left = dataX;
            changed = true;
        }
    } else if (event->buttons() == Qt::MiddleButton) {
        if (dataX > left && dataX < right) {
            middle = dataX;
            changed = true;
        }
    } else if (event->buttons() == Qt::RightButton) {
        if (dataX > left && dataX > middle) {
            right = dataX;
            changed = true;
        }
    }
    if (changed) {
        m_plot.set_thresholds(left, middle, right);
    }
    emit borderChanged(left, middle, right);
    QWidget::update();
}

//=============================================================================================================

void Spline::setThreshold(const QVector3D& vecThresholdValues)
{
    if (m_plot.class_marks().empty()) {
        mne::Log::debug("Data set not found.");
        return;
    }
    const double minX = m_plot.min_x();
    const double maxX = m_plot.max_x();
    double values[3] = {static_cast<double>(vecThresholdValues.x()), static_cast<double>(vecThresholdValues.y()),
                        static_cast<double>(vecThresholdValues.z())};
    if (std::any_of(std::begin(values), std::end(values), [&](double v) { return v < minX || v > maxX; })) {
        mne::Log::debug("One or more of the values given are out of the minimum and maximum range. Changed to default thresholds.");
        // 1 % inside each end of the span; scaling the limits themselves leaves the axis for negative values
        const double span = maxX - minX;
        m_plot.set_thresholds(minX + 0.01 * span, minX + 0.5 * span, maxX - 0.01 * span);
    } else {
        // Sort the three values into left < middle < right
        std::sort(std::begin(values), std::end(values));
        m_plot.set_thresholds(values[0], values[1], values[2]);
    }
    QWidget::update();
}

//=============================================================================================================

void Spline::setColorMap(const QString& colorMap)
{
    m_plot.set_colormap(colorMap.toStdString());
    QWidget::update();
}

//=============================================================================================================

const QVector3D& Spline::getThreshold()
{
    m_vecReturnVector = !m_plot.class_marks().empty() ? QVector3D(static_cast<float>(m_plot.left_threshold()), static_cast<float>(m_plot.middle_threshold()),
                                                                  static_cast<float>(m_plot.right_threshold()))
                                                      : QVector3D();
    return m_vecReturnVector;
}

//=============================================================================================================

void Spline::setXLabel(const QString& label)
{
    m_plot.set_x_label(label.toStdString());
    QWidget::update();
}

//=============================================================================================================

void Spline::setYLabel(const QString& label)
{
    m_plot.set_y_label(label.toStdString());
    QWidget::update();
}
