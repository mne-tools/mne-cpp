//=============================================================================================================
/**
 * @file     progressview.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.6
 * @date     September, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel B Motta. All rights reserved.
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
 * @brief    Declaration of the ProgressView Class.
 *
 */

#ifndef PROGRESSVIEW_H
#define PROGRESSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class ProgressViewWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

class DISPSHARED_EXPORT ProgressView : public AbstractView
{
public:
    //=========================================================================================================
    /**
     * Creates an instance of a ProgressView object
     *
     * @param[in] bHorizontalMessage   Whether message id beside(true) or under(false) the loading bar.
     * @param[in] sStyleSheet          Set the stylesheet of the QLabels that display the loading message.
     */
    ProgressView(bool bHorizontalMessage = false,
                 const QString& sStyleSheet = "");

    //=========================================================================================================
    /**
     * Deletes this instance of a ProgressView object
     */
    ~ProgressView();

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    virtual void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    virtual void loadSettings();

    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param[in] mode     The new mode (Clinical=0, Research=1).
     */
    virtual void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode     The new mode (RealTime=0, Offline=1).
     */
    virtual void updateProcessingMode(ProcessingMode mode);

    //=========================================================================================================
    /**
     * Sets message to appear next to loading bar
     */
    void setHorizontal();

    //=========================================================================================================
    /**
     * Sets message to appear under the loading bar
     */
    void setVertical();

    //=========================================================================================================
    /**
     * Sets stylesheet parameters for the QLabels that display loading message
     *
     * @param[in] styleSheet   string with parameters for the QLabels.
     */
    void setTextStyleSheet(const QString& styleSheet);

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

public slots:
    //=========================================================================================================
    /**
     * Dsiplays a new message below/beside the loading bar
     *
     * @param[in] sMessage     message to be displayed.
     */
    void setMessage(const QString& sMessage);

    //=========================================================================================================
    /**
     * Sets new completion percentage and updates message
     *
     * @param[in] iPercentage      percentage to show on the loading bar.
     * @param[in] sMessage         message to be displayed.
     */
    void updateProgress(int iPercentage,
                        const QString& sMessage = "");

    //=========================================================================================================
    /**
     * Sets whether loading bar is visible
     *
     * @param[in] bVisible     whether the bar visible - True = visible, False = hidden.
     */
    void setLoadingBarVisible(bool bVisible);

private:

    Ui::ProgressViewWidget*     m_pUi;

};
} //Namespace

#endif // PROGRESSVIEW_H
