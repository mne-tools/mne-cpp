//=============================================================================================================
/**
* @file     ftbuffersetupwidget.h
* @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Janurary, 2020
*
* @section  LICENSE
*
* Copyright (C) 2020, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the DummySetupWidget class.
*
*/

#ifndef FTBUFFERSETUPWIDGET_H
#define FTBUFFERSETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_ftbuffersetup.h"
#include "ftbufferaboutwidget.h"
#include "../ftbuffer.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FtBufferPlugin
//=============================================================================================================

namespace FTBUFFERPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FtBuffer;


//=============================================================================================================
/**
* DECLARE CLASS FtBufferSetupWidget
*
* @brief The FtBufferSetupWidget class provides the FtBuffer configuration window.
*/
class FtBufferSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a FtBufferSetupWidget which is a child of parent.
    *
    * @param [in] toolbox a pointer to the corresponding FtBuffer.
    * @param [in] parent pointer to parent widget; If parent is 0, the new FtBufferSetupWidget becomes a window. If parent is another widget, FtBufferSetupWidget becomes a child window inside parent. FtBufferSetupWidget is deleted when its parent is deleted.
    */
    FtBufferSetupWidget(FtBuffer* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the FtBufferSetupWidget.
    * All FtBufferSetupWidget's children are deleted first. The application exits if FtBufferSetupWidget is the main widget.
    */
    ~FtBufferSetupWidget();


private slots:
    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();

    //=========================================================================================================
    /**
    * Connects buffer client to address currently stored in client, or disconnects if already connected.
    * Also changes button text to indicate whether clicking it will connect of disocnnect client.
    * To-do: add change address.
    * @brief attempts to connect/disconnect member buffer client to buffer
    */
    void pressedConnect();

    //=========================================================================================================
    /**
    * Starts trying to receive data from buffer
    * @brief start receiving data from buffer
    */
    void pressedStart();

signals:


private:

    FtBuffer* m_pFtBuffer;	/**< Holds a pointer to corresponding FtBuffer.*/

    Ui::FtBufferSetupWidgetClass ui;	/**< Holds the user interface for the FtBufferSetupWidget.*/
};

} // NAMESPACE

#endif // FTBUFFERSETUPWIDGET_H
