//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     tmsisetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     September, 2013
 * @brief    Contains the declaration of the TMSISetupWidget class.
 */

#ifndef TMSISETUPWIDGET_H
#define TMSISETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include "tmsiimpedanceview.h"

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class TMSISetupClass;
}

//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace TMSIPLUGIN
{

//=============================================================================================================
// TMSIPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class TMSI;

//=============================================================================================================
/**
 * DECLARE CLASS TMSISetupWidget
 *
 * @brief The TMSISetupWidget class provides the TMSI configuration window.
 */
class TMSISetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a TMSISetupWidget which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new TMSISetupWidget becomes a window. If parent is another widget, TMSISetupWidget becomes a child window inside parent. TMSISetupWidget is deleted when its parent is deleted.
     * @param[in] pTMSI a pointer to the corresponding ECGSimulator.
     */
    TMSISetupWidget(TMSI* pTMSI, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the TMSISetupWidget.
     * All TMSISetupWidget's children are deleted first. The application exits if TMSISetupWidget is the main widget.
     */
    ~TMSISetupWidget();

    //=========================================================================================================
    /**
     * Initializes the Connector's GUI properties.
     *
     */
    void initGui();

private:

    //=========================================================================================================
    /**
     * Sets the device sampling properties.
     *
     */
    void setDeviceSamplingProperties();

    //=========================================================================================================
    /**
     * Sets flag for writing to a file.
     *
     */
    void setWriteToDebugFile();

    //=========================================================================================================
    /**
     * Sets the triggering properties
     *
     */
    void setTriggerProperties();

    TMSI*                   m_pTMSI;                 /**< a pointer to corresponding TMSI.*/

    Ui::TMSISetupClass*     m_pUi;                   /**< the user interface for the TMSISetupWidget.*/
};
} // NAMESPACE

#endif // TMSISETUPWIDGET_H
