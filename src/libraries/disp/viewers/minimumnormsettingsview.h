//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   johaenns <j.vorw01@gmail.com>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file minimumnormsettingsview.h
 * @since September 2018
 * @brief Minimum-norm inverse-solution method / regularisation panel (MNE, dSPM, sLORETA).
 *
 * MinimumNormSettingsView selects the inverse-operator method (MNE,
 * dSPM, sLORETA), the SNR lambda regularisation and the time-frame
 * window used by the on-line source estimator. Edits propagate to
 * the @c rtprocessing inverse job through Qt signals.
 */

#ifndef MINIMUMNORMSETTINGSVIEW_H
#define MINIMUMNORMSETTINGSVIEW_H

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
    class MinimumNormSettingsViewWidget;
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
 * @brief Inverse-method / regularisation panel (MNE, dSPM, sLORETA) for the on-line source estimator.
 *
 * Combo boxes pick the inverse method, spinboxes set the SNR lambda
 * and the time-frame window; edits propagate via Qt signals to the
 * @c rtprocessing inverse job.
 */
class DISPSHARED_EXPORT MinimumNormSettingsView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<MinimumNormSettingsView> SPtr;              /**< Shared pointer type for MinimumNormSettingsView. */
    typedef QSharedPointer<const MinimumNormSettingsView> ConstSPtr;   /**< Const shared pointer type for MinimumNormSettingsView. */

    //=========================================================================================================
    /**
     * Constructs a MinimumNormSettingsView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    MinimumNormSettingsView(const QString& sSettingsPath = "",
                            const QString& sMethod = "",
                            QWidget *parent = 0,
                            Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the MinimumNormSettingsView.
     */
    ~MinimumNormSettingsView();

    //=========================================================================================================
    /**
     * Destroys the MinimumNormSettingsView.
     *
     * @param[in] lTriggerTypes        The new trigger types.
     */
    void setTriggerTypes(const QStringList& lTriggerTypes);

    //=========================================================================================================
    /**
     * Sets the model checkpoint path shown in the read-only line edit.
     *
     * @param[in] sPath  Absolute path to the model checkpoint file (e.g. .onnx).
     */
    void setModelCheckpoint(const QString& sPath);

    //=========================================================================================================
    /**
     * Returns the current model checkpoint path.
     */
    QString getModelCheckpoint() const;

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
     * Slot called when the method changed.
     *
     * @param[in] method        The new method.
     */
    void onMethodChanged(const QString& method);

    //=========================================================================================================
    /**
     * Slot called when the trigger type changed.
     *
     * @param[in] sTriggerType        The new trigger type.
     */
    void onTriggerTypeChanged(const QString& sTriggerType);

    //=========================================================================================================
    /**
     * Slot called when the time point changes.
     */
    void onTimePointValueChanged();

    //=========================================================================================================
    /**
     * Show or hide the CMNE-only model checkpoint widgets based on the active method.
     *
     * @param[in] method  The currently selected inverse method.
     */
    void updateCmneWidgetVisibility(const QString& method);

    //=========================================================================================================
    /**
     * Slot called when the user clicks the "browse model checkpoint" button.
     */
    void onBrowseModelCheckpointClicked();

    Ui::MinimumNormSettingsViewWidget* m_pUi;
    QString m_sMethod;
    QString m_sModelCheckpoint;     /**< Persisted model-checkpoint path for CMNE. */

signals:
    //=========================================================================================================
    /**
     * Emit signal whenever the method changed.
     *
     * @param[in] method        The new method.
     */
    void methodChanged(const QString& method);

    //=========================================================================================================
    /**
     * Emit signal whenever the trigger type changed.
     *
     * @param[in] triggerType        The new trigger type.
     */
    void triggerTypeChanged(const QString& triggerType);

    //=========================================================================================================
    /**
     * Emit signal whenever the time point changed.
     *
     * @param[in] iTimePoint        The new time point.
     */
    void timePointChanged(int iTimePoint);

    //=========================================================================================================
    /**
     * Emit signal whenever the CMNE model checkpoint path changed.
     *
     * @param[in] sPath  The new model checkpoint path.
     */
    void modelCheckpointChanged(const QString& sPath);
};
} // NAMESPACE

#endif // MinimumNormSettingsView_H
