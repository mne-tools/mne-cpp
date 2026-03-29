//=============================================================================================================
/**
 * @file     datamarker.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
 * @date     August, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the DataWindow class.
 *
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
    //Set background color
    QPalette Pal(palette());

    QColor color = m_qSettings.value("DataMarker/data_marker_color", QColor(93,177,47)).value<QColor>();
    color.setAlpha(DATA_MARKER_OPACITY);
    Pal.setColor(QPalette::Window, color);

    setAutoFillBackground(true);
    setPalette(Pal);
}


//*************************************************************************************************************

void DataMarker::setMovementBoundary(QRegion rect)
{
    m_movableRegion = rect;
}


//*************************************************************************************************************

void DataMarker::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
        m_oldPos = event->position().toPoint();
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

void DataMarker::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    setCursor(QCursor(Qt::SizeHorCursor));
}


//*************************************************************************************************************

void DataMarker::moveEvent(QMoveEvent *event)
{
    Q_UNUSED(event);
    emit markerMoved();
}
