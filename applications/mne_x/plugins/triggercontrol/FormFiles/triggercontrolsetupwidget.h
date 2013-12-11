//=============================================================================================================
/**
* @file     triggercontrolsetupwidget.h
* @author   Tim Kunze <tim.kunze@tu-ilmenau.de>;
*           Luise Lang <luise.lang@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Tim Kunze, Luise Lang and Christoph Dinh. All rights reserved.
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
* @brief    Contains the declaration of the TriggerControlSetupWidget class.
*
*/

#ifndef TRIGGERCONTROLSETUPWIDGET_H
#define TRIGGERCONTROLSETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_triggercontrolsetup.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TriggerControlPlugin
//=============================================================================================================

namespace TriggerControlPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class TriggerControl;
class SerialPort;


//=============================================================================================================
/**
* DECLARE CLASS TriggerControlSetupWidget
*
* @brief The TriggerControlSetupWidget class provides the TriggerControlToolbox configuration window.
*/
class TriggerControlSetupWidget : public QWidget
{
    Q_OBJECT

    friend class SettingsWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a TriggerControlSetupWidget which is a child of parent.
    *
    * @param [in] toolbox a pointer to the corresponding TriggerControlToolbox.
    * @param [in] parent pointer to parent widget; If parent is 0, the new TriggerControlSetupWidget becomes a window. If parent is another widget, TriggerControlSetupWidget becomes a child window inside parent. TriggerControlSetupWidget is deleted when its parent is deleted.
    */
    TriggerControlSetupWidget(TriggerControl* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the DummySetupWidget.
    * All DummySetupWidget's children are deleted first. The application exits if DummySetupWidget is the main widget.
    */
    ~TriggerControlSetupWidget();


private:
    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();


    //=========================================================================================================
    /**
    * Shows the settings widget
    */
    void showSettings();


    TriggerControl* m_pTriggerControl;

    Ui::TriggerControlSetupWidgetClass ui;       /**< Holds the user interface for the TriggerControlSetupWidget.*/
};

} // NAMESPACE

#endif // TRIGGERCONTROLSETUPWIDGET_H
