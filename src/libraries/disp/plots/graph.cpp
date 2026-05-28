//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     graph.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the Graph base widget (label storage, resize handling, label drawing).
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

    // X FsLabel
    if(!m_sXLabel.isEmpty())
    {
        painter.save();
        painter.translate((m_qSizeWidget.width()-t_iLabelWidth)/2, p_iContentHeight+((m_qSizeWidget.height()-p_iContentHeight-m_iBorderTopBottom)/2));
        painter.drawText(QRect(0, 0, t_iLabelWidth, t_iLabelHeight), Qt::AlignCenter, m_sXLabel);
        painter.restore();
    }

    //Y FsLabel
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

