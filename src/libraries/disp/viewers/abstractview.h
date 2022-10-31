//=============================================================================================================
/**
 * @file     abstractview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.2
 * @date     June, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the AbstractView Class.
 *
 */

#ifndef ABSTRACTVIEW_H
#define ABSTRACTVIEW_H

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
 * DECLARE CLASS AbstractView
 *
 * @brief The AbstractView class provides the base calss for all Disp viewers
 */
class DISPSHARED_EXPORT AbstractView : public QWidget
{
    Q_OBJECT

public:    
    typedef QSharedPointer<AbstractView> SPtr;              /**< Shared pointer type for AbstractView. */
    typedef QSharedPointer<const AbstractView> ConstSPtr;   /**< Const shared pointer type for AbstractView. */


    enum StyleMode {
        Default,
        Dark
    };

    enum GuiMode {
        Clinical,
        Research
    };

    enum ProcessingMode {
        RealTime,
        Offline
    };

    //=========================================================================================================
    /**
     * Constructs a AbstractView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    AbstractView(QWidget *parent = 0,
                 Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Sets the GUI of this view to a specific mode. Clinical = 0, Research = 0.
     *
     * @param mode     The new mode.
     */
    virtual void setGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Sets the GUI of this view to a specific mode. RealTime = 0, Offline = 0.
     *
     * @param mode     The new mode.
     */
    virtual void setProcessingMode(ProcessingMode mode);

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    virtual void saveSettings() = 0;

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    virtual void loadSettings() = 0;

    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param mode     The new mode (Clinical=0, Research=1).
     */
    virtual void updateGuiMode(GuiMode mode) = 0;

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param mode     The new mode (RealTime=0, Offline=1).
     */
    virtual void updateProcessingMode(ProcessingMode mode) = 0;

    //=========================================================================================================
    /**
     * Clears the view
     */
    virtual void clearView() = 0;

signals:
    //=========================================================================================================
    /**
     * Signal emited whenever the gui style mode changes.
     */
    void guiStyleChanged(DISPLIB::AbstractView::StyleMode style);

protected:

    bool            m_bResearchModeIsActive;  /**< The flag describing whether the research mode of the view is active or not. */
    bool            m_bOfflineModeIsActive;  /**< The flag describing whether offline mode of the view is active or not. */

    QString         m_sSettingsPath;            /**< The settings path to store the GUI settings to. */

};
} // NAMESPACE

#endif // ABSTRACTVIEW_H
