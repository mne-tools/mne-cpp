//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file spharasettingsview.h
 * @since 2022
 * @date  March 2023
 * @brief SPHARA spatial-filter basis-order configuration panel.
 *
 * SpharaSettingsView exposes the per-modality SPHARA basis order
 * spinboxes (MAG, GRAD, EEG) and an activation @c QCheckBox; changes
 * are forwarded to the @c rtprocessing SPHARA decomposition that
 * removes high-spatial-frequency artefacts from the live stream.
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
 * @brief SPHARA spatial-filter configuration panel (per-modality basis order spinboxes).
 *
 * An activation @c QCheckBox plus per-modality basis-order spinboxes
 * (MAG, GRAD, EEG) forwarded to the @c rtprocessing SPHARA
 * decomposition.
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
