//=============================================================================================================
/**
* @file     cntrol3dwidget.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
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
* @brief    Control3DWidget class declaration
*
*/

#ifndef CONTROL3DWIDGET_H
#define CONTROL3DWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3dnew_global.h"

#include "disp/helpers/roundededgeswidget.h"
#include "../view3d.h"
#include "../3DObjects/brain/braintreedelegate.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================
namespace Ui {
class Control3DWidget;
}

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DNEWLIB
//=============================================================================================================

namespace DISP3DNEWLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================



//=============================================================================================================
/**
* User GUI control for View3D.
*
* @brief User GUI control for View3D.
*/
class DISP3DNEWSHARED_EXPORT Control3DWidget : public RoundedEdgesWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<Control3DWidget> SPtr;              /**< Shared pointer type for Control3DWidget. */
    typedef QSharedPointer<const Control3DWidget> ConstSPtr;   /**< Const shared pointer type for Control3DWidget. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    */
    explicit Control3DWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Default destructor.
    *
    */
    ~Control3DWidget();

    //=========================================================================================================
    /**
    * Set/Add a View3D to be controlled by the this GUI widget.
    *
    * @param[in] view3D         The view3D to bec connected to this widget.
    */
    void setView3D(View3D::SPtr view3D);

protected slots:
    //=========================================================================================================
    /**
    * Minimizes th ewidget and all its contents.
    *
    * @param[in] state         Whether the widget is to be maximized or minimized
    */
    void onMinimizeWidget(bool state);

    //=========================================================================================================
    /**
    * Slot called when opacity slider was changed
    *
    * @param [in] value         opacity value.
    */
    void onOpacityChange(qint32 value);

    void onSceneColorPicker();

protected:
    Ui::Control3DWidget*    ui;         /**< The pointer to the QtDesigner ui class. */

    QList<View3D::SPtr>     m_lView3D;  /**< List of all connected view3D's. */
};

} // NAMESPACE DISP3DNEWLIB

#endif // CONTROL3DWIDGET_H
