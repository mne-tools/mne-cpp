//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eegosportssetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    Contains the declaration of the EEGoSportsSetupWidget class.
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
