//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     applytoview.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.6
 * @date     August 2020
 * @brief    Three-way radio selector widget choosing scope = Selected / Visible / All channels.
 *
 * ApplyToView is a tiny @ref AbstractView subclass holding a
 * @c QButtonGroup of three radios that lets the user decide whether a
 * subsequent action (filter, scaling, projection, …) should apply to
 * the currently selected channels, the visible-on-screen channels or
 * every channel in the dataset. It emits @c selectionChanged(int) when
 * the active radio flips.
 */

#ifndef APPLYTOVIEW_H
#define APPLYTOVIEW_H

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
    class ApplyToViewWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
/**
 * @brief Three-way radio selector for scope = Selected / Visible / All channels.
 *
 * Emits @c selectionChanged(int) when the active radio flips; the
 * integer matches the @c QButtonGroup id of the chosen scope.
 */
class DISPSHARED_EXPORT ApplyToView : public AbstractView
{
public:
    //=========================================================================================================
    /**
     * @brief ApplyToView
     *
     * @param[in] sSettingsPath    path for saving view settings.
     * @param[in] parent           parent of widget.
     * @param[in] f                flag to denote window porperties of the widget.
     */
    ApplyToView(const QString& sSettingsPath = "",
                QWidget *parent = 0,
                Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    ~ApplyToView();

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
     * Selects all View Select checkboxes
     */
    void selectAll(bool);

    //=========================================================================================================
    /**
     * Clear all View Select checkboxes
     */
    void selectClear(bool);

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

    QList<QString> getSelectedViews();

protected:
    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

    QString                     m_sSettingsPath;

    QList<QString>              m_lViewList;

    Ui::ApplyToViewWidget*      m_pUi;

};
} //NAMESPACE

#endif // APPLYTOVIEW_H
