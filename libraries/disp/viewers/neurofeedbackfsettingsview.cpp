//=============================================================================================================
/**
 * @file     neurofeedbackfsettingsview.cpp
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
 * @brief    Definition of the NeurofeedbackFSettingsView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neurofeedbackfsettingsview.h"
#include "ui_neurofeedbackfsettingsview.h"

#include "neurofeedbackcsettingsview.h"
#include "ui_neurofeedbackcsettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDoubleSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QSlider>
#include <QSettings>
#include <QButtonGroup>

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NeurofeedbackFSettingsView::NeurofeedbackFSettingsView(const QString& sSettingsPath, int iSliders, int iMin, int iMax, bool ballCh, bool bIsRunning,
                                             QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
, m_pui(new Ui::NeurofeedbackFSettingsViewWidget)
, m_sSettingsPath(sSettingsPath)
, m_iSliders(iSliders)
, m_iMin(iMin)
, m_iMax(iMax)
, m_ballCh(ballCh)
, m_bIsRunning(bIsRunning)
{
    m_pui->setupUi(this);

    this->setWindowTitle("Neurofeedback Settings");
    this->setMinimumWidth(450);
    this->setMaximumWidth(450);

    loadSettings(m_sSettingsPath);

    m_pui->spinBox_slider->setMinimum(1);
    m_pui->spinBox_slider->setMaximum(5);
    m_pui->spinBox_slider->setValue(m_iSliders);
    onUpdateSpinBoxSlider(m_iSliders);

    if(m_bMaxAutoScale == true){
        m_pui->checkBox_maxautoscale->setChecked(true);
        onUpdateCheckboxMaxAutoScale(2);
    }
    else{
        m_pui->checkBox_maxautoscale->setChecked(false);
        onUpdateCheckboxMaxAutoScale(1);
    }

    if(m_bMinAutoScale == true){
        m_pui->checkBox_minautoscale->setChecked(true);
        onUpdateCheckboxMinAutoScale(2);
    }
    else{
        m_pui->checkBox_minautoscale->setChecked(false);
        onUpdateCheckboxMinAutoScale(1);
    }


    connect(m_pui->spinBox_slider,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &NeurofeedbackFSettingsView::onUpdateSpinBoxSlider);
    connect(m_pui->checkBox_allCh, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), this, &NeurofeedbackFSettingsView::onUpdateCheckboxallCh);
    connect(m_pui->spinBox_Max,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &NeurofeedbackFSettingsView::onUpdateSpinBoxMax);
    connect(m_pui->spinBox_Min,static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &NeurofeedbackFSettingsView::onUpdateSpinBoxMin);
    connect(m_pui->pushButton_Max, &QPushButton::released, this, &NeurofeedbackFSettingsView::clickedStandardMax);
    connect(m_pui->pushButton_Min, &QPushButton::released, this, &NeurofeedbackFSettingsView::clickedStandardMin);
    connect(m_pui->pushButton_ResetSettings, &QPushButton::released, this, &NeurofeedbackFSettingsView::clickedResetSettings);
    connect(m_pui->checkBox_minautoscale, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), this, &NeurofeedbackFSettingsView::onUpdateCheckboxMinAutoScale);
    connect(m_pui->checkBox_maxautoscale, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), this, &NeurofeedbackFSettingsView::onUpdateCheckboxMaxAutoScale);

    updateDisplay();

}
//=============================================================================================================

NeurofeedbackFSettingsView::~NeurofeedbackFSettingsView()
{
    saveSettings(m_sSettingsPath);

    delete m_pui;
}

//=============================================================================================================

void NeurofeedbackFSettingsView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    // Store Settings
    QSettings settings;

    settings.setValue(settingsPath + QString("/allCh"), m_ballCh);
    settings.setValue(settingsPath + QString("/iSliders"), m_iSliders);
    settings.setValue(settingsPath + QString("/FMax"), m_iMax);
    settings.setValue(settingsPath + QString("/FMin"), m_iMin);
    settings.setValue(settingsPath + QString("/FMinAS"), m_bMinAutoScale);
    settings.setValue(settingsPath + QString("/FMaxAS"), m_bMaxAutoScale);

    updateDisplay();
}

//=============================================================================================================

void NeurofeedbackFSettingsView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings;

    m_ballCh = settings.value(settingsPath + QString("/allCh"), m_ballCh).toBool();
    m_iSliders = settings.value(settingsPath + QString("/iSliders"), m_iSliders).toInt();
    m_iMax = settings.value(settingsPath + QString("/FMax"), m_iMax).toInt();
    m_iMin = settings.value(settingsPath + QString("/FMin"), m_iMin).toInt();
    m_bMinAutoScale = settings.value(settingsPath + QString("/FMinAS"), m_bMinAutoScale).toBool();
    m_bMaxAutoScale = settings.value(settingsPath + QString("/FMaxAS"), m_bMaxAutoScale).toBool();


    updateDisplay();
}

//=============================================================================================================

void NeurofeedbackFSettingsView::emitSignals()
{
    emit changeSliders(m_iSliders);
    emit changeballCh(m_ballCh);
    emit changeMax(m_iMax);
    emit changeMin(m_iMin);
    emit changeMinAutoScale(m_bMinAutoScale);
    emit changeMaxAutoScale(m_bMaxAutoScale);
    emit changeResetAutoScale(true);
}


//=============================================================================================================

void NeurofeedbackFSettingsView::clickedResetSettings()
{
    m_iMax = 25;
    m_iMin = 0;
    m_iSliders = 2;
    m_ballCh = true;

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackFSettingsView::updateDisplay()
{


    if(m_ballCh == true){
        m_pui->checkBox_allCh->setChecked(true);
        m_pui->spinBox_slider->setEnabled(false);
    }
    else if(m_ballCh == false){
        m_pui->spinBox_slider->setValue(m_iSliders);
        m_pui->checkBox_allCh->setChecked(false);
        m_pui->spinBox_slider->setEnabled(true);
    }


}

//=============================================================================================================

void NeurofeedbackFSettingsView::onUpdateCheckboxallCh(int value)
{
    if(value == 2){
        m_ballCh = true;
    }
    else {
        m_ballCh = false;
    }

    emit changeballCh(m_ballCh);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackFSettingsView::onUpdateCheckboxMaxAutoScale(int value)
{
    if(value == 2){
        m_bMaxAutoScale = true;
        m_pui->spinBox_Max->setDisabled(true);
        m_pui->pushButton_Max->setDisabled(true);
        emit changeResetAutoScale(true);
    }
    else {
        m_bMaxAutoScale = false;
        m_pui->spinBox_Max->setEnabled(true);
        m_pui->pushButton_Max->setEnabled(true);
        emit changeResetAutoScale(false);
    }

    emit changeMaxAutoScale(m_bMaxAutoScale);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackFSettingsView::onUpdateCheckboxMinAutoScale(int value)
{
    if(value == 2){
        m_bMinAutoScale = true;
        m_pui->spinBox_Min->setDisabled(true);
        m_pui->pushButton_Min->setDisabled(true);
        emit changeResetAutoScale(true);
    }
    else {
        m_bMinAutoScale = false;
        m_pui->spinBox_Min->setEnabled(true);
        m_pui->pushButton_Min->setEnabled(true);
        emit changeResetAutoScale(false);
    }

    emit changeMinAutoScale(m_bMinAutoScale);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackFSettingsView::onUpdateSpinBoxSlider(int value)
{
    m_iSliders = value;

    emit changeSliders(m_iSliders);

    saveSettings(m_sSettingsPath);
    onUpdateCheckboxallCh(0);
}

//=============================================================================================================

void NeurofeedbackFSettingsView::onUpdateSpinBoxMax(int value)
{
    m_iMax = value;

    emit changeMax(m_iMax);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackFSettingsView::onUpdateSpinBoxMin(int value)
{
    m_iMin = value;

    emit changeMax(m_iMin);

    saveSettings(m_sSettingsPath);
}

//=============================================================================================================

void NeurofeedbackFSettingsView::clickedStandardMax()
{
    onUpdateSpinBoxMax(25);
    m_pui->spinBox_Max->setValue(25);
    updateDisplay();
}


//=============================================================================================================

void NeurofeedbackFSettingsView::clickedStandardMin()
{
    onUpdateSpinBoxMin(0);
    m_pui->spinBox_Min->setValue(0);
    updateDisplay();
}

//=============================================================================================================



