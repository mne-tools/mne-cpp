//=============================================================================================================
/**
 * @file     neurofeedbacksettinsview.h
 * @author   Simon Marxgut <simon.marxgut@umit-tirol.at>
 * @since    0.1.0
 * @date     November, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Simon Marxgut. All rights reserved.
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
 * @brief    Declaration of the NeurofeedbackSettingsView Class.
 *
 */

#ifndef NEUROFEEDBACKSETTINGSVIEW_H
#define NEUROFEEDBACKSETTINGSVIEW_H

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
    class NeurofeedbackSettingsViewWidget;
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
 * DECLARE CLASS NeurofeedbackSettingsView
 *
 * @brief The NeurofeedbackSettingsView class provides a view to control settings for estiamting functional Neurofeedback
 */
class DISPSHARED_EXPORT NeurofeedbackSettingsView : public AbstractView
{
    Q_OBJECT

public:    
    typedef QSharedPointer<NeurofeedbackSettingsView> SPtr;              /**< Shared pointer type for NeurofeedbackSettingsView. */
    typedef QSharedPointer<const NeurofeedbackSettingsView> ConstSPtr;   /**< Const shared pointer type for NeurofeedbackSettingsView. */

    //=========================================================================================================
    /**
     * Constructs a NeurofeedbackSettingsView which is a child of parent.
     *
     * @param [in] parent        parent of widget.
     */
    NeurofeedbackSettingsView(const QString& sSettingsPath = "",
                              int iMin = 0,
                             int iMax = 10,
                             bool bMinAutoScale = false,
                             bool bMaxAutoScale = false,
                             QWidget *parent = 0,
                             Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the NeurofeedbackSettingsView.
     */
    ~NeurofeedbackSettingsView();

    //=========================================================================================================
    /**
     * Destroys the NeurofeedbackSettingsView.
     *
     * @param [in] lTriggerTypes        The new trigger types.
     */
    void setTriggerTypes(const QStringList& lTriggerTypes);

    //=========================================================================================================
    /**
     * Sets the new number of trials.
     *
     * @param [in] iNumberTrials        The new number of trials.
     */
    void setNumberTrials(int iNumberTrials);

    //=========================================================================================================
    /**
     * Get the current Neurofeedback metric.
     *
     * @return   The current Neurofeedback metric.
     */
    QString getNeurofeedbackMetric();

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

    void changeMax(int value);

    void changeMin(int value);

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

    //=========================================================================================================
    /**
     * Slot called when the metric changed.
     *
     * @param [in] sMetric        The new metric.
     */
    void onMetricChanged(const QString& sMetric);

    //=========================================================================================================
    /**
     * Slot called when the window type changed.
     *
     * @param [in] sWindowType        The new window type.
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
     * @param [in] sTriggerType        The new trigger type.
     */
    void onTriggerTypeChanged(const QString& sTriggerType);

    //=========================================================================================================
    /**
     * Slot called when the frequency band changed.
     */
    void onFrequencyBandChanged();

    //=========================================================================================================
    /**
     * Slot called when the minimum changed.
     */
    void onMinChanged();

    //=========================================================================================================
    /**
     * Slot called when the maximum changed.
     */
    void onMaxChanged();

    //=========================================================================================================
    /**
     * Slot called when the button Reset Autoscale is clicked.
     */
    void ResetAutoScaleClicked();

    //=========================================================================================================
    /**
     * Slot called when the checkbox for minimum autoscale changed.
     */
    void onMinAutoScaleChanged(int value);

    //=========================================================================================================
    /**
     * Slot called when the checkbox for maximum autoscale changed.
     */
    void onMaxAutoScaleChanged(int value);

    //=========================================================================================================





    Ui::NeurofeedbackSettingsViewWidget* m_pUi;

    QString         m_sSettingsPath;                /**< The settings path to store the GUI settings to. */
    int             m_iNumberTrials;                /**< The number of trials are stored to check whether the number of trials actually changed. */
    int             m_iMin;
    int             m_iMax;

    bool            m_bMinAutoScale;
    bool            m_bMaxAutoScale;

signals:
    //=========================================================================================================
    /**
     * Emit signal whenever the Neurofeedback metric changed.
     *
     * @param [in] sMetric        The new metric.
     */
    void NeurofeedbackMetricChanged(const QString& sMetric);

    //=========================================================================================================
    /**
     * Emit signal whenever the window type changed.
     *
     * @param [in] windowType        The new window type.
     */
    void windowTypeChanged(const QString& windowType);

    //=========================================================================================================
    /**
     * Emit signal whenever the number of trials changed.
     *
     * @param [in] iNumberTrials        The new number of trials.
     */
    void numberTrialsChanged(int iNumberTrials);

    //=========================================================================================================
    /**
     * Emit signal whenever the trigger type changed.
     *
     * @param [in] sTriggerType        The new trigger type.
     */
    void triggerTypeChanged(const QString& sTriggerType);

    //=========================================================================================================
    /**
     * Emit signal whenever the frequency band changed.
     *
     * @param [in] fFreqLow        The new lower frequency band.
     * @param [in] fFreqHigh       The new higher frequency band.
     */
    void freqBandChanged(float fFreqLow, float fFreqHigh);

    //=========================================================================================================
    /**
     * Emit signal whenever the minimum changed.
     *
     * @param [in] Min              The new minimum.
     */
    void MinChanged(int Min);

    //=========================================================================================================
    /**
     * Emit signal whenever the maximum changed.
     *
     * @param [in] Max              The new maximum.
     */
    void MaxChanged(int Max);

    //=========================================================================================================
    /**
     * Emit signal whenever the distribution changed.
     *
     * @param [in] Distribution     The new type of distribution for the sliders.
     */
    void ResetAutoScaleChanged(bool bResetAutoScale);

    //=========================================================================================================
    /**
     * Emit signal whenever the maximum autocale changed.
     *
     * @param [in] bMaxAutoScale    The bool for maximum autoscale.
     */
    void MaxAutoScaleChanged(bool bMaxAutoScale);

    //=========================================================================================================
    /**
     * Emit signal whenever the minimum autoscale changed.
     *
     * @param [in] bMinAutoScale              The bool for minimum autoscale.
     */
    void MinAutoScaleChanged(bool bMinAutoScale);
};
} // NAMESPACE

#endif // NEUROFEEDBACKSETTINGSVIEW_H
