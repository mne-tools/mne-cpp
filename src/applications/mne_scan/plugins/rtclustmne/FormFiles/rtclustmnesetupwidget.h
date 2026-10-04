//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     rtclustmnesetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the RtClustMneSetupWidget class.
 */

#ifndef RTCLUSTMNESETUPWIDGET_H
#define RTCLUSTMNESETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_rtclustmnesetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE RTCLUSTMNEPLUGIN
//=============================================================================================================

namespace RTCLUSTMNEPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class RtClustMne;

//=============================================================================================================
/**
 * DECLARE CLASS DummySetupWidget
 *
 * @brief The DummySetupWidget class provides the DummyToolbox configuration window.
 */
class RtClustMneSetupWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a RtClustMneSetupWidget which is a child of parent.
     *
     * @param[in] toolbox a pointer to the corresponding MNEToolbox.
     * @param[in] parent pointer to parent widget; If parent is 0, the new RtClustMneSetupWidget becomes a window. If parent is another widget, DummySetupWidget becomes a child window inside parent. DummySetupWidget is deleted when its parent is deleted.
     */
    RtClustMneSetupWidget(RtClustMne* toolbox, QWidget* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RtClustMneSetupWidget.
     * All RtClustMneSetupWidget's children are deleted first. The application exits if RtClustMneSetupWidget is the main widget.
     */
    ~RtClustMneSetupWidget();

private:
    //=========================================================================================================
    /**
     * Shows atlas selection dialog
     */
    void showAtlasDirDialog();

    //=========================================================================================================
    /**
     * Shows surface selection dialog
     */
    void showSurfaceDirDialog();

    //=========================================================================================================
    /**
     * Shows transformation selection dialog
     */
    void showMriHeadFileDialog();

    RtClustMne* m_pMNE;

    Ui::RtClustMneSetupWidgetClass ui; /**< Holds the user interface for the RtClustMneSetupWidgetClass.*/
};
} // NAMESPACE

#endif // RTCLUSTMNESETUPWIDGET_H
