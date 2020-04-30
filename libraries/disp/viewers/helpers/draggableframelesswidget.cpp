//=============================================================================================================
/**
 * @file     draggableframelesswidget.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the DraggableFramelessWidget Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "draggableframelesswidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QMouseEvent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DraggableFramelessWidget::DraggableFramelessWidget(QWidget *parent,
                                                   Qt::WindowFlags flags,
                                                   bool bRoundEdges,
                                                   bool bDraggable,
                                                   bool bFrameless)
: QWidget(parent, flags)
, m_bRoundEdges(bRoundEdges)
, m_bDraggable(bDraggable)
, m_bMousePressed(false)
{
    this->setWindowFlag(Qt::CustomizeWindowHint, bFrameless);
    this->adjustSize();
}

//=============================================================================================================

DraggableFramelessWidget::~DraggableFramelessWidget()
{
}

//=============================================================================================================

void DraggableFramelessWidget::setDraggable(bool bFlag)
{
    m_bDraggable = bFlag;
}

//=============================================================================================================

void DraggableFramelessWidget::mousePressEvent(QMouseEvent *event)
{
    if(!m_bDraggable) {
        return QWidget::mousePressEvent(event);
    }

    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
        m_bMousePressed = true;
    }
}

//=============================================================================================================

void DraggableFramelessWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(!m_bDraggable) {
        return QWidget::mouseMoveEvent(event);
    }

    if (event->buttons() && Qt::LeftButton && m_bMousePressed) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

//=============================================================================================================

void DraggableFramelessWidget::mouseReleaseEvent(QMouseEvent *event )
{
    if(event->button() == Qt::LeftButton) {
        m_bMousePressed = false;
    }
}

//=============================================================================================================

void DraggableFramelessWidget::resizeEvent(QResizeEvent * /* event */)
{
    if(m_bRoundEdges) {
        setMask(roundedRect(QRect(0,0,width(),height()),10));
    }
}

//=============================================================================================================

QRegion DraggableFramelessWidget::roundedRect(const QRect& rect, int r)
{
    QRegion region;
    // middle and borders
    region += rect.adjusted(r, 0, -r, 0);
    region += rect.adjusted(0, r, 0, -r);
    // top left
    QRect corner(rect.topLeft(), QSize(r*2, r*2));
    region += QRegion(corner, QRegion::Ellipse);
    // top right
    corner.moveTopRight(rect.topRight());
    region += QRegion(corner, QRegion::Ellipse);
    // bottom left
    corner.moveBottomLeft(rect.bottomLeft());
    region += QRegion(corner, QRegion::Ellipse);
    // bottom right
    corner.moveBottomRight(rect.bottomRight());
    region += QRegion(corner, QRegion::Ellipse);
    return region;
}
