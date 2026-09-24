//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     neuronalconnectivitysetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Contains the declaration of the NeuronalConnectivitySetupWidget class.
 */

#ifndef NEURONALCONNECTIVITYSETUPWIDGET_H
#define NEURONALCONNECTIVITYSETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_neuronalconnectivitysetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE NEURONALCONNECTIVITYPLUGIN
//=============================================================================================================

namespace NEURONALCONNECTIVITYPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class NeuronalConnectivity;

//=============================================================================================================
/**
 * DECLARE CLASS NeuronalConnectivitySetupWidget
 *
 * @brief The NeuronalConnectivitySetupWidget class provides the NeuronalConnectivity configuration window.
 */
class NeuronalConnectivitySetupWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a NeuronalConnectivitySetupWidget which is a child of parent.
     *
     * @param[in] toolbox a pointer to the corresponding NeuronalConnectivityToolbox.
     * @param[in] parent pointer to parent widget; If parent is 0, the new NeuronalConnectivitySetupWidget becomes a window. If parent is another widget, NeuronalConnectivitySetupWidget becomes a child window inside parent. NeuronalConnectivitySetupWidget is deleted when its parent is deleted.
     */
    NeuronalConnectivitySetupWidget(NeuronalConnectivity* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the NeuronalConnectivitySetupWidget.
     * All NeuronalConnectivitySetupWidget's children are deleted first. The application exits if NeuronalConnectivitySetupWidget is the main widget.
     */
    ~NeuronalConnectivitySetupWidget();

private:
    NeuronalConnectivity*   m_pNeuronalConnectivity;	/**< Holds a pointer to corresponding NeuronalConnectivityToolbox.*/

    Ui::NeuronalConnectivitySetupWidgetClass ui;        /**< Holds the user interface for the NeuronalConnectivitySetupWidget.*/
};
} // NAMESPACE

#endif // NEURONALCONNECTIVITYSETUPWIDGET_H
