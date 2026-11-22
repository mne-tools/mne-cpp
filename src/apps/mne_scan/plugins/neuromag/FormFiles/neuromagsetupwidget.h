//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     neuromagsetupwidget.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2014
 * @brief    Contains the declaration of the NeuromagSetupWidget class.
 */

#ifndef NEUROMAGSETUPWIDGET_H
#define NEUROMAGSETUPWIDGET_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// DEFINE NAMESPACE NEUROMAGPLUGIN
//=============================================================================================================

namespace NEUROMAGPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Neuromag;

//=============================================================================================================
/**
 * @brief Settings of the Neuromag plugin: acquisition directory, buffer and block sizes, autostart, simulation.
 */
class NeuromagSetupWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Shows the settings of @p pNeuromag and writes every change back to it.
     *
     * @param[in] pNeuromag   The plugin.
     * @param[in] parent      Parent widget.
     */
    explicit NeuromagSetupWidget(Neuromag* pNeuromag, QWidget* parent = nullptr);
};

} // NAMESPACE NEUROMAGPLUGIN

#endif // NEUROMAGSETUPWIDGET_H
