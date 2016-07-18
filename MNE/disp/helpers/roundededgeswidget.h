//=============================================================================================================
/**
* @file     projectorwidget.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the RoundedEdgesWidget Class.
*
*/

#ifndef ROUNDEDEDGESWIDGET_H
#define ROUNDEDEDGESWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QMouseEvent>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// STRUCTS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS RoundedEdgesWidget
*
* @brief The ProjectorWidget class provides a quick control widget for scaling, filtering, projector and view options
*/
class DISPSHARED_EXPORT RoundedEdgesWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<RoundedEdgesWidget> SPtr;              /**< Shared pointer type for RoundedEdgesWidget. */
    typedef QSharedPointer<const RoundedEdgesWidget> ConstSPtr;   /**< Const shared pointer type for RoundedEdgesWidget. */

    //=========================================================================================================
    /**
    * Constructs a RoundedEdgesWidget which is a child of parent.
    *
    * @param [in] parent    parent of widget
    */
    RoundedEdgesWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);

    //=========================================================================================================
    /**
    * Destructs a RoundedEdgesWidget
    */
    ~RoundedEdgesWidget();

protected:
    //=========================================================================================================
    /**
    * Reimplmented mouseMoveEvent.
    */
    void mouseMoveEvent(QMouseEvent *event);

    //=========================================================================================================
    /**
    * Reimplmented mouseMoveEvent.
    */
    void mousePressEvent(QMouseEvent *event);

    //=========================================================================================================
    /**
    * Reimplmented mouseMoveEvent.
    */
    void resizeEvent(QResizeEvent *event);

    //=========================================================================================================
    /**
    * Calculates a rect with rounded edged.
    *
    * @param [in] rect the rect which is supposed to be rounded.
    * @param [in] r the radius of round edges.
    * @return the rounded rect in form of a QRegion
    */
    QRegion roundedRect(const QRect& rect, int r);

private:
    QPoint      m_dragPosition;     /**< the drag position of the window */
};

} // NAMESPACE DISPLIB


#endif // ROUNDEDEDGESWIDGET_H
