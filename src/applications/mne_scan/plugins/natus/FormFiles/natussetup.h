//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     natussetup.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     June, 2018
 * @brief    Contains the declaration of the NatusSetup class.
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
