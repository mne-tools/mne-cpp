//=============================================================================================================
/**
 * @file     natussetup.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the NatusSetup class.
 *
 */

#ifndef NATUSSETUP_H
#define NATUSSETUP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_natussetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE NATUSPLUGIN
//=============================================================================================================

namespace NATUSPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Natus;

//=============================================================================================================
/**
 * DECLARE CLASS NatusSetup
 *
 * @brief The NatusSetup class provides the Natus configuration window.
 */
class NatusSetup : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a NatusSetup which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new NatusSetup becomes a window. If parent is another widget, NatusSetup becomes a child window inside parent. NatusSetup is deleted when its parent is deleted.
     * @param[in] pNatus a pointer to the corresponding parent.
     */
    NatusSetup(Natus* pNatus, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the NatusSetup.
     * All NatusSetup's children are deleted first. The application exits if NatusSetup is the main widget.
     */
    ~NatusSetup();

    //=========================================================================================================
    /**
     * Initializes the Connector's GUI properties.
     */
    void initGui();

private:
    //=========================================================================================================
    /**
     * Sets the device sampling properties.
     */
    void setDeviceSamplingProperties();

    //=========================================================================================================
    /**
     * Forward the device sampling frequency.
     */
    void setSamplingFreq();

    //=========================================================================================================
    /**
     * Forward the device number of channels.
     */
    void setNumberChannels();

    //=========================================================================================================
    /**
     * Forward the device samples per block.
     */
    void setSamplesPerBlock();

    Natus*                  m_pNatus;          /**< A pointer to corresponding Natus.*/
    Ui::NatusSetupWidget    ui;                /**< The user interface for the NatusSetup.*/
};
} // NAMESPACE

#endif // NATUSSETUP_H
