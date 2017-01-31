//=============================================================================================================
/**
* @file     epidetectsetupwidget.h
* @author   Louis Eichhorst <louis.eichhorst@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2016
*
* @section  LICENSE
*
* Copyright (C) 2017, Louis Eichhorst Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the EpidetectSetupWidget class.
*
*/

#ifndef EPIDETECTSETUPWIDGET_H
#define EPIDETECTSETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_epidetectabout.h"
#include "epidetectaboutwidget.h"
#include "../epidetect.h"
#include "../ui_epidetectsetup.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE EPIDETECTPLUGIN
//=============================================================================================================

namespace EPIDETECTPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Epidetect;


//=============================================================================================================
/**
* DECLARE CLASS EpidetectSetupWidget
*
* @brief The EpidetectSetupWidget class provides the EpidetectToolbox configuration window.
*/
class EpidetectSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a EpidetectSetupWidget which is a child of parent.
    *
    * @param [in] toolbox a pointer to the corresponding EpidetectToolbox.
    * @param [in] parent pointer to parent widget; If parent is 0, the new EpidetectSetupWidget becomes a window. If parent is another widget, EpidetectSetupWidget becomes a child window inside parent. EpidetectSetupWidget is deleted when its parent is deleted.
    */
    EpidetectSetupWidget(Epidetect* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the EpidetectSetupWidget.
    * All EpidetectSetupWidget's children are deleted first. The application exits if EpidetectSetupWidget is the main widget.
    */
    ~EpidetectSetupWidget();


private slots:
    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();

private:

    Epidetect* m_pEpidetect;	/**< Holds a pointer to corresponding EpidetectToolbox.*/

    Ui::EpidetectSetupWidgetClass ui;	/**< Holds the user interface for the EpidetectSetupWidget.*/
};

} // NAMESPACE

#endif // EPIDETECTSETUPWIDGET_H
