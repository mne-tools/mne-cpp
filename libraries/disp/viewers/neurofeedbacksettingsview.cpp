//=============================================================================================================
/**
 * @file     neurofeedbacksettingsview.cpp
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
 * @brief    Definition of the NeurofeedbackSettingsView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neurofeedbacksettingsview.h"

#include "ui_neurofeedbacksettingsview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>

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

NeurofeedbackSettingsView::NeurofeedbackSettingsView(const QString& sSettingsPath,
                                                     int iMin,
                                                     int iMax,
                                                     bool bMinAutoScale,
                                                     bool bMaxAutoScale,
                                                   QWidget *parent,
                                                   Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(new Ui::NeurofeedbackSettingsViewWidget)
, m_sSettingsPath(sSettingsPath)
, m_iMin(iMin)
, m_iMax(iMax)
, m_bMinAutoScale(bMinAutoScale)
, m_bMaxAutoScale(bMaxAutoScale)
{

    m_pUi->setupUi(this);


//    loadSettings();

    connect(m_pUi->m_spinbox_min, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &NeurofeedbackSettingsView::onMinChanged);

    connect(m_pUi->m_spinbox_max, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &NeurofeedbackSettingsView::onMaxChanged);

    connect(m_pUi->checkBox_MaxAutoScale, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), this, &NeurofeedbackSettingsView::onMaxAutoScaleChanged);

    connect(m_pUi->checkBox_MinAutoScale, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), this, &NeurofeedbackSettingsView::onMinAutoScaleChanged);


    connect(m_pUi->pushButton_ResetAutoScale, &QPushButton::released, this, &NeurofeedbackSettingsView::ResetAutoScaleClicked);


    m_pUi->m_spinbox_min->setValue(m_iMin);
    m_pUi->m_spinbox_max->setValue(m_iMax);
    if(m_bMaxAutoScale == true){
        m_pUi->checkBox_MaxAutoScale->setChecked(true);
    }
    else{
        m_pUi->checkBox_MaxAutoScale->setChecked(false);
    }

    if(m_bMinAutoScale == true){
        m_pUi->checkBox_MinAutoScale->setChecked(true);
    }
    else{
        m_pUi->checkBox_MinAutoScale->setChecked(false);
    }

    this->setWindowTitle("Neurofeedback Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}

//=============================================================================================================

NeurofeedbackSettingsView::~NeurofeedbackSettingsView()
{
    saveSettings();

    delete m_pUi;
}

//=============================================================================================================

void NeurofeedbackSettingsView::saveSettings()
{
//    if(m_sSettingsPath.isEmpty()) {
//        return;
//    }

//    QSettings settings("MNECPP");


//    settings.setValue(m_sSettingsPath + QString("/iMax"), m_iMax);
//    settings.setValue(m_sSettingsPath + QString("/iMin"), m_iMin);
//    settings.setValue(m_sSettingsPath + QString("/bMaxAutoScale"), m_bMaxAutoScale);
//    settings.setValue(m_sSettingsPath + QString("/bMinAutoScale"), m_bMinAutoScale);


}

//=============================================================================================================

void NeurofeedbackSettingsView::loadSettings()
{
//    if(m_sSettingsPath.isEmpty()) {
//        return;
//    }

//    QSettings settings("MNECPP");

//    m_iMax = settings.value(m_sSettingsPath + QString("/iMax"), m_iMax).toInt();
//    m_iMin = settings.value(m_sSettingsPath + QString("/iMin"), m_iMin).toInt();
//    m_bMaxAutoScale = settings.value(m_sSettingsPath + QString("/bMaxAutoScale"), m_bMaxAutoScale).toBool();
//    m_bMinAutoScale = settings.value(m_sSettingsPath + QString("/bMinAutoScale"), m_bMinAutoScale).toBool();



}

//=============================================================================================================

void NeurofeedbackSettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void NeurofeedbackSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void NeurofeedbackSettingsView::onMetricChanged(const QString& sMetric)
{
    emit NeurofeedbackMetricChanged(sMetric);
    saveSettings();
}

//=============================================================================================================

void NeurofeedbackSettingsView::onWindowTypeChanged(const QString& sWindowType)
{
    emit windowTypeChanged(sWindowType);
    saveSettings();
}



//=============================================================================================================

void NeurofeedbackSettingsView::onTriggerTypeChanged(const QString& sTriggerType)
{
    emit triggerTypeChanged(sTriggerType);
    saveSettings();
}

//=============================================================================================================

void NeurofeedbackSettingsView::onMaxChanged()
{
    m_iMax = m_pUi->m_spinbox_max->value();
    emit MaxChanged(m_iMax);
    saveSettings();
}

//=============================================================================================================

void NeurofeedbackSettingsView::onMinChanged()
{
    m_iMin = m_pUi->m_spinbox_min->value();
    emit MinChanged(m_iMin);
    saveSettings();
}

//=============================================================================================================

void NeurofeedbackSettingsView::ResetAutoScaleClicked()
{
    emit ResetAutoScaleChanged(true);
    saveSettings();
}

//=============================================================================================================

void NeurofeedbackSettingsView::onMaxAutoScaleChanged(int value)
{
    if(value == 2){
        m_bMaxAutoScale = true;
        emit MaxAutoScaleChanged(m_bMaxAutoScale);
        emit ResetAutoScaleChanged(true);
    }
    else{
        m_bMaxAutoScale = false;
        emit MaxAutoScaleChanged(m_bMaxAutoScale);
    }
    saveSettings();
}

//=============================================================================================================

void NeurofeedbackSettingsView::onMinAutoScaleChanged(int value)
{
    if(value == 2){
        m_bMinAutoScale = true;
        emit MinAutoScaleChanged(m_bMinAutoScale);
        emit ResetAutoScaleChanged(true);
    }
    else{
        m_bMinAutoScale = false;
        emit MinAutoScaleChanged(m_bMinAutoScale);
    }
    saveSettings();
}

//=============================================================================================================

void NeurofeedbackSettingsView::changeMax(int value)
{
    m_iMax = value;
    m_pUi->m_spinbox_max->setValue(m_iMax);
    saveSettings();
}

//=============================================================================================================

void NeurofeedbackSettingsView::changeMin(int value)
{
    m_iMin = value;
    m_pUi->m_spinbox_min->setValue(m_iMin);
    saveSettings();
}

//=============================================================================================================

void NeurofeedbackSettingsView::clearView()
{

}
