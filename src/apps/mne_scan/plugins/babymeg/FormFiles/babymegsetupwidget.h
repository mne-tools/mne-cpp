//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     babymegsetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    BabyMEGSetupWidget class declaration.
 */

#ifndef BABYMEGSETUPWIDGET_H
#define BABYMEGSETUPWIDGET_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// DEFINE NAMESPACE BABYMEGPLUGIN
//=============================================================================================================

namespace BABYMEGPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BabyMEG;

//=============================================================================================================
/**
 * @brief Settings of the BabyMEG plugin: the acquisition server, the block size, simulation.
 */
class BabyMEGSetupWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Shows the settings of @p pBabyMEG and writes every change back to it.
     *
     * @param[in] pBabyMEG   The plugin.
     * @param[in] parent     Parent widget.
     */
    explicit BabyMEGSetupWidget(BabyMEG* pBabyMEG, QWidget* parent = nullptr);
};

} // NAMESPACE BABYMEGPLUGIN

#endif // BABYMEGSETUPWIDGET_H
