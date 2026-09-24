//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     brainampsetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Contains the declaration of the BrainAMPSetupWidget class.
 */

#ifndef BRAINAMPSETUPWIDGET_H
#define BRAINAMPSETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_brainampsetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE BRAINAMPPLUGIN
//=============================================================================================================

namespace BRAINAMPPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainAMP;

//=============================================================================================================
/**
 * DECLARE CLASS BrainAMPSetupWidget
 *
 * @brief The BrainAMPSetupWidget class provides the BrainAMP configuration window.
 */
class BrainAMPSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a BrainAMPSetupWidget which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new BrainAMPSetupWidget becomes a window. If parent is another widget, BrainAMPSetupWidget becomes a child window inside parent. BrainAMPSetupWidget is deleted when its parent is deleted.
     * @param[in] pBrainAMP a pointer to the corresponding ECGSimulator.
     */
    BrainAMPSetupWidget(BrainAMP* pBrainAMP, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the BrainAMPSetupWidget.
     * All BrainAMPSetupWidget's children are deleted first. The application exits if BrainAMPSetupWidget is the main widget.
     */
    ~BrainAMPSetupWidget();

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
     * Forward the device sampling frequency.
     *
     */
    void setSamplingFreq();

    //=========================================================================================================
    /**
     * Forward the device samples per block.
     *
     */
    void setSamplesPerBlock();

    //=========================================================================================================
    /**
     * Shows the About Dialog
     *
     */
    void showAboutDialog();

    BrainAMP*               m_pBrainAMP;            /**< a pointer to corresponding BrainAMP.*/
    Ui::BrainAMPSetupClass  ui;                     /**< the user interface for the BrainAMPSetupWidget.*/
};
} // NAMESPACE

#endif // BRAINAMPSETUPWIDGET_H
