//=============================================================================================================
/**
 * @file     brainampsetupwidget.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the declaration of the BrainAMPSetupWidget class.
 *
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
