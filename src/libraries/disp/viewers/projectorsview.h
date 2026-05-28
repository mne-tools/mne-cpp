//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     projectorsview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    SSP projector enable / disable checkbox list.
 *
 * ProjectorsView builds a @c QCheckBox per @c FiffProj loaded from the
 * active dataset, plus an "all active / inactive" master toggle, and
 * emits @c projSelectionChanged whenever the user flips one. The
 * @c rtprocessing pipeline uses the resulting active mask to
 * (de)project the streaming data on the fly.
 */

#ifndef PROJECTORSVIEW_H
#define PROJECTORSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"
#include <fiff/fiff_proj.h>

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
 * @brief SSP projector enable / disable checkbox list.
 *
 * One @c QCheckBox per @c FiffProj plus a master toggle; emits
 * @c projSelectionChanged so the @c rtprocessing pipeline can
 * (de)project the streaming data on the fly.
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
