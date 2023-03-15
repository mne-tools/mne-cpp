//=============================================================================================================
/**
 * @file     rtfwdsetupwidget.h
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.1
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains the declaration of the RtFwdSetupWidget class.
 *
 */

#ifndef RTFWDSETUPWIDGET_H
#define RTFWDSETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtfwd.h"
#include "ui_rtfwdsetupwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class RtFwdSetupWidgetClass;
}

//=============================================================================================================
// DEFINE NAMESPACE RTFWDPLUGIN
//=============================================================================================================

namespace RTFWDPLUGIN
{

//=============================================================================================================
// RTFWDPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class RtFwd;

//=============================================================================================================
/**
 * DECLARE CLASS RtFwdSetupWidget
 *
 * @brief The RtFwdSetupWidget class provides the RtFwd configuration window.
 */
class RtFwdSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a RtFwdSetupWidget which is a child of parent.
     *
     * @param[in] toolbox a pointer to the corresponding RtFwd.
     * @param[in] parent pointer to parent widget; If parent is 0, the new RtFwdSetupWidget becomes a window. If parent is another widget, RtFwdSetupWidget becomes a child window inside parent. RtFwdSetupWidget is deleted when its parent is deleted.
     */
    RtFwdSetupWidget(RtFwd* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RtFwdSetupWidget.
     * All RtFwdSetupWidget's children are deleted first. The application exits if RtFwdSetupWidget is the main widget.
     */
    ~RtFwdSetupWidget();

private:
    //=========================================================================================================
    /**
     * Shows forward solution directory selection dialog
     */
    void showFwdDirDialog();

    //=========================================================================================================
    /**
     * change name of solution file
     */
    void onSolNameChanged();

    //=========================================================================================================
    /**
     * Shows measurement selection dialog
     */
    void showMeasFileDialog();

    //=========================================================================================================
    /**
     * Shows source space selection dialog
     */
    void showSourceFileDialog();

    //=========================================================================================================
    /**
     * Shows Bem model selection dialog
     */
    void showBemFileDialog();

    //=========================================================================================================
    /**
     * Shows Mri->Head transformation selection dialog
     */
    void showMriFileDialog();

    //=========================================================================================================
    /**
     * Shows output file selection dialog
     */
    void showMinDistDirDialog();

    QString     m_sMinDistDir;

    //=========================================================================================================
    /**
     * Change name of MinDistDir output
     */
    void onMinDistNameChanged();

    //=========================================================================================================
    /**
     * Shows EEG model selection dialog
     */
    void showEEGModelFileDialog();

    //=========================================================================================================
    /**
     * Shows EEG model name selection selection dialog
     */
    void onEEGModelNameChanged();

    //=========================================================================================================
    /**
     * Change value of minimum distance skull - source
     */
    void onMinDistChanged();

    //=========================================================================================================
    /**
     * Change EEG sphere radius
     */
    void onEEGSphereRadChanged();

    //=========================================================================================================
    /**
     * Change EEG sphere origin
     */
    void onEEGSphereOriginChanged();

    //=========================================================================================================
    /**
     * Change settings from checkboxes
     */
    void onCheckStateChanged();

    //=========================================================================================================

    RtFwd*                     m_pRtFwd;        /**< Holds a pointer to corresponding RtFwd.*/

    Ui::RtFwdSetupWidgetClass  m_ui;            /**< Holds the user interface for the RtFwdSetupWidget.*/
};
} // NAMESPACE

#endif // RTFWDSETUPWIDGET_H
