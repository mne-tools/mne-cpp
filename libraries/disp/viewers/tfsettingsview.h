//=============================================================================================================
/**
 * @file     tfsettingsview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     July, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the TfSettingsView Class.
 *
 */

#ifndef TFSETTINGSVIEW_H
#define TFSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

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

namespace Ui {
    class TfSettingsViewWidget;
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
 * DECLARE CLASS TfSettingsView
 *
 * @brief The TfSettingsView class provides a view to control settings for time frequency analysis
 */
class DISPSHARED_EXPORT TfSettingsView : public QWidget
{
    Q_OBJECT

public:    
    typedef QSharedPointer<TfSettingsView> SPtr;              /**< Shared pointer type for TfSettingsView. */
    typedef QSharedPointer<const TfSettingsView> ConstSPtr;   /**< Const shared pointer type for TfSettingsView. */

    //=========================================================================================================
    /**
     * Constructs a TfSettingsView which is a child of parent.
     *
     * @param [in] parent        parent of widget.
     */
    TfSettingsView(const QString& sSettingsPath = "",
                   QWidget *parent = 0,
                   Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the TfSettingsView.
     */
    ~TfSettingsView();

protected:
    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     *
     * @param[in] settingsPath        the path to store the settings to.
     */
    void saveSettings(const QString& settingsPath);

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     *
     * @param[in] settingsPath        the path to load the settings from.
     */
    void loadSettings(const QString& settingsPath);

    //=========================================================================================================
    /**
     * Slot called when the trial or row number changed.
     */
    void onNumberTrialRowChanged();

    Ui::TfSettingsViewWidget* ui;

    QString         m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

signals:
    //=========================================================================================================
    /**
     * Emit signal whenever trial number changed.
     *
     * @param [in] iNumberTrial        The new trial number.
     * @param [in] iNumberRow        The new row number.
     */
    void numberTrialRowChanged(int iNumberTrial, int iNumberRow);

};

} // NAMESPACE

#endif // CONNECTIVITYSETTINGSVIEW_H
