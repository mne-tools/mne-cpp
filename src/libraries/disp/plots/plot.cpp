//=============================================================================================================
/**
 * @file     plot.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the Plot class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "plot.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPoint>
#include <QPainter>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Plot::Plot(QWidget *parent)
: Graph(parent)
, m_bHoldOn(false)
{
    init();
}

//=============================================================================================================

Plot::Plot(VectorXd &p_dVec, QWidget *parent)
: Graph(parent)
, m_bHoldOn(false)
{
    init();
    updateData(p_dVec);
}

//=============================================================================================================

Plot::~Plot()
{
}

//=============================================================================================================

void Plot::init()
{
    //Parent init
    Graph::init();

    m_qListVecPointFPaths.clear();

    m_dMinX = 0;
    m_dMaxX = 0;
    m_dMinY = 0;
    m_dMaxY = 0;
}

//=============================================================================================================

void Plot::updateData(VectorXd &p_dVec)
{
    if(p_dVec.size() > 0)
    {
        if(!m_bHoldOn)
            init();

        QVector<QPointF> t_qVecPointFPaths;
        //No X data given
        m_dMinX = 0 < m_dMinX ? 0 : m_dMinX;
        m_dMaxX = p_dVec.size()-1 > m_dMaxX ? p_dVec.size()-1 : m_dMaxX;

        m_dMinY = p_dVec.minCoeff() < m_dMinY ? p_dVec.minCoeff() : m_dMinY;
        m_dMaxY = p_dVec.maxCoeff() > m_dMaxY ? p_dVec.maxCoeff() : m_dMaxY;

        double t_dX = 0;
        for(qint32 i = 0; i < p_dVec.size(); ++i)
        {
            t_qVecPointFPaths.append(QPointF(t_dX, p_dVec[i]));
            t_dX += 1;
        }

        m_qListVecPointFPaths.append(t_qVecPointFPaths);

        update();
    }
}

//=============================================================================================================

void Plot::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    if (m_qListVecPointFPaths.size() > 0)
    {
        QPoint t_qPointTopLeft(m_iBorderLeftRight,m_iBorderTopBottom);

        QSize t_qSizePlot = m_qSizeWidget;

        t_qSizePlot.setWidth(t_qSizePlot.width() - 2*m_iBorderLeftRight);
        t_qSizePlot.setHeight(t_qSizePlot.height() - 2*m_iBorderTopBottom);

        //Draw background
        QPainter painter(this);
        painter.fillRect(t_qPointTopLeft.x(), t_qPointTopLeft.y(), t_qSizePlot.width(), t_qSizePlot.height(), Qt::white);

        //Draw border
        painter.drawRect(t_qPointTopLeft.x()-1, t_qPointTopLeft.y()-1, t_qSizePlot.width()+1, t_qSizePlot.height()+1);

        // -- Data --
        painter.save();
        QPen pen;
        pen.setWidth(1);
        pen.setBrush(Qt::blue);
        painter.setPen(pen);
        painter.translate(m_iBorderLeftRight-m_dMinX,m_iBorderTopBottom+t_qSizePlot.height()/2);
        for(qint32 i = 0; i < m_qListVecPointFPaths.size(); ++i)
        {
            double scale_x = t_qSizePlot.width()/(m_dMaxX - m_dMinX);
            double scale_y = (t_qSizePlot.height()-(t_qSizePlot.height()*0.1))/(m_dMaxY - m_dMinY);

            //scale
            QVector<QPointF> t_qVecPointFPath;
            QVector<QPointF>::ConstIterator it;
            for(it = m_qListVecPointFPaths[i].begin(); it != m_qListVecPointFPaths[i].end(); ++it)
                t_qVecPointFPath.append(QPointF(it->x()*scale_x, it->y()*scale_y));

            //draw
            for(it = t_qVecPointFPath.begin()+1; it != t_qVecPointFPath.end(); ++it)
                painter.drawLine(*(it-1), *it);
        }
        painter.restore();

        //Draw title & axes
        Graph::drawLabels(t_qSizePlot.width(), t_qSizePlot.height());
    }
}

