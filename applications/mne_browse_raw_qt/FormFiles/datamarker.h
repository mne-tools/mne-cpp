//=============================================================================================================
/**
* @file     datamarker.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2014
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
* @brief    Contains the declaration of the DataMarker class.
*
*/
#ifndef DATAMARKER_H
#define DATAMARKER_H

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPalette>
#include <QMouseEvent>
#include <QRect>
#include <QRegion>
#include <QDebug>
#include <QEvent>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBrowseRawQt
//=============================================================================================================

namespace MNEBrowseRawQt
{

/**
* DECLARE CLASS DataMarker
*
* @brief The DataWindow class provides the data dock window
*/
class DataMarker : public QWidget
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
    * Constructs a DataMarker dialog which is a child of parent
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new DataMarker becomes a window. If parent is another widget, DataMarker becomes a child window inside parent. DataWindow is deleted when its parent is deleted.
    */
    DataMarker(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * set the m_movementBoundary
    *
    * @param [in] QRect Hols the bounding rect
    */
    void setMovementBoundary(QRegion rect);

private:
    //=========================================================================================================
    /**
    * Reimplemnted mouse press event handler
    *
    * @param event QMouseEvent
    */
    void mousePressEvent(QMouseEvent *event);

    //=========================================================================================================
    /**
    * Reimplemnted mouse move event handler
    *
    * @param event QMouseEvent
    */
    void mouseMoveEvent(QMouseEvent *event);

    void enterEvent(QEvent *event);

    QPoint      m_oldPos;               /**< The old mouse position */

    QRegion     m_movableRegion;        /**< The movement boundary */

signals:
    void markerMoved();

public slots:

};

} // NAMESPACE MNEBrowseRawQt

#endif // DATAMARKER_H
