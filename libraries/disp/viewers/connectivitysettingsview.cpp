//=============================================================================================================
/**
 * @file     connectivitysettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivitysettingsview.h"

#include "ui_connectivitysettingsview.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSettings>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectivitySettingsView::ConnectivitySettingsView(const QString& sSettingsPath,
                                                   QWidget *parent,
                                                   Qt::WindowFlags f)
: QWidget(parent, f)
, ui(new Ui::ConnectivitySettingsViewWidget)
, m_sSettingsPath(sSettingsPath)
{
    ui->setupUi(this);

    loadSettings(m_sSettingsPath);

    connect(ui->m_comboBox_method, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &ConnectivitySettingsView::onMetricChanged);

    connect(ui->m_comboBox_windowType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &ConnectivitySettingsView::onWindowTypeChanged);

//    connect(ui->m_spinBox_numberTrials, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
//            this, &ConnectivitySettingsView::onNumberTrialsChanged);

    connect(ui->m_spinBox_numberTrials, &QSpinBox::editingFinished,
            this, &ConnectivitySettingsView::onNumberTrialsChanged);

    connect(ui->m_comboBox_triggerType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
            this, &ConnectivitySettingsView::onTriggerTypeChanged);

//    connect(ui->m_spinBox_freqLow, &QDoubleSpinBox::editingFinished,
//            this, &ConnectivitySettingsView::onFrequencyBandChanged);

//    connect(ui->m_spinBox_freqHigh, &QDoubleSpinBox::editingFinished,
//            this, &ConnectivitySettingsView::onFrequencyBandChanged);

    connect(ui->m_spinBox_freqLow, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &ConnectivitySettingsView::onFrequencyBandChanged);

    connect(ui->m_spinBox_freqHigh, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &ConnectivitySettingsView::onFrequencyBandChanged);

    this->setWindowTitle("Connectivity Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}


//*************************************************************************************************************

ConnectivitySettingsView::~ConnectivitySettingsView()
{
    saveSettings(m_sSettingsPath);

    delete ui;
}


//*************************************************************************************************************

void ConnectivitySettingsView::setTriggerTypes(const QStringList& lTriggerTypes)
{
    for(const QString &sTriggerType : lTriggerTypes) {
        if(ui->m_comboBox_triggerType->findText(sTriggerType) == -1) {
            ui->m_comboBox_triggerType->addItem(sTriggerType);
        }
    }
}


//*************************************************************************************************************

void ConnectivitySettingsView::setNumberTrials(int iNumberTrials)
{
    ui->m_spinBox_numberTrials->setValue(iNumberTrials);
}


//*************************************************************************************************************

QString ConnectivitySettingsView::getConnectivityMetric()
{
    return ui->m_comboBox_method->currentText();
}


//*************************************************************************************************************

QString ConnectivitySettingsView::getWindowType()
{
    return ui->m_comboBox_windowType->currentText();
}


//*************************************************************************************************************

int ConnectivitySettingsView::getNumberTrials()
{
    return ui->m_spinBox_numberTrials->value();
}


//*************************************************************************************************************

QString ConnectivitySettingsView::getTriggerType()
{
    return ui->m_comboBox_triggerType->currentText();
}


//*************************************************************************************************************

double ConnectivitySettingsView::getLowerFreq()
{
    return ui->m_spinBox_freqLow->value();
}


//*************************************************************************************************************

double ConnectivitySettingsView::getUpperFreq()
{
    return ui->m_spinBox_freqHigh->value();
}


//*************************************************************************************************************

void ConnectivitySettingsView::saveSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;

    settings.setValue(settingsPath + QString("/connMethod"), ui->m_comboBox_method->currentText());
    settings.setValue(settingsPath + QString("/connWindowType"), ui->m_comboBox_windowType->currentText());
    settings.setValue(settingsPath + QString("/connNrTrials"), ui->m_spinBox_numberTrials->value());
    settings.setValue(settingsPath + QString("/connTriggerType"), ui->m_comboBox_triggerType->currentText());
    settings.setValue(settingsPath + QString("/connFreqLow"), ui->m_spinBox_freqLow->value());
    settings.setValue(settingsPath + QString("/connFreqHigh"), ui->m_spinBox_freqHigh->value());
}


//*************************************************************************************************************

void ConnectivitySettingsView::loadSettings(const QString& settingsPath)
{
    if(settingsPath.isEmpty()) {
        return;
    }

    QSettings settings;

    ui->m_comboBox_method->setCurrentText(settings.value(settingsPath + QString("/connMethod"), "COR").toString());
    ui->m_comboBox_windowType->setCurrentText(settings.value(settingsPath + QString("/connWindowType"), "Hanning").toString());
    ui->m_spinBox_numberTrials->setValue(settings.value(settingsPath + QString("/connNrTrials"), 10).toInt());
    m_iNumberTrials = settings.value(settingsPath + QString("/connNrTrials"), 10).toInt();
    ui->m_comboBox_triggerType->setCurrentText(settings.value(settingsPath + QString("/connTriggerType"), "1").toString());
    ui->m_spinBox_freqLow->setValue(settings.value(settingsPath + QString("/connFreqLow"), 7.0).toDouble());
    ui->m_spinBox_freqHigh->setValue(settings.value(settingsPath + QString("/connFreqHigh"), 13.0).toDouble());
}


//*************************************************************************************************************

void ConnectivitySettingsView::onMetricChanged(const QString& sMetric)
{
    emit connectivityMetricChanged(sMetric);
    saveSettings(m_sSettingsPath);
}


//*************************************************************************************************************

void ConnectivitySettingsView::onWindowTypeChanged(const QString& sWindowType)
{
    emit windowTypeChanged(sWindowType);
    saveSettings(m_sSettingsPath);
}


//*************************************************************************************************************

void ConnectivitySettingsView::onNumberTrialsChanged()
{
    if(m_iNumberTrials == ui->m_spinBox_numberTrials->value()) {
        return;
    }

    m_iNumberTrials = ui->m_spinBox_numberTrials->value();

    emit numberTrialsChanged(ui->m_spinBox_numberTrials->value());
    saveSettings(m_sSettingsPath);
}


//*************************************************************************************************************

void ConnectivitySettingsView::onTriggerTypeChanged(const QString& sTriggerType)
{
    emit triggerTypeChanged(sTriggerType);
    saveSettings(m_sSettingsPath);
}


//*************************************************************************************************************

void ConnectivitySettingsView::onFrequencyBandChanged()
{
    //Q_UNUSED(value)
    emit freqBandChanged(ui->m_spinBox_freqLow->value(),
                         ui->m_spinBox_freqHigh->value());
    saveSettings(m_sSettingsPath);
}
