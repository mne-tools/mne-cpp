//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     hpisetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Contains the declaration of the HpiSetupWidget class.
 */

#ifndef WRITETOFILESETUPWIDGET_H
#define WRITETOFILESETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_hpisetup.h"
#include "../hpi.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE HPIPLUGIN
//=============================================================================================================

namespace HPIPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Hpi;

//=============================================================================================================
/**
 * DECLARE CLASS HpiSetupWidget
 *
 * @brief The HpiSetupWidget class provides the Hpi configuration window.
 */
class HpiSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a HpiSetupWidget which is a child of parent.
     *
     * @param[in] toolbox a pointer to the corresponding Hpi.
     * @param[in] parent pointer to parent widget; If parent is 0, the new HpiSetupWidget becomes a window. If parent is another widget, HpiSetupWidget becomes a child window inside parent. HpiSetupWidget is deleted when its parent is deleted.
     */
    HpiSetupWidget(Hpi* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the HpiSetupWidget.
     * All HpiSetupWidget's children are deleted first. The application exits if HpiSetupWidget is the main widget.
     */
    ~HpiSetupWidget();

private:

    Hpi* m_pHpi;	/**< Holds a pointer to corresponding Hpi.*/

    Ui::HpiSetupWidgetClass ui;	/**< Holds the user interface for the HpiSetupWidget.*/
};
} // NAMESPACE

#endif // WRITETOFILESETUPWIDGET_H
