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
// INCLUDES
//=============================================================================================================

#include "../babymeg_global.h"
#include "ui_babymegsetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE BABYMEGPLUGIN
//=============================================================================================================

namespace BABYMEGPLUGIN
{

//=============================================================================================================
// BABYMEGPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class BabyMEG;

//=============================================================================================================
/**
 * DECLARE CLASS BabyMEGSetupWidget
 *
 * @brief The BabyMEGSetupWidget class provides a setup widget for the BabyMEG.
 */
class BABYMEGSHARED_EXPORT BabyMEGSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a BabyMEGSetupWidget which is a child of parent.
     *
     * @param[in] p_pBabyMEG   a pointer to the corresponding BabyMEG.
     * @param[in] parent        pointer to parent widget; If parent is 0, the new BabyMEGSetupWidget becomes a window. If parent is another widget, BabyMEGSetupWidget becomes a child window inside parent. BabyMEGSetupWidget is deleted when its parent is deleted.
     */
    BabyMEGSetupWidget(BabyMEG* p_pBabyMEG, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the BabyMEGSetupWidget.
     * All BabyMEGSetupWidget's children are deleted first. The application exits if BabyMEGSetupWidget is the main widget.
     */
    ~BabyMEGSetupWidget();

    //=========================================================================================================
    /**
     * Asks the new sampling frequency from the BabyMEG plugin and updates the text field.
     */
    void setSamplingFrequency();

private:
    BabyMEG*                    m_pBabyMEG;         /**< a pointer to corresponding mne rt client.*/

    Ui::BabyMEGSetupWidgetClass ui;                 /**< the user interface for the BabyMEGSetupWidget.*/
};
} // NAMESPACE

#endif // BABYMEGSETUPWIDGET_H
