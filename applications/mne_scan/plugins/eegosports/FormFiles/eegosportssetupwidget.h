//=============================================================================================================
/**
 * @file     eegosportssetupwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the declaration of the EEGoSportsSetupWidget class.
 *
 */

#ifndef EEGOSPORTSSETUPWIDGET_H
#define EEGOSPORTSSETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_eegosportssetup.h"
#include <QPixmap>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE EEGOSPORTSPLUGIN
//=============================================================================================================

namespace EEGOSPORTSPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class EEGoSports;

//=============================================================================================================
/**
 * DECLARE CLASS EEGoSportsSetupWidget
 *
 * @brief The EEGoSportsSetupWidget class provides the EEGoSports configuration window.
 */
class EEGoSportsSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a EEGoSportsSetupWidget which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new EEGoSportsSetupWidget becomes a window. If parent is another widget, EEGoSportsSetupWidget becomes a child window inside parent. EEGoSportsSetupWidget is deleted when its parent is deleted.
     * @param[in] pEEGoSports a pointer to the corresponding ECGSimulator.
     */
    EEGoSportsSetupWidget(EEGoSports* pEEGoSports, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the EEGoSportsSetupWidget.
     * All EEGoSportsSetupWidget's children are deleted first. The application exits if EEGoSportsSetupWidget is the main widget.
     */
    ~EEGoSportsSetupWidget();

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
     * Open the impedance measurement dialog.
     *
     */
    void onImpedanceButtonClicked();


    //=========================================================================================================
    /**
     * Sets flag for writing to a file.
     *
     */
    void setWriteToFile();

    EEGoSports*               m_pEEGoSports;           /**< a pointer to corresponding EEGoSports.*/

    Ui::EEGoSportsSetupClass  ui;                      /**< the user interface for the EEGoSportsSetupWidget.*/
};
} // NAMESPACE

#endif // EEGOSPORTSSETUPWIDGET_H
