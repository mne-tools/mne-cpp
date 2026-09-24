//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiffsimulatorsetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the FiffSimulatorSetupWidget class.
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

    Ui::FiffSimulatorSetupWidgetClass ui;   /**< the user interface for the MNERtClientSetupWidget.*/

    bool m_bIsInit;                         /**< false when gui is not initialized jet. Prevents gui from already interacting when not initialized. */
};
} // NAMESPACE

#endif // FIFFSIMULATORSETUPWIDGET_H
