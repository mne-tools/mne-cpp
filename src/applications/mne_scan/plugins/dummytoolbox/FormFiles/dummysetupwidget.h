//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     dummysetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the DummySetupWidget class.
 */

#ifndef DUMMYSETUPWIDGET_H
#define DUMMYSETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dummytoolbox.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class DummySetupWidgetClass;
}

//=============================================================================================================
// DEFINE NAMESPACE DUMMYTOOLBOXPLUGIN
//=============================================================================================================

namespace DUMMYTOOLBOXPLUGIN
{

//=============================================================================================================
// DUMMYTOOLBOXPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class DummyToolbox;

//=============================================================================================================
/**
 * DECLARE CLASS DummySetupWidget
 *
 * @brief The DummySetupWidget class provides the DummyToolbox configuration window.
 */
class DummySetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a DummySetupWidget which is a child of parent.
     *
     * @param [in] toolbox a pointer to the corresponding DummyToolbox.
     * @param [in] parent pointer to parent widget; If parent is 0, the new DummySetupWidget becomes a window. If parent is another widget, DummySetupWidget becomes a child window inside parent. DummySetupWidget is deleted when its parent is deleted.
     */
    DummySetupWidget(DummyToolbox* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the DummySetupWidget.
     * All DummySetupWidget's children are deleted first. The application exits if DummySetupWidget is the main widget.
     */
    ~DummySetupWidget();

private:
    DummyToolbox*               m_pDummyToolbox;	/**< Holds a pointer to corresponding DummyToolbox.*/

    Ui::DummySetupWidgetClass*  m_pUi;              /**< Holds the user interface for the DummySetupWidget.*/
};
} // NAMESPACE

#endif // DUMMYSETUPWIDGET_H
