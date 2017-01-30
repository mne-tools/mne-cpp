//=============================================================================================================
/**
* @file     rerefsetupwidget.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the ReRefSetupWidget class.
*
*/

#ifndef REREFSETUPWIDGET_H
#define REREFSETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_rerefsetup.h"
#include "rerefaboutwidget.h"
#include "../reref.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE REREFPLUGIN
//=============================================================================================================

namespace REREFPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class ReRef;


//=============================================================================================================
/**
* DECLARE CLASS ReRefSetupWidget
*
* @brief The ReRefSetupWidget class provides the ReRef configuration window.
*/
class ReRefSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a ReRefSetupWidget which is a child of parent.
    *
    * @param [in] toolbox a pointer to the corresponding ReRef.
    * @param [in] parent pointer to parent widget; If parent is 0, the new ReRefSetupWidget becomes a window. If parent is another widget, ReRefSetupWidget becomes a child window inside parent. ReRefSetupWidget is deleted when its parent is deleted.
    */
    ReRefSetupWidget(ReRef* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ReRefSetupWidget.
    * All ReRefSetupWidget's children are deleted first. The application exits if ReRefSetupWidget is the main widget.
    */
    ~ReRefSetupWidget();


private slots:
    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();

private:

    ReRef* m_pReRef;	/**< Holds a pointer to corresponding ReRef.*/

    Ui::ReRefSetupWidgetClass ui;	/**< Holds the user interface for the ReRefSetupWidget.*/
};

} // NAMESPACE

#endif // REREFSETUPWIDGET_H
