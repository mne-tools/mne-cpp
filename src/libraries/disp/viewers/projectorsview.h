//=============================================================================================================
/**
 * @file     projectorsview.h
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
 * @brief    Declaration of the ProjectorsView Class.
 *
 */

#ifndef PROJECTORSVIEW_H
#define PROJECTORSVIEW_H

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
    class FiffProj;
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
 * DECLARE CLASS ProjectorsView
 *
 * @brief The ProjectorsView class provides a view to select projectors
 */
class DISPSHARED_EXPORT ProjectorsView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<ProjectorsView> SPtr;              /**< Shared pointer type for ProjectorsView. */
    typedef QSharedPointer<const ProjectorsView> ConstSPtr;   /**< Const shared pointer type for ProjectorsView. */

    //=========================================================================================================
    /**
     * Constructs a ProjectorsView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    ProjectorsView(const QString& sSettingsPath = "",
                   QWidget *parent = 0,
                   Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the ProjectorsView.
     */
    ~ProjectorsView();

    //=========================================================================================================
    /**
     * Get the current projectors
     *
     * @return The current projectors.
     */
    QList<FIFFLIB::FiffProj> getProjectors() const;

    //=========================================================================================================
    /**
     * Set the current projectors
     *
     * @param[in] projs    The new projectors.
     */
    void setProjectors(const QList<FIFFLIB::FiffProj>& projs);

    //=========================================================================================================
    /**
     * Redraw the view.
     */
    void redrawGUI();

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
     * Slot called when user enables/disables all projectors
     */
    void onEnableDisableAllProj(bool status);

    //=========================================================================================================
    /**
     * Slot called when the projector check state changes
     */
    void onCheckProjStatusChanged();

    QList<QCheckBox*>                                   m_qListProjCheckBox;            /**< List of projection CheckBox. */
    QCheckBox*                                          m_pEnableDisableProjectors;     /**< Holds the enable disable all check box. */

    QList<FIFFLIB::FiffProj>                            m_pProjs;                       /**< The current projectors. */

    QString                                             m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

    QMap<QString,bool>                                  m_mapProjActive;

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the user changes the projections.
     */
    void projSelectionChanged(const QList<FIFFLIB::FiffProj>& projs);
};
} // NAMESPACE

#endif // PROJECTORSVIEW_H
