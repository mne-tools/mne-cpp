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

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;
using namespace QtCharts;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LinePlot::LinePlot(QWidget *parent)
: QChartView(parent)
, m_pLineSeries(Q_NULLPTR)
, m_pChart(Q_NULLPTR)
{
    update();
}

//=============================================================================================================

LinePlot::LinePlot(const QVector<double> &y,
                   const QString& title,
                   QWidget *parent)
: QChartView(parent)
, m_sTitle(title)
, m_pLineSeries(Q_NULLPTR)
, m_pChart(Q_NULLPTR)
{
    updateData(y);
}

//=============================================================================================================

LinePlot::LinePlot(const QVector<double> &x,
                   const QVector<double> &y,
                   const QString& title,
                   QWidget *parent)
: QChartView(parent)
, m_sTitle(title)
, m_pLineSeries(Q_NULLPTR)
, m_pChart(Q_NULLPTR)
{
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
    update();
}

//=============================================================================================================

void LinePlot::setXLabel(const QString &p_sXLabel)
{
    m_sXLabel = p_sXLabel;
    update();
}

//=============================================================================================================

void LinePlot::setYLabel(const QString &p_sYLabel)
{
    m_sYLabel = p_sYLabel;
    update();
}

//=============================================================================================================

void LinePlot::updateData(const QVector<double> &y)
{
    QVector<double> x(y.size());

    for(int i = 0; i < x.size(); ++i) {
        x[i] = i;
    }

    return updateData(x, y);
}

//=============================================================================================================

void LinePlot::updateData(const QVector<double> &x,
                          const QVector<double> &y)
{
    if(!m_pLineSeries) {
        m_pLineSeries = new QLineSeries;
    }
    else {
        m_pLineSeries->clear();
    }

    for(int i = 0; i < x.size(); ++i) {
        m_pLineSeries->append(x[i], y[i]);
    }

    if(!m_pChart) {
        m_pChart = new QChart;
    }
    else {
        m_pChart->removeAllSeries();
    }

    m_pChart->legend()->hide();
    m_pChart->addSeries(m_pLineSeries);
    m_pChart->createDefaultAxes();

    update();
}

//=============================================================================================================

void LinePlot::update()
{
    if(!m_pChart)
        return;

    m_pChart->setTitle(m_sTitle);

    this->setChart(m_pChart);
    this->setRenderHint(QPainter::Antialiasing);
    this->setWindowTitle(m_sTitle);
}
