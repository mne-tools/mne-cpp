//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     datamarker.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     August, 2014
 * @version  2.1.0
 * @brief    Definition of the DataWindow class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "datamarker.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DataMarker::DataMarker(QWidget *parent) :
    QWidget(parent),
    m_oldPos(QPoint(0,0)),
    m_movableRegion(QRegion())
{
    QColor color = m_qSettings.value("DataMarker/data_marker_color", QColor(93,177,47)).value<QColor>();
    setMarkerColor(color);
}


//*************************************************************************************************************

void DataMarker::setMovementBoundary(QRegion rect)
{
    m_movableRegion = rect;
}

//*************************************************************************************************************

void DataMarker::setMarkerColor(const QColor &color)
{
    QPalette pal(palette());
    QColor alphaColor = color;
    alphaColor.setAlpha(RawSettingsConstants::DATA_MARKER_OPACITY);
    pal.setColor(QPalette::Window, alphaColor);
    setAutoFillBackground(true);
    setPalette(pal);
}


//*************************************************************************************************************

void DataMarker::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        m_oldPos = event->position().toPoint();
        emit markerPressed();
    } else if(event->button() == Qt::RightButton) {
        emit markerPressed();
        emit removeRequested();
    }
}


//*************************************************************************************************************

void DataMarker::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton) {
        const QRect boundaryRect = m_movableRegion.boundingRect();
        if(boundaryRect.isEmpty()) {
            return;
        }

        const QPoint parentCursorPos = mapToParent(event->position().toPoint());
        const int minX = boundaryRect.left();
        const int maxX = qMax(minX, boundaryRect.right() - width() + 1);
        const int nextX = qBound(minX,
                                 parentCursorPos.x() - m_oldPos.x(),
                                 maxX);

        move(nextX, boundaryRect.top());
    }
}


//*************************************************************************************************************

void DataMarker::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    setCursor(QCursor(Qt::SizeHorCursor));
}


//*************************************************************************************************************

void DataMarker::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    unsetCursor();
}


//*************************************************************************************************************

void DataMarker::moveEvent(QMoveEvent *event)
{
    Q_UNUSED(event);
    emit markerMoved();
}
