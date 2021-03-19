//=============================================================================================================
/**
 * @file     compensatorview.h
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
 * @brief    Declaration of the CompensatorView Class.
 *
 */

#ifndef COMPENSATORVIEW_H
#define COMPENSATORVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QCheckBox;

namespace FIFFLIB {
    class FiffCtfComp;
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
 * DECLARE CLASS CompensatorView
 *
 * @brief The CompensatorView class provides a view to select the compensators
 */
class DISPSHARED_EXPORT CompensatorView : public AbstractView
{
    Q_OBJECT

public:    
    typedef QSharedPointer<CompensatorView> SPtr;              /**< Shared pointer type for CompensatorView. */
    typedef QSharedPointer<const CompensatorView> ConstSPtr;   /**< Const shared pointer type for CompensatorView. */

    //=========================================================================================================
    /**
     * Constructs a CompensatorView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    CompensatorView(const QString& sSettingsPath = "",
                    QWidget *parent = 0,
                    Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the CompensatorView.
     */
    ~CompensatorView();

    //=========================================================================================================
    /**
     * Get the current compensators
     *
     * @return The current compensators.
     */
    QList<FIFFLIB::FiffCtfComp> getCompensators() const;

    //=========================================================================================================
    /**
     * Get the last value of comp.to
     *
     * @return The last value of comp.to.
     */
    int getLastTo() const;

    //=========================================================================================================
    /**
     * Set the current compensators
     *
     * @param[in] comps    The new compensators.
     */
    void setCompensators(const QList<FIFFLIB::FiffCtfComp>& comps);

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
     * Clears the view
     */
    void clearView();

protected:
    //=========================================================================================================
    /**
     * Redraw the selection.
     */
    void redrawGUI();

    //=========================================================================================================
    /**
     * Slot called when the compensator check state changes
     */
    void onCheckCompStatusChanged();

    QList<QCheckBox*>                                   m_qListCompCheckBox;            /**< List of compensator CheckBox. */

    QList<FIFFLIB::FiffCtfComp>                         m_pComps;                       /**< The current compensators. */

    QString                                             m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

    int                                                 m_iLastTo;

    QMap<int,bool>                                      m_mapCompActive;

signals:
    //=========================================================================================================
    /**
     * Signal mapper signal for compensator changes.
     */
    void compClicked(const QString& text);

    //=========================================================================================================
    /**
     * Emit this signal whenever the user changes the compensator.
     */
    void compSelectionChanged(int to);
};
} // NAMESPACE

#endif // COMPENSATORVIEW_H
