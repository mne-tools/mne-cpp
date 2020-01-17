//=============================================================================================================
/**
* @file     rtcmnesetupwidget.h
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
* @brief    Contains the declaration of the RtcMneSetupWidget class.
*
*/

#ifndef RTCMNESETUPWIDGET_H
#define RTCMNESETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_rtcmnesetup.h"


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
// DEFINE NAMESPACE RTCMNEPLUGIN
//=============================================================================================================

namespace RTCMNEPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class RtcMne;


//=============================================================================================================
/**
* DECLARE CLASS DummySetupWidget
*
* @brief The DummySetupWidget class provides the DummyToolbox configuration window.
*/
class RtcMneSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a RtcMneSetupWidget which is a child of parent.
    *
    * @param [in] toolbox a pointer to the corresponding MNEToolbox.
    * @param [in] parent pointer to parent widget; If parent is 0, the new RtcMneSetupWidget becomes a window. If parent is another widget, DummySetupWidget becomes a child window inside parent. DummySetupWidget is deleted when its parent is deleted.
    */
    RtcMneSetupWidget(RtcMne* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RtcMneSetupWidget.
    * All RtcMneSetupWidget's children are deleted first. The application exits if RtcMneSetupWidget is the main widget.
    */
    ~RtcMneSetupWidget();

    //=========================================================================================================
    /**
    * Adapts the UI to clustering state
    */
    void setClusteringState();

    //=========================================================================================================
    /**
    * Adapts the UI to setup state
    */
    void setSetupState();

private:

    //=========================================================================================================
    /**
    * Triggers th cluster process
    */
    void clusteringTriggered();

    //=========================================================================================================
    /**
    * Shows the About Dialogs
    */
    void showAboutDialog();

    //=========================================================================================================
    /**
    * Shows forward solution selection dialog
    */
    void showFwdFileDialog();

    //=========================================================================================================
    /**
    * Shows atlas selection dialog
    */
    void showAtlasDirDialog();

    //=========================================================================================================
    /**
    * Shows atlas selection dialog
    */
    void showSurfaceDirDialog();


    RtcMne* m_pMNE;

    Ui::RtcMneSetupWidgetClass ui;   /**< Holds the user interface for the RtcMneSetupWidgetClass.*/
};

} // NAMESPACE

#endif // RTCMNESETUPWIDGET_H
