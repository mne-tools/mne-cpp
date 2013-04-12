//=============================================================================================================
/**
* @file     rtserversetupwidget.h
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
* @brief    Contains the declaration of the RtServerSetupWidget class.
*
*/

#ifndef RTSERVERSETUPWIDGET_H
#define RTSERVERSETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_rtserversetup.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QTimer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RtServerPlugin
//=============================================================================================================

namespace RtServerPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class RtServer;


//=============================================================================================================
/**
* DECLARE CLASS RtServerSetupWidget
*
* @brief The RtServerSetupWidget class provides the ECG configuration window.
*/
class RtServerSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a RtServerSetupWidget which is a child of parent.
    *
    * @param [in] p_pRtServer   a pointer to the corresponding RtServer.
    * @param [in] parent        pointer to parent widget; If parent is 0, the new RtServerSetupWidget becomes a window. If parent is another widget, RtServerSetupWidget becomes a child window inside parent. RtServerSetupWidget is deleted when its parent is deleted.
    */
    RtServerSetupWidget(RtServer* p_pRtServer, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RtServerSetupWidget.
    * All RtServerSetupWidget's children are deleted first. The application exits if RtServerSetupWidget is the main widget.
    */
    ~RtServerSetupWidget();

    //=========================================================================================================
    /**
    * Inits the setup widget
    */
    void init();

//slots
    void printToLog(QString message);   /**< Implements printing messages to rtproc log.*/

    void pressedConnect();      /**< Triggers a connection trial to rt_server.*/

    void pressedSendCLI();      /**< Triggers a send request of a cli command.*/

private:
    //=========================================================================================================
    /**
    * Set command connection status
    *
    * @param[in] p_bConnectionStatus    the connection status
    */
    void cmdConnectionChanged(bool p_bConnectionStatus);

    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();

    RtServer*   m_pRtServer;    /**< Holds a pointer to corresponding ECGSimulator.*/

    Ui::RtServerSetupWidgetClass ui;       /**< Holds the user interface for the RtServerSetupWidget.*/

    QTimer m_cmdConnectionTimer;    /**< Timer for convinient command client connection. When timer times out a connection is tried to be established. */

};

} // NAMESPACE

#endif // RTSERVERSETUPWIDGET_H
