//=============================================================================================================
/**
 * @file     spharasettingsview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the SpharaSettingsView Class.
 *
 */

#ifndef SPHARASETTINGSVIEW_H
#define SPHARASETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class SpharaSettingsViewWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS SpharaSettingsView
 *
 * @brief The SpharaSettingsView class provides a view to select the SPHARA settings
 */
class DISPSHARED_EXPORT SpharaSettingsView : public AbstractView
{
    Q_OBJECT

public:    
    typedef QSharedPointer<SpharaSettingsView> SPtr;              /**< Shared pointer type for SpharaSettingsView. */
    typedef QSharedPointer<const SpharaSettingsView> ConstSPtr;   /**< Const shared pointer type for SpharaSettingsView. */

    //=========================================================================================================
    /**
     * Constructs a SpharaSettingsView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    SpharaSettingsView(const QString& sSettingsPath = "",
                       QWidget *parent = 0,
                       Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the SpharaSettingsView.
     */
    ~SpharaSettingsView();

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

protected:
    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param[in] mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

    //=========================================================================================================
    /**
     * Slot called when the sphara tool was toggled
     */
    void onSpharaButtonClicked(bool state);

    //=========================================================================================================
    /**
     * Slot called when the user changes the sphara options
     */
    void onSpharaOptionsChanged();

    Ui::SpharaSettingsViewWidget* m_pUi;

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the user toggled the SPHARA operator.
     */
    void spharaActivationChanged(bool state);

    //=========================================================================================================
    /**
     * Emit this signal whenever the user changes the SPHARA operator.
     */
    void spharaOptionsChanged(const QString& sSytemType, int nBaseFctsFirst, int nBaseFctsSecond);
};
} // NAMESPACE

#endif // SPHARASETTINGSVIEW_H
