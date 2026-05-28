//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file progressview.h
 * @since September 2020
 * @brief Labelled QProgressBar with an automatic show / hide behaviour.
 *
 * ProgressView combines a description label and a @c QProgressBar in a
 * single small widget and is the standard way DISPLIB plugins report
 * the state of long-running jobs (recording write-out, BEM solving,
 * filter design …). It auto-hides when the value reaches 100 % unless
 * the caller opts out.
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

//=============================================================================================================
/**
 * @brief Labelled @c QProgressBar with optional auto-hide on completion.
 *
 * Standard progress widget used by DISPLIB plugins to report
 * long-running jobs (BEM solving, recording write-out, …).
 */
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
