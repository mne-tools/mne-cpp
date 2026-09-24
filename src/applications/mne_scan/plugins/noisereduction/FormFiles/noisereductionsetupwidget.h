//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     noisereductionsetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2016
 * @brief    Contains the declaration of the NoiseReductionSetupWidget class.
 */

#ifndef NOISEREDUCTIONSETUPWIDGET_H
#define NOISEREDUCTIONSETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_noisereductionsetup.h"
#include "../noisereduction.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE NOISEREDUCTIONPLUGIN
//=============================================================================================================

namespace NOISEREDUCTIONPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class NoiseReduction;

//=============================================================================================================
/**
 * DECLARE CLASS NoiseReductionSetupWidget
 *
 * @brief The NoiseReductionSetupWidget class provides the NoiseReduction configuration window.
 */
class NoiseReductionSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a NoiseReductionSetupWidget which is a child of parent.
     *
     * @param[in] toolbox a pointer to the corresponding NoiseReduction.
     * @param[in] parent pointer to parent widget; If parent is 0, the new NoiseReductionSetupWidget becomes a window. If parent is another widget, NoiseReductionSetupWidget becomes a child window inside parent. NoiseReductionSetupWidget is deleted when its parent is deleted.
     */
    NoiseReductionSetupWidget(NoiseReduction* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the NoiseReductionSetupWidget.
     * All NoiseReductionSetupWidget's children are deleted first. The application exits if NoiseReductionSetupWidget is the main widget.
     */
    ~NoiseReductionSetupWidget();

private:

    NoiseReduction* m_pNoiseReduction;	/**< Holds a pointer to corresponding NoiseReduction.*/

    Ui::NoiseReductionSetupWidgetClass ui;	/**< Holds the user interface for the NoiseReductionSetupWidget.*/
};
} // NAMESPACE

#endif // NOISEREDUCTIONSETUPWIDGET_H
