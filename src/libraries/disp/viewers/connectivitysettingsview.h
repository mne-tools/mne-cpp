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
 * @file connectivitysettingsview.h
 * @since July 2018
 * @brief Settings panel choosing connectivity metric, window length and FFT parameters.
 *
 * ConnectivitySettingsView lets the user pick a connectivity estimator
 * (coherence, PLV, PLI, wPLI, debiased wPLI, imaginary coherence,
 * Granger / PDC / DTF …), a windowing function and a number of
 * trials. Changes are forwarded to the connectivity-estimation
 * @c rtprocessing job, and the panel also persists its state through
 * @ref AbstractView::saveSettings.
 */

#ifndef CONNECTIVITYSETTINGSVIEW_H
#define CONNECTIVITYSETTINGSVIEW_H

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
    class ConnectivitySettingsViewWidget;
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
 * @brief Settings panel for the on-line connectivity estimator (metric, window, trials).
 *
 * Combo boxes pick the connectivity metric (coherence, PLV, PLI,
 * wPLI, dwPLI, ImCoh, Granger / PDC / DTF) and the windowing
 * function; spinboxes set the number of trials. Edits propagate via
 * Qt signals to the @c rtprocessing connectivity job.
 */
class DISPSHARED_EXPORT ConnectivitySettingsView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<ConnectivitySettingsView> SPtr;              /**< Shared pointer type for ConnectivitySettingsView. */
    typedef QSharedPointer<const ConnectivitySettingsView> ConstSPtr;   /**< Const shared pointer type for ConnectivitySettingsView. */

    //=========================================================================================================
    /**
     * Constructs a ConnectivitySettingsView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    ConnectivitySettingsView(const QString& sSettingsPath = "",
                             QWidget *parent = 0,
                             Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the ConnectivitySettingsView.
     */
    ~ConnectivitySettingsView();

    //=========================================================================================================
    /**
     * Destroys the ConnectivitySettingsView.
     *
     * @param[in] lTriggerTypes        The new trigger types.
     */
    void setTriggerTypes(const QStringList& lTriggerTypes);

    //=========================================================================================================
    /**
     * Sets the new number of trials.
     *
     * @param[in] iNumberTrials        The new number of trials.
     */
    void setNumberTrials(int iNumberTrials);


    //=========================================================================================================
    /**
     * Sets the frequency band.
     *
     * @param[in] dFreqLow        The lower frequency.
     * @param[in] dFreqHigh       The upper frequency.
     */
    void setFrequencyBand(double dFreqLow, double dFreqHigh);

    //=========================================================================================================
    /**
     * Get the current connectivity metric.
     *
     * @return   The current connectivity metric.
     */
    QString getConnectivityMetric();

    //=========================================================================================================
    /**
     * Get the current window type.
     *
     * @return   The current window type.
     */
    QString getWindowType();

    //=========================================================================================================
    /**
     * Get the current number of trials.
     *
     * @return   The current number of trials.
     */
    int getNumberTrials();

    //=========================================================================================================
    /**
     * Get the current trigger type.
     *
     * @return   The current trigger type.
     */
    QString getTriggerType();

    //=========================================================================================================
    /**
     * Get the current lower frequency range.
     *
     * @return   The current lower frequency range.
     */
    double getLowerFreq();

    //=========================================================================================================
    /**
     * Get the current upper frequency range.
     *
     * @return   The current upper frequency range.
     */
    double getUpperFreq();

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
     * Slot called when the metric changed.
     *
     * @param[in] sMetric        The new metric.
     */
    void onMetricChanged(const QString& sMetric);

    //=========================================================================================================
    /**
     * Slot called when the window type changed.
     *
     * @param[in] sWindowType        The new window type.
     */
    void onWindowTypeChanged(const QString& sWindowType);

    //=========================================================================================================
    /**
     * Slot called when the number of trials changed.
     */
    void onNumberTrialsChanged();

    //=========================================================================================================
    /**
     * Slot called when the trigger type changed.
     *
     * @param[in] sTriggerType        The new trigger type.
     */
    void onTriggerTypeChanged(const QString& sTriggerType);

    //=========================================================================================================
    /**
     * Slot called when the frequency band changed.
     */
    void onFrequencyBandChanged();

    Ui::ConnectivitySettingsViewWidget* m_pUi;

    QString         m_sSettingsPath;                /**< The settings path to store the GUI settings to. */
    int             m_iNumberTrials;                /**< The number of trials are stored to check whether the number of trials actually changed. */

signals:
    //=========================================================================================================
    /**
     * Emit signal whenever the connectivity metric changed.
     *
     * @param[in] sMetric        The new metric.
     */
    void connectivityMetricChanged(const QString& sMetric);

    //=========================================================================================================
    /**
     * Emit signal whenever the window type changed.
     *
     * @param[in] windowType        The new window type.
     */
    void windowTypeChanged(const QString& windowType);

    //=========================================================================================================
    /**
     * Emit signal whenever the number of trials changed.
     *
     * @param[in] iNumberTrials        The new number of trials.
     */
    void numberTrialsChanged(int iNumberTrials);

    //=========================================================================================================
    /**
     * Emit signal whenever the trigger type changed.
     *
     * @param[in] sTriggerType        The new trigger type.
     */
    void triggerTypeChanged(const QString& sTriggerType);

    //=========================================================================================================
    /**
     * Emit signal whenever the frequency band changed.
     *
     * @param[in] fFreqLow        The new lower frequency band.
     * @param[in] fFreqHigh       The new higher frequency band.
     */
    void freqBandChanged(float fFreqLow, float fFreqHigh);
};
} // NAMESPACE

#endif // CONNECTIVITYSETTINGSVIEW_H
