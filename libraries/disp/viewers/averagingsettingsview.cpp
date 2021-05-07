//=============================================================================================================
/**
 * @file     averagingsettingsview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the AveragingSettingsView class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagingsettingsview.h"
#include "ui_averagingsettingsview.h"

#include <fiff/fiff_evoked_set.h>

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
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AveragingSettingsView::AveragingSettingsView(const QString& sSettingsPath,
                                             const QMap<QString, int> &mapStimChsIndexNames,
                                             QWidget *parent)
: AbstractView(parent)
, m_pUi(new Ui::AverageSettingsViewWidget)
, m_mapStimChsIndexNames(mapStimChsIndexNames)
{
    m_sSettingsPath = sSettingsPath;
    m_pUi->setupUi(this);

    this->setWindowTitle("Averaging Settings");
    this->setMinimumWidth(330);
    //this->setMaximumWidth(330);

    loadSettings();
    redrawGUI();
}

//=============================================================================================================

AveragingSettingsView::~AveragingSettingsView()
{
    saveSettings();
    delete m_pUi;
}

//=============================================================================================================

void AveragingSettingsView::setStimChannels(const QMap<QString,int>& mapStimChsIndexNames)
{
    if(!mapStimChsIndexNames.isEmpty()) {
        m_mapStimChsIndexNames = mapStimChsIndexNames;

        m_pUi->m_pComboBoxChSelection->clear();

        QMapIterator<QString, int> i(mapStimChsIndexNames);
        while (i.hasNext()) {
            i.next();
            m_pUi->m_pComboBoxChSelection->insertItem(m_pUi->m_pComboBoxChSelection->count(),i.key());
        }

        m_pUi->m_pComboBoxChSelection->setCurrentText(m_sCurrentStimChan);

        connect(m_pUi->m_pComboBoxChSelection, &QComboBox::currentTextChanged,
                this, &AveragingSettingsView::onChangeStimChannel);
    }
}

//=============================================================================================================

QString AveragingSettingsView::getCurrentStimCh()
{
    return m_sCurrentStimChan;
}

//=============================================================================================================

bool AveragingSettingsView::getDoBaselineCorrection()
{
    return m_bDoBaselineCorrection;
}

//=============================================================================================================

int AveragingSettingsView::getNumAverages()
{
    return m_iNumAverages;
}

//=============================================================================================================

int AveragingSettingsView::getBaselineFromSeconds()
{
    return m_iBaselineFromSeconds;
}

//=============================================================================================================

int AveragingSettingsView::getBaselineToSeconds()
{
    return m_iBaselineToSeconds;
}

//=============================================================================================================

int AveragingSettingsView::getPreStimMSeconds()
{
    return m_iPreStimSeconds;
}

//=============================================================================================================

int AveragingSettingsView::getPostStimMSeconds()
{
    return m_iPostStimSeconds;
}

//=============================================================================================================

int AveragingSettingsView::getStimChannelIdx()
{
    return m_pUi->m_pComboBoxChSelection->currentData().toInt();
}

//=============================================================================================================

void AveragingSettingsView::redrawGUI()
{
    if(!m_mapStimChsIndexNames.isEmpty()) {
        m_pUi->m_pComboBoxChSelection->clear();

        QMapIterator<QString, int> i(m_mapStimChsIndexNames);
        while (i.hasNext()) {
            i.next();
            m_pUi->m_pComboBoxChSelection->insertItem(m_pUi->m_pComboBoxChSelection->count(),i.key());
        }

        m_pUi->m_pComboBoxChSelection->setCurrentText(m_sCurrentStimChan);

        connect(m_pUi->m_pComboBoxChSelection, &QComboBox::currentTextChanged,
                this, &AveragingSettingsView::changeStimChannel);
    }

    m_pUi->m_pSpinBoxNumAverages->setValue(m_iNumAverages);
    connect(m_pUi->m_pSpinBoxNumAverages, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangeNumAverages);

    //Pre Post stimulus
    m_pUi->m_pSpinBoxPreStimMSeconds->setValue(m_iPreStimSeconds);
    connect(m_pUi->m_pSpinBoxPreStimMSeconds, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangePreStim);

    m_pUi->m_pSpinBoxPostStimMSeconds->setValue(m_iPostStimSeconds);
    connect(m_pUi->m_pSpinBoxPostStimMSeconds, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangePostStim);

    //Baseline Correction
    m_pUi->m_pcheckBoxBaselineCorrection->setChecked(m_bDoBaselineCorrection);
    connect(m_pUi->m_pcheckBoxBaselineCorrection, &QCheckBox::clicked,
            this, &AveragingSettingsView::changeBaselineActive);

    m_pUi->m_pSpinBoxBaselineFromMSeconds->setMinimum(m_pUi->m_pSpinBoxPreStimMSeconds->value()*-1);
    m_pUi->m_pSpinBoxBaselineFromMSeconds->setMaximum(m_pUi->m_pSpinBoxPostStimMSeconds->value());
    m_pUi->m_pSpinBoxBaselineFromMSeconds->setValue(m_iBaselineFromSeconds);
    connect(m_pUi->m_pSpinBoxBaselineFromMSeconds, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangeBaselineFrom);

    m_pUi->m_pSpinBoxBaselineToMSeconds->setMinimum(m_pUi->m_pSpinBoxPreStimMSeconds->value()*-1);
    m_pUi->m_pSpinBoxBaselineToMSeconds->setMaximum(m_pUi->m_pSpinBoxPostStimMSeconds->value());
    m_pUi->m_pSpinBoxBaselineToMSeconds->setValue(m_iBaselineToSeconds);
    connect(m_pUi->m_pSpinBoxBaselineToMSeconds, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &AveragingSettingsView::onChangeBaselineTo);

    connect(m_pUi->m_pushButton_reset, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &AveragingSettingsView::resetAverage);

    connect(m_pUi->m_pushButton_compute, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &AveragingSettingsView::calculateAverage);
    connect(m_pUi->m_checkBox_reject, &QCheckBox::clicked,
            this, &AveragingSettingsView::changeDropActive);

    connect(m_pUi->checkBox_autoCompute, &QCheckBox::clicked,
            this, &AveragingSettingsView::setAutoCompute);

    m_pUi->m_pushButton_compute->hide();
    m_pUi->m_checkBox_reject->hide();
    m_pUi->m_pushButton_compute->hide();

    setWindowFlags(Qt::WindowStaysOnTopHint);

    m_pUi->m_groupBox_detectedTrials->hide();
}

//=============================================================================================================

void AveragingSettingsView::setDetectedEpochs(const FiffEvokedSet& evokedSet)
{
    if(evokedSet.evoked.isEmpty()) {
        m_pUi->m_groupBox_detectedTrials->hide();
        return;
    } else {
        m_pUi->m_groupBox_detectedTrials->show();
    }

    QGridLayout* topLayout = static_cast<QGridLayout*>(m_pUi->m_groupBox_detectedTrials->layout());
    if(!topLayout) {
       topLayout = new QGridLayout();
    }

    QLayoutItem *child;
    while ((child = topLayout->takeAt(0)) != 0) {
        delete child->widget();
        delete child;
    }

    topLayout->addWidget(new QLabel("Type"),0,0);
    topLayout->addWidget(new QLabel("#"),0,1);

    for(int i = 0; i < evokedSet.evoked.size(); i++) {
        if(i < 10) {
            // Show only a maximum of 10 average types
            topLayout->addWidget(new QLabel(evokedSet.evoked.at(i).comment),i+1,0);
            topLayout->addWidget(new QLabel(QString::number(evokedSet.evoked.at(i).nave)),i+1,1);
        }
    }

    //Find Filter tab and add current layout
    m_pUi->m_groupBox_detectedTrials->setLayout(topLayout);
}

//=============================================================================================================

void AveragingSettingsView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/AveragingSettingsView/preStimSeconds"), m_iPreStimSeconds);
    settings.setValue(m_sSettingsPath + QString("/AveragingSettingsView/postStimSeconds"), m_iPostStimSeconds);
    settings.setValue(m_sSettingsPath + QString("/AveragingSettingsView/numAverages"), m_iNumAverages);
    settings.setValue(m_sSettingsPath + QString("/AveragingSettingsView/currentStimChannel"), m_sCurrentStimChan);
    settings.setValue(m_sSettingsPath + QString("/AveragingSettingsView/baselineFromSeconds"), m_iBaselineFromSeconds);
    settings.setValue(m_sSettingsPath + QString("/AveragingSettingsView/baselineToSeconds"), m_iBaselineToSeconds);
    settings.setValue(m_sSettingsPath + QString("/AveragingSettingsView/doBaselineCorrection"), m_bDoBaselineCorrection);
}

//=============================================================================================================

void AveragingSettingsView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");

    m_iPreStimSeconds = settings.value(m_sSettingsPath + QString("/AveragingSettingsView/preStimSeconds"), 100).toInt();
    m_iPostStimSeconds = settings.value(m_sSettingsPath + QString("/AveragingSettingsView/postStimSeconds"), 400).toInt();
    m_iBaselineFromSeconds = settings.value(m_sSettingsPath + QString("/AveragingSettingsView/baselineFromSeconds"), 0).toInt();
    m_iBaselineToSeconds = settings.value(m_sSettingsPath + QString("/AveragingSettingsView/baselineToSeconds"), 0).toInt();

    if(m_iBaselineFromSeconds < -1 * m_iPreStimSeconds || m_iBaselineFromSeconds > m_iPostStimSeconds) {
        m_iBaselineFromSeconds = -1 * m_iPreStimSeconds;
    }

    if(m_iBaselineToSeconds > m_iPostStimSeconds  || m_iBaselineToSeconds < m_iPreStimSeconds) {
        m_iBaselineToSeconds = 0;
    }

    m_iNumAverages = settings.value(m_sSettingsPath + QString("/AveragingSettingsView/numAverages"), 10).toInt();
    m_sCurrentStimChan = settings.value(m_sSettingsPath + QString("/AveragingSettingsView/currentStimChannel"), "STI014").toString();
    m_bDoBaselineCorrection = settings.value(m_sSettingsPath + QString("/AveragingSettingsView/doBaselineCorrection"), false).toBool();
}

//=============================================================================================================

void AveragingSettingsView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void AveragingSettingsView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            m_pUi->m_pSpinBoxNumAverages->hide();
            m_pUi->m_pComboBoxChSelection->hide();
            m_pUi->m_pushButton_reset->hide();
            m_pUi->label->hide();
            m_pUi->m_label_numberAverages->hide();
            m_pUi->m_pushButton_compute->show();
            m_pUi->m_checkBox_reject->show();
            m_pUi->checkBox_autoCompute->show();
            break;
        default: // default is realtime mode
            m_pUi->m_pSpinBoxNumAverages->show();
            m_pUi->m_pComboBoxChSelection->show();
            m_pUi->m_pushButton_reset->show();
            m_pUi->label->show();
            m_pUi->m_label_numberAverages->show();
            m_pUi->m_pushButton_compute->hide();
            m_pUi->m_checkBox_reject->hide();
            m_pUi->checkBox_autoCompute->hide();
            break;
    }
}
//=============================================================================================================

void AveragingSettingsView::onChangePreStim()
{
    qint32 mSeconds = m_pUi->m_pSpinBoxPreStimMSeconds->value();
    m_pUi->m_pSpinBoxBaselineToMSeconds->setMinimum(m_pUi->m_pSpinBoxPreStimMSeconds->value()*-1);
    m_pUi->m_pSpinBoxBaselineFromMSeconds->setMinimum(m_pUi->m_pSpinBoxPreStimMSeconds->value()*-1);

    m_iPreStimSeconds = mSeconds;

    emit changePreStim(mSeconds);

    saveSettings();
}

//=============================================================================================================

void AveragingSettingsView::onChangePostStim()
{
    qint32 mSeconds = m_pUi->m_pSpinBoxPostStimMSeconds->value();
    m_pUi->m_pSpinBoxBaselineToMSeconds->setMaximum(m_pUi->m_pSpinBoxPostStimMSeconds->value());
    m_pUi->m_pSpinBoxBaselineFromMSeconds->setMaximum(m_pUi->m_pSpinBoxPostStimMSeconds->value());

    m_iPostStimSeconds = mSeconds;

    emit changePostStim(mSeconds);

    saveSettings();
}

//=============================================================================================================

void AveragingSettingsView::onChangeBaselineFrom()
{
    qint32 mSeconds = m_pUi->m_pSpinBoxBaselineFromMSeconds->value();
    m_pUi->m_pSpinBoxBaselineToMSeconds->setMinimum(mSeconds);

    m_iBaselineFromSeconds = mSeconds;

    emit changeBaselineFrom(mSeconds);

    saveSettings();
}

//=============================================================================================================

void AveragingSettingsView::onChangeBaselineTo()
{
    qint32 mSeconds = m_pUi->m_pSpinBoxBaselineToMSeconds->value();
    m_pUi->m_pSpinBoxBaselineFromMSeconds->setMaximum(mSeconds);

    m_iBaselineToSeconds = mSeconds;

    emit changeBaselineTo(mSeconds);

    saveSettings();
}

//=============================================================================================================

void AveragingSettingsView::onChangeNumAverages()
{
    m_iNumAverages = m_pUi->m_pSpinBoxNumAverages->value();

    emit changeNumAverages(m_pUi->m_pSpinBoxNumAverages->value());

    saveSettings();
}

//=============================================================================================================

void AveragingSettingsView::onChangeStimChannel()
{
    m_sCurrentStimChan = m_pUi->m_pComboBoxChSelection->currentText();

    emit changeStimChannel(m_sCurrentStimChan);

    saveSettings();
}

//=============================================================================================================

void AveragingSettingsView::clearView()
{

}

//=============================================================================================================

bool AveragingSettingsView::getAutoComputeStatus()
{
    return m_pUi->checkBox_autoCompute->isChecked();
}
