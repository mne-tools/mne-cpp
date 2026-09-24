//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     rtcmnesetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the RtcMneSetupWidget class.
 */

#ifndef RTCMNESETUPWIDGET_H
#define RTCMNESETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_rtcmnesetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE RTCMNEPLUGIN
//=============================================================================================================

namespace RTCMNEPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class RtcMne;

//=============================================================================================================
/**
 * DECLARE CLASS DummySetupWidget
 *
 * @brief The DummySetupWidget class provides the DummyToolbox configuration window.
 */
class RtcMneSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a RtcMneSetupWidget which is a child of parent.
     *
     * @param[in] toolbox a pointer to the corresponding MNEToolbox.
     * @param[in] parent pointer to parent widget; If parent is 0, the new RtcMneSetupWidget becomes a window. If parent is another widget, DummySetupWidget becomes a child window inside parent. DummySetupWidget is deleted when its parent is deleted.
     */
    RtcMneSetupWidget(RtcMne* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RtcMneSetupWidget.
     * All RtcMneSetupWidget's children are deleted first. The application exits if RtcMneSetupWidget is the main widget.
     */
    ~RtcMneSetupWidget();

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

    RtcMne* m_pMNE;

    Ui::RtcMneSetupWidgetClass ui;   /**< Holds the user interface for the RtcMneSetupWidgetClass.*/
};
} // NAMESPACE

#endif // RTCMNESETUPWIDGET_H
