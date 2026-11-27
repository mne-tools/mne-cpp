//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     graph.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     June 2013
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

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Graph::Graph(QWidget* parent)
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

    this->setMinimumWidth(m_iBorderLeftRight * 2.5);
    this->setMinimumHeight(m_iBorderTopBottom * 2.5);

    //Set Fonts
    // Grayscale antialiasing: subpixel fringes would colour the text differently per platform
    m_qFontAxes.setPixelSize(12);
    m_qFontAxes.setStyleStrategy(QFont::NoSubpixelAntialias);
    m_qPenAxes = QPen(Qt::black);

    m_qFontTitle.setPixelSize(20);
    m_qFontTitle.setBold(true);
    m_qFontTitle.setStyleStrategy(QFont::NoSubpixelAntialias);
    m_qPenTitle = QPen(Qt::black);
}

//=============================================================================================================

void Graph::setTitle(const QString& p_sTitle)
{
    m_sTitle = p_sTitle;
    update();
}

//=============================================================================================================

void Graph::setXLabel(const QString& p_sXLabel)
{
    m_sXLabel = p_sXLabel;
    update();
}

//=============================================================================================================

void Graph::setYLabel(const QString& p_sYLabel)
{
    m_sYLabel = p_sYLabel;
    update();
}

//=============================================================================================================

void Graph::resizeEvent(QResizeEvent* event)
{
    m_qSizeWidget = event->size();
    // Call base class impl
    QWidget::resizeEvent(event);
}
