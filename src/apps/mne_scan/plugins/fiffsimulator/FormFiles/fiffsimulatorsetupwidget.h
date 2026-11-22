//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fiffsimulatorsetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the FiffSimulatorSetupWidget class.
 */

#ifndef FIFFSIMULATORSETUPWIDGET_H
#define FIFFSIMULATORSETUPWIDGET_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// DEFINE NAMESPACE FIFFSIMULATORPLUGIN
//=============================================================================================================

namespace FIFFSIMULATORPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffSimulator;

//=============================================================================================================
/**
 * @brief Settings of the FiffSimulator plugin: the server, the file it replays, the block size, simulation.
 */
class FiffSimulatorSetupWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Shows the settings of @p pFiffSimulator and writes every change back to it.
     *
     * @param[in] pFiffSimulator   The plugin.
     * @param[in] parent           Parent widget.
     */
    explicit FiffSimulatorSetupWidget(FiffSimulator* pFiffSimulator, QWidget* parent = nullptr);
};

} // NAMESPACE FIFFSIMULATORPLUGIN

#endif // FIFFSIMULATORSETUPWIDGET_H
