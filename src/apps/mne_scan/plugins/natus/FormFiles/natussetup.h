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
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

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
 * @brief Settings of the Natus plugin: where the datagrams arrive, the amplifier's channels and rate, the block size,
 *        simulation.
 */
class NatusSetup : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Shows the settings of @p pNatus and writes every change back to it.
     *
     * @param[in] pNatus   The plugin.
     * @param[in] parent   Parent widget.
     */
    explicit NatusSetup(Natus* pNatus, QWidget* parent = nullptr);
};

} // NAMESPACE NATUSPLUGIN

#endif // NATUSSETUP_H
