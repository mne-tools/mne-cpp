//=============================================================================================================
/**
* @file     realtimesamplearray_new_widget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of the RoiSelectionWidget Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "roiselectionwidget.h"
#include "roi.h"

#include <math.h>

#include <QKeyEvent>

#include <QGraphicsSvgItem>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RoiSelectionWidget::RoiSelectionWidget(QWidget *parent)
: QGraphicsView(parent)
{
    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    scene->setSceneRect(-200, -200, 400, 400);
    setScene(scene);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(AnchorUnderMouse);
    scale(qreal(0.8), qreal(0.8));
    setMinimumSize(400, 400);
    setWindowTitle(tr("ROI Channel Selection"));

    Roi *frontal_left = new Roi(":/images/frontal_left", this);
    Roi *frontal_right = new Roi(":/images/frontal_right", this);
    Roi *parietal_left = new Roi(":/images/parietal_left", this);
    Roi *parietal_right = new Roi(":/images/parietal_right", this);
    Roi *occipital_left = new Roi(":/images/occipital_left", this);
    Roi *occipital_right = new Roi(":/images/occipital_right", this);

    scene->addItem(frontal_left);
    scene->addItem(frontal_right);
    scene->addItem(parietal_left);
    scene->addItem(parietal_right);
    scene->addItem(occipital_left);
    scene->addItem(occipital_right);

    frontal_left->setPos(118, 140);
    frontal_right->setPos(-134, 140);
    parietal_left->setPos(126, 0);
    parietal_right->setPos(-126, 0);
    occipital_left->setPos(124, -136);
    occipital_right->setPos(-76, -136);

    scene->setSceneRect(-154.0f, -156.0f, 540, 670);
}


//*************************************************************************************************************

void RoiSelectionWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        zoomIn();
        break;
    case Qt::Key_Minus:
        zoomOut();
        break;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}

#ifndef QT_NO_WHEELEVENT
void RoiSelectionWidget::wheelEvent(QWheelEvent *event)
{
    scaleView(pow((double)2, -event->delta() / 240.0));
}
#endif


//*************************************************************************************************************

void RoiSelectionWidget::scaleView(qreal scaleFactor)
{
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;

    scale(scaleFactor, scaleFactor);
}


//*************************************************************************************************************

void RoiSelectionWidget::zoomIn()
{
    scaleView(qreal(1.2));
}


//*************************************************************************************************************

void RoiSelectionWidget::zoomOut()
{
    scaleView(1 / qreal(1.2));
}
