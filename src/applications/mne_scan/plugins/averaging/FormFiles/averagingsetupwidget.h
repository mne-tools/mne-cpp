//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     averagingsetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the AveragingSetupWidget class.
 */

#ifndef AVERAGINGSETUPWIDGET_H
#define AVERAGINGSETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_averagingsetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE AVERAGINGPLUGIN
//=============================================================================================================

namespace AVERAGINGPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Averaging;

//=============================================================================================================
/**
 * DECLARE CLASS AveragingSetupWidget
 *
 * @brief The AveragingSetupWidget class provides the AveragingToolbox configuration window.
 */
class AveragingSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a AveragingSetupWidget which is a child of parent.
     *
     * @param[in] toolbox a pointer to the corresponding Averaging toolbox.
     * @param[in] parent pointer to parent widget; If parent is 0, the new AveragingSetupWidget becomes a window. If parent is another widget, AveragingSetupWidget becomes a child window inside parent. AveragingSetupWidget is deleted when its parent is deleted.
     */
    AveragingSetupWidget(Averaging* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the AveragingSetupWidget.
     * All AveragingSetupWidget's children are deleted first. The application exits if AveragingSetupWidget is the main widget.
     */
    ~AveragingSetupWidget();

private:

    Averaging* m_pAveraging;        /**< Holds a pointer to corresponding Averaging.*/

    Ui::AveragingSetupWidgetClass ui;   /**< Holds the user interface for the AveragingSetupWidget.*/
};
} // NAMESPACE

#endif // AVERAGINGSETUPWIDGET_H
