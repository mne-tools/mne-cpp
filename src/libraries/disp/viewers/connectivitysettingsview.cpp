//=============================================================================================================
/**
 * @file     connectivitysettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the ConnectivitySettingsView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivitysettingsview.h"

#include "ui_connectivitysettingsview.h"

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

ConnectivitySettingsView::ConnectivitySettingsView(const QString& sSettingsPath,
                                                   QWidget *parent,
                                                   Qt::WindowFlags f)
: AbstractView(parent, f)
, m_pUi(new Ui::ConnectivitySettingsViewWidget)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    loadSettings();

    connect(m_pUi->m_comboBox_method, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &ConnectivitySettingsView::onMetricChanged);

    connect(m_pUi->m_comboBox_windowType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &ConnectivitySettingsView::onWindowTypeChanged);

//    connect(m_pUi->m_spinBox_numberTrials, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
//            this, &ConnectivitySettingsView::onNumberTrialsChanged);

    connect(m_pUi->m_spinBox_numberTrials, &QSpinBox::editingFinished,
            this, &ConnectivitySettingsView::onNumberTrialsChanged);

    connect(m_pUi->m_comboBox_triggerType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &ConnectivitySettingsView::onTriggerTypeChanged);

//    connect(m_pUi->m_spinBox_freqLow, &QDoubleSpinBox::editingFinished,
//            this, &ConnectivitySettingsView::onFrequencyBandChanged);

//    connect(m_pUi->m_spinBox_freqHigh, &QDoubleSpinBox::editingFinished,
//            this, &ConnectivitySettingsView::onFrequencyBandChanged);

    connect(m_pUi->m_spinBox_freqLow, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &ConnectivitySettingsView::onFrequencyBandChanged);

    connect(m_pUi->m_spinBox_freqHigh, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &ConnectivitySettingsView::onFrequencyBandChanged);

    this->setWindowTitle("Connectivity Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}

//=============================================================================================================

ConnectivitySettingsView::~ConnectivitySettingsView()
{
    saveSettings();

    delete m_pUi;
}

//=============================================================================================================

void ConnectivitySettingsView::setTriggerTypes(const QStringList& lTriggerTypes)
{
    for(const QString &sTriggerType : lTriggerTypes) {
        if(m_pUi->m_comboBox_triggerType->findText(sTriggerType) == -1) {
            m_pUi->m_comboBox_triggerType->addItem(sTriggerType);
        }
    }
}

//=============================================================================================================

void ConnectivitySettingsView::setNumberTrials(int iNumberTrials)
{
    m_pUi->m_spinBox_numberTrials->setValue(iNumberTrials);
}

//=============================================================================================================

QString ConnectivitySettingsView::getConnectivityMetric()
{
    return m_pUi->m_comboBox_method->currentText();
}

//=============================================================================================================

QString ConnectivitySettingsView::getWindowType()
{
    return m_pUi->m_comboBox_windowType->currentText();
}

//=============================================================================================================

int ConnectivitySettingsView::getNumberTrials()
{
    return m_pUi->m_spinBox_numberTrials->value();
}

//=============================================================================================================

QString ConnectivitySettingsView::getTriggerType()
{
    return m_pUi->m_comboBox_triggerType->currentText();
}

//=============================================================================================================

double ConnectivitySettingsView::getLowerFreq()
{
    return m_pUi->m_spinBox_freqLow->value();
}

//=============================================================================================================

double ConnectivitySettingsView::getUpperFreq()
{
    return m_pUi->m_spinBox_freqHigh->value();
}

//=============================================================================================================

void ConnectivitySettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/ConnectivitySettingsView/connMethod"), m_pUi->m_comboBox_method->currentText());
    settings.setValue(m_sSettingsPath + QString("/ConnectivitySettingsView/connWindowType"), m_pUi->m_comboBox_windowType->currentText());
    settings.setValue(m_sSettingsPath + QString("/ConnectivitySettingsView/connNrTrials"), m_pUi->m_spinBox_numberTrials->value());
    settings.setValue(m_sSettingsPath + QString("/ConnectivitySettingsView/connTriggerType"), m_pUi->m_comboBox_triggerType->currentText());
    settings.setValue(m_sSettingsPath + QString("/ConnectivitySettingsView/connFreqLow"), m_pUi->m_spinBox_freqLow->value());
    settings.setValue(m_sSettingsPath + QString("/ConnectivitySettingsView/connFreqHigh"), m_pUi->m_spinBox_freqHigh->value());
}

//=============================================================================================================

void ConnectivitySettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    m_pUi->m_comboBox_method->setCurrentText(settings.value(m_sSettingsPath + QString("/ConnectivitySettingsView/connMethod"), "COR").toString());
    m_pUi->m_comboBox_windowType->setCurrentText(settings.value(m_sSettingsPath + QString("/ConnectivitySettingsView/connWindowType"), "Hanning").toString());
    m_pUi->m_spinBox_numberTrials->setValue(settings.value(m_sSettingsPath + QString("/ConnectivitySettingsView/connNrTrials"), 10).toInt());
    m_iNumberTrials = settings.value(m_sSettingsPath + QString("/ConnectivitySettingsView/connNrTrials"), 10).toInt();
    m_pUi->m_comboBox_triggerType->setCurrentText(settings.value(m_sSettingsPath + QString("/ConnectivitySettingsView/connTriggerType"), "1").toString());
    m_pUi->m_spinBox_freqLow->setValue(settings.value(m_sSettingsPath + QString("/ConnectivitySettingsView/connFreqLow"), 7.0).toDouble());
    m_pUi->m_spinBox_freqHigh->setValue(settings.value(m_sSettingsPath + QString("/ConnectivitySettingsView/connFreqHigh"), 13.0).toDouble());
}

//=============================================================================================================

void ConnectivitySettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void ConnectivitySettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void ConnectivitySettingsView::onMetricChanged(const QString& sMetric)
{
    emit connectivityMetricChanged(sMetric);
    saveSettings();
}

//=============================================================================================================

void ConnectivitySettingsView::onWindowTypeChanged(const QString& sWindowType)
{
    emit windowTypeChanged(sWindowType);
    saveSettings();
}

//=============================================================================================================

void ConnectivitySettingsView::onNumberTrialsChanged()
{
    if(m_iNumberTrials == m_pUi->m_spinBox_numberTrials->value()) {
        return;
    }

    m_iNumberTrials = m_pUi->m_spinBox_numberTrials->value();

    emit numberTrialsChanged(m_pUi->m_spinBox_numberTrials->value());
    saveSettings();
}

//=============================================================================================================

void ConnectivitySettingsView::onTriggerTypeChanged(const QString& sTriggerType)
{
    emit triggerTypeChanged(sTriggerType);
    saveSettings();
}

//=============================================================================================================

void ConnectivitySettingsView::onFrequencyBandChanged()
{
    //Q_UNUSED(value)
    emit freqBandChanged(m_pUi->m_spinBox_freqLow->value(),
                         m_pUi->m_spinBox_freqHigh->value());
    saveSettings();
}

//=============================================================================================================

void ConnectivitySettingsView::clearView()
{

}
