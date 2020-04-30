//=============================================================================================================
/**
 * @file     graph.cpp
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
 * @brief    Definition of the Graph class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "graph.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QResizeEvent>
#include <QPainter>

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

Graph::Graph(QWidget *parent)
: QWidget(parent)
{
    init();
}

//=============================================================================================================

void Graph::init()
{
    m_sTitle = QString("");
    m_sXLabel = QString("");
    m_sYLabel = QString("");

    //Set Borders
    m_iBorderLeftRight = 100;
    m_iBorderTopBottom = 50;

    this->setMinimumWidth(m_iBorderLeftRight*2.5);
    this->setMinimumHeight(m_iBorderTopBottom*2.5);

    //Set Fonts
    m_qFontAxes.setPixelSize(12);
    m_qPenAxes = QPen(Qt::black);

    m_qFontTitle.setPixelSize(20);
    m_qFontTitle.setBold(true);
    m_qPenTitle = QPen(Qt::black);
}

//=============================================================================================================

void Graph::setTitle(const QString &p_sTitle)
{
    m_sTitle = p_sTitle;
    update();
}

//=============================================================================================================

void Graph::setXLabel(const QString &p_sXLabel)
{
    m_sXLabel = p_sXLabel;
    update();
}

//=============================================================================================================

void Graph::setYLabel(const QString &p_sYLabel)
{
    m_sYLabel = p_sYLabel;
    update();
}

//=============================================================================================================

void Graph::drawLabels(qint32 p_iContentWidth, qint32 p_iContentHeight)
{
    QPainter painter(this);

    qint32 t_iLabelWidth = m_qSizeWidget.width()-2*m_iBorderLeftRight;
    qint32 t_iLabelHeight = 100;

    // -- Title --
    if(!m_sTitle.isEmpty())
    {
        painter.save();
        painter.setPen(m_qPenTitle);
        painter.setFont(m_qFontTitle);

        painter.translate((m_qSizeWidget.width()-t_iLabelWidth)/2, (m_qSizeWidget.height()-p_iContentHeight)/2 - m_iBorderTopBottom*1.5);
        painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sTitle);

        painter.restore();
    }

    // -- Axes --
    painter.setPen(m_qPenAxes);
    painter.setFont(m_qFontAxes);

    // X Label
    if(!m_sXLabel.isEmpty())
    {
        painter.save();
        painter.translate((m_qSizeWidget.width()-t_iLabelWidth)/2, p_iContentHeight+((m_qSizeWidget.height()-p_iContentHeight-m_iBorderTopBottom)/2));
        painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sXLabel);
        painter.restore();
    }

    //Y Label
    if(!m_sYLabel.isEmpty())
    {
        painter.save();
        painter.rotate(270);
        painter.translate(-(m_qSizeWidget.height()+t_iLabelWidth)/2,(m_qSizeWidget.width()-p_iContentWidth)/2-t_iLabelHeight*0.75);
        painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sYLabel);
        painter.restore();
    }
}

//=============================================================================================================

void Graph::resizeEvent(QResizeEvent* event)
{
    m_qSizeWidget = event->size();
    // Call base class impl
    QWidget::resizeEvent(event);
}

