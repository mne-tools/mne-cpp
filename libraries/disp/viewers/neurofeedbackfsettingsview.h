//=============================================================================================================
/**
 * @file     neurofeedbackfsettingsview.h
 * @author   Simon Marxgut <simon.marxgut@umit-tirol.at>
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
 * @brief    Declaration of the NeurofeedbackFSettingsView Class.
 *
 */

#ifndef NEUROFEEDBACKFSETTINGSVIEW_H
#define NEUROFEEDBACKFSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPointer>
#include <QLineEdit>
#include <QLabel>
#include <QSpinBox>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QSlider;
class QDoubleSpinBox;

namespace Ui {
class NeurofeedbackFSettingsViewWidget;
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
 * DECLARE CLASS NeurofeedbackCSettingsView
 */
class DISPSHARED_EXPORT NeurofeedbackFSettingsView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<NeurofeedbackFSettingsView> SPtr;              /**< Shared pointer type for NeurofeedbackCSettingsView. */
    typedef QSharedPointer<const NeurofeedbackFSettingsView> ConstSPtr;   /**< Const shared pointer type for NeurofeedbackCSettingsView. */

    //=========================================================================================================
    /**
     * Constructs a NeurofeedbackCSettingsView which is a child of parent.
     *
     * @param [in] parent    parent of widget
     */
    NeurofeedbackFSettingsView(const QString& sSettingsPath = "",
                          int iSliders = 3,
                          int iMin = 0,
                          int iMax = 25,
                          bool ballCh = true,
                          bool bIsRunning = false,
                          QWidget *parent = 0,
                          Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================

    ~NeurofeedbackFSettingsView();

    //=========================================================================================================
    /**
     * Emits all settings once.
     *
     */
    void emitSignals();

protected:
    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     *
     * @param[in] settingsPath        the path to store the settings to.
     */
    void saveSettings(const QString& settingsPath);

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     *
     * @param[in] settingsPath        the path to load the settings from.
     */
    void loadSettings(const QString& settingsPath);

    //=========================================================================================================

    /**
     * Slot called when checkbox change
     */
    void onUpdateCheckboxallCh(int value);

    //=========================================================================================================

    /**
     * Slot called when checkbox change
     */
    void onUpdateCheckboxMinAutoScale(int value);

    //=========================================================================================================

    /**
     * Slot called when checkbox change
     */
    void onUpdateCheckboxMaxAutoScale(int value);

    //=========================================================================================================
    /**
     * Slot called when number of slider spin box changes
     *
     * @param[in] value        number of sliders.
     */
    void onUpdateSpinBoxSlider(int value);


    //=========================================================================================================
    /**
     * Slot is called when standard button for minimum is clicked
     */
    void clickedStandardMin();

    //=========================================================================================================
    /**
     * Slot is called when standard button for maximum is clicked
     */
    void clickedStandardMax();

    //=========================================================================================================
    /**
     * Slot is called when value in the Spinbox for maximum is changed
     *
     * @param[in] value        minimum of frequency.
     */
    void onUpdateSpinBoxMin(int value);

    //=========================================================================================================
    /**
     * Slot is called when value in the Spinbox for maximum is changed
     *
     * @param[in] value        maximum of frequency.
     */
    void onUpdateSpinBoxMax(int value);

    //=========================================================================================================
    /**
     * Update all Settings on the NeurofeedbackFSettingsView Display.
     */
    void updateDisplay();

    //=========================================================================================================
    /**
     * Slot is called when Reset Settings Button is clicket.
     */
    void clickedResetSettings();


    Ui::NeurofeedbackFSettingsViewWidget    *m_pui;

    QString     m_sSettingsPath;


    int         m_iSliders;
    int         m_iMin;
    int         m_iMax;


    bool        m_ballCh;
    bool        m_bMinAutoScale;
    bool        m_bMaxAutoScale;
    bool        m_bIsRunning;


signals:
    //=========================================================================================================
    /**
     * Emitted whenever the settings changed and are ready to be retreived.
     */
    void changeSliders(int value);
    void changeballCh(bool ballCh);
    void changeMin(int value);
    void changeMax(int value);
    void changeMinAutoScale(bool MinAutoScale);
    void changeMaxAutoScale(bool MaxAutoScale);
    void changeResetAutoScale(bool ResetAutoscale);


};
} // NAMESPACE

#endif // NEUROFEEDBACKCSETTINGSVIEW_H
