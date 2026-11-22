//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     brainflowsetupwidget.h
 * @author   Andrey Parfenov <a1994ndrey@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Contains the declaration of the BrainFlowSetupWidget class.
 */

#ifndef BRAINFLOWSETUPWIDGET_H
#define BRAINFLOWSETUPWIDGET_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// DEFINE NAMESPACE BRAINFLOWBOARDPLUGIN
//=============================================================================================================

namespace BRAINFLOWBOARDPLUGIN
{

class BrainFlowBoard;

//=============================================================================================================
/**
 * @brief Settings of the BrainFlow plugin: the board, BrainFlow's input parameters, BrainFlow's library and the
 *        block size.
 */
class BrainFlowSetupWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Shows the settings of @p board and writes every change back to it.
     *
     * @param[in] board    The plugin.
     * @param[in] parent   Parent widget.
     */
    explicit BrainFlowSetupWidget(BrainFlowBoard* board, QWidget* parent = nullptr);
};
} // NAMESPACE BRAINFLOWBOARDPLUGIN

#endif // BRAINFLOWSETUPWIDGET_H
