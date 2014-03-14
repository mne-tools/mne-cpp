//=============================================================================================================
/**
* @file     roiselectionwidget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the RealTimeMultiSampleArrayNewWidget Class.
*
*/

#ifndef ROISELECTIONWIDGET_H
#define ROISELECTIONWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xdisp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGraphicsView>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{

//=============================================================================================================
/**
* DECLARE CLASS RoiSelectionWidget
*
* @brief The RoiSelectionWidget class provides a region of interest selection for the displayed channels.
*/
class XDISPSHARED_EXPORT RoiSelectionWidget : public QGraphicsView
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a RoiSelectionWidget which is a child of parent.
    *
    * @param [in] parent        pointer to parent widget; If parent is 0, the new RoiSelectionWidget becomes a window. If parent is another widget, RoiSelectionWidget becomes a child window inside parent. RoiSelectionWidget is deleted when its parent is deleted.
    */
    RoiSelectionWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Implements the zoom in functionality. One call scales the view with factor 1.2 up.
    */
    void zoomIn();

    //=========================================================================================================
    /**
    * Implements the zoom out functionality. One call scales the view with factor 1.2 down.
    */
    void zoomOut();

protected:
//    void keyPressEvent(QKeyEvent *event);
//#ifndef QT_NO_WHEELEVENT
//    void wheelEvent(QWheelEvent *event);
//#endif

    //=========================================================================================================
    /**
    * Scales the view with a given factor.
    *
    *
    * @param [in] scaleFactor   scaling factor
    */
    void scaleView(qreal scaleFactor);

};

} // NAMESPACE

#endif // ROISELECTIONWIDGET_H
