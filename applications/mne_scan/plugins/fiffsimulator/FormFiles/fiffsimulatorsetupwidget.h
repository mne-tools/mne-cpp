//=============================================================================================================
/**
 * @file     fiffsimulatorsetupwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the FiffSimulatorSetupWidget class.
 *
 */

#ifndef FIFFSIMULATORSETUPWIDGET_H
#define FIFFSIMULATORSETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_fiffsimulatorsetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE FIFFSIMULATORPLUGIN
//=============================================================================================================

namespace FIFFSIMULATORPLUGIN
{

//=============================================================================================================
// FIFFSIMULATORPLUGIN FORWARD DECLARATIONS 
//=============================================================================================================

class FiffSimulator;

//=============================================================================================================
/**
 * DECLARE CLASS FiffSimulatorSetupWidget
 *
 * @brief The FiffSimulatorSetupWidget class provides the Fiff configuration window.
 */
class FiffSimulatorSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a FiffSimulatorSetupWidget which is a child of parent.
     *
     * @param[in] p_pFiffSimulator   a pointer to the corresponding FiffSimulator.
     * @param[in] parent        pointer to parent widget; If parent is 0, the new FiffSimulatorSetupWidget becomes a window. If parent is another widget, FiffSimulatorSetupWidget becomes a child window inside parent. FiffSimulatorSetupWidget is deleted when its parent is deleted.
     */
    FiffSimulatorSetupWidget(FiffSimulator* p_pFiffSimulator, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the FiffSimulatorSetupWidget.
     * All FiffSimulatorSetupWidget's children are deleted first. The application exits if FiffSimulatorSetupWidget is the main widget.
     */
    ~FiffSimulatorSetupWidget();

    //=========================================================================================================
    /**
     * Inits the setup widget
     */
    void init();

//slots
    void bufferSizeEdited();        /**< Buffer size edited and set new buffer size.*/

    void printToLog(QString message);   /**< Implements printing messages to rtproc log.*/

    void pressedConnect();          /**< Triggers a connection trial to rt_server.*/

    void pressedSendCLI();          /**< Triggers a send request of a cli command.*/

    void fiffInfoReceived();        /**< Triggered when new fiff info is recieved by producer and stored intor rt_server. */

private:
    //=========================================================================================================
    /**
     * Set command connection status
     *
     * @param[in] p_bConnectionStatus    the connection status.
     */
    void cmdConnectionChanged(bool p_bConnectionStatus);

    FiffSimulator*   m_pFiffSimulator;      /**< a pointer to corresponding mne rt client.*/

    Ui::FiffSimulatorSetupWidgetClass ui;   /**< the user interface for the MneRtClientSetupWidget.*/

    bool m_bIsInit;                         /**< false when gui is not initialized jet. Prevents gui from already interacting when not initialized. */
};
} // NAMESPACE

#endif // FIFFSIMULATORSETUPWIDGET_H
