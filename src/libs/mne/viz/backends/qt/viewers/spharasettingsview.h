//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     spharasettingsview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    SPHARA spatial-filter basis-order configuration panel.
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

#include "../viz_qt_global.h"
#include "abstractview.h"
#include <mne/viz/forms/settings_form.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
// mne::viz::qt FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief SPHARA spatial-filter configuration panel (per-modality basis order spinboxes).
 *
 * An activation @c QCheckBox plus per-modality basis-order spinboxes
 * (MAG, GRAD, EEG) forwarded to the @c rtprocessing SPHARA
 * decomposition.
 */
class VIZQTSHARED_EXPORT SpharaSettingsView : public AbstractView
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a SpharaSettingsView which is a child of parent.
     *
     * @param[in] sSettingsPath Settings path under which the widget persists its state (empty = no persistence).
     * @param[in] parent        parent of widget.
     * @param[in] f Qt window flags passed to the QWidget constructor.
     */
    SpharaSettingsView(const QString& sSettingsPath = "",
                       QWidget* parent = 0,
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
     *
     * @param[in] state  True if SPHARA filtering was activated.
     */
    void onSpharaButtonClicked(bool state);

    //=========================================================================================================
    /**
     * Slot called when the user changes the sphara options
     */
    void onSpharaOptionsChanged();

    //=========================================================================================================
    /**
     * Labels the basis-function fields and sets their maxima for the selected system: "Mag"/"Grad" for
     * VectorView, "Inner layer"/"Outer layer" for BabyMEG and a single "EEG" field for EEG.
     *
     * @since 3.0.0
     */
    void updateSystemFields();

    std::shared_ptr<viz::SettingsForm> m_pForm; /**< Activation, system and basis functions, built by a SettingsPanel. */
    int m_listener = 0;                         /**< Id of the view's listener on the form. */
    bool m_bSilent = false;                     /**< Whether form changes are applied without signals. */

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
