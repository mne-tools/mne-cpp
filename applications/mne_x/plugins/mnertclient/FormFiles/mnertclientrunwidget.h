//=============================================================================================================
/**
* @file     mnertclientrunwidget.h
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
* @brief    Contains the declaration of the MneRtClientRunWidget class.
*
*/

#ifndef MNERTCLIENTRUNWIDGET_H
#define MNERTCLIENTRUNWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_mnertclientrun.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MneRtClientPlugin
//=============================================================================================================

namespace MneRtClientPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MneRtClient;


//=============================================================================================================
/**
* DECLARE CLASS MneRtClientRunWidget
*
* @brief The MneRtClientRunWidget class provides the ECG configuration window for the run mode.
*/
class MneRtClientRunWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a MneRtClientRunWidget which is a child of parent.
    *
    * @param [in] simulator a pointer to the corresponding ECG Simulator.
    * @param [in] parent pointer to parent widget; If parent is 0, the new MneRtClientRunWidget becomes a window. If parent is another widget, MneRtClientRunWidget becomes a child window inside parent. MneRtClientRunWidget is deleted when its parent is deleted.
    */
    MneRtClientRunWidget(MneRtClient* simulator, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the MneRtClientRunWidget.
    * All MneRtClientRunWidget's children are deleted first. The application exits if MneRtClientRunWidget is the main widget.
    */
    ~MneRtClientRunWidget();

private slots:
    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();

private:
    MneRtClient* m_pMneRtClient;      /**< Holds a pointer to corresponding ECGSimulator.*/

    Ui::MneRtClientRunClass ui;    /**< Holds the user interface for the MneRtClientRunWidget.*/
};

} // NAMESPACE

#endif // MNERTCLIENTRUNWIDGET_H
