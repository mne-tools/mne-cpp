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
#include "helpers/scenepainter.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QPaintEvent>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LinePlot::LinePlot(QWidget* parent)
: QWidget(parent)
{
    setMinimumSize(200, 150);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

//=============================================================================================================

LinePlot::LinePlot(const std::vector<double>& y,
                   const QString& title,
                   QWidget* parent)
: LinePlot(parent)
{
    m_plot.set_title(title.toStdString());
    updateData(y);
}

//=============================================================================================================

LinePlot::LinePlot(const std::vector<double>& x,
                   const std::vector<double>& y,
                   const QString& title,
                   QWidget* parent)
: LinePlot(parent)
{
    m_plot.set_title(title.toStdString());
    updateData(x, y);
}

//=============================================================================================================

LinePlot::~LinePlot()
{
}

//=============================================================================================================

void LinePlot::setTitle(const QString& p_sTitle)
{
    m_plot.set_title(p_sTitle.toStdString());
    QWidget::update();
}

//=============================================================================================================

void LinePlot::setXLabel(const QString& p_sXLabel)
{
    m_plot.set_x_label(p_sXLabel.toStdString());
    QWidget::update();
}

//=============================================================================================================

void LinePlot::setYLabel(const QString& p_sYLabel)
{
    m_plot.set_y_label(p_sYLabel.toStdString());
    QWidget::update();
}

//=============================================================================================================

void LinePlot::updateData(const std::vector<double>& y)
{
    std::vector<double> x(y.size());
    for (int i = 0; i < std::ssize(x); ++i) {
        x[i] = static_cast<double>(i);
    }
    updateData(x, y);
}

//=============================================================================================================

void LinePlot::updateData(const std::vector<double>& x,
                          const std::vector<double>& y)
{
    m_plot.clear();
    addSeries(x, y);
}

//=============================================================================================================

void LinePlot::addSeries(const std::vector<double>& x,
                         const std::vector<double>& y,
                         const QString& label,
                         const QColor& color,
                         Qt::PenStyle style)
{
    m_plot.add_series(x, y, label.toStdString(), color.isValid() ? std::optional(toSceneColor(color)) : std::nullopt,
                      toSceneLineStyle(style));
    setWindowTitle(QString::fromStdString(m_plot.title()));
    QWidget::update();
}

//=============================================================================================================

int LinePlot::seriesCount() const
{
    return static_cast<int>(m_plot.series().size());
}

//=============================================================================================================

void LinePlot::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    const QtTextMetrics metrics(font());
    viz::Canvas canvas(width(), height());
    m_plot.draw(canvas, metrics, toSceneFont(font()));
    paintScene(painter, canvas.display_list(), font());
}
