//=============================================================================================================
/**
 * @file     babymegsetupwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    BabyMEGSetupWidget class declaration.
 *
 */

#ifndef BABYMEGSETUPWIDGET_H
#define BABYMEGSETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_global.h"
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
