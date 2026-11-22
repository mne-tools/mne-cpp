//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     ftbuffersetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January, 2020
 * @brief    Contains the declaration of the FtBufferSetupWidget class.
 */

#ifndef FTBUFFERSETUPWIDGET_H
#define FTBUFFERSETUPWIDGET_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// DEFINE NAMESPACE FTBUFFERPLUGIN
//=============================================================================================================

namespace FTBUFFERPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FtBuffer;

//=============================================================================================================
/**
 * @brief Settings of the FtBuffer plugin: host and port of the buffer server, block size, simulation.
 */
class FtBufferSetupWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Shows the settings of @p pFtBuffer and writes every change back to it.
     *
     * @param[in] pFtBuffer   The plugin.
     * @param[in] parent      Parent widget.
     */
    explicit FtBufferSetupWidget(FtBuffer* pFtBuffer, QWidget* parent = nullptr);
};

} // NAMESPACE FTBUFFERPLUGIN

#endif // FTBUFFERSETUPWIDGET_H
