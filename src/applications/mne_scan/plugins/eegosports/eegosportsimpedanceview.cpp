//=============================================================================================================
/**
 * @file     eegosportsimpedanceview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 * @since    0.1.0
 * @date     June, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
 * @brief    Contains the implementation of the EEGoSportsImpedanceView class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportsimpedanceview.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGOSPORTSPLUGIN;
using namespace std;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsImpedanceView::EEGoSportsImpedanceView(QWidget *parent)
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

void EEGoSportsImpedanceView::wheelEvent(QWheelEvent* event)
{
    if(event->angleDelta().y()>0) // wheel was rotated forward
        this->scale(1.25,1.25);

    if(event->angleDelta().y()<0) // wheel was rotated backward
        this->scale(0.75,0.75);

    // Don't call superclass handler here as wheel is normally used for moving scrollbars
    //QGraphicsView::wheelEvent(event);
}

//=============================================================================================================

void EEGoSportsImpedanceView::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    this->fitInView(this->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);

    QGraphicsView::resizeEvent(event);
}

//=============================================================================================================

void EEGoSportsImpedanceView::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    this->fitInView(this->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);

    QGraphicsView::mouseDoubleClickEvent(event);
}