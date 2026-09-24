//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     tmsiimpedanceview.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2014
 * @brief    Contains the implementation of the TMSIImpedanceView class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsiimpedanceview.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPLUGIN;
using namespace std;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSIImpedanceView::TMSIImpedanceView(QWidget *parent)
: QGraphicsView(parent)
{
    // Enable scene interactions
    this->setInteractive(true);

    // Set scene rectangle
    this->setSceneRect(-25000, -25000, 50000, 50000);

    // Disable scroll bars - only use drag mode to navigate through scene
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Activate dragging
    this->setDragMode(QGraphicsView::ScrollHandDrag);

    // Zoom to mouse cursor position
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
}

//=============================================================================================================

void TMSIImpedanceView::wheelEvent(QWheelEvent* event)
{
    if(event->angleDelta().y()>0) // wheel was rotated forward
        this->scale(1.25,1.25);

    if(event->angleDelta().y()<0) // wheel was rotated backward
        this->scale(0.75,0.75);

    // Don't call superclass handler here as wheel is normally used for moving scrollbars
    //QGraphicsView::wheelEvent(event);
}

//=============================================================================================================

void TMSIImpedanceView::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    this->fitInView(this->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);

    QGraphicsView::resizeEvent(event);
}

//=============================================================================================================

void TMSIImpedanceView::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    this->fitInView(this->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);

    QGraphicsView::mouseDoubleClickEvent(event);
}
