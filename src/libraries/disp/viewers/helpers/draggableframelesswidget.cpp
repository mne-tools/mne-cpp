//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file draggableframelesswidget.cpp
 * @since 2022
 * @date  March 2026
 * @brief Implementation of the DraggableFramelessWidget frameless draggable container.
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
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
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

    if ((event->buttons() & Qt::LeftButton) && m_bMousePressed) {
        move(event->globalPosition().toPoint() - m_dragPosition);
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
