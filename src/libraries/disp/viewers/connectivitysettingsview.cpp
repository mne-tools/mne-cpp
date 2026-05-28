//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     connectivitysettingsview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   johaenns <j.vorw01@gmail.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the ConnectivitySettingsView connectivity-metric panel.
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

void ConnectivitySettingsView::setFrequencyBand(double dFreqLow, double dFreqHigh)
{
    m_pUi->m_spinBox_freqLow->setValue(dFreqLow);
    m_pUi->m_spinBox_freqHigh->setValue(dFreqHigh);
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
